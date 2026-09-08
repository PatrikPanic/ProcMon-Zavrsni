#pragma once

#include "ProcessCollector.h"

#include <vector>

// Jedno ocitanje opterecenja u vremenu.
struct CHistorySample
{
    double    systemCpu         = 0.0;      // udio zauzeca procesora u postocima
    double    processCpu        = 0.0;
    ULONGLONG systemUsed        = 0;        // zauzeta radna memorija u bajtovima
    ULONGLONG processWorkingSet = 0;
    bool      hasProcess        = false;    // je li proces tada bio odabran
};

// CHistoryCollector - pamti zadnjih maxSamples ocitanja opterecenja procesora
// i radne memorije, za sustav u cjelini i za odabrani proces.
class CHistoryCollector
{
public:
    CHistoryCollector();

    enum { maxSamples = 120 };

    // Dodaje ocitanje. Ako proces nije zadan, biljezi se samo stanje sustava.
    void Add(const CProcessInfo* pSelected);

    // Brise podatke o procesu iz svih ocitanja; poziva se kad se promijeni
    // odabir, da se u istoj krivulji ne nadu dva razlicita procesa.
    void ResetProcess();

    const std::vector<CHistorySample>& GetAll() const { return m_items; }

    ULONGLONG GetTotalMemory() const { return m_totalMemory; }

private:
    // Udio zauzeca procesora od prethodnog ocitanja.
    double    ReadSystemCpu();
    ULONGLONG ReadSystemMemory();

    std::vector<CHistorySample> m_items;
    ULONGLONG                   m_previousIdle;    // vrijeme neradne dretve
    ULONGLONG                   m_previousTotal;   // ukupno vrijeme procesora
    ULONGLONG                   m_totalMemory;
};
