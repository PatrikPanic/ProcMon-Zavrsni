#pragma once

// CChildFrame - okvir jedne kartice. Sve kartice rade nad istim dokumentom, pa
// bi im MFC dao isti naslov s rednim brojem prozora ("Procesi:1", "Procesi:2");
// zato okvir sam odreduje naslov prema vrsti pogleda koji sadrzi. Kartice se ne
// mogu zatvoriti jer aplikacija stalno prikazuje sve prikaze.
class CChildFrame : public CMDIChildWndEx
{
    DECLARE_DYNCREATE(CChildFrame)

public:
    CChildFrame();

    // Okvir ovu metodu zove kad crta natpis kartice.
    virtual CString GetFrameText() const;

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
    virtual void ActivateFrame(int nCmdShow);
    virtual void OnUpdateFrameTitle(BOOL bAddToTitle);

    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()

    // Izvedeni okvir sam stvara svoj sadrzaj, pa mu trebaju i naslov i nacin
    // na koji se odreduje.
    static CString TitleForView(CRuntimeClass* pViewClass);

    CString m_strTitle;

public:
    virtual ~CChildFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
};
