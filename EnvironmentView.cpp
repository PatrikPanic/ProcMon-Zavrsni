#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "EnvironmentView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEnvironmentView, CSortedListView)

BEGIN_MESSAGE_MAP(CEnvironmentView, CSortedListView)
END_MESSAGE_MAP()

CEnvironmentView::CEnvironmentView()
{
}

CEnvironmentView::~CEnvironmentView()
{
}

CProcMonDoc* CEnvironmentView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CEnvironmentView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CSortedListView::PreCreateWindow(cs);
}

void CEnvironmentView::OnInitialUpdate()
{
    CSortedListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CEnvironmentView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    // Okolina se ne mijenja za zivota procesa, pa se popis ne osvjezava uz
    // redovno ocitanje. Iznimka je nestanak odabira, kad treba vratiti uputu.
    if (lHint == HINT_PROCESSES && GetDocument()->GetSelectedPid() != 0)
        return;

    FillList();
}

void CEnvironmentView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    list.InsertColumn(colName,  CSysUtil::LoadStr(IDS_COL_ENV_NAME),  LVCFMT_LEFT, 240);
    list.InsertColumn(colValue, CSysUtil::LoadStr(IDS_COL_ENV_VALUE), LVCFMT_LEFT, 700);
}

void CEnvironmentView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    if (pDoc->GetSelectedPid() == 0)
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_THREADS_NO_SELECTION));
        return;
    }

    if (!pDoc->IsEnvironmentAccessible())
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_ENV_DENIED));
        return;
    }

    const std::vector<CEnvironmentVariable>& items = pDoc->GetEnvironment();

    std::vector<size_t> order;
    BuildOrder(items.size(), order);

    for (size_t i = 0; i < order.size(); ++i)
    {
        const CEnvironmentVariable& info = items[order[i]];

        const int index = list.InsertItem(static_cast<int>(i), info.name);
        if (index < 0)
            continue;

        list.SetItemText(index, colValue, info.value);
    }

    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
}

bool CEnvironmentView::IsLess(size_t leftIndex, size_t rightIndex) const
{
    const std::vector<CEnvironmentVariable>& items = GetDocument()->GetEnvironment();

    const CEnvironmentVariable& left  = items[leftIndex];
    const CEnvironmentVariable& right = items[rightIndex];

    if (GetSortColumn() == colValue)
    {
        const int result = left.value.CompareNoCase(right.value);
        if (result != 0)
            return result < 0;
    }

    return (left.name.CompareNoCase(right.name) < 0);
}
