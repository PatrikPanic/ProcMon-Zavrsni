#pragma once

#include <vector>

// Podaci o jednoj regiji virtualnog adresnog prostora. Regija je niz stranica
// s jednakim stanjem, zastitom i tipom; sustav ih tako i vraca.
struct CRegionInfo
{
    ULONGLONG baseAddress    = 0;
    ULONGLONG allocationBase = 0;   // pocetak rezervacije kojoj regija pripada
    ULONGLONG size           = 0;   // velicina u bajtovima
    DWORD     state          = 0;   // MEM_COMMIT, MEM_RESERVE ili MEM_FREE
    DWORD     protect        = 0;   // PAGE_* zastita pristupa
    DWORD     type           = 0;   // MEM_IMAGE, MEM_MAPPED ili MEM_PRIVATE
    CString   mappedFile;           // datoteka preslikana u regiju, ako postoji
};

// CMemoryCollector - obilazi virtualni adresni prostor zadanog procesa i
// biljezi sve regije, ukljucujuci slobodne.
class CMemoryCollector
{
public:
    // Ako je pid jednak nuli, popis se samo prazni.
    void Refresh(DWORD pid);

    const std::vector<CRegionInfo>& GetAll() const { return m_items; }

    // Je li zadnje ocitanje uspjelo; kod zasticenih procesa ne uspije.
    bool IsAccessible() const { return m_accessible; }

    ULONGLONG GetCommittedBytes() const { return m_committedBytes; }
    ULONGLONG GetReservedBytes() const { return m_reservedBytes; }

    // Opisi vrijednosti iz strukture regije, pripremljeni za ispis.
    static CString FormatState(DWORD state);
    static CString FormatType(DWORD type);
    static CString FormatProtection(DWORD protect);

private:
    void AddRegion(HANDLE hProcess, const MEMORY_BASIC_INFORMATION& mbi);

    // Vraca putanju datoteke preslikane na zadanu adresu ili prazan string.
    static CString ReadMappedFileName(HANDLE hProcess, LPVOID address);

    std::vector<CRegionInfo> m_items;
    ULONGLONG                m_committedBytes = 0;
    ULONGLONG                m_reservedBytes  = 0;
    bool                     m_accessible     = false;
};
