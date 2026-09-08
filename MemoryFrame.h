#pragma once

#include "ChildFrm.h"

// CMemoryFrame - okvir kartice s mapom memorije. Ta kartica jedina sadrzi dva
// pogleda, pa okvir umjesto jednog pogleda stvara podijeljeni prozor: gore je
// graficki prikaz, dolje popis regija.
class CMemoryFrame : public CChildFrame
{
    DECLARE_DYNCREATE(CMemoryFrame)

public:
    CMemoryFrame();

protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

private:
    enum { mapHeight = 200 };   // pocetna visina gornjeg dijela

    CSplitterWnd m_splitter;

public:
    virtual ~CMemoryFrame();
};
