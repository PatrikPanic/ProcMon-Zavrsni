#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "ThreadView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CThreadView, CSortedListView)

BEGIN_MESSAGE_MAP(CThreadView, CSortedListView)
END_MESSAGE_MAP()

CThreadView::CThreadView()
{
}

CThreadView::~CThreadView()
{
}

CProcMonDoc* CThreadView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CThreadView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CSortedListView::PreCreateWindow(cs);
}

void CThreadView::OnInitialUpdate()
{
    CSortedListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CThreadView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    // Popis se osvjezava i kod promjene odabira i kod redovnog osvjezavanja
    // podataka, jer se dretve procesa stalno stvaraju i zavrsavaju.
    FillList();
}

void CThreadView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    list.InsertColumn(colTid,        CSysUtil::LoadStr(IDS_COL_TID),        LVCFMT_RIGHT,  90);
    list.InsertColumn(colPriority,   CSysUtil::LoadStr(IDS_COL_PRIORITY),   LVCFMT_RIGHT,  80);
    list.InsertColumn(colKernelTime, CSysUtil::LoadStr(IDS_COL_KERNELTIME), LVCFMT_RIGHT, 120);
    list.InsertColumn(colUserTime,   CSysUtil::LoadStr(IDS_COL_USERTIME),   LVCFMT_RIGHT, 120);
    list.InsertColumn(colCreated,    CSysUtil::LoadStr(IDS_COL_CREATED),    LVCFMT_LEFT,  150);
}

void CThreadView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    const std::vector<CThreadInfo>& items = pDoc->GetThreads();
    const int topIndex = list.GetTopIndex();

    std::vector<size_t> order;
    BuildOrder(items.size(), order);

    // Odabir se izricito ponistava prije praznjenja popisa, inace obojeni redak
    // ostaje nacrtan i nakon brisanja stavke.
    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    // Kartica je otvorena i prije nego je proces odabran, pa se umjesto praznog
    // popisa ispisuje uputa.
    if (pDoc->GetSelectedPid() == 0)
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_THREADS_NO_SELECTION));
        return;
    }

    CString text;

    for (size_t i = 0; i < order.size(); ++i)
    {
        const CThreadInfo& info = items[order[i]];

        text.Format(_T("%u"), info.tid);
        const int index = list.InsertItem(static_cast<int>(i), text);
        if (index < 0)
            continue;

        list.SetItemData(index, info.tid);

        text.Format(_T("%ld"), info.basePriority);
        list.SetItemText(index, colPriority, text);

        if (info.accessible)
        {
            list.SetItemText(index, colKernelTime, CSysUtil::FormatDuration(info.kernelTime));
            list.SetItemText(index, colUserTime,   CSysUtil::FormatDuration(info.userTime));
            list.SetItemText(index, colCreated,    CSysUtil::FormatTimeStamp(info.creationTime));
        }
        else
        {
            const CString notAvailable = CSysUtil::LoadStr(IDS_NOT_AVAILABLE);
            list.SetItemText(index, colKernelTime, notAvailable);
            list.SetItemText(index, colUserTime,   notAvailable);
            list.SetItemText(index, colCreated,    notAvailable);
        }
    }

    if (topIndex > 0 && topIndex < list.GetItemCount())
        list.EnsureVisible(topIndex, FALSE);

    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
}

bool CThreadView::IsLess(size_t leftIndex, size_t rightIndex) const
{
    const std::vector<CThreadInfo>& items = GetDocument()->GetThreads();

    const CThreadInfo& left  = items[leftIndex];
    const CThreadInfo& right = items[rightIndex];

    switch (GetSortColumn())
    {
    case colPriority:
        if (left.basePriority != right.basePriority)
            return left.basePriority < right.basePriority;
        break;

    case colKernelTime:
        if (left.kernelTime != right.kernelTime)
            return left.kernelTime < right.kernelTime;
        break;

    case colUserTime:
        if (left.userTime != right.userTime)
            return left.userTime < right.userTime;
        break;

    case colCreated:
        {
            const ULONGLONG leftTime  = CSysUtil::ToUInt64(left.creationTime);
            const ULONGLONG rightTime = CSysUtil::ToUInt64(right.creationTime);

            if (leftTime != rightTime)
                return leftTime < rightTime;
        }
        break;

    case colTid:
    default:
        break;
    }

    // Dretve s jednakom vrijednoscu razvrstavaju se po identifikatoru, cime je
    // redoslijed jednoznacan i popis ne poskakuje pri osvjezavanju.
    return left.tid < right.tid;
}
