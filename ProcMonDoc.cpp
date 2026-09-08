#include "pch.h"
#include "framework.h"
#include "ProcMon.h"
#include "ProcMonDoc.h"
#include "MainFrm.h"
#include "ScanProgressDlg.h"
#include "SysUtil.h"
#include "Commands.h"
#include "StringIDs.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CProcMonDoc, CDocument)

BEGIN_MESSAGE_MAP(CProcMonDoc, CDocument)
    ON_COMMAND(ID_CMD_REFRESH, &CProcMonDoc::OnRefresh)
    ON_COMMAND(ID_CMD_AUTOREFRESH, &CProcMonDoc::OnAutoRefresh)
    ON_UPDATE_COMMAND_UI(ID_CMD_AUTOREFRESH, &CProcMonDoc::OnUpdateAutoRefresh)
    ON_COMMAND(ID_CMD_TREE, &CProcMonDoc::OnTreeMode)
    ON_UPDATE_COMMAND_UI(ID_CMD_TREE, &CProcMonDoc::OnUpdateTreeMode)
    ON_COMMAND(ID_CMD_KILL_PROCESS, &CProcMonDoc::OnKillProcess)
    ON_UPDATE_COMMAND_UI(ID_CMD_KILL_PROCESS, &CProcMonDoc::OnUpdateNeedsSelection)
END_MESSAGE_MAP()

CProcMonDoc::CProcMonDoc()
    : m_selectedPid(0),
      m_memoryPid(0),
      m_handlesPid(0),
      m_stringsPid(0),
      m_selectedRegion(-1),
      m_sortColumn(colCpu),
      m_bSortAscending(false),      // najzahtjevniji procesi na vrhu
      m_bAutoRefresh(true),
      m_bTreeMode(true)
{
}

CProcMonDoc::~CProcMonDoc()
{
}

BOOL CProcMonDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    SetTitle(CSysUtil::LoadStr(IDS_TITLE_PROCESSES));
    RefreshData();

    return TRUE;
}

void CProcMonDoc::Serialize(CArchive& /*ar*/)
{
    // Aplikacija prikazuje trenutno stanje sustava i nema sadrzaj koji bi se
    // spremao u datoteku, pa je serijalizacija namjerno prazna.
}

void CProcMonDoc::RefreshData()
{
    m_processes.Refresh();

    // Ako je odabrani proces u meduvremenu zavrsio, odabir se ponistava.
    if (m_selectedPid != 0 && m_processes.Find(m_selectedPid) == nullptr)
    {
        m_selectedPid = 0;
        ClearMemoryMap();
        ClearHandles();
        ClearStrings();
        RefreshEnvironment();
    }

    // Povijest se dopunjava jednim ocitanjem, i to prije gradnje popisa jer ne
    // ovisi o filtriranju ni o sortiranju.
    m_history.Add(m_processes.Find(m_selectedPid));

    BuildVisibleList();
    RefreshDetails();
    UpdateStatusBar();

    UpdateAllViews(nullptr, HINT_PROCESSES);
}

void CProcMonDoc::RefreshDetails()
{
    m_threads.Refresh(m_selectedPid);
    m_modules.Refresh(m_selectedPid);
}

void CProcMonDoc::AnalyzeProcess()
{
    if (m_selectedPid == 0)
        return;

    // Pretraga nizova ide zadnja jer jedina otvara prozor s napretkom; do tada
    // su ostali prikazi vec popunjeni.
    RefreshMemoryMap();
    RefreshHandles();
    RefreshStrings();
}

void CProcMonDoc::RefreshMemoryMap()
{
    // Naredba je dostupna samo dok je proces odabran, pa je provjera zastita
    // za slucaj da naredba stigne iz nekog drugog izvora.
    if (m_selectedPid == 0)
        return;

    // Obilazak cijelog adresnog prostora traje osjetno dulje od ostalih
    // ocitanja, pa se za to vrijeme mijenja oblik pokazivaca.
    CWaitCursor wait;

    m_memory.Refresh(m_selectedPid);
    m_memoryPid      = m_selectedPid;
    m_selectedRegion = -1;

    UpdateAllViews(nullptr, HINT_MEMORY);
}

void CProcMonDoc::SetHexAddress(ULONGLONG address)
{
    if (m_reader.GetAddress() == address)
        return;

    m_reader.Read(m_selectedPid, address);

    // Traka s adresom prati prikaz, jer se adresa mijenja i dvoklikom na regiju,
    // ne samo upisom.
    CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    if (pFrame != nullptr)
        pFrame->SetAddressText(address);

    UpdateAllViews(nullptr, HINT_HEX);
}

