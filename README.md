# Process & Memory Viewer

[![Build](https://github.com/PatrikPanic/ProcMon-Zavrsni/actions/workflows/build.yml/badge.svg)](https://github.com/PatrikPanic/ProcMon-Zavrsni/actions/workflows/build.yml)

Završni rad – aplikacija za pregled procesa i njihove memorije u operacijskom
sustavu Windows, s osvježavanjem podataka u stvarnom vremenu. Rad proširuje
seminarski (NWP projekt), koji je obuhvaćao popis procesa, dretvi i modula;
oznaka `seminarski` u ovom repozitoriju označava stanje na kraju tog rada.

## Funkcionalnost

Pregled procesa:

- popis svih aktivnih procesa: PID, naziv, iskorištenje procesora, radni skup,
  privatna memorija, broj dretvi i puna putanja izvršne datoteke
- prikaz procesa u obliku stabla, po procesu koji ih je pokrenuo, uz sklapanje
  i širenje pojedinog čvora
- sortiranje popisa klikom na zaglavlje stupca i filtriranje po nazivu procesa
- pregled dretvi odabranog procesa: identifikator dretve, osnovni prioritet,
  vrijeme provedeno u jezgri i u korisničkom načinu rada, vrijeme nastanka
- pregled učitanih modula i DLL-ova odabranog procesa: naziv, bazna adresa,
  veličina i putanja
- prekid odabranog procesa, zajedno sa svim procesima koje je pokrenuo, uz
  potvrdu korisnika

Pregled memorije procesa:

- mapa virtualnog adresnog prostora: sve regije s adresom, veličinom, stanjem,
  zaštitom, tipom i preslikanom datotekom, uz slobodne dijelove prostora
- grafički prikaz mape: regije se crtaju kao obojeni blokovi u rastućem
  redoslijedu adresa, širinom razmjernom veličini, a regije iste rezervacije
  obrubljene su zajedničkim okvirom; klik na blok i odabir retka u popisu
  međusobno se prate
- heksadekadski prikaz sadržaja memorije s prikazom nedostupnih dijelova, uz
  listanje po regijama i upis proizvoljne adrese
- izdvajanje znakovnih nizova iz memorije, u jednobajtnom i dvobajtnom zapisu,
  s prikazom napretka i mogućnošću prekida

Ostali podaci o procesu:

- popis otvorenih objekata (handle-ova): vrijednost, vrsta objekta, prava
  pristupa i naziv, s putanjama datoteka i ključeva registra
- varijable okoline procesa, pročitane iz njegovog adresnog prostora
- grafikoni opterećenja procesora i radne memorije kroz vrijeme, za sustav u
  cjelini i za odabrani proces

Zajedničko:

- osvježavanje podataka svake 3 sekunde, uz mogućnost isključivanja
- skupa očitanja (mapa memorije, handle-ovi, nizovi) pokreću se na zahtjev,
  jednom naredbom nad odabranim procesom

## Zahtjevi

- Windows 10 ili noviji, 64-bitni
- Visual Studio 2022 s instaliranom komponentom *C++ MFC for latest v143 build tools (x86 & x64)*

## Prevođenje

1. Otvoriti `ProcMon.sln` u razvojnom okruženju Visual Studio 2022.
2. Odabrati konfiguraciju `Debug` ili `Release`; platforma je `x64` (32-bitni program ne može čitati module 64-bitnih procesa, pa se ne gradi).
3. Pokrenuti *Build → Build Solution*.

Aplikacija u manifestu traži ovlasti administratora, pa se pri pokretanju
prikazuje upit sustava za kontrolu korisničkih računa (UAC). Bez tih ovlasti
aplikacija se pokreće, ali podaci o sistemskim procesima ostaju nepotpuni.

## Korištenje

Nakon pokretanja otvara se 9 kartica: **Procesi**, **Dretve**, **Moduli**,
**Mapa memorije**, **Hex prikaz**, **Handle-ovi**, **Nizovi**, **Okolina** i
**Grafikon**. Kartice su stalno otvorene i ne mogu se zatvoriti, a sve
prikazuju podatke o istom odabranom procesu.

### Odabir procesa

U kartici **Procesi** klik na proces odabire ga, a njegov naziv i PID ispisani
su u statusnoj traci. Kartice **Dretve**, **Moduli** i **Okolina** odmah
prikazuju podatke tog procesa, dok se **Mapa memorije**, **Handle-ovi** i
**Nizovi** popunjavaju tek naredbom **Analiziraj proces**, jer su ta očitanja
preskupa da bi se ponavljala svake 3 sekunde.

Klik na zaglavlje stupca sortira popis po tom stupcu, a ponovni klik obrće
redoslijed; to vrijedi u svim popisima. Oznaka `[+]` ili `[-]` ispred naziva
sklapa i širi čvor stabla, isto rade dvostruki klik na redak te tipke `+` i `-`.

### Naredbe na Ribbon traci

| Ploča | Naredba | Opis |
| ----- | ------- | ---- |
| Osvježavanje | **Osvježi** | ručno očitavanje trenutnog stanja sustava |
| Osvježavanje | **Automatski (3 s)** | uključuje ili isključuje osvježavanje u stvarnom vremenu |
| Odabrani proces | **Prekini proces** | prekida odabrani proces nakon potvrde |
| Odabrani proces | **Analiziraj proces** | očitava mapu memorije i handle-ove te pokreće pretragu nizova |
| Memorija | **Hex prikaz** | otvara heksadekadski prikaz na adresi označene regije |
| Memorija | **Adresa:** | upis proizvoljne adrese, heksadekadski, sa ili bez `0x` |
| Prikaz | **Stablo procesa** | prebacuje između hijerarhijskog i ravnog popisa |
| Filtar | **Naziv:** | prikazuju se samo procesi čiji naziv sadrži upisani tekst (dok je filtar upisan, popis je ravan) |

### Mapa memorije

Kartica je podijeljena na grafički prikaz i popis regija. U grafičkom prikazu
boja bloka označava tip regije: modul, preslikana datoteka, privatno zauzeće
ili rezervirani dio. Slobodne regije se ne crtaju, ali su u popisu. Mjerilo se
računa iz ukupne veličine svih nacrtanih regija, uz najmanju širinu bloka od
3 piksela, da i najsitnije regije ostanu vidljive.

Dvoklik na redak u popisu otvara **Hex prikaz** na adresi te regije.

### Hex prikaz

Iznad ispisa stoji redak s podacima o regiji kojoj pripada prikazana adresa i s
odmakom od njezinog početka. Prikaz nema klizač jer 32-bitni raspon klizača ne
može obuhvatiti 64-bitni adresni prostor; umjesto toga se listanjem pomiče
početna adresa, tipkama PageUp i PageDown, strelicama, kotačićem miša i tipkom
Home za skok na početak regije. Listanje preskače nedostupne dijelove adresnog
prostora, pa uvijek završi na memoriji iz koje se može čitati. Nedostupni
bajtovi ispisani su upitnicima.

### Napomene o dostupnosti podataka

Za procese koje štiti sam operacijski sustav pojedini podaci nisu dostupni; u
popisu procesa tada je ispisano `nedostupno`, a u ostalim karticama poruka da
podatak nije dostupan za taj proces. Kod handle-ova jezgra neke vrste objekata
uopće ne daje preslikati, pa im umjesto naziva vrste stoji redni broj koji je
zajednički cijelom sustavu.

## Struktura projekta

Podaci se dohvaćaju u razredima `*Collector`, koji ne ovise o sučelju, pa se
isti podaci mogu prikazati u bilo kojem pogledu. Svaki prikaz ima vlastiti
pogled, a svi rade nad istim dokumentom.

### Dohvat podataka

| Datoteka | Opis |
| -------- | ---- |
| `ProcessCollector.h/.cpp` | očitavanje popisa procesa i izračun iskorištenja procesora |
| `ThreadCollector.h/.cpp` | očitavanje dretvi zadanog procesa |
| `ModuleCollector.h/.cpp` | očitavanje modula zadanog procesa |
| `MemoryCollector.h/.cpp` | obilazak virtualnog adresnog prostora i popis regija |
| `MemoryReader.h/.cpp` | čitanje prozora memorije i traženje dostupne adrese |
| `HandleCollector.h/.cpp` | popis otvorenih objekata procesa, s vrstom i nazivom |
| `EnvironmentCollector.h/.cpp` | varijable okoline, čitanjem strukture PEB |
| `StringScanner.h/.cpp` | izdvajanje znakovnih nizova iz memorije procesa |
| `HistoryCollector.h/.cpp` | povijest opterećenja procesora i radne memorije |
| `NtApi.h/.cpp` | pristup funkcijama iz `ntdll.dll` koje nemaju uvoznu biblioteku |
| `ObjectNameQuery.h/.cpp` | dohvat naziva objekta u pomoćnoj niti, s vremenskim ograničenjem |
| `SysUtil.h/.cpp` | pomoćne metode (ovlasti, oblikovanje brojeva, vremena i putanja) |

### Sučelje

| Datoteka | Opis |
| -------- | ---- |
| `ProcMonDoc.h/.cpp` | dokument: podaci, filtriranje, sortiranje i obrada naredbi |
| `ProcessView.h/.cpp` | pogled s popisom procesa i stablom |
| `SortedListView.h/.cpp` | zajednička osnova popisa koji se sortiraju klikom na stupac |
| `ThreadView.h/.cpp` | pogled s popisom dretvi |
| `ModuleView.h/.cpp` | pogled s popisom modula |
| `MemoryView.h/.cpp` | pogled s popisom regija memorije |
| `MemoryMapView.h/.cpp` | grafički prikaz adresnog prostora |
| `HexView.h/.cpp` | heksadekadski prikaz sadržaja memorije |
| `HandleView.h/.cpp` | pogled s popisom otvorenih objekata |
| `StringView.h/.cpp` | pogled s popisom pronađenih znakovnih nizova |
| `EnvironmentView.h/.cpp` | pogled s varijablama okoline |
| `GraphView.h/.cpp` | grafikoni opterećenja, crtani GDI-jem |
| `ScanProgressDlg.h/.cpp` | prozor s napretkom pretrage i pomoćnom niti |
| `ChildFrm.h/.cpp` | okvir kartice: naslov kartice i zabrana zatvaranja |
| `MemoryFrame.h/.cpp` | okvir kartice s mapom memorije, s podijeljenim prozorom |
| `MainFrm.h/.cpp` | glavni okvir, Ribbon traka, statusna traka i mjerač vremena |
| `ProcMon.h/.cpp` | klasa aplikacije i predlošci dokumenata |
| `StringIDs.h`, `res\ProcMon.rc2` | identifikatori i tekstovi sučelja u resursima |
| `Commands.h` | oznake naredbi i obavijesti prema pogledima |
