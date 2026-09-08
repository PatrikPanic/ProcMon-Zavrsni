#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ChildFrm.h"
#include "ProcessView.h"
#include "ThreadView.h"
#include "ModuleView.h"
#include "MemoryView.h"
#include "GraphView.h"
#include "EnvironmentView.h"
#include "HexView.h"
#include "HandleView.h"
#include "StringView.h"
#include "SysUtil.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

CChildFrame::CChildFrame()
{
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CMDIChildWndEx::PreCreateWindow(cs))
        return FALSE;

    return TRUE;
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    // Vrsta pogleda poznata je vec ovdje, prije nego se kartica pojavi na
    // traci, pa se naslov odreduje odmah i poslije se vise ne mijenja.
    if (pContext != nullptr)
        m_strTitle = TitleForView(pContext->m_pNewViewClass);

    return CMDIChildWndEx::OnCreateClient(lpcs, pContext);
}

CString CChildFrame::TitleForView(CRuntimeClass* pViewClass)
{
    if (pViewClass == RUNTIME_CLASS(CProcessView))
        return CSysUtil::LoadStr(IDS_TITLE_PROCESSES);

    if (pViewClass == RUNTIME_CLASS(CThreadView))
        return CSysUtil::LoadStr(IDS_TITLE_THREADS);

    if (pViewClass == RUNTIME_CLASS(CModuleView))
        return CSysUtil::LoadStr(IDS_TITLE_MODULES);

    if (pViewClass == RUNTIME_CLASS(CMemoryView))
        return CSysUtil::LoadStr(IDS_TITLE_MEMORY);

    if (pViewClass == RUNTIME_CLASS(CGraphView))
        return CSysUtil::LoadStr(IDS_TITLE_GRAPH);

    if (pViewClass == RUNTIME_CLASS(CEnvironmentView))
        return CSysUtil::LoadStr(IDS_TITLE_ENVIRONMENT);

    if (pViewClass == RUNTIME_CLASS(CHexView))
        return CSysUtil::LoadStr(IDS_TITLE_HEX);

    if (pViewClass == RUNTIME_CLASS(CHandleView))
        return CSysUtil::LoadStr(IDS_TITLE_HANDLES);

    if (pViewClass == RUNTIME_CLASS(CStringView))
        return CSysUtil::LoadStr(IDS_TITLE_STRINGS);

    return CString();
}

CString CChildFrame::GetFrameText() const
{
    return m_strTitle.IsEmpty() ? CMDIChildWndEx::GetFrameText() : m_strTitle;
}

void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
    if (m_strTitle.IsEmpty())
    {
        CMDIChildWndEx::OnUpdateFrameTitle(bAddToTitle);
        return;
    }

    // Osnovna izvedba poziva se s FALSE kako bi osvjezila glavni okvir, ali ne
    // i naslov kartice. S TRUE bi dopisala naziv dokumenta i redni broj
    // prozora, a sve kartice dijele isti dokument.
    CMDIChildWndEx::OnUpdateFrameTitle(FALSE);
    SetWindowText(m_strTitle);
}

void CChildFrame::ActivateFrame(int nCmdShow)
{
    CMDIChildWndEx::ActivateFrame(nCmdShow);

    // Iz izbornika prozora uklanja se naredba za zatvaranje, da korisniku ne bi
    // bila ponudena mogucnost koja se ionako nece izvrsiti.
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
        pSysMenu->RemoveMenu(SC_CLOSE, MF_BYCOMMAND);
}

void CChildFrame::OnClose()
{
    // Zahtjev za zatvaranjem kartice se zanemaruje. Zatvaranje glavnog prozora
    // ne prolazi ovuda, nego kroz zatvaranje dokumenta, pa aplikacija i dalje
    // moze normalno zavrsiti.
}

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
    CMDIChildWndEx::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
    CMDIChildWndEx::Dump(dc);
}
#endif
