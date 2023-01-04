#pragma once
#include "winpfilter.h"

#define HOOK_LIST_COUNT 5

#define FILTER_POINT_PREROUTING	 0
#define FILTER_POINT_INPUT		 1
#define FILTER_POINT_FORWARDING	 2
#define FILTER_POINT_OUTPUT		 3
#define FILTER_POINT_POSTROUTING 4

typedef struct _HOOK_DATA {
	BYTE*	Buffer;
	ULONG	DataLength;
}HOOK_DATA, * PHOOK_DATA;

typedef ULONG HOOK_ACTION;

//ULONG HOOKFUNCTION(ULONG InterfaceID,ULONG FilterPoint,ULONG BufferLength,PHOOK_DATA Buffer);
//At most time BufferLength = MTU
//The hook function can modify the hook_data but DataLength must <= BufferLength
typedef HOOK_ACTION(*HOOK_FUNCTION)(ULONG, ULONG,ULONG,PHOOK_DATA) ;

// HOOK_ACTION values
// Drop the packet 
#define HOOK_ACTION_DROP			0
// Accept the packet 
#define HOOK_ACTION_ACCEPT			1
// The hook function modified the data in buffer
#define HOOK_ACTION_MODIFIED		2
// Accept the packet and truncate this Winpfilter hook processing chain
#define HOOK_ACTION_TRUNCATE_CHAIN	3

 
typedef struct _HOOK_ENTRY {
	LIST_ENTRY		HookLink;
	ULONG			Priority;
	HOOK_FUNCTION	HookFunction;
	ULONG	FilterPoint;
}HOOK_ENTRY,*PHOOK_ENTRY;

VOID InitializeFilterHookManager();
NTSTATUS RegisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, ULONG FilterPoint);
VOID UnregisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, ULONG FilterPoint);
VOID UnregisterAllHooks(ULONG FilterPoint);
VOID FreeFilterHookManager();

#ifdef DBG
VOID PrintHookTable();
#endif
