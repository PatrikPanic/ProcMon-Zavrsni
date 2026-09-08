#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "MainFrm.h"
#include "ProcMonDoc.h"
#include "HexView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#include "afxvisualmanageroffice2007.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
    ON_WM_CREATE()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_COMMAND(ID_CMD_FILTER, &CMainFrame::OnFilterChanged)
    ON_COMMAND(ID_CMD_ANALYZE, &CMainFrame::OnAnalyze)
    ON_UPDATE_COMMAND_UI(ID_CMD_ANALYZE, &CMainFrame::OnUpdateNeedsProcess)
    ON_COMMAND(ID_CMD_HEX, &CMainFrame::OnHexView)
    ON_UPDATE_COMMAND_UI(ID_CMD_HEX, &CMainFrame::OnUpdateNeedsProcess)
    ON_COMMAND(ID_CMD_ADDRESS, &CMainFrame::OnAddressChanged)
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Kartice se ne mogu zatvoriti, pa se uklanjaju oba gumba za zatvaranje:
    // onaj na aktivnoj kartici i onaj u desnom kutu trake s karticama.
    CMDITabInfo tabInfo;
    tabInfo.m_style                  = CMFCTabCtrl::STYLE_3D_ONENOTE;
    tabInfo.m_bTabCloseButton        = FALSE;
    tabInfo.m_bActiveTabCloseButton  = FALSE;
    tabInfo.m_bTabIcons              = FALSE;
    tabInfo.m_bAutoColor             = TRUE;
    tabInfo.m_bDocumentMenu          = FALSE;
    EnableMDITabbedGroups(TRUE, tabInfo);

    CreateRibbon();

    if (!m_wndStatusBar.Create(this))
        return -1;

    m_wndStatusBar.AddElement(
        new CMFCRibbonStatusBarPane(ID_CMD_STATUS_PANE,
                                    CSysUtil::LoadStr(IDS_STATUS_READY), TRUE),
        _T(""));

    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
    CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);

    // Mjerac vremena drzi glavni okvir, a ne pojedini pogled, pa osvjezavanje
    // radi neovisno o tome koja je kartica prikazana.
    SetTimer(timerRefresh, refreshIntervalMs, nullptr);

    return 0;
}

void CMainFrame::OnDestroy()
{
    KillTimer(timerRefresh);
    CMDIFrameWndEx::OnDestroy();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == timerRefresh)
    {
        CProcMonDoc* pDoc = GetInspectorDoc();
        if (pDoc != nullptr && pDoc->IsAutoRefresh())
            pDoc->RefreshData();
    }

    CMDIFrameWndEx::OnTimer(nIDEvent);
}

void CMainFrame::CreateRibbon()
{
    m_wndRibbonBar.Create(this);
    m_wndRibbonBar.SetWindows7Look(FALSE);

    // Kategorija bez slikovnih resursa, gumbi su tekstualni.
    CMFCRibbonCategory* pCategory =
        m_wndRibbonBar.AddCategory(CSysUtil::LoadStr(IDS_RIBBON_CATEGORY), 0, 0);

    CMFCRibbonPanel* pPanelRefresh = pCategory->AddPanel(CSysUtil::LoadStr(IDS_PANEL_REFRESH));
    pPanelRefresh->Add(new CMFCRibbonButton(ID_CMD_REFRESH,
                                            CSysUtil::LoadStr(IDS_CMD_REFRESH), -1, -1));
    pPanelRefresh->Add(new CMFCRibbonCheckBox(ID_CMD_AUTOREFRESH,
                                              CSysUtil::LoadStr(IDS_CMD_AUTOREFRESH)));

    CMFCRibbonPanel* pPanelProcess = pCategory->AddPanel(CSysUtil::LoadStr(IDS_PANEL_PROCESS));
    pPanelProcess->Add(new CMFCRibbonButton(ID_CMD_KILL_PROCESS,
                                            CSysUtil::LoadStr(IDS_CMD_KILL), -1, -1));
    pPanelProcess->Add(new CMFCRibbonButton(ID_CMD_ANALYZE,
                                            CSysUtil::LoadStr(IDS_CMD_ANALYZE), -1, -1));

    CMFCRibbonPanel* pPanelMemory = pCategory->AddPanel(CSysUtil::LoadStr(IDS_PANEL_MEMORY));
    pPanelMemory->Add(new CMFCRibbonButton(ID_CMD_HEX,
                                           CSysUtil::LoadStr(IDS_CMD_HEX), -1, -1));
    pPanelMemory->Add(new CMFCRibbonEdit(ID_CMD_ADDRESS, 150,
                                         CSysUtil::LoadStr(IDS_ADDRESS_LABEL)));

    CMFCRibbonPanel* pPanelView = pCategory->AddPanel(CSysUtil::LoadStr(IDS_PANEL_VIEW));
    pPanelView->Add(new CMFCRibbonCheckBox(ID_CMD_TREE,
                                           CSysUtil::LoadStr(IDS_CMD_TREE)));

    CMFCRibbonPanel* pPanelFilter = pCategory->AddPanel(CSysUtil::LoadStr(IDS_PANEL_FILTER));
    pPanelFilter->Add(new CMFCRibbonEdit(ID_CMD_FILTER, 120,
                                            CSysUtil::LoadStr(IDS_FILTER_LABEL)));

    // Gumb se smjesta desno od trake s karticama; naredbu obraduje klasa aplikacije.
    m_wndRibbonBar.AddToTabs(new CMFCRibbonButton(ID_APP_ABOUT,
                                        CSysUtil::LoadStr(IDS_CMD_ABOUT), -1, -1));
}

