#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "MemoryView.h"
#include "SysUtil.h"
#include "Commands.h"
#include "MainFrm.h"
#include "HexView.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMemoryView, CSortedListView)

BEGIN_MESSAGE_MAP(CMemoryView, CSortedListView)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CMemoryView::OnItemChanged)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CMemoryView::OnDoubleClick)
END_MESSAGE_MAP()

CMemoryView::CMemoryView()
    : m_bFilling(false)
{
}

CMemoryView::~CMemoryView()
{
}

CProcMonDoc* CMemoryView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CProcMonDoc)));
    return (CProcMonDoc*)m_pDocument;
}

BOOL CMemoryView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    return CSortedListView::PreCreateWindow(cs);
}

void CMemoryView::OnInitialUpdate()
{
    CSortedListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                   LVS_EX_DOUBLEBUFFER);
    InsertColumns();
    FillList();
}

void CMemoryView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    // Promjena odabrane regije ne mijenja sadrzaj popisa, nego samo odabrani
    // redak.
    if (lHint == HINT_REGION)
    {
        SelectRegion(GetDocument()->GetSelectedRegion());
        return;
    }

    if (lHint != HINT_MEMORY && lHint != HINT_SELECTION && lHint != HINT_PROCESSES)
        return;

    // Mapa se ocitava na zahtjev i ima nekoliko tisuca redaka, pa je redovno
    // ocitanje procesa ne dira. Iznimka je stanje bez mape: popis je tada
    // kratak, a osvjezavanje treba da bi se uputa pojavila cim odabrani proces
    // zavrsi.
    if (lHint == HINT_PROCESSES && GetDocument()->GetMemoryPid() != 0)
        return;

    FillList();
}

void CMemoryView::InsertColumns()
{
    CListCtrl& list = GetListCtrl();

    // Prvi stupac popisa Windows uvijek poravnava lijevo; adrese su ionako
    // jednake sirine pa stupac izgleda uredno.
    list.InsertColumn(colAddress,    CSysUtil::LoadStr(IDS_COL_REGION_ADDRESS), LVCFMT_LEFT,  150);
    list.InsertColumn(colSize,       CSysUtil::LoadStr(IDS_COL_REGION_SIZE),    LVCFMT_RIGHT, 100);
    list.InsertColumn(colState,      CSysUtil::LoadStr(IDS_COL_REGION_STATE),   LVCFMT_LEFT,  100);
    list.InsertColumn(colProtection, CSysUtil::LoadStr(IDS_COL_REGION_PROTECT), LVCFMT_LEFT,  220);
    list.InsertColumn(colType,       CSysUtil::LoadStr(IDS_COL_REGION_TYPE),    LVCFMT_LEFT,  140);
    list.InsertColumn(colFile,       CSysUtil::LoadStr(IDS_COL_REGION_FILE),    LVCFMT_LEFT,  420);
}

