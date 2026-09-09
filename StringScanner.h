#pragma once

#include <vector>

// Jedan pronadeni znakovni niz u memoriji procesa.
struct CStringInfo
{
    ULONGLONG address = 0;
    bool      bWide   = false;  // je li niz zapisan dvobajtnim znakovima
    CString   text;
};

// CScanObserver - preko ovog sucelja pretraga javlja napredak i pita treba li
// stati. Tako sam pretrazivac ne ovisi o sucelju, a prozor s napretkom ne mora
// znati kako pretraga radi.
class CScanObserver
{
public:
    virtual void OnScanProgress(int percent, int found) = 0;
    virtual bool IsScanCancelled() const = 0;
};

// CStringScanner - trazi citljive znakovne nizove u memoriji procesa. Pokrece
// se iz pomocne niti jer kod velikih procesa traje i nekoliko sekundi.
class CStringScanner
{
public:
    enum
    {
        minLength    = 4,                   // najmanja duljina niza
        maxResults   = 50000,               // gornja granica broja nalaza
        chunkSize    = 64 * 1024,           // velicina jednog citanja
        maxScanBytes = 512 * 1024 * 1024    // gornja granica pregledane memorije
    };

    void Clear();

    // Ako je pid jednak nuli, popis se samo prazni.
    void Scan(DWORD pid, CScanObserver* pObserver);

    const std::vector<CStringInfo>& GetAll() const { return m_items; }

    bool IsAccessible() const { return m_accessible; }

    // Je li pretraga prekinuta prije kraja, rucno ili zbog granice nalaza.
    bool IsPartial() const { return m_bPartial; }

private:
    // Regije koje se uopce pregledavaju: zauzete i citljive.
    static bool IsReadable(const MEMORY_BASIC_INFORMATION& mbi);
    static bool IsPrintable(BYTE value);

    ULONGLONG MeasureTotal(HANDLE hProcess) const;

    // Pregledava jednu regiju; vraca false ako pretragu treba prekinuti.
    bool ScanRegion(HANDLE hProcess, ULONGLONG base, ULONGLONG size,
                    CScanObserver* pObserver, ULONGLONG total, ULONGLONG& done);

    // Zastavica bTailPending kaze da se citanje vraca unatrag, pa niz koji
    // dopire do kraja spremnika treba prepustiti iducem komadu.
    void ScanBuffer(const std::vector<BYTE>& buffer, size_t size, ULONGLONG base,
                    bool bTailPending);
    void ScanAscii(const std::vector<BYTE>& buffer, size_t size, ULONGLONG base,
                   bool bTailPending);
    void ScanWide(const std::vector<BYTE>& buffer, size_t size, ULONGLONG base,
                  bool bTailPending);

    void AddString(const std::vector<BYTE>& buffer, size_t start, size_t length,
                   ULONGLONG base, bool bWide);

    // Duljina niza ispisivih znakova na kraju spremnika. Za toliko se pretraga
    // vraca unatrag, da niz presjecen na granici citanja ne bude izgubljen.
    size_t TailLength(const std::vector<BYTE>& buffer, size_t size) const;

    // Isti rep, ali gledan kao dvobajtni zapis: parovi (ispisiv znak, nula).
    size_t WideTailLength(const std::vector<BYTE>& buffer, size_t size) const;

    std::vector<CStringInfo> m_items;
    bool                     m_accessible = false;
    bool                     m_bPartial   = false;
};
