#pragma once

// Identifikatori stringova iz datoteke res\ProcMon.rc2. Raspon 40000+ je
// odabran da se ne sudari s onim sto generira Visual Studio u resource.h
// niti sa standardnim MFC porukama (61000+).

// Ribbon
#define IDS_RIBBON_CATEGORY        40000
#define IDS_PANEL_REFRESH          40001
#define IDS_PANEL_PROCESS          40002
#define IDS_PANEL_FILTER           40003
#define IDS_CMD_REFRESH            40004
#define IDS_CMD_AUTOREFRESH        40005
#define IDS_CMD_KILL               40006
#define IDS_FILTER_LABEL           40007
#define IDS_STATUS_READY           40008
#define IDS_PANEL_VIEW             40009
#define IDS_CMD_TREE               40010
#define IDS_CMD_ABOUT              40011
#define IDS_PANEL_MEMORY           40012
#define IDS_CMD_ANALYZE            40013
#define IDS_CMD_HEX                40014
#define IDS_ADDRESS_LABEL          40015

// Naslovi kartica
#define IDS_TITLE_PROCESSES        40020
#define IDS_TITLE_THREADS          40021
#define IDS_TITLE_MODULES          40022
#define IDS_TITLE_MEMORY           40023
#define IDS_TITLE_GRAPH            40024
#define IDS_TITLE_ENVIRONMENT      40025
#define IDS_TITLE_HEX              40026
#define IDS_TITLE_HANDLES          40027
#define IDS_TITLE_STRINGS          40028

// Stupci: procesi
#define IDS_COL_PID                40030
#define IDS_COL_NAME               40031
#define IDS_COL_CPU                40032
#define IDS_COL_WORKINGSET         40033
#define IDS_COL_PRIVATE            40034
#define IDS_COL_THREADCOUNT        40035
#define IDS_COL_PATH               40036

// Stupci: dretve
#define IDS_COL_TID                40040
#define IDS_COL_PRIORITY           40041
#define IDS_COL_KERNELTIME         40042
#define IDS_COL_USERTIME           40043
#define IDS_COL_CREATED            40044

// Stupci: moduli
#define IDS_COL_MODULE             40050
#define IDS_COL_BASEADDRESS        40051
#define IDS_COL_SIZE               40052
#define IDS_COL_MODULEPATH         40053

// Poruke i formati
#define IDS_STATUS_FORMAT          40060
#define IDS_STATUS_SELECTED        40061
#define IDS_STATUS_NO_SELECTION    40062
#define IDS_NOT_AVAILABLE          40063
#define IDS_CONFIRM_KILL           40064
#define IDS_CONFIRM_KILL_TREE      40065
#define IDS_ERR_KILL_FAILED        40066
#define IDS_ERR_NO_SELECTION       40067
#define IDS_WARN_NO_DEBUG_PRIV     40068
#define IDS_MODULES_DENIED         40069
#define IDS_THREADS_NO_SELECTION   40070
#define IDS_MEMORY_NO_MAP          40071
#define IDS_MEMORY_DENIED          40072
#define IDS_ENV_DENIED             40073
#define IDS_HEX_NO_DATA            40074
#define IDS_HEX_DENIED             40075
#define IDS_HEX_REGION             40076

// Stupci i poruke: handle-ovi
#define IDS_COL_HANDLE_VALUE       40130
#define IDS_COL_HANDLE_TYPE        40131
#define IDS_COL_HANDLE_ACCESS      40132
#define IDS_COL_HANDLE_NAME        40133
#define IDS_HANDLES_NO_LIST        40134
#define IDS_HANDLES_DENIED         40135
#define IDS_HANDLE_TYPE_UNKNOWN    40136

// Stupci i poruke: znakovni nizovi
#define IDS_COL_STRING_ADDRESS     40140
#define IDS_COL_STRING_KIND        40141
#define IDS_COL_STRING_LENGTH      40142
#define IDS_COL_STRING_TEXT        40143
#define IDS_STRING_ASCII           40144
#define IDS_STRING_WIDE            40145
#define IDS_STRINGS_NO_LIST        40146
#define IDS_STRINGS_DENIED         40147
#define IDS_STRINGS_PARTIAL        40151
#define IDS_SCAN_STARTING          40148
#define IDS_SCAN_STATUS            40149
#define IDS_SCAN_STOPPING          40150

// Prozor s napretkom pretrage
#define IDD_SCAN_PROGRESS          40200
#define IDC_SCAN_STATUS            40201
#define IDC_SCAN_PROGRESS          40202

// Stupci: mapa memorije
#define IDS_COL_REGION_ADDRESS     40080
#define IDS_COL_REGION_SIZE        40081
#define IDS_COL_REGION_STATE       40082
#define IDS_COL_REGION_PROTECT     40083
#define IDS_COL_REGION_TYPE        40084
#define IDS_COL_REGION_FILE        40085

// Stupci: varijable okoline
#define IDS_COL_ENV_NAME           40086
#define IDS_COL_ENV_VALUE          40087

// Stanje, tip i zastita regije
#define IDS_MEM_STATE_COMMIT       40090
#define IDS_MEM_STATE_RESERVE      40091
#define IDS_MEM_STATE_FREE         40092
#define IDS_MEM_TYPE_IMAGE         40093
#define IDS_MEM_TYPE_MAPPED        40094
#define IDS_MEM_TYPE_PRIVATE       40095
#define IDS_MEM_PROT_NOACCESS      40096
#define IDS_MEM_PROT_READ          40097
#define IDS_MEM_PROT_READWRITE     40098
#define IDS_MEM_PROT_WRITECOPY     40099
#define IDS_MEM_PROT_EXECUTE       40100
#define IDS_MEM_PROT_EXECREAD      40101
#define IDS_MEM_PROT_EXECWRITE     40102
#define IDS_MEM_PROT_EXECCOPY      40103
#define IDS_MEM_PROT_GUARD         40104
#define IDS_MEM_PROT_NOCACHE       40105
#define IDS_MEM_SUMMARY            40106

// Grafikoni opterecenja
#define IDS_GRAPH_CPU              40120
#define IDS_GRAPH_MEMORY           40121
#define IDS_GRAPH_CPU_SYSTEM       40122
#define IDS_GRAPH_CPU_PROCESS      40123
#define IDS_GRAPH_MEM_SYSTEM       40124
#define IDS_GRAPH_MEM_PROCESS      40125
