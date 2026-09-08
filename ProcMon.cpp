#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "ProcMon.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "ProcMonDoc.h"
#include "ProcessView.h"
#include "ThreadView.h"
#include "ModuleView.h"
#include "MemoryView.h"
#include "MemoryFrame.h"
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

BEGIN_MESSAGE_MAP(CProcMonApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, &CProcMonApp::OnAppAbout)
END_MESSAGE_MAP()

// Jedini primjerak aplikacije; MFC zahtijeva da bude globalan.
CProcMonApp theApp;

CProcMonApp::CProcMonApp()
    : m_pProcessTemplate(nullptr), m_pThreadTemplate(nullptr),
      m_pModuleTemplate(nullptr), m_pMemoryTemplate(nullptr),
      m_pGraphTemplate(nullptr), m_pEnvironmentTemplate(nullptr),
      m_pHexTemplate(nullptr), m_pHandleTemplate(nullptr),
      m_pStringTemplate(nullptr)
{
    SetAppID(_T("Vsite.ProcMon.1"));
}

BOOL CProcMonApp::InitInstance()
{
    INITCOMMONCONTROLSEX initCtrls = {};
    initCtrls.dwSize = sizeof(initCtrls);
    initCtrls.dwICC  = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&initCtrls);

    CWinAppEx::InitInstance();

    EnableTaskbarInteraction(FALSE);

    SetRegistryKey(_T("Vsite"));

    InitContextMenuManager();
    InitKeyboardManager();
    InitTooltipManager();

    CMFCToolTipInfo params;
    params.m_bVislManagerTheme = TRUE;
    GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
                                          RUNTIME_CLASS(CMFCToolTipCtrl), &params);

    // Bez ovlasti SeDebugPrivilege vidljiv je samo dio podataka o procesima.
    if (!CSysUtil::EnableDebugPrivilege())
        AfxMessageBox(CSysUtil::LoadStr(IDS_WARN_NO_DEBUG_PRIV), MB_OK | MB_ICONINFORMATION);

    m_pProcessTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CProcessView));
    if (m_pProcessTemplate == nullptr)
        return FALSE;
    AddDocTemplate(m_pProcessTemplate);

    // Predlosci za dretve i module namjerno se ne dodaju u popis predlozaka jer
    // se iz njih ne otvaraju novi dokumenti, nego samo dodatni pogledi.
    m_pThreadTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CThreadView));

    m_pModuleTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CModuleView));

    // Kartica s mapom memorije koristi vlastiti okvir jer sadrzi podijeljeni
    // prozor s dva pogleda.
    m_pMemoryTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CMemoryFrame),
        RUNTIME_CLASS(CMemoryView));

    m_pGraphTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CGraphView));

    m_pHexTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CHexView));

    m_pHandleTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CHandleView));

    m_pStringTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CStringView));

    m_pEnvironmentTemplate = new CMultiDocTemplate(
        IDR_ProcMonTYPE,
        RUNTIME_CLASS(CProcMonDoc),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CEnvironmentView));

    CMainFrame* pMainFrame = new CMainFrame;
    if (pMainFrame == nullptr || !pMainFrame->LoadFrame(IDR_MAINFRAME))
    {
        delete pMainFrame;
        return FALSE;
    }
    m_pMainWnd = pMainFrame;

    // Aplikacija ne otvara datoteke, nego odmah prikazuje stanje sustava.
    // S dokumentom nastaje i prva kartica, ona s popisom procesa.
    CDocument* pDoc = m_pProcessTemplate->OpenDocumentFile(nullptr);
    if (pDoc == nullptr)
        return FALSE;

    CMDIChildWnd* pProcessFrame = pMainFrame->MDIGetActive();

    CreateAdditionalView(m_pThreadTemplate, pDoc);
    CreateAdditionalView(m_pModuleTemplate, pDoc);
    CreateAdditionalView(m_pMemoryTemplate, pDoc);
    CreateAdditionalView(m_pHexTemplate, pDoc);
    CreateAdditionalView(m_pHandleTemplate, pDoc);
    CreateAdditionalView(m_pStringTemplate, pDoc);
    CreateAdditionalView(m_pEnvironmentTemplate, pDoc);
    CreateAdditionalView(m_pGraphTemplate, pDoc);

    // Nakon otvaranja aktivna je zadnja kartica, pa se odabir vraca na popis
    // procesa jer se iz njega upravlja ostalim prikazima.
    if (pProcessFrame != nullptr)
        pProcessFrame->MDIActivate();

    pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
    pMainFrame->UpdateWindow();

    return TRUE;
}

void CProcMonApp::CreateAdditionalView(CMultiDocTemplate* pTemplate, CDocument* pDoc)
{
    if (pTemplate == nullptr || pDoc == nullptr)
        return;

    CFrameWnd* pFrame = pTemplate->CreateNewFrame(pDoc, nullptr);
    if (pFrame != nullptr)
        pTemplate->InitialUpdateFrame(pFrame, pDoc);
}

int CProcMonApp::ExitInstance()
{
    // Predlosci koji nisu dodani u popis moraju se osloboditi rucno.
    delete m_pThreadTemplate;
    delete m_pModuleTemplate;
    delete m_pMemoryTemplate;
    delete m_pGraphTemplate;
    delete m_pEnvironmentTemplate;
    delete m_pHexTemplate;
    delete m_pHandleTemplate;
    delete m_pStringTemplate;

    m_pThreadTemplate = nullptr;
    m_pModuleTemplate = nullptr;
    m_pMemoryTemplate = nullptr;
    m_pGraphTemplate  = nullptr;
    m_pEnvironmentTemplate = nullptr;
    m_pHexTemplate         = nullptr;
    m_pHandleTemplate      = nullptr;
    m_pStringTemplate      = nullptr;

    return CWinAppEx::ExitInstance();
}

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

void CProcMonApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}
