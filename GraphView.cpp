#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "GraphView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CGraphView, CView)

BEGIN_MESSAGE_MAP(CGraphView, CView)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CGraphView::CGraphView()
{
}

CGraphView::~CGraphView()
{
}

CProcMonDoc* CGraphView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

void CGraphView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    // Nova tocka nastaje pri svakom ocitanju, a promjena odabira brise krivulju
    // procesa, pa se crtez u oba slucaja iscrtava ponovno.
    if (lHint != HINT_PROCESSES && lHint != HINT_SELECTION)
        return;

    Invalidate();
}

BOOL CGraphView::OnEraseBkgnd(CDC* /*pDC*/)
{
    // Podloga se crta zajedno s ostatkom prikaza, u meduspremnik.
    return TRUE;
}

void CGraphView::OnDraw(CDC* pDC)
{
    CRect client;
    GetClientRect(client);

    // Kod vrlo malog prozora nema smisla crtati, ali podlogu treba obrisati
    // jer je OnEraseBkgnd prepustio crtanje ovoj metodi.
    if (client.Width() < 2 * margin || client.Height() < 4 * margin)
    {
        pDC->FillSolidRect(client, GetSysColor(COLOR_WINDOW));
        return;
    }

    // Crtez se sastavlja u meduspremniku pa se odjednom prenosi na zaslon,
    // inace prikaz treperi pri svakom osvjezavanju podataka.
    CDC memDC;
    if (!memDC.CreateCompatibleDC(pDC))
        return;

    CBitmap bitmap;
    if (!bitmap.CreateCompatibleBitmap(pDC, client.Width(), client.Height()))
        return;

    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    memDC.FillSolidRect(client, GetSysColor(COLOR_WINDOW));

    CFont* pOldFont = memDC.SelectObject(
        CFont::FromHandle(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));

    memDC.SetBkMode(TRANSPARENT);
    memDC.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));

    // Dva panela jednake visine, jedan iznad drugoga.
    const int panelHeight = (client.Height() - 2 * margin - panelSpacing) / 2;

    CRect cpu(client.left + margin, client.top + margin,
              client.right - margin, client.top + margin + panelHeight);

    CRect memory(cpu.left, cpu.bottom + panelSpacing,
                 cpu.right, cpu.bottom + panelSpacing + panelHeight);

    DrawPanel(&memDC, cpu, false);
    DrawPanel(&memDC, memory, true);

    pDC->BitBlt(0, 0, client.Width(), client.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldFont);
    memDC.SelectObject(pOldBitmap);
}

void CGraphView::DrawPanel(CDC* pDC, const CRect& rect, bool bMemory) const
{
    // Gornji dio panela nosi natpis, a ostatak je povrsina za crtanje.
    CRect label(rect.left, rect.top, rect.right, rect.top + labelHeight);
    CRect plot(rect.left, label.bottom, rect.right, rect.bottom);

    pDC->TextOut(label.left, label.top,
                 CSysUtil::LoadStr(bMemory ? IDS_GRAPH_MEMORY : IDS_GRAPH_CPU));

    // Trenutne vrijednosti ispisuju se desno od natpisa, u boji krivulje.
    const COLORREF systemColor  = RGB(86, 130, 184);
    const COLORREF processColor = RGB(224, 158, 74);

    const CString processText = BuildProcessLabel(bMemory);
    int right = label.right;

    if (!processText.IsEmpty())
    {
        const CSize size = pDC->GetTextExtent(processText);

        pDC->SetTextColor(processColor);
        pDC->TextOut(right - size.cx, label.top, processText);

        right -= size.cx + 16;
    }

    const CString systemText = BuildSystemLabel(bMemory);
    const CSize size = pDC->GetTextExtent(systemText);

    pDC->SetTextColor(systemColor);
    pDC->TextOut(right - size.cx, label.top, systemText);

    pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));

    DrawGrid(pDC, plot);

    const std::vector<CHistorySample>& items = GetDocument()->GetHistory();
    const ULONGLONG totalMemory = GetDocument()->GetTotalMemory();

    std::vector<double> systemValues;
    std::vector<double> processValues;

    systemValues.reserve(items.size());
    processValues.reserve(items.size());

    for (size_t i = 0; i < items.size(); ++i)
    {
        if (!bMemory)
        {
            systemValues.push_back(items[i].systemCpu);
            processValues.push_back(items[i].hasProcess ? items[i].processCpu : -1.0);
            continue;
        }

        // Memorija se prikazuje kao udio ukupne radne memorije, pa oba niza
        // stanu na istu skalu od 0 do 100 posto.
        if (totalMemory == 0)
        {
            systemValues.push_back(0.0);
            processValues.push_back(-1.0);
            continue;
        }

        systemValues.push_back((100.0 * items[i].systemUsed) / totalMemory);

        processValues.push_back(items[i].hasProcess
            ? (100.0 * items[i].processWorkingSet) / totalMemory
            : -1.0);
    }

    DrawSeries(pDC, plot, systemValues, systemColor);
    DrawSeries(pDC, plot, processValues, processColor);
}