void CProcMonDoc::MoveHexAddress(LONGLONG delta)
{
    const ULONGLONG current = m_reader.GetAddress();
    if (current == 0)
        return;

    // Pomak ispod pocetka adresnog prostora se zanemaruje.
    if (delta < 0 && current < static_cast<ULONGLONG>(-delta))
        return;

    // Listanje ne staje na kraju regije nego preskace na iducu dostupnu, pa
    // prikaz nikad ne zavrsi na adresi s koje se ne moze citati. Ako takve
    // regije nema, adresa ostaje gdje je bila.
    ULONGLONG address = 0;
    if (!m_reader.FindReadable(m_selectedPid, current + delta, delta > 0, address))
        return;

    SetHexAddress(address);
}

void CProcMonDoc::SetSelectedRegion(int index)
{
    if (m_selectedRegion == index)
        return;

    m_selectedRegion = index;

    UpdateAllViews(nullptr, HINT_REGION);
}

void CProcMonDoc::RefreshHandles()
{
    if (m_selectedPid == 0)
        return;

    // Kod procesa s nekoliko tisuca otvorenih objekata ocitanje traje osjetno
    // dulje od ostalih, pa se mijenja oblik pokazivaca.
    CWaitCursor wait;

    m_handles.Refresh(m_selectedPid);
    m_handlesPid = m_selectedPid;

    UpdateAllViews(nullptr, HINT_HANDLES);
}

void CProcMonDoc::RefreshStrings()
{
    if (m_selectedPid == 0)
        return;

    // Pretraga tece u pomocnoj niti prozora s napretkom, pa sucelje ostaje
    // odzivno i korisnik je moze prekinuti.
    CScanProgressDlg dialog(m_strings, m_selectedPid);
    dialog.DoModal();

    m_stringsPid = m_selectedPid;

    UpdateAllViews(nullptr, HINT_STRINGS);
}

void CProcMonDoc::RefreshEnvironment()
{
    m_environment.Refresh(m_selectedPid);
}

void CProcMonDoc::ClearMemoryMap()
{
    // Ocitanje s nulom samo prazni popis regija.
    m_memory.Refresh(0);
    m_memoryPid      = 0;
    m_selectedRegion = -1;

    // Prozor memorije pripada istom procesu, pa i on ostaje bez sadrzaja.
    m_reader.Read(0, 0);
}

void CProcMonDoc::ClearHandles()
{
    m_handles.Refresh(0);
    m_handlesPid = 0;
}

void CProcMonDoc::ClearStrings()
{
    m_strings.Clear();
    m_stringsPid = 0;
}

void CProcMonDoc::BuildVisibleList()
{
    m_visible.clear();
    m_visible.reserve(m_processes.GetAll().size());

    // Dok je filtar upisan, prikazuje se ravan popis pogodaka. Stablo tada nema
    // smisla jer bi roditelji pogodaka morali biti prikazani iako ne odgovaraju
    // uvjetu filtriranja.
    if (m_bTreeMode && m_filter.IsEmpty())
        BuildTreeList();
    else
        BuildFlatList();
}

void CProcMonDoc::BuildFlatList()
{
    const std::vector<CProcessInfo>& all = m_processes.GetAll();

    for (size_t i = 0; i < all.size(); ++i)
    {
        if (!MatchesFilter(all[i]))
            continue;

        CProcessRow row;
        row.info = all[i];
        m_visible.push_back(row);
    }

    std::sort(m_visible.begin(), m_visible.end(),
              [this](const CProcessRow& left, const CProcessRow& right)
              {
                  return CompareProcesses(left.info, right.info);
              });
}

void CProcMonDoc::BuildTreeList()
{
    // Krece se od procesa koji nemaju vidljivog roditelja, a zatim se rekurzivno
    // dodaju njihova djeca. Skup obidenih procesa sprjecava beskonacnu rekurziju
    // u slucaju neocekivanih podataka.
    const std::vector<CProcessInfo>& all = m_processes.GetAll();
    std::set<DWORD> visited;

    std::vector<const CProcessInfo*> roots;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (!IsRealParent(all[i]))
            roots.push_back(&all[i]);
    }

    std::sort(roots.begin(), roots.end(),
              [this](const CProcessInfo* left, const CProcessInfo* right)
              {
                  return CompareProcesses(*left, *right);
              });

    for (size_t i = 0; i < roots.size(); ++i)
    {
        if (visited.find(roots[i]->pid) != visited.end())
            continue;

        visited.insert(roots[i]->pid);

        CProcessRow row;
        row.info     = *roots[i];
        row.depth    = 0;
        row.expanded = (m_collapsed.find(roots[i]->pid) == m_collapsed.end());

        const size_t index = m_visible.size();
        m_visible.push_back(row);

        AddSubtree(index, 1, visited);
    }
}

