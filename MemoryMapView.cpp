#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "MemoryMapView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMemoryMapView, CScrollView)

BEGIN_MESSAGE_MAP(CMemoryMapView, CScrollView)
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CMemoryMapView::CMemoryMapView()
    : m_layoutWidth(0)
{
}

CMemoryMapView::~CMemoryMapView()
{
}

CProcMonDoc* CMemoryMapView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

void CMemoryMapView::OnInitialUpdate()
{
    CScrollView::OnInitialUpdate();

    BuildLayout();
}

void CMemoryMapView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    // Promjena odabrane regije ne mijenja raspored blokova, nego samo isticanje.
    if (lHint == HINT_REGION)
    {
        EnsureSelectionVisible();
        Invalidate();
        return;
    }

    if (lHint != HINT_MEMORY && lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    // Kao i popis regija, crtez se ne dira uz redovno ocitanje procesa; jedino
    // se prazni kad mapa vise ne postoji.
    if (lHint == HINT_PROCESSES && GetDocument()->GetMemoryPid() != 0)
        return;

    BuildLayout();
}

void CMemoryMapView::OnSize(UINT nType, int cx, int cy)
{
    CScrollView::OnSize(nType, cx, cy);

    // Raspored ovisi o sirini prozora, pa se racuna samo kad se ona promijeni.
    // Bez te provjere bi postavljanje klizaca vratilo novu poruku o promjeni
    // velicine i racunanje se ne bi zaustavilo.
    if (cx != m_layoutWidth)
        BuildLayout();
}

BOOL CMemoryMapView::OnEraseBkgnd(CDC* /*pDC*/)
{
    // Podloga se crta zajedno s ostatkom prikaza, u meduspremnik.
    return TRUE;
}

void CMemoryMapView::BuildLayout()
{
    m_blocks.clear();

    CRect client;
    GetClientRect(client);

    m_layoutWidth = client.Width();

    const int rowWidth = m_layoutWidth - 2 * margin;

    int bottom = margin + legendHeight;

    if (rowWidth > minBlockWidth)
        bottom = LayoutBlocks(rowWidth);

    int height = bottom + margin;
    if (height < client.Height())
        height = client.Height();

    // Mjerilo prikaza postavlja se i kad nema sto crtati, jer klizaci moraju
    // biti postavljeni prije prvog iscrtavanja.
    SetScrollSizes(MM_TEXT, CSize(m_layoutWidth, height));
    Invalidate();
}

int CMemoryMapView::LayoutBlocks(int rowWidth)
{
    const std::vector<CRegionInfo>& items = GetDocument()->GetRegions();

    // Mjerilo se racuna iz ukupne velicine svih nacrtanih regija tako da crtez
    // bude sirok otprilike targetRows redaka. Bez toga bi jedna regija od
    // nekoliko gigabajta zauzela cijeli prostor, a sve ostale bi nestale.
    ULONGLONG totalSize = 0;
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (items[i].state != MEM_FREE)
            totalSize += items[i].size;
    }

    int y = margin + legendHeight;

    if (totalSize == 0)
        return y;

    const double scale = (static_cast<double>(rowWidth) * targetRows) /
                         static_cast<double>(totalSize);

    int x = margin;

    for (size_t i = 0; i < items.size(); ++i)
    {
        if (items[i].state == MEM_FREE)
            continue;

        // Najmanja sirina osigurava da i regija od jedne stranice ostane
        // vidljiva i da se u nju moze kliknuti.
        int width = static_cast<int>(items[i].size * scale);

        if (width < minBlockWidth)
            width = minBlockWidth;

        if (width > rowWidth)
            width = rowWidth;

        if (x + width > margin + rowWidth)
        {
            x = margin;
            y += blockHeight + rowSpacing;
        }

        CMapBlock block;
        block.index = i;
        block.rect.SetRect(x, y, x + width, y + blockHeight);

        m_blocks.push_back(block);

        x += width;
    }

    return y + blockHeight;
}

