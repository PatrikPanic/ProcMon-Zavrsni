#include "pch.h"
#include "framework.h"
#include "SortedListView.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CSortedListView, CListView)

BEGIN_MESSAGE_MAP(CSortedListView, CListView)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CSortedListView::OnColumnClick)
END_MESSAGE_MAP()

CSortedListView::CSortedListView()
    : m_sortColumn(-1), m_bSortAscending(true)
{
}

CSortedListView::~CSortedListView()
{
}

void CSortedListView::BuildOrder(size_t count, std::vector<size_t>& order) const
{
    order.resize(count);

    for (size_t i = 0; i < count; ++i)
        order[i] = i;

    if (m_sortColumn < 0)
        return;

    // Silazni redoslijed dobiva se zamjenom argumenata, a ne negacijom
    // rezultata: negacija bi kod jednakih vrijednosti dala da je istovremeno
    // left < right i right < left, sto nije dopusteno.
    std::sort(order.begin(), order.end(),
              [this](size_t left, size_t right)
              {
                  return m_bSortAscending ? IsLess(left, right) : IsLess(right, left);
              });
}

void CSortedListView::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pInfo = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;

    // Prvi klik na stupac sortira uzlazno, ponovni klik na isti stupac obrce
    // redoslijed.
    if (m_sortColumn == pInfo->iSubItem)
    {
        m_bSortAscending = !m_bSortAscending;
    }
    else
    {
        m_sortColumn     = pInfo->iSubItem;
        m_bSortAscending = true;
    }

    FillList();
}
