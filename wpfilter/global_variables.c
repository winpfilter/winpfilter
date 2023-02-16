#include "global_variables.h"

HANDLE WinPFilterDevice = NULL;
HANDLE ConsoleHandle = NULL;
BOOLEAN WarningFlag = TRUE;

PCHAR FilterPointText[] = {
	"PREROUTING",
	"INPUT",
	"FORWARDING",
	"OUTPUT",
	"POSTROUTING"
};

PCHAR CTLCodeTextShort[] = { 
	"dv",
	"fm",
	NULL,
	"rxipcsum", 
	"rxtcpcsum",
	"rxudpcsum",
	NULL,
	"txipcsum",
	"txtcpcsum",
	"txudpcsum" 
};

PCHAR CTLCodeText[] = { 
	"Driver  version",
	"Forwarding mode",
	"Indicate bad checksum in modified packet as good",
	"\tRx IP  ",
	"\tRx TCP",
	"\tRx UDP",
	"Indicate checksum offloads for modified packet",
	"\tTx IP  ", 
	"\tTx TCP",
	"\tTx UDP"
};

ULONG CTLCodeDisplayType[] = {
	DT_VERSION,
	DT_FORWARDING_MODE,
	DT_NONE,
	DT_ABILITY,
	DT_ABILITY,
	DT_ABILITY,
	DT_NONE,
	DT_ABILITY,
	DT_ABILITY,
	DT_ABILITY 
};

const ULONG CTLCodeCount = (ULONG)(sizeof(CTLCodeText) / (sizeof(PVOID)));
const ULONG FilterPointCount = (ULONG)(sizeof(FilterPointText) / (sizeof(PVOID)));

