#pragma once

#include "SortedListView.h"

class CProcMonDoc;

// CEnvironmentView - pogled s varijablama okoline odabranog procesa.
class CEnvironmentView : public CSortedListView
{
    DECLARE_DYNCREATE(CEnvironmentView)

protected:
    CEnvironmentView();

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
        colName = 0,
        colValue
    };

    void InsertColumns();

    virtual void FillList();
    virtual bool IsLess(size_t leftIndex, size_t rightIndex) const;

public:
    virtual ~CEnvironmentView();
};
