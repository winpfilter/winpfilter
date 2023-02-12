#include "display.h"
#include "global_variables.h"

PCHAR BooleanText[] = { "False","True" };
PCHAR AbilityText[] = { "Disable","Enable" };
PCHAR ForwardingModeText[] = { "MODE_SYSTEM","MODE_WINPFILTER","MODE_DISABLE" };

ULONG ValueDisplayType = VDT_DEFAULT;

VOID DisplayValue(ULONG DisplayType, ULONG Value) {

	switch (DisplayType)
	{
	case DT_ULONG:
		printf("%u", Value);
		break;
	case DT_LONG:
		printf("%d", Value);
		break;
	case DT_BOOLEAN:
		if (ValueDisplayType == VDT_TEXT)
			printf("%s", BooleanText[Value]);
		else if (ValueDisplayType == VDT_VALUE)
			printf("%d", Value);
		else
			printf("%s(%d)", BooleanText[Value], Value);
		break;
	case DT_ABILITY:
		if (ValueDisplayType == VDT_TEXT)
			printf("%s", AbilityText[Value]);
		else if (ValueDisplayType == VDT_VALUE)
			printf("%d", Value);
		else
			printf("%s(%d)", AbilityText[Value], Value);
		break;
	case DT_FORWARDING_MODE:
		if (ValueDisplayType == VDT_TEXT)
			printf("%s", ForwardingModeText[Value]);
		else if (ValueDisplayType == VDT_VALUE)
			printf("%d", Value);
		else
			printf("%s(%d)", ForwardingModeText[Value], Value);
		break;
	case DT_HEX_LOWER:
		printf("0x%x", Value);
		break;
	case DT_VERSION:
		printf("v%d.%d.%d.%d", (Value & 0xff000000) >> 24, (Value & 0xff0000) >> 16, (Value & 0xff00) >> 8, Value & 0xff);
		break;
	default:
		printf("0x%X", Value);
		break;
	}
}

VOID DisplaySingleItem(ULONG Index, ULONG NonCount, BOOLEAN Title) {

	BOOLEAN SuccessFlag = FALSE;
	ULONG Data[2] = { 0 };
	ULONG ReturnLength = 0;
	SuccessFlag = DeviceIoControl(WinPFilterDevice, WINPFILTER_CTL_CODE_BY_INDEX(Index - NonCount), Data, sizeof(Data), Data, sizeof(Data), &ReturnLength, NULL);

	if (Title) {
		printf("%s(%s): ", CTLCodeText[Index], CTLCodeTextShort[Index]);
	}

	if (!SuccessFlag || Data[0] != (0x800 + Index - NonCount)) {
		printf("Unknown ");
		SetConsoleError();
		printf("(Failed)\n");
		SetConsoleDefault();
		return;
	}
	DisplayValue(CTLCodeDisplayType[Index], Data[1]);
	printf("\n");
}

DisplayProgramVersion(BOOLEAN Title) {
	if (Title) {
		printf("Program version(pv): ");
	}
	DisplayValue(DT_VERSION, WPFILTER_VERSION);
	printf("\n");
}

VOID DisplayAllItems() {
	DisplayProgramVersion(TRUE);

	ULONG NonCount = 0;
	for (ULONG i = 0; i < CTLCodeCount; i++) {
		ULONG DisplayType = CTLCodeDisplayType[i];
		if (DisplayType == DT_NONE) {
			NonCount++;
			printf("%s: \n", CTLCodeText[i]);
			continue;
		}

		DisplaySingleItem(i, NonCount, TRUE);
	}
}
