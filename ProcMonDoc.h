#pragma once

#include "ProcessCollector.h"
#include "ThreadCollector.h"
#include "ModuleCollector.h"
#include "MemoryCollector.h"
#include "HistoryCollector.h"
#include "EnvironmentCollector.h"
#include "MemoryReader.h"
#include "HandleCollector.h"
#include "StringScanner.h"

#include <set>

// Jedan redak u prikazu. Osim podataka o procesu nosi i podatke potrebne za
// crtanje stabla: dubinu u hijerarhiji, ima li proces djecu i je li rasiren.
struct CProcessRow
{
    CProcessInfo info;
    int  depth       = 0;
    bool hasChildren = false;
    bool expanded    = true;
};

// CProcMonDoc - dokument u arhitekturi dokument/pogled. Sadrzi sve podatke,
// postavke prikaza i obradu naredbi s Ribbon trake. Sva tri pogleda rade nad
// istim dokumentom.
class CProcMonDoc : public CDocument
{
protected: // dokument stvara samo okvir aplikacije
    CProcMonDoc();
    DECLARE_DYNCREATE(CProcMonDoc)

public:
    // Redoslijed stupaca u popisu procesa; koristi se i za sortiranje.
    enum ProcessColumn
    {
        colPid = 0,
        colName,
        colCpu,
        colWorkingSet,
        colPrivate,
        colThreadCount,
        colPath,
        colCount
    };

    const std::vector<CProcessRow>&  GetVisibleProcesses() const { return m_visible; }
    const std::vector<CThreadInfo>&  GetThreads() const { return m_threads.GetAll(); }
    const std::vector<CModuleInfo>&  GetModules() const { return m_modules.GetAll(); }
    const std::vector<CRegionInfo>&  GetRegions() const { return m_memory.GetAll(); }

    const std::vector<CHistorySample>&       GetHistory() const { return m_history.GetAll(); }
    const std::vector<CEnvironmentVariable>& GetEnvironment() const { return m_environment.GetAll(); }
    const std::vector<CHandleInfo>&          GetHandles() const { return m_handles.GetAll(); }
    const std::vector<CStringInfo>&          GetStrings() const { return m_strings.GetAll(); }

    bool AreModulesAccessible() const { return m_modules.IsAccessible(); }
    bool AreRegionsAccessible() const { return m_memory.IsAccessible(); }
    bool IsEnvironmentAccessible() const { return m_environment.IsAccessible(); }
    bool AreHandlesAccessible() const { return m_handles.IsAccessible(); }
    bool AreStringsAccessible() const { return m_strings.IsAccessible(); }
    bool AreStringsPartial() const { return m_strings.IsPartial(); }
    bool IsHexAccessible() const { return m_reader.IsAccessible(); }
    bool IsHexValid(size_t offset) const { return m_reader.IsValid(offset); }

    ULONGLONG GetCommittedBytes() const { return m_memory.GetCommittedBytes(); }
    ULONGLONG GetReservedBytes() const { return m_memory.GetReservedBytes(); }
    ULONGLONG GetTotalMemory() const { return m_history.GetTotalMemory(); }

    DWORD   GetSelectedPid() const { return m_selectedPid; }
    CString GetSelectedProcessName() const;
    bool    IsAutoRefresh() const { return m_bAutoRefresh; }
    bool    IsTreeMode() const { return m_bTreeMode; }

    // Proces za koji je ocitana mapa memorije; nula znaci da mape nema.
    DWORD   GetMemoryPid() const { return m_memoryPid; }

    // Proces za koji je ocitan popis handle-ova; nula znaci da popisa nema.
    DWORD   GetHandlesPid() const { return m_handlesPid; }

    // Proces nad kojim je obavljena pretraga nizova.
    DWORD   GetStringsPid() const { return m_stringsPid; }

    // Sadrzaj prozora memorije koji prikazuje heksadekadski pogled.
    const std::vector<BYTE>& GetHexData() const { return m_reader.GetData(); }
    ULONGLONG GetHexAddress() const { return m_reader.GetAddress(); }
    const CRegionInfo& GetHexRegion() const { return m_reader.GetRegion(); }

