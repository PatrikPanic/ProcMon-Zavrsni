#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "ProcessView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CProcessView, CListView)

BEGIN_MESSAGE_MAP(CProcessView, CListView)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CProcessView::OnItemChanged)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CProcessView::OnColumnClick)
    ON_NOTIFY_REFLECT(NM_CLICK, &CProcessView::OnClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CProcessView::OnDblClick)
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CProcessView::CProcessView()
    : m_bFilling(false)
{
}

CProcessView::~CProcessView()
{
}

CProcMonDoc* CProcessView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CProcessView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CListView::PreCreateWindow(cs);
}

void CProcessView::OnInitialUpdate()
{
    CListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CProcessView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    // Popis procesa mijenja se samo novim ocitanjem; promjena odabira i
    // ocitanje mape memorije ne diraju njegov sadrzaj.
    if (lHint != HINT_PROCESSES)
        return;

    FillList();
}

void CProcessView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    list.InsertColumn(CProcMonDoc::colPid,         CSysUtil::LoadStr(IDS_COL_PID),         LVCFMT_RIGHT,  60);
    list.InsertColumn(CProcMonDoc::colName,        CSysUtil::LoadStr(IDS_COL_NAME),        LVCFMT_LEFT,  260);
    list.InsertColumn(CProcMonDoc::colCpu,         CSysUtil::LoadStr(IDS_COL_CPU),         LVCFMT_RIGHT,  80);
    list.InsertColumn(CProcMonDoc::colWorkingSet,  CSysUtil::LoadStr(IDS_COL_WORKINGSET),  LVCFMT_RIGHT,  90);
    list.InsertColumn(CProcMonDoc::colPrivate,     CSysUtil::LoadStr(IDS_COL_PRIVATE),     LVCFMT_RIGHT, 120);
    list.InsertColumn(CProcMonDoc::colThreadCount, CSysUtil::LoadStr(IDS_COL_THREADCOUNT), LVCFMT_RIGHT,  90);
    list.InsertColumn(CProcMonDoc::colPath,        CSysUtil::LoadStr(IDS_COL_PATH),        LVCFMT_LEFT,  420);
}

CString CProcessView::BuildNameText(const CProcessRow& row) const
{
    // Uvlaka se postize razmacima, a stanje cvora oznakom ispred naziva, pa
    // nije potrebno vlastito crtanje ni kontrola stabla.
    CString text;

    for (int i = 0; i < row.depth; ++i)
        text += _T("      ");

    if (row.hasChildren)
        text += row.expanded ? _T("[-] ") : _T("[+] ");
    else
        text += _T("     ");

    text += row.info.name;
    return text;
}

void CProcessView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    const std::vector<CProcessRow>& items = pDoc->GetVisibleProcesses();
    const DWORD selectedPid = pDoc->GetSelectedPid();
    const int topIndex = list.GetTopIndex();

    m_bFilling = true;

    // Odabir se izricito ponistava prije praznjenja popisa. Bez toga kontrola
    // zadrzava obojeni redak i on ostaje nacrtan i nakon brisanja stavke.
    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    int selectedIndex = -1;
    CString text;

    for (size_t i = 0; i < items.size(); ++i)
    {
        const CProcessInfo& info = items[i].info;

        text.Format(_T("%u"), info.pid);
        const int index = list.InsertItem(static_cast<int>(i), text);
        if (index < 0)
            continue;

        list.SetItemData(index, info.pid);
        list.SetItemText(index, CProcMonDoc::colName, BuildNameText(items[i]));

        text.Format(_T("%.1f"), info.cpuPercent);
        list.SetItemText(index, CProcMonDoc::colCpu, text);

        list.SetItemText(index, CProcMonDoc::colWorkingSet, CSysUtil::FormatBytes(info.workingSet));
        list.SetItemText(index, CProcMonDoc::colPrivate,    CSysUtil::FormatBytes(info.privateBytes));

        text.Format(_T("%u"), info.threadCount);
        list.SetItemText(index, CProcMonDoc::colThreadCount, text);

        list.SetItemText(index, CProcMonDoc::colPath,
                         info.path.IsEmpty() ? CSysUtil::LoadStr(IDS_NOT_AVAILABLE) : info.path);

        if (info.pid == selectedPid)
            selectedIndex = index;
    }

    // Vracanje odabira i priblizno istog polozaja klizaca nakon osvjezavanja.
    if (selectedIndex >= 0)
        list.SetItemState(selectedIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    if (topIndex > 0 && topIndex < list.GetItemCount())
        list.EnsureVisible(topIndex, FALSE);

    // Popis se tijekom punjenja pomice, pa se na kraju cijela kontrola izricito
    // ponovno iscrtava. Treperenje sprjecava stil LVS_EX_DOUBLEBUFFER.
    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
    m_bFilling = false;
}

