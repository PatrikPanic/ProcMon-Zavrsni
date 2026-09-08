#pragma once

#include "SortedListView.h"

class CProcMonDoc;

// CHandleView - pogled s popisom objekata koje je odabrani proces otvorio.
class CHandleView : public CSortedListView
{
    DECLARE_DYNCREATE(CHandleView)

protected:
    CHandleView();

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
        colHandle = 0,
        colType,
        colAccess,
        colName
    };

    void InsertColumns();

    virtual void FillList();
    virtual bool IsLess(size_t leftIndex, size_t rightIndex) const;

public:
    virtual ~CHandleView();
};
