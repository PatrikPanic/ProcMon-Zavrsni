#pragma once

#include "NtApi.h"

#include <vector>

// Jedna varijabla okoline procesa.
struct CEnvironmentVariable
{
    CString name;
    CString value;
};

// CEnvironmentCollector - cita blok varijabli okoline zadanog procesa. Do bloka
// se dolazi kroz strukturu PEB, koju sustav ne objavljuje u zaglavljima, pa se
// citaju samo pojedina polja na poznatim odmacima.
class CEnvironmentCollector
{
public:
    // Ako je pid jednak nuli, popis se samo prazni.
    void Refresh(DWORD pid);

    const std::vector<CEnvironmentVariable>& GetAll() const { return m_items; }

    // Je li zadnje ocitanje uspjelo; kod zasticenih procesa ne uspije.
    bool IsAccessible() const { return m_accessible; }

private:
    // Trazi adresu bloka okoline; kod 32-bitnih procesa cita se PEB s 32-bitnim
    // pokazivacima.
    bool ReadBlockAddress(HANDLE hProcess, ULONGLONG& address) const;
    bool ReadBlockAddress32(HANDLE hProcess, ULONGLONG peb32, ULONGLONG& address) const;

    bool ReadValue(HANDLE hProcess, ULONGLONG address, LPVOID buffer, SIZE_T size) const;

    void ReadBlock(HANDLE hProcess, ULONGLONG address);
    void Parse(const std::vector<wchar_t>& buffer);

    std::vector<CEnvironmentVariable> m_items;
    CNtApi                            m_ntApi;
    bool                              m_accessible = false;
};
