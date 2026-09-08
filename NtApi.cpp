#include "pch.h"
#include "NtApi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CNtApi::CNtApi()
    : m_pfnQueryInformationProcess(nullptr),
      m_pfnQuerySystemInformation(nullptr),
      m_pfnQueryObject(nullptr)
{
    HMODULE hModule = GetModuleHandle(_T("ntdll.dll"));
    if (hModule == nullptr)
        return;

    m_pfnQueryInformationProcess = reinterpret_cast<PFN_QUERY_INFORMATION_PROCESS>(
        GetProcAddress(hModule, "NtQueryInformationProcess"));

    m_pfnQuerySystemInformation = reinterpret_cast<PFN_QUERY_SYSTEM_INFORMATION>(
        GetProcAddress(hModule, "NtQuerySystemInformation"));

    m_pfnQueryObject = reinterpret_cast<PFN_QUERY_OBJECT>(
        GetProcAddress(hModule, "NtQueryObject"));
}

bool CNtApi::QuerySystem(ULONG infoClass, PVOID buffer, ULONG size, ULONG* pNeeded) const
{
    if (m_pfnQuerySystemInformation == nullptr)
        return false;

    ULONG needed = 0;

    const NTSTATUS status = m_pfnQuerySystemInformation(infoClass, buffer, size, &needed);

    if (pNeeded != nullptr)
        *pNeeded = needed;

    return (status >= 0);
}

bool CNtApi::QueryObject(HANDLE hObject, ULONG infoClass,
                         PVOID buffer, ULONG size, ULONG* pNeeded) const
{
    if (m_pfnQueryObject == nullptr)
        return false;

    ULONG needed = 0;

    const NTSTATUS status = m_pfnQueryObject(hObject, infoClass, buffer, size, &needed);

    if (pNeeded != nullptr)
        *pNeeded = needed;

    return (status >= 0);
}

bool CNtApi::QueryProcess(HANDLE hProcess, PROCESSINFOCLASS infoClass,
                          PVOID buffer, ULONG size) const
{
    if (m_pfnQueryInformationProcess == nullptr)
        return false;

    // Funkcije iz ntdll.dll ne vracaju oznaku greske kroz GetLastError, nego
    // vrijednost tipa NTSTATUS; negativna znaci neuspjeh.
    const NTSTATUS status =
        m_pfnQueryInformationProcess(hProcess, infoClass, buffer, size, nullptr);

    return (status >= 0);
}