void CProcMonDoc::AddSubtree(size_t parentIndex, int depth, std::set<DWORD>& visited)
{
    const DWORD parentPid = m_visible[parentIndex].info.pid;
    const std::vector<CProcessInfo>& all = m_processes.GetAll();

    std::vector<const CProcessInfo*> children;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i].parentPid == parentPid && all[i].pid != parentPid &&
            IsRealParent(all[i]) && visited.find(all[i].pid) == visited.end())
        {
            children.push_back(&all[i]);
        }
    }

    if (children.empty())
        return;

    // Cvor ima djecu bez obzira na to jesu li trenutno prikazana, jer o tome
    // ovisi hoce li se ispred naziva nacrtati oznaka za sklapanje.
    m_visible[parentIndex].hasChildren = true;

    if (!m_visible[parentIndex].expanded)
        return;

    std::sort(children.begin(), children.end(),
              [this](const CProcessInfo* left, const CProcessInfo* right)
              {
                  return CompareProcesses(*left, *right);
              });

    for (size_t i = 0; i < children.size(); ++i)
    {
        visited.insert(children[i]->pid);

        CProcessRow row;
        row.info     = *children[i];
        row.depth    = depth;
        row.expanded = (m_collapsed.find(children[i]->pid) == m_collapsed.end());

        const size_t index = m_visible.size();
        m_visible.push_back(row);

        AddSubtree(index, depth + 1, visited);
    }
}

bool CProcMonDoc::IsRealParent(const CProcessInfo& child) const
{
    if (child.parentPid == 0 || child.parentPid == child.pid)
        return false;

    const CProcessInfo* pParent = m_processes.Find(child.parentPid);
    if (pParent == nullptr)
        return false;

    // Windows ponovno dodjeljuje identifikatore zavrsenih procesa, pa se moze
    // dogoditi da "roditelj" bude noviji od svojeg "djeteta". Takva veza nije
    // stvarna i takav se proces prikazuje kao korijen.
    const ULONGLONG parentTime = CSysUtil::ToUInt64(pParent->creationTime);
    const ULONGLONG childTime  = CSysUtil::ToUInt64(child.creationTime);

    if (parentTime != 0 && childTime != 0 && parentTime > childTime)
        return false;

    return true;
}

bool CProcMonDoc::MatchesFilter(const CProcessInfo& info) const
{
    if (m_filter.IsEmpty())
        return true;

    CString name = info.name;
    CString filter = m_filter;

    name.MakeLower();
    filter.MakeLower();

    return (name.Find(filter) >= 0);
}

bool CProcMonDoc::CompareProcesses(const CProcessInfo& left, const CProcessInfo& right) const
{
    // Silazni redoslijed dobiva se zamjenom argumenata, a ne negacijom
    // rezultata: negacija bi kod jednakih vrijednosti dala da je istovremeno
    // left < right i right < left, sto nije dopusteno.
    return m_bSortAscending ? IsLess(left, right) : IsLess(right, left);
}

bool CProcMonDoc::IsLess(const CProcessInfo& left, const CProcessInfo& right) const
{
    switch (m_sortColumn)
    {
    case colName:
        {
            const int result = left.name.CompareNoCase(right.name);
            if (result != 0)
                return result < 0;
        }
        break;

    case colCpu:
        if (left.cpuPercent != right.cpuPercent)
            return left.cpuPercent < right.cpuPercent;
        break;

    case colWorkingSet:
        if (left.workingSet != right.workingSet)
            return left.workingSet < right.workingSet;
        break;

    case colPrivate:
        if (left.privateBytes != right.privateBytes)
            return left.privateBytes < right.privateBytes;
        break;

    case colThreadCount:
        if (left.threadCount != right.threadCount)
            return left.threadCount < right.threadCount;
        break;

    case colPath:
        {
            const int result = left.path.CompareNoCase(right.path);
            if (result != 0)
                return result < 0;
        }
        break;

    case colPid:
    default:
        break;
    }

    // Procesi s jednakom vrijednoscu razvrstavaju se po PID-u, cime je
    // redoslijed jednoznacan i popis ne poskakuje pri osvjezavanju.
    return left.pid < right.pid;
}

