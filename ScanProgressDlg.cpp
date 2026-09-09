#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "StringIDs.h"
#include "ScanProgressDlg.h"
#include "SysUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    // Poruke kojima pomocna nit javlja stanje glavnoj niti.
    const UINT messageScanUpdate = WM_APP + 1;
    const UINT messageScanDone   = WM_APP + 2;
}

IMPLEMENT_DYNAMIC(CScanProgressDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CScanProgressDlg, CDialogEx)
    ON_MESSAGE(messageScanUpdate, &CScanProgressDlg::OnScanUpdate)
    ON_MESSAGE(messageScanDone, &CScanProgressDlg::OnScanDone)
END_MESSAGE_MAP()

CScanProgressDlg::CScanProgressDlg(CStringScanner& scanner, DWORD pid, CWnd* pParent)
    : CDialogEx(IDD, pParent), m_scanner(scanner), m_pid(pid), m_cancel(0)
{
}

CScanProgressDlg::~CScanProgressDlg()
{
}

void CScanProgressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SCAN_PROGRESS, m_progress);
}

BOOL CScanProgressDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_progress.SetRange(0, 100);
    m_progress.SetPos(0);

    SetDlgItemText(IDC_SCAN_STATUS, CSysUtil::LoadStr(IDS_SCAN_STARTING));

    // Pretraga se pokrece tek kad prozor postoji, jer joj salje poruke.
    if (AfxBeginThread(ThreadProc, this) == nullptr)
    {
        // Prozor se zatvara tek na poruku o kraju pretrage, koju salje upravo
        // ta nit; bez nje bi ostao otvoren bez ijednog nacina zatvaranja.
        AfxMessageBox(CSysUtil::LoadStr(IDS_ERR_SCAN_THREAD), MB_OK | MB_ICONEXCLAMATION);
        EndDialog(IDCANCEL);
    }

    return TRUE;
}

UINT __cdecl CScanProgressDlg::ThreadProc(LPVOID pParam)
{
    CScanProgressDlg* pDialog = static_cast<CScanProgressDlg*>(pParam);

    pDialog->m_scanner.Scan(pDialog->m_pid, pDialog);

    // Prozor se zatvara iz glavne niti, nakon sto pretraga zaista zavrsi.
    pDialog->PostMessage(messageScanDone);

    return 0;
}

void CScanProgressDlg::OnScanProgress(int percent, int found)
{
    // Metodu poziva pomocna nit, pa se prozoru ne smije pristupati izravno
    // nego se saljom poruka koju obraduje glavna nit.
    PostMessage(messageScanUpdate, static_cast<WPARAM>(percent), static_cast<LPARAM>(found));
}

bool CScanProgressDlg::IsScanCancelled() const
{
    return (InterlockedCompareExchange(const_cast<LONG*>(&m_cancel), 0, 0) != 0);
}

LRESULT CScanProgressDlg::OnScanUpdate(WPARAM wParam, LPARAM lParam)
{
    m_progress.SetPos(static_cast<int>(wParam));

    CString text;
    text.Format(CSysUtil::LoadStr(IDS_SCAN_STATUS),
                static_cast<int>(wParam), static_cast<int>(lParam));

    SetDlgItemText(IDC_SCAN_STATUS, text);

    return 0;
}

LRESULT CScanProgressDlg::OnScanDone(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    EndDialog(IDOK);

    return 0;
}

void CScanProgressDlg::OnCancel()
{
    // Prozor se ne zatvara odmah: nit jos radi i javlja mu se, pa se ceka da
    // sama zavrsi i posalje poruku o kraju.
    InterlockedExchange(&m_cancel, 1);

    GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
    SetDlgItemText(IDC_SCAN_STATUS, CSysUtil::LoadStr(IDS_SCAN_STOPPING));
}
