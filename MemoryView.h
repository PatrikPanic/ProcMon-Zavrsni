#pragma once

#include "SortedListView.h"

class CProcMonDoc;

// CMemoryView - pogled s popisom regija virtualnog adresnog prostora
// odabranog procesa.
class CMemoryView : public CSortedListView
{
    DECLARE_DYNCREATE(CMemoryView)

protected:
    CMemoryView();

public:
    CProcMonDoc* GetDocument() const;

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

    afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    enum Column
    {
        colAddress = 0,
        colSize,
        colState,
        colProtection,
        colType,
        colFile
    };

    void InsertColumns();

    // Postavlja odabir na redak koji prikazuje zadanu regiju.
    void SelectRegion(int index);

    virtual void FillList();
    virtual bool IsLess(size_t leftIndex, size_t rightIndex) const;

    bool m_bFilling;    // sprjecava reakciju na promjenu odabira tijekom punjenja

public:
    virtual ~CMemoryView();
};
