#pragma once

#include <vector>

class CProcMonDoc;

// CGraphView - crta kretanje opterecenja kroz vrijeme u dva panela: gore
// procesor, dolje radna memorija. Oba su u postocima, pa se krivulja sustava i
// krivulja odabranog procesa mogu prikazati na istoj skali.
class CGraphView : public CView
{
    DECLARE_DYNCREATE(CGraphView)

protected:
    CGraphView();

public:
    CProcMonDoc* GetDocument() const;

protected:
    virtual void OnDraw(CDC* pDC);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    enum
    {
        margin       = 12,
        panelSpacing = 12,
        labelHeight  = 18,
        gridLines    = 4        // vodoravne crte koje dijele panel
    };

    // Jedan panel s okvirom, mrezom, natpisom i dvjema krivuljama.
    void DrawPanel(CDC* pDC, const CRect& rect, bool bMemory) const;
    void DrawGrid(CDC* pDC, const CRect& rect) const;

    // Crta krivulju iz postotaka; polje mora imati jedan clan po ocitanju.
    void DrawSeries(CDC* pDC, const CRect& rect, const std::vector<double>& values,
                    COLORREF color) const;

    CString BuildSystemLabel(bool bMemory) const;
    CString BuildProcessLabel(bool bMemory) const;

public:
    virtual ~CGraphView();
};
