#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "StringView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CStringView, CSortedListView)

BEGIN_MESSAGE_MAP(CStringView, CSortedListView)
END_MESSAGE_MAP()

CStringView::CStringView()
{
}

CStringView::~CStringView()
{
}

CProcMonDoc* CStringView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CStringView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CSortedListView::PreCreateWindow(cs);
}

void CStringView::OnInitialUpdate()
{
    CSortedListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CStringView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint != HINT_STRINGS && lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    // Pretraga se pokrece na zahtjev i moze dati desetke tisuca redaka, pa je
    // redovno ocitanje procesa ne dira.
    if (lHint == HINT_PROCESSES && GetDocument()->GetStringsPid() != 0)
        return;

    FillList();
}

void CStringView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    list.InsertColumn(colAddress, CSysUtil::LoadStr(IDS_COL_STRING_ADDRESS), LVCFMT_LEFT,  150);
    list.InsertColumn(colKind,    CSysUtil::LoadStr(IDS_COL_STRING_KIND),    LVCFMT_LEFT,  90);
    list.InsertColumn(colLength,  CSysUtil::LoadStr(IDS_COL_STRING_LENGTH),  LVCFMT_RIGHT, 70);
    list.InsertColumn(colText,    CSysUtil::LoadStr(IDS_COL_STRING_TEXT),    LVCFMT_LEFT,  700);
}

void CStringView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    if (pDoc->GetStringsPid() == 0)
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_STRINGS_NO_LIST));
        return;
    }

    if (!pDoc->AreStringsAccessible())
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_STRINGS_DENIED));
        return;
    }

    const std::vector<CStringInfo>& items = pDoc->GetStrings();

    std::vector<size_t> order;
    BuildOrder(items.size(), order);

    // Redaka zna biti nekoliko desetaka tisuca, pa se crtanje zaustavlja dok se
    // popis puni.
    list.SetRedraw(FALSE);

    CString text;

    for (size_t i = 0; i < order.size(); ++i)
    {
        const CStringInfo& info = items[order[i]];

        text.Format(_T("0x%016llX"), info.address);

        const int index = list.InsertItem(static_cast<int>(i), text);
        if (index < 0)
            continue;

        list.SetItemText(index, colKind,
                         CSysUtil::LoadStr(info.bWide ? IDS_STRING_WIDE : IDS_STRING_ASCII));

        text.Format(_T("%d"), info.text.GetLength());
        list.SetItemText(index, colLength, text);

        list.SetItemText(index, colText, info.text);
    }

    list.SetRedraw(TRUE);

    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
}

bool CStringView::IsLess(size_t leftIndex, size_t rightIndex) const
{
    const std::vector<CStringInfo>& items = GetDocument()->GetStrings();

    const CStringInfo& left  = items[leftIndex];
    const CStringInfo& right = items[rightIndex];

    switch (GetSortColumn())
    {
    case colKind:
        if (left.bWide != right.bWide)
            return (!left.bWide && right.bWide);
        break;

    case colLength:
        if (left.text.GetLength() != right.text.GetLength())
            return left.text.GetLength() < right.text.GetLength();
        break;

    case colText:
        {
            const int result = left.text.CompareNoCase(right.text);
            if (result != 0)
                return result < 0;
        }
        break;

    case colAddress:
    default:
        break;
    }

    return left.address < right.address;
}
