#pragma once
#include "wpfilter.h"


enum DisplayTypeCode
{
	DT_NONE,
	DT_ULONG,
	DT_LONG,
	DT_BOOLEAN,
	DT_ABILITY,
	DT_FORWARDING_MODE,
	DT_HEX_UPPER,
	DT_HEX_LOWER,
	DT_VERSION
};

enum ValueDisplayTypeCode
{
	VDT_DEFAULT,
	VDT_TEXT,
	VDT_VALUE,
};

extern ULONG ValueDisplayType;

VOID DisplayValue(ULONG DisplayType, ULONG Value);

VOID DisplaySingleItem(ULONG Index, ULONG NonCount, BOOLEAN Title);

DisplayProgramVersion(BOOLEAN Title);

VOID DisplayAllItems();

