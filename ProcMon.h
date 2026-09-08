#pragma once

#ifndef __AFXWIN_H__
    #error "ukljucite 'pch.h' prije ove datoteke"
#endif

#include "resource.h"

// CProcMonApp - klasa aplikacije. Stvara devet predlozaka dokumenta: jedan za
// popis procesa (registriran kao glavni) te po jedan za prikaz dretvi, modula,
// mape memorije, heksadekadskog prikaza, handle-ova, znakovnih nizova,
// varijabli okoline i grafikona. Sve kartice otvaraju se pri pokretanju, nad
// istim dokumentom.
class CProcMonApp : public CWinAppEx
{
public:
    CProcMonApp();

    virtual BOOL InitInstance();
    virtual int  ExitInstance();

protected:
    afx_msg void OnAppAbout();
    DECLARE_MESSAGE_MAP()

private:
    // Otvara dodatnu karticu s pogledom iz zadanog predloska, nad vec
    // postojecim dokumentom.
    void CreateAdditionalView(CMultiDocTemplate* pTemplate, CDocument* pDoc);

    CMultiDocTemplate* m_pProcessTemplate;
    CMultiDocTemplate* m_pThreadTemplate;
    CMultiDocTemplate* m_pModuleTemplate;
    CMultiDocTemplate* m_pMemoryTemplate;
    CMultiDocTemplate* m_pGraphTemplate;
    CMultiDocTemplate* m_pEnvironmentTemplate;
    CMultiDocTemplate* m_pHexTemplate;
    CMultiDocTemplate* m_pHandleTemplate;
    CMultiDocTemplate* m_pStringTemplate;
};

extern CProcMonApp theApp;
