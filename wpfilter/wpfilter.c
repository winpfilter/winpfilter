#include "wpfilter.h"
#include "display.h"
#include "global_variables.h"

enum Command
{
	CMD_NULL,
	CMD_SHOW,
	CMD_SET,
	CMD_HOOKS
};


VOID SetConsoleWarning() {
	SetConsoleTextAttribute(ConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

VOID SetConsoleError() {
	SetConsoleTextAttribute(ConsoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
}

VOID SetConsoleDefault() {
	SetConsoleTextAttribute(ConsoleHandle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
}

VOID SetSingleItem(ULONG Index, ULONG NonCount, PCHAR ValueString) {

	BOOLEAN SuccessFlag = FALSE;
	ULONG Value;
	if (sscanf_s(ValueString, "%u", &Value) == 0) {
		SetConsoleError();
		printf("[ERROR] Failed to convert '%s' to integer.\n", ValueString);
		SetConsoleDefault();
		return;
	}

	ULONG Data[2] = { 1, Value };
	ULONG ReturnLength = 0;
	SuccessFlag = DeviceIoControl(WinPFilterDevice, WINPFILTER_CTL_CODE_BY_INDEX(Index - NonCount), Data, sizeof(Data), Data, sizeof(Data), &ReturnLength, NULL);

	if (!SuccessFlag || Data[0] != (0x800 + Index - NonCount)) {
		SetConsoleError();
		printf("[ERROR] Failed to set value.\n");
		SetConsoleDefault();
		return;
	}

	if (WarningFlag && Data[1] != Value) {
		SetConsoleWarning();
		printf("[WARNING] The operation was rejected by winpfilter.\n");
		SetConsoleDefault();
	}
}

int main(int argc,PCHAR argv[])
{
	BOOLEAN SuccessFlag = FALSE;
	ULONG Data[2] = { 0 };
	ULONG ReturnLength = 0;

	do
	{

		ULONG Command = CMD_NULL;
		PCHAR Item = NULL;
		PCHAR Value = NULL;
		for (LONG i = 0; i < argc; i++) {
			if (!_stricmp(argv[i], "-dw") || !_stricmp(argv[i], "-disablewarning") || !_stricmp(argv[i], "/dw") || !_stricmp(argv[i], "/disablewarning")) {
				WarningFlag = FALSE;
				continue;
			}
			if (!_stricmp(argv[i], "-show") || !_stricmp(argv[i], "-ls") || !_stricmp(argv[i], "/show") || !_stricmp(argv[i], "/ls")) {
				if (Command == CMD_NULL) {
					Command = CMD_SHOW;
					if (i + 1 >= argc) {
						break;
					}
					Item = argv[++i];
				}
				continue;
			}
			if (!_stricmp(argv[i], "-set") || !_stricmp(argv[i], "-s") || !_stricmp(argv[i], "/set") || !_stricmp(argv[i], "/s")) {
				if (Command == CMD_NULL) {
					if (i + 2 >= argc) {
						break;
					}
					Command = CMD_SET;
					Item = argv[++i];
					Value = argv[++i];
				}
				continue;
			}			
			if (!_stricmp(argv[i], "-showhooks") || !_stricmp(argv[i], "-lh") || !_stricmp(argv[i], "/showhooks") || !_stricmp(argv[i], "/lh")) {
				if (Command == CMD_NULL) {
					if (i + 1 >= argc) {
						break;
					}
					Item = argv[++i];
					Command = CMD_HOOKS;
				}
				continue;
			}
			if (!_stricmp(argv[i], "-displaytype") || !_stricmp(argv[i], "-dt") || !_stricmp(argv[i], "/displaytype") || !_stricmp(argv[i], "/dt")) {
				if (i + 1 >= argc) {
					ValueDisplayType = VDT_DEFAULT;
					break;
				}
				PCHAR Type = argv[++i];
				if (!_stricmp(Type, "value")) {
					ValueDisplayType = VDT_VALUE;
				}
				else if (!_stricmp(Type, "text")) {
					ValueDisplayType = VDT_TEXT;
				}
				else {
					ValueDisplayType = VDT_DEFAULT;
				}
				continue;
			}
		}

		ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (ConsoleHandle == INVALID_HANDLE_VALUE || ConsoleHandle == NULL) {
			printf("[ERROR] Failed to open console handle. HandleValue=0x%p\n", WinPFilterDevice);
			break;
		}
		SetConsoleDefault();

		WinPFilterDevice = CreateFile(WINPFILTER_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (WinPFilterDevice == INVALID_HANDLE_VALUE || WinPFilterDevice == NULL) {
			SetConsoleError();
			printf("[ERROR] Failed to open winpfilter device. HandleValue=0x%p\n", WinPFilterDevice);
			SetConsoleDefault();
			break;
		}
		
		SuccessFlag = DeviceIoControl(WinPFilterDevice,WINPFILTER_CTL_CODE_VERSION,Data,sizeof(Data), Data, sizeof(Data), &ReturnLength, NULL);

		if (!SuccessFlag || Data[0]!= 0x800) {
			SetConsoleError();
			printf("[ERROR] Failed to get winpfilter version.\n");
			SetConsoleDefault();
			break;
		}

		if (WarningFlag && Data[1] != WPFILTER_VERSION) {
			SetConsoleWarning();
			printf("[WARNING] version warning.\n"); 
			SetConsoleDefault();
		}

		ULONG NonCount = 0;

		switch (Command)
		{
		case CMD_SHOW:
			if (Item == NULL) {
				break;
			}

			if (!_stricmp(Item, "all")) {
				DisplayAllItems();
				break;
			}
			if (!_stricmp(Item, "pv")) {
				DisplayProgramVersion(FALSE);
				break;
			}
			for (ULONG i = 0; i < CTLCodeCount; i++) {
				if (CTLCodeTextShort[i] == NULL) {
					NonCount++;
					continue;
				}
				if (!_stricmp(Item, CTLCodeTextShort[i])) {
					DisplaySingleItem(i, NonCount, FALSE);
					break;
				}
			}
			break;
		case CMD_SET:
			if (Item == NULL || Value == NULL) {
				break;
			}
			for (ULONG i = 0; i < CTLCodeCount; i++) {
				if (CTLCodeTextShort[i] == NULL) {
					NonCount++;
					continue;
				}
				if (!_stricmp(Item, CTLCodeTextShort[i])) {
					SetSingleItem(i, NonCount, Value);
					break;
				}
			}
			break;
		case CMD_HOOKS:
			if (Item == NULL) {
				break;
			}
			DisplayHooks(Item);
			break;
		default:
			break;
		}

	} while (FALSE);

	return 0;
}