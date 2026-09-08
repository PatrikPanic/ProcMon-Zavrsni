#include "pch.h"
#include "MemoryCollector.h"
#include "SysUtil.h"
#include "StringIDs.h"

#include <psapi.h>

#pragma comment(lib, "Psapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CMemoryCollector::Refresh(DWORD pid)
{
    m_items.clear();
    m_committedBytes = 0;
    m_reservedBytes  = 0;
    m_accessible     = false;

    if (pid == 0)
        return;

    // Za obilazak adresnog prostora dovoljna je ogranicena ovlast, ali naziv
    // preslikane datoteke trazi i pravo citanja memorije, pa se prvo pokusava
    // s punim pravima.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == nullptr)
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

    if (hProcess == nullptr)
        return;

    m_accessible = true;

    SYSTEM_INFO si = {};
    GetSystemInfo(&si);

    ULONGLONG       address    = reinterpret_cast<ULONGLONG>(si.lpMinimumApplicationAddress);
    const ULONGLONG maxAddress = reinterpret_cast<ULONGLONG>(si.lpMaximumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi = {};

    // Slobodni dijelovi adresnog prostora vracaju se kao jedna velika regija,
    // pa je i kod 64-bitnih procesa broj koraka petlje mali.
    while (address <= maxAddress &&
           VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == sizeof(mbi))
    {
        AddRegion(hProcess, mbi);

        const ULONGLONG next = reinterpret_cast<ULONGLONG>(mbi.BaseAddress) + mbi.RegionSize;

        // Zastita od vrtnje u mjestu ako sustav vrati regiju bez velicine.
        if (next <= address)
            break;

        address = next;
    }

    CloseHandle(hProcess);
}

void CMemoryCollector::AddRegion(HANDLE hProcess, const MEMORY_BASIC_INFORMATION& mbi)
{
    CRegionInfo info;
    info.baseAddress    = reinterpret_cast<ULONGLONG>(mbi.BaseAddress);
    info.allocationBase = reinterpret_cast<ULONGLONG>(mbi.AllocationBase);
    info.size           = mbi.RegionSize;
    info.state          = mbi.State;
    info.type           = mbi.Type;

    // Kod slobodnih regija sustav u polju zastite vraca PAGE_NOACCESS iako
    // ondje uopce nema stranica, pa bi ispis izgledao kao stvarno pravo
    // pristupa.
    info.protect        = (mbi.State == MEM_FREE) ? 0 : mbi.Protect;

    const bool bMapped = (mbi.State != MEM_FREE) &&
                         (mbi.Type == MEM_IMAGE || mbi.Type == MEM_MAPPED);

    if (bMapped)
    {
        // Sve regije iste rezervacije potjecu iz iste datoteke, pa se naziv
        // trazi samo za prvu od njih. Kod procesa s vise stotina modula to
        // stedi jednak broj poziva sustavu.
        if (info.allocationBase != 0 && !m_items.empty() &&
            m_items.back().allocationBase == info.allocationBase)
        {
            info.mappedFile = m_items.back().mappedFile;
        }
        else
        {
            info.mappedFile = ReadMappedFileName(hProcess, mbi.BaseAddress);
        }
    }

    if (mbi.State == MEM_COMMIT)
        m_committedBytes += info.size;
    else if (mbi.State == MEM_RESERVE)
        m_reservedBytes += info.size;

    m_items.push_back(info);
}

CString CMemoryCollector::ReadMappedFileName(HANDLE hProcess, LPVOID address)
{
    TCHAR szPath[MAX_PATH] = {};

    if (GetMappedFileName(hProcess, address, szPath, MAX_PATH) == 0)
        return CString();

    // Naziv se dobiva u obliku uredaja, npr. "\Device\HarddiskVolume3\...",
    // pa se pretvara u uobicajen oblik s oznakom pogona.
    return CSysUtil::ToDosPath(szPath);
}

CString CMemoryCollector::FormatState(DWORD state)
{
    switch (state)
    {
    case MEM_COMMIT:  return CSysUtil::LoadStr(IDS_MEM_STATE_COMMIT);
    case MEM_RESERVE: return CSysUtil::LoadStr(IDS_MEM_STATE_RESERVE);
    case MEM_FREE:    return CSysUtil::LoadStr(IDS_MEM_STATE_FREE);
    default:          return CString();
    }
}

CString CMemoryCollector::FormatType(DWORD type)
{
    switch (type)
    {
    case MEM_IMAGE:   return CSysUtil::LoadStr(IDS_MEM_TYPE_IMAGE);
    case MEM_MAPPED:  return CSysUtil::LoadStr(IDS_MEM_TYPE_MAPPED);
    case MEM_PRIVATE: return CSysUtil::LoadStr(IDS_MEM_TYPE_PRIVATE);
    default:          return CString();
    }
}

CString CMemoryCollector::FormatProtection(DWORD protect)
{
    if (protect == 0)
        return CString();

    CString text;

    // Donji bajt nosi osnovnu zastitu, a visi bitovi dodatna svojstva stranice.
    switch (protect & 0xFF)
    {
    case PAGE_NOACCESS:          text = CSysUtil::LoadStr(IDS_MEM_PROT_NOACCESS);  break;
    case PAGE_READONLY:          text = CSysUtil::LoadStr(IDS_MEM_PROT_READ);      break;
    case PAGE_READWRITE:         text = CSysUtil::LoadStr(IDS_MEM_PROT_READWRITE); break;
    case PAGE_WRITECOPY:         text = CSysUtil::LoadStr(IDS_MEM_PROT_WRITECOPY); break;
    case PAGE_EXECUTE:           text = CSysUtil::LoadStr(IDS_MEM_PROT_EXECUTE);   break;
    case PAGE_EXECUTE_READ:      text = CSysUtil::LoadStr(IDS_MEM_PROT_EXECREAD);  break;
    case PAGE_EXECUTE_READWRITE: text = CSysUtil::LoadStr(IDS_MEM_PROT_EXECWRITE); break;
    case PAGE_EXECUTE_WRITECOPY: text = CSysUtil::LoadStr(IDS_MEM_PROT_EXECCOPY);  break;
    default: break;
    }

    if ((protect & PAGE_GUARD) != 0)
        text += CSysUtil::LoadStr(IDS_MEM_PROT_GUARD);

    if ((protect & PAGE_NOCACHE) != 0)
        text += CSysUtil::LoadStr(IDS_MEM_PROT_NOCACHE);

    return text;
}