void CProcMonDoc::SetSelectedPid(DWORD pid)
{
    if (m_selectedPid == pid)
        return;

    m_selectedPid = pid;

    // Mapa memorije i popis handle-ova opisuju proces za koji su ocitani, pa se
    // kod promjene odabira ponistavaju i cekaju novu naredbu korisnika.
    ClearMemoryMap();
    ClearHandles();
    ClearStrings();

    // Krivulja opterecenja odnosi se na prethodni proces, pa bi se bez brisanja
    // u istom crtezu nasla dva razlicita procesa.
    m_history.ResetProcess();

    RefreshEnvironment();

    RefreshDetails();
    UpdateStatusBar();

    UpdateAllViews(nullptr, HINT_SELECTION);
}

void CProcMonDoc::SetFilter(const CString& filter)
{
    if (m_filter == filter)
        return;

    m_filter = filter;
    BuildVisibleList();
    UpdateStatusBar();

    UpdateAllViews(nullptr, HINT_PROCESSES);
}

void CProcMonDoc::SortByColumn(int column)
{
    if (column < 0 || column >= colCount)
        return;

    if (m_sortColumn == column)
    {
        m_bSortAscending = !m_bSortAscending;
    }
    else
    {
        m_sortColumn = column;
        // Tekstualni stupci se prirodno citaju uzlazno, brojcani silazno.
        m_bSortAscending = (column == colName || column == colPath || column == colPid);
    }

    BuildVisibleList();
    UpdateAllViews(nullptr, HINT_PROCESSES);
}

void CProcMonDoc::ToggleExpand(DWORD pid)
{
    if (pid == 0)
        return;

    std::set<DWORD>::iterator it = m_collapsed.find(pid);
    if (it == m_collapsed.end())
        m_collapsed.insert(pid);
    else
        m_collapsed.erase(it);

    BuildVisibleList();
    UpdateAllViews(nullptr, HINT_PROCESSES);
}

CString CProcMonDoc::GetSelectedProcessName() const
{
    const CProcessInfo* pInfo = m_processes.Find(m_selectedPid);
    return (pInfo != nullptr) ? pInfo->name : CString();
}

void CProcMonDoc::OnRefresh()
{
    RefreshData();
}

void CProcMonDoc::OnAutoRefresh()
{
    m_bAutoRefresh = !m_bAutoRefresh;
}

void CProcMonDoc::OnUpdateAutoRefresh(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bAutoRefresh ? 1 : 0);
}

void CProcMonDoc::OnTreeMode()
{
    m_bTreeMode = !m_bTreeMode;

    BuildVisibleList();
    UpdateAllViews(nullptr, HINT_PROCESSES);
}

void CProcMonDoc::OnUpdateTreeMode(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bTreeMode ? 1 : 0);
}

void CProcMonDoc::OnUpdateNeedsSelection(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_selectedPid != 0);
}

