#pragma once
#include "wpfilter.h"
#include "display.h"

extern HANDLE WinPFilterDevice;
extern HANDLE ConsoleHandle;
extern BOOLEAN WarningFlag;

extern PCHAR FilterPointText[];
extern PCHAR CTLCodeTextShort[];
extern PCHAR CTLCodeText[];
extern ULONG CTLCodeDisplayType[];

extern const ULONG CTLCodeCount;
extern const ULONG FilterPointCount;