#pragma once
#include "winpfilter.h"

typedef struct _FORWARDING_BUFFER {
	BYTE* Buffer;
	ULONG BufferLength;
	ULONG DataLength;
	SINGLE_LIST_ENTRY Link;
}FORWARDING_BUFFER,*PFORWARDING_BUFFER;