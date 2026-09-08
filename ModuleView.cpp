#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "ModuleView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CModuleView, CSortedListView)

BEGIN_MESSAGE_MAP(CModuleView, CSortedListView)
END_MESSAGE_MAP()

CModuleView::CModuleView()
{
}

CModuleView::~CModuleView()
{
}

CProcMonDoc* CModuleView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CModuleView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CSortedListView::PreCreateWindow(cs);
}

void CModuleView::OnInitialUpdate()
{
    CSortedListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CModuleView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    FillList();
}

void CModuleView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    list.InsertColumn(colModule,      CSysUtil::LoadStr(IDS_COL_MODULE),      LVCFMT_LEFT,  200);
    list.InsertColumn(colBaseAddress, CSysUtil::LoadStr(IDS_COL_BASEADDRESS), LVCFMT_RIGHT, 150);
    list.InsertColumn(colSize,        CSysUtil::LoadStr(IDS_COL_SIZE),        LVCFMT_RIGHT, 100);
    list.InsertColumn(colPath,        CSysUtil::LoadStr(IDS_COL_MODULEPATH),  LVCFMT_LEFT,  450);
}

void CModuleView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    const std::vector<CModuleInfo>& items = pDoc->GetModules();
    const int topIndex = list.GetTopIndex();

    std::vector<size_t> order;
    BuildOrder(items.size(), order);

    // Odabir se izricito ponistava prije praznjenja popisa, inace obojeni redak
    // ostaje nacrtan i nakon brisanja stavke.
    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    if (pDoc->GetSelectedPid() == 0)
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_THREADS_NO_SELECTION));
        return;
    }

    // Snimka modula ne uspijeva za procese koje stiti sam sustav. Bez poruke bi
    // korisnik vidio samo prazan popis i ne bi znao zasto.
    if (!pDoc->AreModulesAccessible())
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_MODULES_DENIED));
        return;
    }

    CString text;

    for (size_t i = 0; i < order.size(); ++i)
    {
        const CModuleInfo& info = items[order[i]];

        const int index = list.InsertItem(static_cast<int>(i), info.name);
        if (index < 0)
            continue;

        text.Format(_T("0x%016llX"), info.baseAddress);
        list.SetItemText(index, colBaseAddress, text);

        list.SetItemText(index, colSize, CSysUtil::FormatBytes(info.size));
        list.SetItemText(index, colPath, info.path);
    }

    if (topIndex > 0 && topIndex < list.GetItemCount())
        list.EnsureVisible(topIndex, FALSE);

    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
}

bool CModuleView::IsLess(size_t leftIndex, size_t rightIndex) const
{
    const std::vector<CModuleInfo>& items = GetDocument()->GetModules();

    const CModuleInfo& left  = items[leftIndex];
    const CModuleInfo& right = items[rightIndex];

    switch (GetSortColumn())
    {
    case colModule:
        {
            const int result = left.name.CompareNoCase(right.name);
            if (result != 0)
                return result < 0;
        }
        break;

    case colSize:
        if (left.size != right.size)
            return left.size < right.size;
        break;

    case colPath:
        {
            const int result = left.path.CompareNoCase(right.path);
            if (result != 0)
                return result < 0;
        }
        break;

    case colBaseAddress:
    default:
        break;
    }

    // Moduli s jednakom vrijednoscu razvrstavaju se po baznoj adresi, koja je
    // u jednom procesu jedinstvena.
    return left.baseAddress < right.baseAddress;
}
