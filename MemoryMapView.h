#pragma once

#include <vector>

class CProcMonDoc;
struct CRegionInfo;

// Jedan nacrtani blok mape: pravokutnik na ekranu i polozaj regije koju
// prikazuje u popisu dokumenta.
struct CMapBlock
{
    size_t index = 0;
    CRect  rect;
};

// CMemoryMapView - graficki prikaz adresnog prostora. Regije se crtaju u
// rastucem redoslijedu adresa, sirinom razmjernom velicini, a regije iste
// rezervacije obrubljene su zajednickim okvirom.
class CMemoryMapView : public CScrollView
{
    DECLARE_DYNCREATE(CMemoryMapView)

protected:
    CMemoryMapView();

public:
    CProcMonDoc* GetDocument() const;

protected:
    virtual void OnDraw(CDC* pDC);
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    enum
    {
        margin        = 10,
        legendHeight  = 24,
        blockHeight   = 18,
        rowSpacing    = 4,
        minBlockWidth = 3,
        swatchSize    = 12,
        targetRows    = 8      // ciljani broj redaka; iz njega slijedi mjerilo
    };

    // Racuna polozaj svih blokova za trenutnu sirinu prozora.
    void BuildLayout();

    // Slaze blokove u retke i vraca donji rub zadnjeg retka.
    int  LayoutBlocks(int rowWidth);

    void DrawLegend(CDC* pDC, int width) const;
    void DrawBlocks(CDC* pDC) const;
    void DrawLegendItem(CDC* pDC, int& x, int y, COLORREF color, const CString& text) const;

    // Pomice prikaz tako da odabrani blok bude vidljiv.
    void EnsureSelectionVisible();

    int FindBlock(const CPoint& point) const;

    static COLORREF ColorForRegion(const CRegionInfo& info);

    std::vector<CMapBlock> m_blocks;
    int                    m_layoutWidth;   // sirina za koju su blokovi racunati

public:
    virtual ~CMemoryMapView();
};
