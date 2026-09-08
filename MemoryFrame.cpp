#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "MemoryFrame.h"
#include "MemoryMapView.h"
#include "MemoryView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMemoryFrame, CChildFrame)

CMemoryFrame::CMemoryFrame()
{
}

CMemoryFrame::~CMemoryFrame()
{
}

BOOL CMemoryFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    if (pContext == nullptr)
        return FALSE;

    // Naslov kartice odreduje se iz vrste pogleda upisane u predlozak, kao i
    // kod ostalih kartica.
    m_strTitle = TitleForView(pContext->m_pNewViewClass);

    if (!m_splitter.CreateStatic(this, 2, 1))
        return FALSE;

    // Oba pogleda rade nad istim dokumentom jer ga podijeljeni prozor preuzima
    // iz konteksta stvaranja.
    if (!m_splitter.CreateView(0, 0, RUNTIME_CLASS(CMemoryMapView),
                               CSize(0, mapHeight), pContext))
    {
        return FALSE;
    }

    if (!m_splitter.CreateView(1, 0, RUNTIME_CLASS(CMemoryView),
                               CSize(0, 0), pContext))
    {
        return FALSE;
    }

    // Pocetna visina gornjeg dijela i najmanja na koju se moze smanjiti; ispod
    // toga se od crteza ne bi vidjelo nista korisno.
    m_splitter.SetRowInfo(0, mapHeight, 60);

    return TRUE;
}
