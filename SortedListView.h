#pragma once

#include <afxcview.h>
#include <vector>

// CSortedListView - zajednicka osnova pogleda ciji se popis sortira klikom na
// zaglavlje stupca. Izvedeni pogled odreduje samo kako se usporeduju dvije
// stavke; stanje sortiranja i reakciju na klik vodi ova klasa.
class CSortedListView : public CListView
{
    DECLARE_DYNAMIC(CSortedListView)

protected:
    CSortedListView();

    // Popunjava redoslijed kojim se stavke ispisuju. Dok stupac za sortiranje
    // nije odabran, redoslijed ostaje onakav kakvim ga je vratio sustav.
    void BuildOrder(size_t count, std::vector<size_t>& order) const;

    int GetSortColumn() const { return m_sortColumn; }

    // Usporeduje stavke na zadanim polozajima po odabranom stupcu.
    virtual bool IsLess(size_t leftIndex, size_t rightIndex) const = 0;

    // Ponovno ispisuje popis; poziva se nakon promjene sortiranja.
    virtual void FillList() = 0;

    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    int  m_sortColumn;      // -1 dok korisnik ne odabere stupac
    bool m_bSortAscending;

public:
    virtual ~CSortedListView();
};