    // Postavlja odnosno pomice pocetnu adresu heksadekadskog prikaza.
    void SetHexAddress(ULONGLONG address);
    void MoveHexAddress(LONGLONG delta);

    // Polozaj odabrane regije u popisu; -1 znaci da nijedna nije odabrana.
    // Preko dokumenta se odabir prenosi izmedu grafickog prikaza i popisa.
    int  GetSelectedRegion() const { return m_selectedRegion; }
    void SetSelectedRegion(int index);

    // Ocitava stanje sustava i obavjescuje poglede.
    void RefreshData();

    // Obavlja sva ocitanja koja se ne osvjezavaju sama: mapu memorije, popis
    // otvorenih objekata i pretragu znakovnih nizova. Ta su ocitanja preskupa
    // da bi se ponavljala svake tri sekunde, pa ih korisnik pokrece jednom
    // naredbom nad odabranim procesom.
    void AnalyzeProcess();

    void SetSelectedPid(DWORD pid);
    void SetFilter(const CString& filter);

    // Ponovni klik na isti stupac obrce redoslijed.
    void SortByColumn(int column);

    void ToggleExpand(DWORD pid);

    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);

protected:
    afx_msg void OnRefresh();
    afx_msg void OnAutoRefresh();
    afx_msg void OnUpdateAutoRefresh(CCmdUI* pCmdUI);
    afx_msg void OnTreeMode();
    afx_msg void OnUpdateTreeMode(CCmdUI* pCmdUI);
    afx_msg void OnKillProcess();
    afx_msg void OnUpdateNeedsSelection(CCmdUI* pCmdUI);

    DECLARE_MESSAGE_MAP()

private:
    // Iz svih ocitanih procesa gradi popis pripremljen za prikaz.
    void BuildVisibleList();
    void BuildFlatList();
    void BuildTreeList();
    void AddSubtree(size_t parentIndex, int depth, std::set<DWORD>& visited);

    bool CompareProcesses(const CProcessInfo& left, const CProcessInfo& right) const;
    bool IsLess(const CProcessInfo& left, const CProcessInfo& right) const;
    bool MatchesFilter(const CProcessInfo& info) const;
    bool IsRealParent(const CProcessInfo& child) const;

    // Ocitava dretve i module odabranog procesa.
    void RefreshDetails();

    void RefreshMemoryMap();
    void RefreshHandles();
    void RefreshStrings();

    // Ponistava mapu memorije i popis handle-ova kad vise ne opisuju odabrani
    // proces.
    void ClearMemoryMap();
    void ClearHandles();
    void ClearStrings();

    // Cita varijable okoline odabranog procesa. Za razliku od dretvi i modula
    // one se ne mijenjaju za zivota procesa, pa se citaju samo kod promjene
    // odabira.
    void RefreshEnvironment();

    void UpdateStatusBar();
    void ReportKillError(const CString& name, DWORD pid, DWORD dwError) const;
    void CollectDescendants(DWORD pid, std::vector<DWORD>& result) const;
    bool TerminateOne(DWORD pid, DWORD& dwError) const;

    CProcessCollector     m_processes;
    CThreadCollector      m_threads;
    CModuleCollector      m_modules;
    CMemoryCollector      m_memory;
    CHistoryCollector     m_history;
    CEnvironmentCollector m_environment;
    CMemoryReader         m_reader;
    CHandleCollector      m_handles;
    CStringScanner        m_strings;

    std::vector<CProcessRow> m_visible;     // redci pripremljeni za prikaz
    std::set<DWORD>          m_collapsed;   // sklopljeni cvorovi stabla

    DWORD   m_selectedPid;
    DWORD   m_memoryPid;            // proces za koji vrijedi ocitana mapa
    DWORD   m_handlesPid;           // proces za koji vrijedi popis handle-ova
    DWORD   m_stringsPid;           // proces nad kojim je pretraga obavljena
    int     m_selectedRegion;       // odabrana regija u mapi memorije
    CString m_filter;
    int     m_sortColumn;
    bool    m_bSortAscending;
    bool    m_bAutoRefresh;
    bool    m_bTreeMode;

public:
    virtual ~CProcMonDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
};