void CGraphView::DrawGrid(CDC* pDC, const CRect& rect) const
{
    CPen grid(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
    CPen frame(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

    CPen* pOldPen = pDC->SelectObject(&grid);

    for (int i = 1; i < gridLines; ++i)
    {
        const int y = rect.top + (rect.Height() * i) / gridLines;

        pDC->MoveTo(rect.left, y);
        pDC->LineTo(rect.right, y);
    }

    pDC->SelectObject(&frame);

    CGdiObject* pOldBrush = pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(rect);

    pDC->SelectObject(pOldBrush);
    pDC->SelectObject(pOldPen);
}

void CGraphView::DrawSeries(CDC* pDC, const CRect& rect,
                            const std::vector<double>& values, COLORREF color) const
{
    if (values.size() < 2)
        return;

    CPen pen(PS_SOLID, 2, color);
    CPen* pOldPen = pDC->SelectObject(&pen);

    // Vodoravno mjerilo je stalno: najnovije ocitanje je uvijek uz desni rub,
    // pa se crtez ne rasteze dok se povijest puni.
    const double step = static_cast<double>(rect.Width()) / (CHistoryCollector::maxSamples - 1);

    bool bStarted = false;

    for (size_t i = 0; i < values.size(); ++i)
    {
        // Negativna vrijednost oznacava ocitanje bez podatka; krivulja se tada
        // prekida i nastavlja tek kad podaci opet postoje.
        if (values[i] < 0.0)
        {
            bStarted = false;
            continue;
        }

        const int x = rect.right - static_cast<int>((values.size() - 1 - i) * step);
        const int y = rect.bottom - static_cast<int>((rect.Height() * values[i]) / 100.0);

        if (!bStarted)
        {
            pDC->MoveTo(x, y);
            bStarted = true;
        }
        else
        {
            pDC->LineTo(x, y);
        }
    }

    pDC->SelectObject(pOldPen);
}

CString CGraphView::BuildSystemLabel(bool bMemory) const
{
    CProcMonDoc* pDoc = GetDocument();
    const std::vector<CHistorySample>& items = pDoc->GetHistory();

    if (items.empty())
        return CString();

    const CHistorySample& last = items.back();

    CString text;

    if (!bMemory)
    {
        text.Format(CSysUtil::LoadStr(IDS_GRAPH_CPU_SYSTEM), last.systemCpu);
        return text;
    }

    text.Format(CSysUtil::LoadStr(IDS_GRAPH_MEM_SYSTEM),
                (LPCTSTR)CSysUtil::FormatBytes(last.systemUsed),
                (LPCTSTR)CSysUtil::FormatBytes(pDoc->GetTotalMemory()));

    return text;
}

CString CGraphView::BuildProcessLabel(bool bMemory) const
{
    CProcMonDoc* pDoc = GetDocument();
    const std::vector<CHistorySample>& items = pDoc->GetHistory();

    if (items.empty() || !items.back().hasProcess)
        return CString();

    const CHistorySample& last = items.back();
    const CString name = pDoc->GetSelectedProcessName();

    CString text;

    if (!bMemory)
    {
        text.Format(CSysUtil::LoadStr(IDS_GRAPH_CPU_PROCESS), (LPCTSTR)name, last.processCpu);
        return text;
    }

    text.Format(CSysUtil::LoadStr(IDS_GRAPH_MEM_PROCESS), (LPCTSTR)name,
                (LPCTSTR)CSysUtil::FormatBytes(last.processWorkingSet));

    return text;
}
