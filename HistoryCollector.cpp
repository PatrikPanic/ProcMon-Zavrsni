#include "pch.h"
#include "HistoryCollector.h"
#include "SysUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHistoryCollector::CHistoryCollector()
    : m_previousIdle(0), m_previousTotal(0), m_totalMemory(0)
{
}

void CHistoryCollector::Add(const CProcessInfo* pSelected)
{
    CHistorySample sample;
    sample.systemCpu  = ReadSystemCpu();
    sample.systemUsed = ReadSystemMemory();

    if (pSelected != nullptr)
    {
        sample.processCpu        = pSelected->cpuPercent;
        sample.processWorkingSet = pSelected->workingSet;
        sample.hasProcess        = true;
    }

    m_items.push_back(sample);

    // Cuva se samo zadnjih maxSamples ocitanja, pa najstarije ispada.
    if (m_items.size() > maxSamples)
        m_items.erase(m_items.begin());
}

void CHistoryCollector::ResetProcess()
{
    for (size_t i = 0; i < m_items.size(); ++i)
    {
        m_items[i].processCpu        = 0.0;
        m_items[i].processWorkingSet = 0;
        m_items[i].hasProcess        = false;
    }
}

double CHistoryCollector::ReadSystemCpu()
{
    FILETIME ftIdle = {}, ftKernel = {}, ftUser = {};
    if (!GetSystemTimes(&ftIdle, &ftKernel, &ftUser))
        return 0.0;

    const ULONGLONG idle  = CSysUtil::ToUInt64(ftIdle);
    const ULONGLONG total = CSysUtil::ToUInt64(ftKernel) + CSysUtil::ToUInt64(ftUser);

    double percent = 0.0;

    // Prvo ocitanje sluzi samo kao polaziste. Vrijeme neradne dretve vec je
    // ukljuceno u vrijeme jezgre, pa je zauzece razlika ukupnog i neradnog.
    if (m_previousTotal != 0 && total > m_previousTotal && idle >= m_previousIdle)
    {
        const ULONGLONG elapsed   = total - m_previousTotal;
        const ULONGLONG idleDelta = idle - m_previousIdle;

        // Oduzimanje je nad neoznacenim brojevima, pa bi razlika u smjeru koji
        // se ne ocekuje omotala u ogroman broj i jednom podigla grafikon na
        // 100 %; obje se razlike zato provjeravaju prije oduzimanja.
        const ULONGLONG busy = (elapsed > idleDelta) ? (elapsed - idleDelta) : 0;

        percent = (100.0 * busy) / elapsed;
    }

    m_previousIdle  = idle;
    m_previousTotal = total;

    if (percent < 0.0)
        percent = 0.0;

    if (percent > 100.0)
        percent = 100.0;

    return percent;
}

ULONGLONG CHistoryCollector::ReadSystemMemory()
{
    MEMORYSTATUSEX status = {};
    status.dwLength = sizeof(status);

    if (!GlobalMemoryStatusEx(&status))
        return 0;

    m_totalMemory = status.ullTotalPhys;

    return status.ullTotalPhys - status.ullAvailPhys;
}
