#include "pch.h"
#include "SysUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool CSysUtil::EnableDebugPrivilege()
{
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }

    LUID luid = {};
    bool bResult = false;

    if (LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid))
    {
        TOKEN_PRIVILEGES tp = {};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);

        // AdjustTokenPrivileges vraca TRUE i kad ovlast nije dodijeljena,
        // pa se stvarni ishod provjerava preko GetLastError.
        bResult = (GetLastError() == ERROR_SUCCESS);
    }

    CloseHandle(hToken);
    return bResult;
}

CString CSysUtil::LoadStr(UINT nID)
{
    CString str;

    if (!str.LoadString(nID))
    {
        // Ako string nedostaje, sucelje bi ostalo bez natpisa; najcesci je uzrok
        // to sto datoteka res\ProcMon.rc2 nije ukljucena u projekt.
        TRACE(_T("Nedostaje string u resursima, oznaka %u\n"), nID);
        ASSERT(FALSE);
    }

    return str;
}

CString CSysUtil::FormatBytes(ULONGLONG bytes)
{
    static const LPCTSTR units[] = { _T("B"), _T("kB"), _T("MB"), _T("GB"), _T("TB") };
    const int unitCount = sizeof(units) / sizeof(units[0]);

    double value = static_cast<double>(bytes);
    int unit = 0;

    while (value >= 1024.0 && unit < unitCount - 1)
    {
        value /= 1024.0;
        ++unit;
    }

    CString str;
    if (unit == 0)
        str.Format(_T("%.0f %s"), value, units[unit]);
    else
        str.Format(_T("%.1f %s"), value, units[unit]);

    return str;
}

CString CSysUtil::FormatTimeStamp(const FILETIME& ft)
{
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0)
        return CString();

    FILETIME local = {};
    SYSTEMTIME st = {};

    if (!FileTimeToLocalFileTime(&ft, &local) || !FileTimeToSystemTime(&local, &st))
        return CString();

    CString str;
    str.Format(_T("%02d.%02d.%04d. %02d:%02d:%02d"),
               st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    return str;
}

CString CSysUtil::FormatDuration(ULONGLONG time100ns)
{
    // 1 jedinica = 100 ns, dakle 10 000 jedinica cini jednu milisekundu.
    const ULONGLONG totalMs = time100ns / 10000ULL;

    const ULONGLONG ms      = totalMs % 1000ULL;
    const ULONGLONG totalS  = totalMs / 1000ULL;
    const ULONGLONG s       = totalS % 60ULL;
    const ULONGLONG totalM  = totalS / 60ULL;
    const ULONGLONG m       = totalM % 60ULL;
    const ULONGLONG h       = totalM / 60ULL;

    CString str;
    str.Format(_T("%llu:%02llu:%02llu.%03llu"), h, m, s, ms);
    return str;
}

CString CSysUtil::FormatSystemError(DWORD dwError)
{
    LPTSTR lpBuffer = nullptr;

    const DWORD cch = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, dwError, 0, reinterpret_cast<LPTSTR>(&lpBuffer), 0, nullptr);

    CString str;
    if (cch > 0 && lpBuffer != nullptr)
    {
        str = lpBuffer;
        str.Trim();
    }

    if (lpBuffer != nullptr)
        LocalFree(lpBuffer);

    return str;
}

CString CSysUtil::ToDosPath(const CString& devicePath)
{
    if (devicePath.IsEmpty())
        return devicePath;

    TCHAR szDrive[3]         = _T(" :");
    TCHAR szTarget[MAX_PATH] = {};

    // Za svako slovo pogona trazi se uredaj na koji upucuje, pa se usporeduje
    // s pocetkom zadane putanje.
    for (TCHAR letter = _T('A'); letter <= _T('Z'); ++letter)
    {
        szDrive[0] = letter;

        // Pogon koji ne postoji, kao i naziv predug za spremnik, samo se
        // preskace; putanja tada ostaje u izvornom obliku.
        if (QueryDosDevice(szDrive, szTarget, MAX_PATH) == 0)
            continue;

        const int length = static_cast<int>(_tcslen(szTarget));

        if (devicePath.GetLength() > length &&
            devicePath.Left(length).CompareNoCase(szTarget) == 0 &&
            devicePath[length] == _T('\\'))
        {
            return CString(szDrive) + devicePath.Mid(length);
        }
    }

    return devicePath;
}

ULONGLONG CSysUtil::ToUInt64(const FILETIME& ft)
{
    ULARGE_INTEGER value;
    value.LowPart  = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return value.QuadPart;
}
