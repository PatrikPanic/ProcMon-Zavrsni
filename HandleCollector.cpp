#include "pch.h"
#include "HandleCollector.h"
#include "SysUtil.h"
#include "StringIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
    // Vrste podataka koje sustav ne objavljuje u zaglavljima.
    const ULONG systemExtendedHandleInformation = 64;
    const ULONG objectTypeInformation           = 2;

    // Jedan zapis u tablici handle-ova cijelog sustava.
    struct CHandleTableEntry
    {
        PVOID     object;
        ULONG_PTR processId;
        ULONG_PTR handleValue;
        ULONG     grantedAccess;
        USHORT    creatorBackTraceIndex;
        USHORT    objectTypeIndex;
        ULONG     attributes;
        ULONG     reserved;
    };

    struct CHandleTable
    {
        ULONG_PTR         count;
        ULONG_PTR         reserved;
        CHandleTableEntry entries[1];
    };

    const ULONG typeBufferSize  = 1024;
    const ULONG firstTableSize  = 1024 * 1024;
    const int   maxTableTries   = 6;
}

CHandleCollector::CHandleCollector()
    : m_nameQuery(m_ntApi)
{
}

void CHandleCollector::Refresh(DWORD pid)
{
    m_items.clear();
    m_accessible = false;

    if (pid == 0)
        return;

    // Za preslikavanje handle-a u ovaj proces treba posebno pravo, koje
    // zasticeni procesi ne daju.
    HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == nullptr)
        return;

    std::vector<BYTE> buffer;

    if (!ReadTable(buffer))
    {
        CloseHandle(hProcess);
        return;
    }

    m_accessible = true;

    const CHandleTable* pTable = reinterpret_cast<const CHandleTable*>(&buffer[0]);

    // Uz svaki handle tablica nosi i redni broj vrste objekta. Vrste koje se
    // uspiju prepoznati pamte se po tom broju, pa se njima kasnije popune
    // zapisi kojima preslikavanje nije uspjelo.
    std::map<USHORT, CString> types;

    for (ULONG_PTR i = 0; i < pTable->count && m_items.size() < maxHandles; ++i)
    {
        const CHandleTableEntry& entry = pTable->entries[i];

        if (entry.processId != pid)
            continue;

        CHandleInfo info;
        info.value     = entry.handleValue;
        info.access    = entry.grantedAccess;
        info.typeIndex = entry.objectTypeIndex;

        HANDLE hDuplicate = nullptr;

        if (DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(entry.handleValue),
                            GetCurrentProcess(), &hDuplicate, 0, FALSE,
                            DUPLICATE_SAME_ACCESS))
        {
            info.type = ReadType(hDuplicate);

            if (!info.type.IsEmpty())
                types[info.typeIndex] = info.type;

            // Cjevovod koji ceka drugu stranu najcesci je uzrok zaustavljanja
            // upita za nazivom, pa se kod njega naziv ni ne trazi.
            if (GetFileType(hDuplicate) != FILE_TYPE_PIPE)
            {
                // Datoteke se javljaju putanjom u obliku uredaja, pa se
                // pretvaraju u uobicajen oblik s oznakom pogona.
                info.name = CSysUtil::ToDosPath(m_nameQuery.Query(hDuplicate));
            }

            CloseHandle(hDuplicate);
        }

        m_items.push_back(info);
    }

    CloseHandle(hProcess);

    FillMissingTypes(types);
}

void CHandleCollector::FillMissingTypes(const std::map<USHORT, CString>& types)
{
    for (size_t i = 0; i < m_items.size(); ++i)
    {
        if (!m_items[i].type.IsEmpty())
            continue;

        const std::map<USHORT, CString>::const_iterator it = types.find(m_items[i].typeIndex);

        if (it != types.end())
        {
            m_items[i].type = it->second;
            continue;
        }

        // Neke vrste objekata jezgra uopce ne daje preslikati, primjerice
        // registracije za pracenje dogadaja, pa im naziv vrste ostaje nepoznat.
        // Ispisuje se barem redni broj, po kojem se vidi da su takvi zapisi
        // medusobno iste vrste.
        m_items[i].type.Format(CSysUtil::LoadStr(IDS_HANDLE_TYPE_UNKNOWN),
                               static_cast<unsigned int>(m_items[i].typeIndex));
    }
}

bool CHandleCollector::ReadTable(std::vector<BYTE>& buffer) const
{
    ULONG size = firstTableSize;

    // Tablica obuhvaca cijeli sustav i mijenja se dok se cita, pa se kod
    // premalog spremnika upit ponavlja s vecim.
    for (int attempt = 0; attempt < maxTableTries; ++attempt)
    {
        buffer.assign(size, 0);

        ULONG needed = 0;

        if (m_ntApi.QuerySystem(systemExtendedHandleInformation, &buffer[0], size, &needed))
            return true;

        size = (needed > size) ? needed + firstTableSize : size * 2;
    }

    return false;
}

CString CHandleCollector::ReadType(HANDLE hObject) const
{
    BYTE buffer[typeBufferSize] = {};

    if (!m_ntApi.QueryObject(hObject, objectTypeInformation, buffer, typeBufferSize, nullptr))
        return CString();

    // Struktura pocinje opisom niza znakova s nazivom vrste objekta.
    const UNICODE_STRING* pName = reinterpret_cast<const UNICODE_STRING*>(buffer);

    if (pName->Buffer == nullptr || pName->Length == 0)
        return CString();

    return CString(pName->Buffer, pName->Length / sizeof(WCHAR));
}
