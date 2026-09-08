#include "pch.h"
#include "StringScanner.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CStringScanner::Clear()
{
    m_items.clear();
    m_accessible = false;
    m_bPartial   = false;
}

void CStringScanner::Scan(DWORD pid, CScanObserver* pObserver)
{
    Clear();

    if (pid == 0)
        return;

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == nullptr)
        return;

    m_accessible = true;

    const ULONGLONG total = MeasureTotal(hProcess);
    ULONGLONG done = 0;

    SYSTEM_INFO si = {};
    GetSystemInfo(&si);

    ULONGLONG       address    = reinterpret_cast<ULONGLONG>(si.lpMinimumApplicationAddress);
    const ULONGLONG maxAddress = reinterpret_cast<ULONGLONG>(si.lpMaximumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi = {};

    while (address <= maxAddress && done < maxScanBytes &&
           VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == sizeof(mbi))
    {
        const ULONGLONG base = reinterpret_cast<ULONGLONG>(mbi.BaseAddress);
        const ULONGLONG next = base + mbi.RegionSize;

        if (next <= address)
            break;

        if (IsReadable(mbi))
        {
            if (!ScanRegion(hProcess, base, mbi.RegionSize, pObserver, total, done))
            {
                m_bPartial = true;
                break;
            }
        }

        address = next;
    }

    if (done >= maxScanBytes)
        m_bPartial = true;

    CloseHandle(hProcess);
}

bool CStringScanner::IsReadable(const MEMORY_BASIC_INFORMATION& mbi)
{
    if (mbi.State != MEM_COMMIT)
        return false;

    // Zastitna stranica javlja iznimku pri prvom pristupu, pa se preskace.
    if ((mbi.Protect & PAGE_GUARD) != 0)
        return false;

    const DWORD access = mbi.Protect & 0xFF;

    return (access != 0) && (access != PAGE_NOACCESS);
}

bool CStringScanner::IsPrintable(BYTE value)
{
    return (value >= 32) && (value < 127);
}

ULONGLONG CStringScanner::MeasureTotal(HANDLE hProcess) const
{
    // Napredak se moze prikazati tek kad se zna koliko memorije treba
    // pregledati, pa se regije najprije samo prebroje.
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);

    ULONGLONG       address    = reinterpret_cast<ULONGLONG>(si.lpMinimumApplicationAddress);
    const ULONGLONG maxAddress = reinterpret_cast<ULONGLONG>(si.lpMaximumApplicationAddress);

    ULONGLONG total = 0;

    MEMORY_BASIC_INFORMATION mbi = {};

    while (address <= maxAddress &&
           VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == sizeof(mbi))
    {
        const ULONGLONG next = reinterpret_cast<ULONGLONG>(mbi.BaseAddress) + mbi.RegionSize;

        if (next <= address)
            break;

        if (IsReadable(mbi))
            total += mbi.RegionSize;

        address = next;
    }

    return (total < maxScanBytes) ? total : maxScanBytes;
}

bool CStringScanner::ScanRegion(HANDLE hProcess, ULONGLONG base, ULONGLONG size,
                                CScanObserver* pObserver, ULONGLONG total, ULONGLONG& done)
{
    std::vector<BYTE> buffer(chunkSize, 0);

    ULONGLONG offset = 0;

    while (offset < size)
    {
        if (pObserver != nullptr && pObserver->IsScanCancelled())
            return false;

        if (m_items.size() >= maxResults || done >= maxScanBytes)
            return false;

        const SIZE_T request = (size - offset < chunkSize)
            ? static_cast<SIZE_T>(size - offset)
            : chunkSize;

        SIZE_T read = 0;

        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(base + offset),
                              &buffer[0], request, &read) && read > 0)
        {
            ScanBuffer(buffer, read, base + offset);

            // Niz presjecen na granici citanja nasao bi se dvaput prekratak, pa
            // se pretraga za njegovu duljinu vraca unatrag.
            const size_t tail = (read < request) ? 0 : TailLength(buffer, read);

            offset += (read > tail) ? (read - tail) : read;
        }
        else
        {
            offset += request;
        }

        done += request;

        if (pObserver != nullptr && total > 0)
        {
            const int percent = static_cast<int>((100 * done) / total);

            pObserver->OnScanProgress((percent > 100) ? 100 : percent,
                                      static_cast<int>(m_items.size()));
        }
    }

    return true;
}

size_t CStringScanner::TailLength(const std::vector<BYTE>& buffer, size_t size) const
{
    size_t length = 0;

    while (length < size && length < chunkSize / 2 && IsPrintable(buffer[size - 1 - length]))
        ++length;

    return length;
}

void CStringScanner::ScanBuffer(const std::vector<BYTE>& buffer, size_t size, ULONGLONG base)
{
    ScanAscii(buffer, size, base);
    ScanWide(buffer, size, base);
}

void CStringScanner::ScanAscii(const std::vector<BYTE>& buffer, size_t size, ULONGLONG base)
{
    size_t start  = 0;
    size_t length = 0;

    for (size_t i = 0; i < size; ++i)
    {
        if (IsPrintable(buffer[i]))
        {
            if (length == 0)
                start = i;

            ++length;
            continue;
        }

        if (length >= minLength)
            AddString(buffer, start, length, base, false);

        length = 0;
    }

    // Niz koji dopire do kraja spremnika zavrsava zajedno s njim.
    if (length >= minLength)
        AddString(buffer, start, length, base, false);
}

void CStringScanner::ScanWide(const std::vector<BYTE>& buffer, size_t size, ULONGLONG base)
{
    size_t i = 0;

    while (i + 1 < size)
    {
        // Dvobajtni zapis prepoznaje se po tome sto je svaki drugi bajt nula.
        if (!IsPrintable(buffer[i]) || buffer[i + 1] != 0)
        {
            ++i;
            continue;
        }

        const size_t start = i;
        size_t length = 0;

        while (i + 1 < size && IsPrintable(buffer[i]) && buffer[i + 1] == 0)
        {
            ++length;
            i += 2;
        }

        if (length >= minLength)
            AddString(buffer, start, length, base, true);
    }
}

void CStringScanner::AddString(const std::vector<BYTE>& buffer, size_t start, size_t length,
                               ULONGLONG base, bool bWide)
{
    if (m_items.size() >= maxResults)
        return;

    CStringInfo info;
    info.address = base + start;
    info.bWide   = bWide;

    // Procitani bajtovi nisu zavrseni nulom, pa se duljina uvijek zadaje.
    if (bWide)
    {
        info.text = CString(reinterpret_cast<const wchar_t*>(&buffer[start]),
                            static_cast<int>(length));
    }
    else
    {
        info.text = CString(reinterpret_cast<const char*>(&buffer[start]),
                            static_cast<int>(length));
    }

    m_items.push_back(info);
}
