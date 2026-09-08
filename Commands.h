#pragma once

// Vlastite oznake naredbi (Ribbon gumbi). Raspon 33000+ je odabran da se ne
// sudare s onima koje generira Visual Studio u resource.h.
#define ID_CMD_REFRESH          33000
#define ID_CMD_AUTOREFRESH      33001
#define ID_CMD_KILL_PROCESS     33002
#define ID_CMD_TREE             33003
#define ID_CMD_FILTER           33004
#define ID_CMD_ANALYZE          33005
#define ID_CMD_HEX              33006
#define ID_CMD_ADDRESS          33007
#define ID_CMD_STATUS_PANE      33010

// Oznake koje dokument salje pogledima kroz UpdateAllViews.
#define HINT_PROCESSES          1       // osvjezena lista procesa
#define HINT_SELECTION          2       // promijenjen odabrani proces
#define HINT_MEMORY             3       // ocitana ili ponistena mapa memorije
#define HINT_REGION             4       // promijenjena odabrana regija
#define HINT_HEX                5       // pomaknut heksadekadski prikaz
#define HINT_HANDLES            6       // ocitan ili ponisten popis handle-ova
#define HINT_STRINGS            7       // zavrsena ili ponistena pretraga nizova
