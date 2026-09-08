#pragma once

#include "StringScanner.h"
#include "StringIDs.h"

// CScanProgressDlg - prozor s napretkom pretrage. Sama pretraga tece u
// pomocnoj niti, pa prozor ostaje odzivan i moze je prekinuti. Prozor je
// ujedno promatrac pretrage: prima napredak i odgovara treba li stati.
class CScanProgressDlg : public CDialogEx, public CScanObserver
{
    DECLARE_DYNAMIC(CScanProgressDlg)

public:
    CScanProgressDlg(CStringScanner& scanner, DWORD pid, CWnd* pParent = nullptr);
    virtual ~CScanProgressDlg();

    enum { IDD = IDD_SCAN_PROGRESS };

    // Sucelje promatraca; poziva ga pomocna nit.
    virtual void OnScanProgress(int percent, int found);
    virtual bool IsScanCancelled() const;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();

    afx_msg LRESULT OnScanUpdate(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScanDone(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    static UINT __cdecl ThreadProc(LPVOID pParam);

    CStringScanner& m_scanner;
    DWORD           m_pid;
    CProgressCtrl   m_progress;
    LONG            m_cancel;
};
