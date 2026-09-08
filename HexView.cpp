#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "HexView.h"
#include "SysUtil.h"
#include "MemoryCollector.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexView, CView)

BEGIN_MESSAGE_MAP(CHexView, CView)
    ON_WM_KEYDOWN()
    ON_WM_MOUSEWHEEL()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CHexView::CHexView()
    : m_lineHeight(16)
{
}

CHexView::~CHexView()
{
}

CProcMonDoc* CHexView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

void CHexView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    // Prikaz trazi font stalne sirine, inace se stupci bajtova ne poklapaju.
    if (m_font.GetSafeHandle() == nullptr)
        m_font.CreatePointFont(100, _T("Consolas"));

    CDC* pDC = GetDC();
    if (pDC != nullptr)
    {
        CFont* pOldFont = pDC->SelectObject(&m_font);

        TEXTMETRIC metrics = {};
        pDC->GetTextMetrics(&metrics);
        m_lineHeight = metrics.tmHeight + 2;

        pDC->SelectObject(pOldFont);
        ReleaseDC(pDC);
    }
}

void CHexView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    // Sadrzaj se mijenja samo kad se pomakne adresa ili promijeni proces.
    if (lHint != HINT_HEX && lHint != HINT_SELECTION)
        return;

    Invalidate();
}

void CHexView::OnDraw(CDC* pDC)
{
    CProcMonDoc* pDoc = GetDocument();

    CFont* pOldFont = pDC->SelectObject(&m_font);
    pDC->SetBkMode(TRANSPARENT);

    int y = margin;

    if (pDoc->GetHexAddress() == 0)
    {
        pDC->TextOut(margin, y, CSysUtil::LoadStr(IDS_HEX_NO_DATA));
        pDC->SelectObject(pOldFont);
        return;
    }

    if (!pDoc->IsHexAccessible())
    {
        pDC->TextOut(margin, y, CSysUtil::LoadStr(IDS_HEX_DENIED));
        pDC->SelectObject(pOldFont);
        return;
    }

    pDC->TextOut(margin, y, BuildRegionLine());
    y += m_lineHeight * 2;

    pDC->TextOut(margin, y, BuildHeader());
    y += m_lineHeight;

    const int rows = GetVisibleRows();

    for (int i = 0; i < rows; ++i)
    {
        const size_t offset = static_cast<size_t>(i) * bytesPerRow;

        if (offset >= pDoc->GetHexData().size())
            break;

        pDC->TextOut(margin, y, BuildRow(offset));
        y += m_lineHeight;
    }

    pDC->SelectObject(pOldFont);
}

CString CHexView::BuildRegionLine() const
{
    CProcMonDoc* pDoc = GetDocument();
    const CRegionInfo& region = pDoc->GetHexRegion();

    CString text;
    text.Format(CSysUtil::LoadStr(IDS_HEX_REGION),
                region.baseAddress,
                (LPCTSTR)CSysUtil::FormatBytes(region.size),
                (LPCTSTR)CMemoryCollector::FormatState(region.state),
                (LPCTSTR)CMemoryCollector::FormatProtection(region.protect),
                pDoc->GetHexAddress() - region.baseAddress);

    return text;
}

CString CHexView::BuildHeader() const
{
    // Zaglavlje pocinje iznad prvog bajta, pa mu prethodi razmak jednake
    // sirine kao stupac s adresom.
    CString header(_T(' '), addressWidth);
    CString text;

    for (int i = 0; i < bytesPerRow; ++i)
    {
        text.Format(_T("%02X "), i);
        header += text;
    }

    return header;
}

CString CHexView::BuildRow(size_t offset) const
{
    CProcMonDoc* pDoc = GetDocument();
    const std::vector<BYTE>& data = pDoc->GetHexData();

    CString row;
    row.Format(_T("0x%016llX  "), pDoc->GetHexAddress() + offset);

    CString bytes;
    CString text;

    for (int i = 0; i < bytesPerRow; ++i)
    {
        const size_t position = offset + i;

        if (position >= data.size())
            break;

        // Nedostupan dio prozora prikazuje se upitnicima, da se razlikuje od
        // stvarnih nula u memoriji.
        if (!pDoc->IsHexValid(position))
        {
            bytes += _T("?? ");
            text  += _T(".");
            continue;
        }

        const BYTE value = data[position];

        CString number;
        number.Format(_T("%02X "), value);
        bytes += number;

        text += (value >= 32 && value < 127) ? static_cast<TCHAR>(value) : _T('.');
    }

    return row + bytes + _T(" ") + text;
}

int CHexView::GetVisibleRows() const
{
    CRect client;
    GetClientRect(client);

    // Prva tri retka zauzimaju podaci o regiji, prazan redak i zaglavlje
    // stupaca.
    const int rows = (client.Height() - 2 * margin) / m_lineHeight - 3;

    return (rows > 0) ? rows : 0;
}

void CHexView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // Bez zarista prikaz ne bi primao tipke za listanje.
    SetFocus();

    CView::OnLButtonDown(nFlags, point);
}

void CHexView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    const LONGLONG page = static_cast<LONGLONG>(GetVisibleRows()) * bytesPerRow;

    switch (nChar)
    {
    case VK_NEXT:
        GetDocument()->MoveHexAddress(page);
        return;

    case VK_PRIOR:
        GetDocument()->MoveHexAddress(-page);
        return;

    case VK_DOWN:
        GetDocument()->MoveHexAddress(bytesPerRow);
        return;

    case VK_UP:
        GetDocument()->MoveHexAddress(-bytesPerRow);
        return;

    case VK_HOME:
        GetDocument()->SetHexAddress(GetDocument()->GetHexRegion().baseAddress);
        return;

    default:
        break;
    }

    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CHexView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // Jedan zubac kotacica pomice prikaz za tri retka, kao u ostalim popisima.
    const LONGLONG step = 3 * bytesPerRow;

    GetDocument()->MoveHexAddress((zDelta > 0) ? -step : step);

    return CView::OnMouseWheel(nFlags, zDelta, pt);
}
