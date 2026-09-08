#include "pch.h"
#include "ObjectNameQuery.h"

#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    // Vrsta podatka koju NtQueryObject vraca kao naziv objekta.
    const ULONG objectNameInformation = 1;

    const ULONG nameBufferSize = 2048;
}

CObjectNameQuery::CObjectNameQuery(const CNtApi& ntApi)
    : m_ntApi(ntApi), m_pContext(nullptr), m_abandoned(0)
{
    Start();
}

CObjectNameQuery::~CObjectNameQuery()
{
    Stop();
}

bool CObjectNameQuery::Start()
{
    CNameQueryContext* pContext = new CNameQueryContext;

    pContext->hRequest = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    pContext->hDone    = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    if (pContext->hRequest == nullptr || pContext->hDone == nullptr)
    {
        if (pContext->hRequest != nullptr)
            CloseHandle(pContext->hRequest);

        if (pContext->hDone != nullptr)
            CloseHandle(pContext->hDone);

        delete pContext;
        return false;
    }

    // Nit prima pokazivac na strukturu i preuzima obvezu da je oslobodi kad
    // zavrsi, bez obzira zavrsava li uredno ili je napustena.
    if (AfxBeginThread(ThreadProc, pContext) == nullptr)
    {
        CloseHandle(pContext->hRequest);
        CloseHandle(pContext->hDone);
        delete pContext;
        return false;
    }

    m_pContext = pContext;
    return true;
}

void CObjectNameQuery::Stop()
{
    if (m_pContext == nullptr)
        return;

    InterlockedExchange(&m_pContext->exit, 1);
    SetEvent(m_pContext->hRequest);

    m_pContext = nullptr;
}

CString CObjectNameQuery::Query(HANDLE hObject)
{
    // Kad se previse upita zaustavilo, nazivi se prestaju traziti; ocitanje bi
    // inace za svaki takav objekt trajalo jos jedno vremensko ogranicenje i
    // ostavljalo za sobom nove niti.
    if (m_pContext == nullptr || m_abandoned >= maxAbandoned)
        return CString();

    m_pContext->hObject = hObject;
    m_pContext->name.Empty();

    SetEvent(m_pContext->hRequest);

    if (WaitForSingleObject(m_pContext->hDone, timeoutMs) == WAIT_OBJECT_0)
        return m_pContext->name;

    // Upit se zaustavio. Nit se ne prekida jer bi TerminateThread ostavio
    // zakljucane strukture u ntdll.dll; umjesto toga se napusta zajedno sa
    // svojom strukturom, a za iduce upite se pokrece nova.
    InterlockedExchange(&m_pContext->abandoned, 1);

    // Ako nit ceka novi zahtjev umjesto da je zaglavljena u upitu, ovime se
    // budi da moze zavrsiti.
    SetEvent(m_pContext->hRequest);

    m_pContext = nullptr;
    ++m_abandoned;

    Start();

    return CString();
}

UINT __cdecl CObjectNameQuery::ThreadProc(LPVOID pParam)
{
    CNameQueryContext* pContext = static_cast<CNameQueryContext*>(pParam);

    // Omotac oko ntdll.dll je bez stanja, pa ga nit moze imati vlastitog.
    const CNtApi ntApi;

    Run(pContext, &ntApi);

    CloseHandle(pContext->hRequest);
    CloseHandle(pContext->hDone);

    delete pContext;

    return 0;
}

void CObjectNameQuery::Run(CNameQueryContext* pContext, const CNtApi* pNtApi)
{
    std::vector<BYTE> buffer(nameBufferSize, 0);

    for (;;)
    {
        WaitForSingleObject(pContext->hRequest, INFINITE);

        if (InterlockedCompareExchange(&pContext->exit, 0, 0) != 0 ||
            InterlockedCompareExchange(&pContext->abandoned, 0, 0) != 0)
        {
            return;
        }

        ULONG needed = 0;

        if (pNtApi->QueryObject(pContext->hObject, objectNameInformation,
                                &buffer[0], nameBufferSize, &needed))
        {
            // Struktura pocinje opisom niza znakova, a sam tekst slijedi iza
            // njega u istom spremniku.
            const UNICODE_STRING* pName = reinterpret_cast<const UNICODE_STRING*>(&buffer[0]);

            if (pName->Buffer != nullptr && pName->Length > 0)
                pContext->name = CString(pName->Buffer, pName->Length / sizeof(WCHAR));
        }

        // Ako glavna nit vise ne ceka odgovor, ovdje zavrsava i zivot niti.
        if (InterlockedCompareExchange(&pContext->abandoned, 0, 0) != 0)
            return;

        SetEvent(pContext->hDone);
    }
}
