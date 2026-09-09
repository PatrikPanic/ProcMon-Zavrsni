#pragma once

#include "NtApi.h"
#include "ObjectNameQuery.h"

#include <map>
#include <vector>

// Jedan otvoreni handle procesa.
struct CHandleInfo
{
    ULONGLONG value     = 0;    // vrijednost handle-a unutar ciljnog procesa
    DWORD     access    = 0;    // dodijeljena prava pristupa
    USHORT    typeIndex = 0;    // redni broj vrste objekta u jezgri
    CString   type;             // vrsta objekta, primjerice datoteka ili kljuc
    CString   name;             // naziv objekta, ako ga ima
};

// CHandleCollector - popis objekata koje je zadani proces otvorio. Tablicu svih
// handle-ova u sustavu daje ntdll.dll, a vrsta i naziv pojedinog objekta
// dohvacaju se tek nakon sto se handle preslika u ovaj proces.
class CHandleCollector
{
public:
    CHandleCollector();

    // Ako je pid jednak nuli, popis se samo prazni.
    void Refresh(DWORD pid);

    const std::vector<CHandleInfo>& GetAll() const { return m_items; }

    // Je li zadnje ocitanje uspjelo; bez prava preslikavanja handle-ova ne moze.
    bool IsAccessible() const { return m_accessible; }

private:
    enum { maxHandles = 20000 };

    bool ReadTable(std::vector<BYTE>& buffer) const;

    CString ReadType(HANDLE hObject) const;

    // Popunjava vrstu onim zapisima kojima preslikavanje nije uspjelo, prema
    // vrstama prepoznatim kod ostalih handle-ova istog rednog broja.
    void FillMissingTypes(const std::map<USHORT, CString>& types);

    std::vector<CHandleInfo> m_items;
    CNtApi                   m_ntApi;

    // Naziv objekta trazi pomocna nit, jer se taj upit kod nekih objekata zna
    // zaustaviti do kraja rada procesa. Upit zivi koliko i sam sakupljac: kad
    // bi se stvarao pri svakom ocitanju, brojac napustenih niti krenuo bi
    // iznova od nule, pa bi ponovljena ocitanja ostavljala za sobom sve vise
    // napustenih niti i njihovih handle-ova.
    CObjectNameQuery         m_nameQuery;

    bool                     m_accessible = false;
};
