#pragma once

class CProcMonDoc;

// CMainFrame - glavni MDI okvir. Gradi Ribbon i statusnu traku, drzi mjerac
// vremena za osvjezavanje i prosljeduje sadrzaj polja za filtriranje dokumentu.
class CMainFrame : public CMDIFrameWndEx
{
    DECLARE_DYNAMIC(CMainFrame)

public:
    CMainFrame();

    void SetStatusText(LPCTSTR lpszText);
    void SetAddressText(ULONGLONG address);

    // Prebacuje prikaz na karticu s pogledom zadane vrste.
    void ActivateView(CRuntimeClass* pViewClass);

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnFilterChanged();
    afx_msg void OnAnalyze();
    afx_msg void OnUpdateNeedsProcess(CCmdUI* pCmdUI);
    afx_msg void OnHexView();
    afx_msg void OnAddressChanged();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

private:
    // Identifikator je namjerno velik broj. Standardne kontrole sustava koriste
    // vlastite mjerace vremena s malim identifikatorima, pa bi moglo doci do
    // sukoba. Ocitanje se odvija u glavnoj dretvi, pa je razmak od 3 s odabran
    // tako da sucelje ostane odzivno i na sporijim racunalima.
    enum { timerRefresh = 1001, refreshIntervalMs = 3000 };

    void CreateRibbon();
    CProcMonDoc* GetInspectorDoc() const;

    CMFCRibbonBar       m_wndRibbonBar;
    CMFCRibbonStatusBar m_wndStatusBar;

public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
};