void CProcessView::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pInfo = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;

    if (m_bFilling)
        return;

    if ((pInfo->uChanged & LVIF_STATE) && (pInfo->uNewState & LVIS_SELECTED))
    {
        const DWORD pid = static_cast<DWORD>(GetListCtrl().GetItemData(pInfo->iItem));
        GetDocument()->SetSelectedPid(pid);
    }
}

void CProcessView::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pInfo = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;

    GetDocument()->SortByColumn(pInfo->iSubItem);
}

void CProcessView::OnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pInfo = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    *pResult = 0;

    if (pInfo->iItem < 0)
        return;

    // Sklapanje se pokrece samo klikom na oznaku ispred naziva, a ne bilo gdje
    // u retku, kako bi obican odabir procesa i dalje radio.
    if (IsClickOnMarker(pInfo->iItem, pInfo->ptAction))
        ToggleRow(pInfo->iItem);
}

void CProcessView::OnDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pInfo = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    *pResult = 0;

    if (pInfo->iItem >= 0)
        ToggleRow(pInfo->iItem);
}

void CProcessView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // Tipke + i - rade kao u stablu: sire i sklapaju oznaceni cvor.
    if (nChar == VK_ADD || nChar == VK_SUBTRACT)
    {
        const int index = GetListCtrl().GetNextItem(-1, LVNI_SELECTED);
        if (index >= 0)
        {
            ToggleRow(index);
            return;
        }
    }

    CListView::OnKeyDown(nChar, nRepCnt, nFlags);
}

bool CProcessView::IsClickOnMarker(int index, const POINT& point) const
{
    const std::vector<CProcessRow>& items = GetDocument()->GetVisibleProcesses();
    if (index < 0 || static_cast<size_t>(index) >= items.size())
        return false;

    if (!items[index].hasChildren)
        return false;

    CListCtrl& list = const_cast<CProcessView*>(this)->GetListCtrl();

    CRect rect;
    if (!list.GetSubItemRect(index, CProcMonDoc::colName, LVIR_LABEL, rect))
        return false;

    // Sirina uvlake i oznake racuna se iz stvarne sirine ispisanog teksta, pa
    // ostaje tocna i kad se promijeni font.
    CDC* pDC = list.GetDC();
    if (pDC == nullptr)
        return false;

    CFont* pOldFont = pDC->SelectObject(list.GetFont());

    CString indent;
    for (int i = 0; i < items[index].depth; ++i)
        indent += _T("      ");

    const int indentWidth = pDC->GetTextExtent(indent).cx;
    const int markerWidth = pDC->GetTextExtent(_T("[-] ")).cx;

    pDC->SelectObject(pOldFont);
    list.ReleaseDC(pDC);

    // Zona klika prosirena je s obje strane oznake kako se u nju ne bi trebalo
    // pogadati piksel po piksel.
    const int tolerance = 8;
    const int left  = rect.left + indentWidth - tolerance;
    const int right = rect.left + indentWidth + markerWidth + tolerance;

    return (point.x >= left && point.x <= right);
}

void CProcessView::ToggleRow(int index)
{
    const std::vector<CProcessRow>& items = GetDocument()->GetVisibleProcesses();
    if (index < 0 || static_cast<size_t>(index) >= items.size())
        return;

    if (!items[index].hasChildren)
        return;

    GetDocument()->ToggleExpand(items[index].info.pid);
}