void CProcMonDoc::OnKillProcess()
{
    const CProcessInfo* pInfo = m_processes.Find(m_selectedPid);
    if (pInfo == nullptr)
    {
        AfxMessageBox(CSysUtil::LoadStr(IDS_ERR_NO_SELECTION), MB_OK | MB_ICONINFORMATION);
        return;
    }

    const CString name = pInfo->name;
    const DWORD   pid  = pInfo->pid;

    // Programi poput preglednika sastoje se od vise procesa, pa bi prekid samo
    // glavnog procesa ostavio ostale pokrenutima.
    std::vector<DWORD> descendants;
    CollectDescendants(pid, descendants);

    CString message;
    if (descendants.empty())
    {
        message.Format(CSysUtil::LoadStr(IDS_CONFIRM_KILL), (LPCTSTR)name, pid);
    }
    else
    {
        message.Format(CSysUtil::LoadStr(IDS_CONFIRM_KILL_TREE),
                       (LPCTSTR)name, pid, static_cast<int>(descendants.size()));
    }

    if (AfxMessageBox(message, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return;

    // Potomci se prekidaju od najdubljih prema gore, kako roditelj ne bi stigao
    // pokrenuti zamjenu za dijete koje je upravo prekinuto.
    for (size_t i = descendants.size(); i > 0; --i)
    {
        DWORD dwChildError = ERROR_SUCCESS;
        if (TerminateOne(descendants[i - 1], dwChildError))
            m_processes.Remove(descendants[i - 1]);
    }

    // Neuspjeh se prijavljuje samo za odabrani proces. Pojedini potomak moze
    // zavrsiti sam od sebe cim mu roditelj nestane, pa to nije greska.
    DWORD dwError = ERROR_SUCCESS;
    if (!TerminateOne(pid, dwError))
    {
        ReportKillError(name, pid, dwError);
        RefreshData();
        return;
    }

    // TerminateProcess samo zatrazi prekid i odmah se vraca, pa bi procesi u
    // sljedecoj snimci jos uvijek bili vidljivi. Umjesto cekanja, koje bi
    // zaustavilo sucelje, uklanjaju se odmah iz ocitanog popisa; ako prekid
    // ipak ne uspije, redak ce se vratiti pri sljedecem ocitanju.
    m_processes.Remove(pid);
    m_selectedPid = 0;
    ClearMemoryMap();
    ClearHandles();
    ClearStrings();
    RefreshEnvironment();

    BuildVisibleList();
    RefreshDetails();
    UpdateStatusBar();

    UpdateAllViews(nullptr, HINT_PROCESSES);
}

void CProcMonDoc::CollectDescendants(DWORD pid, std::vector<DWORD>& result) const
{
    const std::vector<CProcessInfo>& all = m_processes.GetAll();

    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i].parentPid != pid || all[i].pid == pid)
            continue;

        // Provjera stvarnog roditeljstva sprjecava da se zbog ponovno
        // dodijeljenog identifikatora prekine posve nepovezan proces.
        if (!IsRealParent(all[i]))
            continue;

        bool bAlreadyListed = false;
        for (size_t j = 0; j < result.size(); ++j)
        {
            if (result[j] == all[i].pid)
            {
                bAlreadyListed = true;
                break;
            }
        }

        if (bAlreadyListed)
            continue;

        result.push_back(all[i].pid);
        CollectDescendants(all[i].pid, result);
    }
}

bool CProcMonDoc::TerminateOne(DWORD pid, DWORD& dwError) const
{
    dwError = ERROR_SUCCESS;

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == nullptr)
    {
        dwError = GetLastError();
        return false;
    }

    const BOOL bTerminated = TerminateProcess(hProcess, 0);
    if (!bTerminated)
        dwError = GetLastError();

    CloseHandle(hProcess);
    return (bTerminated != FALSE);
}

void CProcMonDoc::ReportKillError(const CString& name, DWORD pid, DWORD dwError) const
{
    CString message;
    message.Format(CSysUtil::LoadStr(IDS_ERR_KILL_FAILED),
                   (LPCTSTR)name, pid, dwError,
                   (LPCTSTR)CSysUtil::FormatSystemError(dwError));

    AfxMessageBox(message, MB_OK | MB_ICONEXCLAMATION);
}

void CProcMonDoc::UpdateStatusBar()
{
    ULONGLONG totalWorkingSet = 0;
    const std::vector<CProcessInfo>& all = m_processes.GetAll();

    for (size_t i = 0; i < all.size(); ++i)
        totalWorkingSet += all[i].workingSet;

    // Odabrani proces prikazuje se u statusnoj traci, a ne u naslovu kartice,
    // kako se natpisi kartica ne bi mijenjali pri svakom odabiru.
    CString selection;
    const CProcessInfo* pSelected = m_processes.Find(m_selectedPid);

    if (pSelected == nullptr)
        selection = CSysUtil::LoadStr(IDS_STATUS_NO_SELECTION);
    else
        selection.Format(CSysUtil::LoadStr(IDS_STATUS_SELECTED),
                         (LPCTSTR)pSelected->name, pSelected->pid);

    CString text;
    text.Format(CSysUtil::LoadStr(IDS_STATUS_FORMAT),
                static_cast<int>(all.size()),
                (LPCTSTR)CSysUtil::FormatBytes(totalWorkingSet),
                (LPCTSTR)selection,
                (LPCTSTR)CTime::GetCurrentTime().Format(_T("%H:%M:%S")));

    CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    if (pFrame != nullptr)
        pFrame->SetStatusText(text);
}

#ifdef _DEBUG
void CProcMonDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CProcMonDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif
