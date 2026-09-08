#include "pch.h"
#include "MemoryReader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CMemoryReader::Read(DWORD pid, ULONGLONG address)
{
    m_data.clear();
    m_valid.clear();
    m_region     = CRegionInfo();
    m_address    = address;
    m_accessible = false;

    if (pid == 0)
        return;

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == nullptr)
        return;

    ReadRegion(hProcess, address);

    m_data.resize(windowSize, 0);

    for (size_t offset = 0; offset < windowSize; offset += chunkSize)
    {
        SIZE_T read = 0;

        const BOOL bRead = ReadProcessMemory(hProcess,
                                             reinterpret_cast<LPCVOID>(address + offset),
                                             &m_data[offset], chunkSize, &read);

        const bool bValid = (bRead != FALSE) && (read == chunkSize);

        m_valid.push_back(bValid);

        if (bValid)
            m_accessible = true;
    }

    CloseHandle(hProcess);
}

void CMemoryReader::ReadRegion(HANDLE hProcess, ULONGLONG address)
{
    MEMORY_BASIC_INFORMATION mbi = {};

    if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) != sizeof(mbi))
        return;

    m_region.baseAddress    = reinterpret_cast<ULONGLONG>(mbi.BaseAddress);
    m_region.allocationBase = reinterpret_cast<ULONGLONG>(mbi.AllocationBase);
    m_region.size           = mbi.RegionSize;
    m_region.state          = mbi.State;
    m_region.type           = mbi.Type;
    m_region.protect        = (mbi.State == MEM_FREE) ? 0 : mbi.Protect;
}

bool CMemoryReader::IsReadable(const MEMORY_BASIC_INFORMATION& mbi)
{
    if (mbi.State != MEM_COMMIT)
        return false;

    // Zastitna stranica javlja iznimku pri prvom pristupu, pa se preskace.
    if ((mbi.Protect & PAGE_GUARD) != 0)
        return false;

    const DWORD access = mbi.Protect & 0xFF;

    return (access != 0) && (access != PAGE_NOACCESS);
}

bool CMemoryReader::FindReadable(DWORD pid, ULONGLONG address, bool bForward,
                                 ULONGLONG& result) const
{
    if (pid == 0)
        return false;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == nullptr)
        return false;

    bool bFound = false;
    ULONGLONG current = address;

    // Slobodni i rezervirani dijelovi vracaju se kao cijele regije, pa je
    // koraka malo i kod velikih praznina. Gornja granica je ipak postavljena da
    // petlja ne moze ostati zaglavljena.
    for (int step = 0; step < maxSteps; ++step)
    {
        MEMORY_BASIC_INFORMATION mbi = {};

        if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)) != sizeof(mbi))
            break;

        const ULONGLONG base = reinterpret_cast<ULONGLONG>(mbi.BaseAddress);

        if (IsReadable(mbi))
        {
            if (step == 0)
            {
                // Trazena adresa je vec u dostupnoj regiji.
                result = current;
            }
            else if (bForward)
            {
                result = base;
            }
            else
            {
                // Unatrag se staje na kraj prethodne dostupne regije, da se
                // prikaz nastavlja ondje gdje je prethodni zavrsio.
                result = (mbi.RegionSize > windowSize)
                    ? base + mbi.RegionSize - windowSize
                    : base;
            }

            bFound = true;
            break;
        }

        if (bForward)
        {
            current = base + mbi.RegionSize;
        }
        else
        {
            if (base == 0)
                break;

            current = base - 1;
        }
    }

    CloseHandle(hProcess);

    return bFound;
}

bool CMemoryReader::IsValid(size_t offset) const
{
    const size_t chunk = offset / chunkSize;

    if (chunk >= m_valid.size())
        return false;

    return m_valid[chunk];
}
