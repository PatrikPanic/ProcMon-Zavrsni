#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "HandleView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHandleView, CSortedListView)

BEGIN_MESSAGE_MAP(CHandleView, CSortedListView)
END_MESSAGE_MAP()

CHandleView::CHandleView()
{
}

CHandleView::~CHandleView()
{
}

CProcMonDoc* CHandleView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CHandleView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CSortedListView::PreCreateWindow(cs);
}

void CHandleView::OnInitialUpdate()
{
    CSortedListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CHandleView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint != HINT_HANDLES && lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    // Popis se ocitava na zahtjev jer je za velike procese skup; uz redovno
    // ocitanje procesa osvjezava se samo dok popisa nema.
    if (lHint == HINT_PROCESSES && GetDocument()->GetHandlesPid() != 0)
        return;

    FillList();
}

void CHandleView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    list.InsertColumn(colHandle, CSysUtil::LoadStr(IDS_COL_HANDLE_VALUE),  LVCFMT_LEFT,  100);
    list.InsertColumn(colType,   CSysUtil::LoadStr(IDS_COL_HANDLE_TYPE),   LVCFMT_LEFT,  160);
    list.InsertColumn(colAccess, CSysUtil::LoadStr(IDS_COL_HANDLE_ACCESS), LVCFMT_RIGHT, 120);
    list.InsertColumn(colName,   CSysUtil::LoadStr(IDS_COL_HANDLE_NAME),   LVCFMT_LEFT,  620);
}

void CHandleView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    if (pDoc->GetHandlesPid() == 0)
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_HANDLES_NO_LIST));
        return;
    }

    if (!pDoc->AreHandlesAccessible())
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_HANDLES_DENIED));
        return;
    }

    const std::vector<CHandleInfo>& items = pDoc->GetHandles();

    std::vector<size_t> order;
    BuildOrder(items.size(), order);

    list.SetRedraw(FALSE);

    CString text;

    for (size_t i = 0; i < order.size(); ++i)
    {
        const CHandleInfo& info = items[order[i]];

        text.Format(_T("0x%04llX"), info.value);

        const int index = list.InsertItem(static_cast<int>(i), text);
        if (index < 0)
            continue;

        text.Format(_T("0x%08X"), info.access);

        list.SetItemText(index, colType,   info.type);
        list.SetItemText(index, colAccess, text);
        list.SetItemText(index, colName,   info.name);
    }

    list.SetRedraw(TRUE);

    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
}

bool CHandleView::IsLess(size_t leftIndex, size_t rightIndex) const
{
    const std::vector<CHandleInfo>& items = GetDocument()->GetHandles();

    const CHandleInfo& left  = items[leftIndex];
    const CHandleInfo& right = items[rightIndex];

    switch (GetSortColumn())
    {
    case colType:
        {
            const int result = left.type.CompareNoCase(right.type);
            if (result != 0)
                return result < 0;
        }
        break;

    case colAccess:
        if (left.access != right.access)
            return left.access < right.access;
        break;

    case colName:
        {
            const int result = left.name.CompareNoCase(right.name);
            if (result != 0)
                return result < 0;
        }
        break;

    case colHandle:
    default:
        break;
    }

    // Vrijednost handle-a je u jednom procesu jedinstvena, pa daje jednoznacan
    // redoslijed kod jednakih vrijednosti.
    return left.value < right.value;
}
