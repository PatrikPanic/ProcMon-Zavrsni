#pragma once

#include "MemoryCollector.h"

#include <vector>

// CMemoryReader - cita prozor memorije zadanog procesa. Citanje ide u manjim
// komadima jer ReadProcessMemory ne uspije ako je bilo koji dio zatrazenog
// podrucja nedostupan, a tako se izgubi i ono sto se moglo procitati.
class CMemoryReader
{
public:
    enum
    {
        windowSize = 4096,  // velicina prozora u bajtovima
        chunkSize  = 256,   // velicina jednog citanja
        maxSteps   = 512    // najveci broj regija koje se preskacu u potrazi
    };

    // Ako je pid jednak nuli, spremnik se samo prazni.
    void Read(DWORD pid, ULONGLONG address);

    ULONGLONG GetAddress() const { return m_address; }

    // Regija kojoj pripada trenutna adresa; naziv datoteke se ne popunjava.
    const CRegionInfo& GetRegion() const { return m_region; }

    // Trazi najblizu adresu s koje se moze citati, pocevsi od zadane i iduci u
    // zadanom smjeru. Vraca false ako takve adrese nema.
    bool FindReadable(DWORD pid, ULONGLONG address, bool bForward, ULONGLONG& result) const;

    const std::vector<BYTE>& GetData() const { return m_data; }

    // Je li bajt na zadanom odmaku uspjesno procitan.
    bool IsValid(size_t offset) const;

    // Je li ijedan komad prozora bio dostupan.
    bool IsAccessible() const { return m_accessible; }

private:
    // Moze li se iz regije uopce citati.
    static bool IsReadable(const MEMORY_BASIC_INFORMATION& mbi);

    void ReadRegion(HANDLE hProcess, ULONGLONG address);

    CRegionInfo       m_region;
    std::vector<BYTE> m_data;
    std::vector<bool> m_valid;          // jedan clan po komadu citanja
    ULONGLONG         m_address   = 0;
    bool              m_accessible = false;
};
