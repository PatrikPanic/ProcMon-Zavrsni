#include "pch.h"
#include "EnvironmentCollector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    // Odmaci polja unutar struktura PEB i RTL_USER_PROCESS_PARAMETERS. Sustav
    // ih ne objavljuje u zaglavljima, a razlikuju se za 64-bitne i 32-bitne
    // procese jer ovise o velicini pokazivaca.
    enum
    {
        peb64Parameters    = 0x20,
        params64Environment = 0x80,
        peb32Parameters    = 0x10,
        params32Environment = 0x48
    };

    // Blok okoline nema zapisanu velicinu, nego zavrsava dvjema nulama, pa se
    // cita u komadima do te oznake ili do gornje granice.
    enum
    {
        chunkChars = 512,
        maxChars   = 64 * 1024
    };
}

void CEnvironmentCollector::Refresh(DWORD pid)
{
    m_items.clear();
    m_accessible = false;

    if (pid == 0)
        return;

    // Uz podatke o procesu treba i pravo citanja memorije, jer se sam blok
    // dohvaca iz adresnog prostora tog procesa.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == nullptr)
        return;

    ULONGLONG address = 0;

    if (ReadBlockAddress(hProcess, address) && address != 0)
    {
        m_accessible = true;
        ReadBlock(hProcess, address);
    }

    CloseHandle(hProcess);
}

bool CEnvironmentCollector::ReadBlockAddress(HANDLE hProcess, ULONGLONG& address) const
{
    // Proces pokrenut kroz WOW64 ima vlastiti 32-bitni PEB; ako upit vrati
    // njegovu adresu, dalje se citaju 32-bitni pokazivaci.
    ULONG_PTR peb32 = 0;
    if (m_ntApi.QueryProcess(hProcess, ProcessWow64Information, &peb32, sizeof(peb32)) &&
        peb32 != 0)
    {
        return ReadBlockAddress32(hProcess, peb32, address);
    }

    PROCESS_BASIC_INFORMATION info = {};
    if (!m_ntApi.QueryProcess(hProcess, ProcessBasicInformation, &info, sizeof(info)))
        return false;

    const ULONGLONG peb = reinterpret_cast<ULONGLONG>(info.PebBaseAddress);
    if (peb == 0)
        return false;

    ULONGLONG parameters = 0;
    if (!ReadValue(hProcess, peb + peb64Parameters, &parameters, sizeof(parameters)))
        return false;

    if (parameters == 0)
        return false;

    return ReadValue(hProcess, parameters + params64Environment, &address, sizeof(address));
}

bool CEnvironmentCollector::ReadBlockAddress32(HANDLE hProcess, ULONGLONG peb32,
                                               ULONGLONG& address) const
{
    DWORD parameters = 0;
    if (!ReadValue(hProcess, peb32 + peb32Parameters, &parameters, sizeof(parameters)))
        return false;

    if (parameters == 0)
        return false;

    DWORD block = 0;
    if (!ReadValue(hProcess, parameters + params32Environment, &block, sizeof(block)))
        return false;

    address = block;
    return true;
}

bool CEnvironmentCollector::ReadValue(HANDLE hProcess, ULONGLONG address,
                                      LPVOID buffer, SIZE_T size) const
{
    SIZE_T read = 0;

    if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &read))
        return false;

    return (read == size);
}

void CEnvironmentCollector::ReadBlock(HANDLE hProcess, ULONGLONG address)
{
    // Blok je uvijek zapisan dvobajtnim znakovima, bez obzira na to je li
    // proces 32-bitni ili 64-bitni.
    std::vector<wchar_t> buffer;

    while (buffer.size() < maxChars)
    {
        const size_t offset = buffer.size();
        buffer.resize(offset + chunkChars);

        if (!ReadValue(hProcess, address + offset * sizeof(wchar_t),
                       &buffer[offset], chunkChars * sizeof(wchar_t)))
        {
            buffer.resize(offset);
            break;
        }

        // Dvije uzastopne nule zavrsavaju blok; prva je kraj zadnje varijable.
        bool bComplete = false;

        for (size_t i = offset; i + 1 < buffer.size(); ++i)
        {
            if (buffer[i] == 0 && buffer[i + 1] == 0)
            {
                bComplete = true;
                break;
            }
        }

        if (bComplete)
            break;
    }

    Parse(buffer);
}

void CEnvironmentCollector::Parse(const std::vector<wchar_t>& buffer)
{
    size_t start = 0;

    while (start < buffer.size() && buffer[start] != 0)
    {
        size_t end = start;
        while (end < buffer.size() && buffer[end] != 0)
            ++end;

        const CString text(&buffer[start], static_cast<int>(end - start));

        // Neke varijable sustava pocinju znakom jednakosti, primjerice "=C:",
        // i nose radni direktorij pojedinog pogona. Zato se razdvajanje trazi
        // tek od drugog znaka, da taj znak ostane dio naziva.
        const int split = text.Find(_T('='), 1);

        if (split > 0)
        {
            CEnvironmentVariable item;
            item.name  = text.Left(split);
            item.value = text.Mid(split + 1);

            m_items.push_back(item);
        }

        start = end + 1;
    }
}