void CMemoryView::FillList()
{
    CListCtrl& list = GetListCtrl();
    CProcMonDoc* pDoc = GetDocument();

    m_bFilling = true;

    list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    list.DeleteAllItems();

    // Kartica je otvorena i prije nego je mapa ocitana, pa se umjesto praznog
    // popisa ispisuje uputa.
    if (pDoc->GetMemoryPid() == 0)
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_MEMORY_NO_MAP));
        m_bFilling = false;
        return;
    }

    // Kod zasticenih procesa otvaranje ne uspije, a kod nekih sistemskih uspije
    // ali obilazak ne vrati nijednu regiju; korisniku je ishod isti.
    if (!pDoc->AreRegionsAccessible() || pDoc->GetRegions().empty())
    {
        list.InsertItem(0, CSysUtil::LoadStr(IDS_MEMORY_DENIED));
        m_bFilling = false;
        return;
    }

    const std::vector<CRegionInfo>& items = pDoc->GetRegions();

    std::vector<size_t> order;
    BuildOrder(items.size(), order);

    // Redaka je nekoliko tisuca, pa se crtanje zaustavlja dok se popis puni.
    list.SetRedraw(FALSE);

    CString text;

    for (size_t i = 0; i < order.size(); ++i)
    {
        const CRegionInfo& info = items[order[i]];
        const int index = static_cast<int>(i);

        text.Format(_T("0x%016llX"), info.baseAddress);
        if (list.InsertItem(index, text) < 0)
            continue;

        // Uz redak se pamti polozaj regije u popisu dokumenta, kako bi se
        // kasnije mogao povezati s grafickim prikazom.
        list.SetItemData(index, static_cast<DWORD_PTR>(order[i]));

        list.SetItemText(index, colSize,       CSysUtil::FormatBytes(info.size));
        list.SetItemText(index, colState,      CMemoryCollector::FormatState(info.state));
        list.SetItemText(index, colProtection, CMemoryCollector::FormatProtection(info.protect));
        list.SetItemText(index, colType,       CMemoryCollector::FormatType(info.type));
        list.SetItemText(index, colFile,       info.mappedFile);
    }

    list.SetRedraw(TRUE);
    m_bFilling = false;

    // Nakon ponovnog punjenja, primjerice zbog sortiranja, odabir se vraca na
    // isti redak.
    SelectRegion(pDoc->GetSelectedRegion());

    list.RedrawWindow(nullptr, nullptr,
                      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                      RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CMemoryView::SelectRegion(int index)
{
    CListCtrl& list = GetListCtrl();

    if (index < 0)
    {
        list.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        return;
    }

    for (int i = 0; i < list.GetItemCount(); ++i)
    {
        if (static_cast<int>(list.GetItemData(i)) != index)
            continue;

        list.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        list.EnsureVisible(i, FALSE);
        return;
    }
}

void CMemoryView::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pInfo = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;

    if (m_bFilling || GetDocument()->GetMemoryPid() == 0)
        return;

    if ((pInfo->uChanged & LVIF_STATE) && (pInfo->uNewState & LVIS_SELECTED))
    {
        const int index = static_cast<int>(GetListCtrl().GetItemData(pInfo->iItem));
        GetDocument()->SetSelectedRegion(index);
    }
}

bool CMemoryView::IsLess(size_t leftIndex, size_t rightIndex) const
{
    const std::vector<CRegionInfo>& items = GetDocument()->GetRegions();

    const CRegionInfo& left  = items[leftIndex];
    const CRegionInfo& right = items[rightIndex];

    switch (GetSortColumn())
    {
    case colSize:
        if (left.size != right.size)
            return left.size < right.size;
        break;

    // Stanje, zastita i tip usporeduju se po vrijednosti koju vraca sustav, pa
    // se jednake regije grupiraju: zauzete pred rezerviranima, a rezervirane
    // pred slobodnima.
    case colState:
        if (left.state != right.state)
            return left.state < right.state;
        break;

    case colProtection:
        if (left.protect != right.protect)
            return left.protect < right.protect;
        break;

    case colType:
        if (left.type != right.type)
            return left.type < right.type;
        break;

    case colFile:
        {
            const int result = left.mappedFile.CompareNoCase(right.mappedFile);
            if (result != 0)
                return result < 0;
        }
        break;

    case colAddress:
    default:
        break;
    }

    // Regije s jednakom vrijednoscu ostaju poredane po adresi.
    return left.baseAddress < right.baseAddress;
}

void CMemoryView::OnDoubleClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    *pResult = 0;

    CProcMonDoc* pDoc = GetDocument();
    const int selected = pDoc->GetSelectedRegion();

    if (pDoc->GetMemoryPid() == 0 || selected < 0)
        return;

    const std::vector<CRegionInfo>& items = pDoc->GetRegions();
    if (static_cast<size_t>(selected) >= items.size())
        return;

    pDoc->SetHexAddress(items[selected].baseAddress);

    // Prebacivanje na drugu karticu je posao glavnog okvira.
    CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    if (pFrame != nullptr)
        pFrame->ActivateView(RUNTIME_CLASS(CHexView));
}
