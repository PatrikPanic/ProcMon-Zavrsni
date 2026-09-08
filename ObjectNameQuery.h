#pragma once

#include "NtApi.h"

// Podaci koje dijele glavna i pomocna nit. Struktura se ne unistava zajedno s
// razrednikom, nego je oslobada nit koja je zadnja koristi.
struct CNameQueryContext
{
    HANDLE  hRequest  = nullptr;    // glavna nit trazi upit
    HANDLE  hDone     = nullptr;    // pomocna nit javlja da je gotov
    HANDLE  hObject   = nullptr;    // objekt o kojem se pita
    CString name;                   // odgovor
    LONG    exit      = 0;          // zahtjev za urednim zavrsetkom
    LONG    abandoned = 0;          // glavna nit vise ne ceka odgovor
};

// CObjectNameQuery - dohvat naziva objekta u pomocnoj niti. Upit za nazivom
// zna se zaustaviti do kraja rada procesa, primjerice na cjevovodu koji ceka
// drugu stranu, pa ga glavna nit ne smije postaviti izravno.
class CObjectNameQuery
{
public:
    CObjectNameQuery(const CNtApi& ntApi);
    ~CObjectNameQuery();

    // Vraca naziv objekta ili prazan string ako ga nema odnosno ako upit nije
    // zavrsio na vrijeme.
    CString Query(HANDLE hObject);

    // Koliko je niti do sada napusteno zbog zaustavljenog upita.
    int GetAbandonedCount() const { return m_abandoned; }

private:
    enum
    {
        timeoutMs     = 100,    // koliko se ceka odgovor
        maxAbandoned  = 5       // nakon toliko zaustavljanja nazivi se preskacu
    };

    static UINT __cdecl ThreadProc(LPVOID pParam);
    static void Run(CNameQueryContext* pContext, const CNtApi* pNtApi);

    bool Start();
    void Stop();

    const CNtApi&      m_ntApi;
    CNameQueryContext* m_pContext;
    int                m_abandoned;
};
