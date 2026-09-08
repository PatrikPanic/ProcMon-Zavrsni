#pragma once

#include "SortedListView.h"

class CProcMonDoc;

// CStringView - pogled s popisom znakovnih nizova pronadenih u memoriji
// odabranog procesa.
class CStringView : public CSortedListView
{
    DECLARE_DYNCREATE(CStringView)

protected:
    CStringView();

public:
    CProcMonDoc* GetDocument() const;

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

    DECLARE_MESSAGE_MAP()

private:
    enum Column
    {
        colAddress = 0,
        colKind,
        colLength,
        colText
    };

    void InsertColumns();

    virtual void FillList();
    virtual bool IsLess(size_t leftIndex, size_t rightIndex) const;

public:
    virtual ~CStringView();
};
