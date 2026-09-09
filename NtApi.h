#pragma once

#include <winternl.h>

// CNtApi - pristup funkcijama iz ntdll.dll koje nemaju uvoznu biblioteku, pa se
// njihove adrese dohvacaju tijekom rada. Knjiznica se ne ucitava izricito jer
// je ntdll.dll ucitan u svaki proces, dovoljno je zatraziti njezinu oznaku.
class CNtApi
{
public:
    CNtApi();

    // Postavlja podatak o procesu u zadani spremnik; vraca true ako je upit
    // uspio.
    bool QueryProcess(HANDLE hProcess, PROCESSINFOCLASS infoClass,
                      PVOID buffer, ULONG size) const;

    // Podaci o cijelom sustavu, primjerice tablica svih otvorenih handle-ova.
    // U pNeeded se vraca velicina koju sustav trazi, pa se spremnik moze
    // povecati i upit ponoviti.
    bool QuerySystem(ULONG infoClass, PVOID buffer, ULONG size, ULONG* pNeeded) const;

    // Podaci o jednom objektu, primjerice njegova vrsta ili naziv.
    bool QueryObject(HANDLE hObject, ULONG infoClass,
                     PVOID buffer, ULONG size, ULONG* pNeeded) const;

private:
    typedef NTSTATUS (NTAPI* PFN_QUERY_INFORMATION_PROCESS)(
        HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

    typedef NTSTATUS (NTAPI* PFN_QUERY_SYSTEM_INFORMATION)(
        ULONG, PVOID, ULONG, PULONG);

    typedef NTSTATUS (NTAPI* PFN_QUERY_OBJECT)(
        HANDLE, ULONG, PVOID, ULONG, PULONG);

    PFN_QUERY_INFORMATION_PROCESS m_pfnQueryInformationProcess;
    PFN_QUERY_SYSTEM_INFORMATION  m_pfnQuerySystemInformation;
    PFN_QUERY_OBJECT              m_pfnQueryObject;
};
