#pragma once

class CProcMonDoc;

// CHexView - heksadekadski prikaz sadrzaja memorije odabranog procesa. Prikaz
// nema klizac jer se adresni prostor ne moze obuhvatiti 32-bitnim rasponom
// klizaca; umjesto toga se listanjem pomice pocetna adresa.
class CHexView : public CView
{
    DECLARE_DYNCREATE(CHexView)

protected:
    CHexView();

public:
    CProcMonDoc* GetDocument() const;

protected:
    virtual void OnDraw(CDC* pDC);
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    enum
    {
        bytesPerRow  = 16,
        margin       = 8,

        // Sirina stupca s adresom: "0x", 16 znamenki i dva razmaka do bajtova.
        addressWidth = 20
    };

    // Broj redaka koji stane u prozor.
    int  GetVisibleRows() const;

    // Redak s podacima o regiji kojoj pripada prikazana adresa.
    CString BuildRegionLine() const;

    // Redak zaglavlja s odmacima unutar retka.
    CString BuildHeader() const;
    CString BuildRow(size_t offset) const;

    CFont m_font;
    int   m_lineHeight;

public:
    virtual ~CHexView();
};