void CMainFrame::OnFilterChanged()
{
    CMFCRibbonEdit* pEdit =
        DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_wndRibbonBar.FindByID(ID_CMD_FILTER));
    if (pEdit == nullptr)
        return;

    CProcMonDoc* pDoc = GetInspectorDoc();
    if (pDoc != nullptr)
        pDoc->SetFilter(pEdit->GetEditText());
}

// Naredbe obraduje okvir, a ne dokument, jer uz ocitanje jos i prebacuju prikaz
// na pripadnu karticu, a kartice su posao okvira.
// Prikaz ostaje na kartici na kojoj je korisnik bio, jer se popunjavaju sve
// odjednom pa nema jedne na koju bi imalo smisla prebaciti.
void CMainFrame::OnAnalyze()
{
    CProcMonDoc* pDoc = GetInspectorDoc();
    if (pDoc == nullptr)
        return;

    pDoc->AnalyzeProcess();
}

// Naredbe nad odabranim procesom dostupne su tek kad je proces odabran.
void CMainFrame::OnUpdateNeedsProcess(CCmdUI* pCmdUI)
{
    CProcMonDoc* pDoc = GetInspectorDoc();
    pCmdUI->Enable(pDoc != nullptr && pDoc->GetSelectedPid() != 0);
}

// Prikaz se otvara na adresi odabrane regije; ako regija nije odabrana, uzima
// se pocetak mape.
void CMainFrame::OnHexView()
{
    CProcMonDoc* pDoc = GetInspectorDoc();
    if (pDoc == nullptr)
        return;

    const std::vector<CRegionInfo>& items = pDoc->GetRegions();
    const int selected = pDoc->GetSelectedRegion();

    if (selected >= 0 && static_cast<size_t>(selected) < items.size())
        pDoc->SetHexAddress(items[selected].baseAddress);
    else if (!items.empty())
        pDoc->SetHexAddress(items[0].baseAddress);

    ActivateView(RUNTIME_CLASS(CHexView));
}

void CMainFrame::OnAddressChanged()
{
    CMFCRibbonEdit* pEdit =
        DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_wndRibbonBar.FindByID(ID_CMD_ADDRESS));
    if (pEdit == nullptr)
        return;

    CProcMonDoc* pDoc = GetInspectorDoc();
    if (pDoc == nullptr)
        return;

    // Adresa se uvijek upisuje heksadekadski, sa ili bez pocetnog "0x".
    CString text = pEdit->GetEditText();
    text.Trim();

    if (text.Left(2).CompareNoCase(_T("0x")) == 0)
        text = text.Mid(2);

    if (text.IsEmpty())
        return;

    LPTSTR pEnd = nullptr;
    const ULONGLONG address = _tcstoui64(text, &pEnd, 16);

    // Nepotpun ili neispravan upis se zanemaruje; korisnik jos tipka.
    if (pEnd == nullptr || *pEnd != _T('\0'))
        return;

    pDoc->SetHexAddress(address);
}

void CMainFrame::SetAddressText(ULONGLONG address)
{
    CMFCRibbonEdit* pEdit =
        DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_wndRibbonBar.FindByID(ID_CMD_ADDRESS));
    if (pEdit == nullptr)
        return;

    CString text;
    text.Format(_T("%016llX"), address);

    if (pEdit->GetEditText() != text)
        pEdit->SetEditText(text);
}

void CMainFrame::ActivateView(CRuntimeClass* pViewClass)
{
    CProcMonDoc* pDoc = GetInspectorDoc();
    if (pDoc == nullptr)
        return;

    POSITION pos = pDoc->GetFirstViewPosition();
    while (pos != nullptr)
    {
        CView* pView = pDoc->GetNextView(pos);
        if (pView == nullptr || !pView->IsKindOf(pViewClass))
            continue;

        CMDIChildWnd* pChild = DYNAMIC_DOWNCAST(CMDIChildWnd, pView->GetParentFrame());
        if (pChild != nullptr)
            pChild->MDIActivate();

        return;
    }
}

void CMainFrame::SetStatusText(LPCTSTR lpszText)
{
    CMFCRibbonStatusBarPane* pPane =
        DYNAMIC_DOWNCAST(CMFCRibbonStatusBarPane, m_wndStatusBar.FindElement(ID_CMD_STATUS_PANE));

    if (pPane != nullptr)
    {
        pPane->SetText(lpszText);
        m_wndStatusBar.Invalidate();
        m_wndStatusBar.UpdateWindow();
    }
}

CProcMonDoc* CMainFrame::GetInspectorDoc() const
{
    // Sve kartice dijele isti dokument, pa je dovoljno pitati aktivnu.
    CMDIChildWnd* pChild = const_cast<CMainFrame*>(this)->MDIGetActive();
    if (pChild == nullptr)
        return nullptr;

    return DYNAMIC_DOWNCAST(CProcMonDoc, pChild->GetActiveDocument());
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CMDIFrameWndEx::Dump(dc);
}
#endif