void CMemoryMapView::OnDraw(CDC* pDC)
{
    CRect client;
    GetClientRect(client);

    // Crtez se sastavlja u meduspremniku pa se odjednom prenosi na zaslon,
    // inace prikaz treperi pri svakoj promjeni velicine prozora.
    CDC memDC;
    if (!memDC.CreateCompatibleDC(pDC))
        return;

    CBitmap bitmap;
    if (!bitmap.CreateCompatibleBitmap(pDC, client.Width(), client.Height()))
        return;

    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // Ishodiste meduspremnika pomice se za polozaj klizaca, pa crtanje racuna
    // s istim koordinatama kao da klizaca nema.
    const CPoint scroll = GetScrollPosition();
    memDC.SetWindowOrg(scroll);

    memDC.FillSolidRect(CRect(scroll, client.Size()), GetSysColor(COLOR_WINDOW));

    CFont* pOldFont = memDC.SelectObject(
        CFont::FromHandle(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));

    memDC.SetBkMode(TRANSPARENT);
    memDC.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));

    if (GetDocument()->GetMemoryPid() == 0)
    {
        CRect text(scroll, client.Size());
        memDC.DrawText(CSysUtil::LoadStr(IDS_MEMORY_NO_MAP), text,
                       DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    else
    {
        DrawLegend(&memDC, client.Width());
        DrawBlocks(&memDC);
    }

    pDC->BitBlt(scroll.x, scroll.y, client.Width(), client.Height(),
                &memDC, scroll.x, scroll.y, SRCCOPY);

    memDC.SelectObject(pOldFont);
    memDC.SelectObject(pOldBitmap);
}

void CMemoryMapView::DrawLegend(CDC* pDC, int width) const
{
    CProcMonDoc* pDoc = GetDocument();

    const int y = margin;
    int x = margin;

    DrawLegendItem(pDC, x, y, RGB(86, 130, 184),  CSysUtil::LoadStr(IDS_MEM_TYPE_IMAGE));
    DrawLegendItem(pDC, x, y, RGB(96, 166, 140),  CSysUtil::LoadStr(IDS_MEM_TYPE_MAPPED));
    DrawLegendItem(pDC, x, y, RGB(224, 158, 74),  CSysUtil::LoadStr(IDS_MEM_TYPE_PRIVATE));
    DrawLegendItem(pDC, x, y, RGB(214, 214, 206), CSysUtil::LoadStr(IDS_MEM_STATE_RESERVE));

    // Sazetak se ispisuje desno od legende, a ako za njega nema mjesta,
    // DrawText ga skraduje.
    CString text;
    text.Format(CSysUtil::LoadStr(IDS_MEM_SUMMARY),
                static_cast<int>(m_blocks.size()),
                (LPCTSTR)CSysUtil::FormatBytes(pDoc->GetCommittedBytes()),
                (LPCTSTR)CSysUtil::FormatBytes(pDoc->GetReservedBytes()));

    CRect rect(x + margin, y, width - margin, y + swatchSize + 2);
    pDC->DrawText(text, rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CMemoryMapView::DrawLegendItem(CDC* pDC, int& x, int y, COLORREF color,
                                    const CString& text) const
{
    pDC->FillSolidRect(x, y, swatchSize, swatchSize, color);
    pDC->Draw3dRect(x, y, swatchSize, swatchSize,
                    GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DSHADOW));

    x += swatchSize + 4;

    const CSize size = pDC->GetTextExtent(text);
    pDC->TextOut(x, y + (swatchSize - size.cy) / 2, text);

    x += size.cx + 16;
}

void CMemoryMapView::DrawBlocks(CDC* pDC) const
{
    const std::vector<CRegionInfo>& items = GetDocument()->GetRegions();
    const int selected = GetDocument()->GetSelectedRegion();

    for (size_t i = 0; i < m_blocks.size(); ++i)
    {
        const CRegionInfo& info = items[m_blocks[i].index];
        pDC->FillSolidRect(m_blocks[i].rect, ColorForRegion(info));
    }

    // Blokovi iste rezervacije koji leze jedan do drugoga obrubljuju se
    // zajednickim okvirom, pa se vidi dokle seze pojedini modul.
    CPen pen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
    CPen* pOldPen = pDC->SelectObject(&pen);
    CGdiObject* pOldBrush = pDC->SelectStockObject(NULL_BRUSH);

    size_t start = 0;
    while (start < m_blocks.size())
    {
        const ULONGLONG base = items[m_blocks[start].index].allocationBase;

        size_t end = start;
        while (end + 1 < m_blocks.size() &&
               items[m_blocks[end + 1].index].allocationBase == base &&
               m_blocks[end + 1].rect.top == m_blocks[start].rect.top)
        {
            ++end;
        }

        CRect frame(m_blocks[start].rect.left, m_blocks[start].rect.top,
                    m_blocks[end].rect.right, m_blocks[end].rect.bottom);

        pDC->Rectangle(frame);
        start = end + 1;
    }

    // Odabrana regija istice se debljim okvirom koji je uvijek vidljiv, i kod
    // blokova sirokih svega nekoliko piksela.
    if (selected >= 0)
    {
        for (size_t i = 0; i < m_blocks.size(); ++i)
        {
            if (static_cast<int>(m_blocks[i].index) != selected)
                continue;

            CPen selectPen(PS_SOLID, 2, GetSysColor(COLOR_HIGHLIGHT));
            pDC->SelectObject(&selectPen);

            CRect frame = m_blocks[i].rect;
            frame.InflateRect(2, 2);
            pDC->Rectangle(frame);

            pDC->SelectObject(&pen);
            break;
        }
    }

    pDC->SelectObject(pOldBrush);
    pDC->SelectObject(pOldPen);
}

COLORREF CMemoryMapView::ColorForRegion(const CRegionInfo& info)
{
    if (info.state == MEM_RESERVE)
        return RGB(214, 214, 206);

    switch (info.type)
    {
    case MEM_IMAGE:  return RGB(86, 130, 184);
    case MEM_MAPPED: return RGB(96, 166, 140);
    default:         return RGB(224, 158, 74);
    }
}

void CMemoryMapView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // Klik stize u koordinatama prozora, a blokovi su racunati bez klizaca.
    const int index = FindBlock(point + GetScrollPosition());

    if (index >= 0)
        GetDocument()->SetSelectedRegion(index);

    CScrollView::OnLButtonDown(nFlags, point);
}

int CMemoryMapView::FindBlock(const CPoint& point) const
{
    for (size_t i = 0; i < m_blocks.size(); ++i)
    {
        if (m_blocks[i].rect.PtInRect(point))
            return static_cast<int>(m_blocks[i].index);
    }

    return -1;
}

void CMemoryMapView::EnsureSelectionVisible()
{
    const int selected = GetDocument()->GetSelectedRegion();
    if (selected < 0)
        return;

    CRect client;
    GetClientRect(client);

    const int scrollY = GetScrollPosition().y;

    for (size_t i = 0; i < m_blocks.size(); ++i)
    {
        if (static_cast<int>(m_blocks[i].index) != selected)
            continue;

        const CRect& rect = m_blocks[i].rect;

        if (rect.top < scrollY)
        {
            int target = rect.top - margin;
            if (target < 0)
                target = 0;

            ScrollToPosition(CPoint(0, target));
        }
        else if (rect.bottom > scrollY + client.Height())
        {
            ScrollToPosition(CPoint(0, rect.bottom - client.Height() + margin));
        }

        return;
    }
}
