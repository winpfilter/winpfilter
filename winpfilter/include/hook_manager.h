#pragma once
#include "winpfilter.h"

#define HOOK_LIST_COUNT 5


typedef ULONG FILTER_PONIT;

#define FILTER_POINT_PREROUTING	 0
#define FILTER_POINT_INPUT		 1
#define FILTER_POINT_FORWARDING	 2
#define FILTER_POINT_OUTPUT		 3
#define FILTER_POINT_POSTROUTING 4

typedef ULONG HOOK_ACTION;

//ULONG HOOKFUNCTION(NET_LUID InterfaceLuid, FILTER_PONIT FilterPoint, ULONG BufferLength, BYTE* Buffer, ULONG* pDataLength);
//At most time BufferLength = MTU
//The hook function can modify the hook_data but make sure DataLength <= BufferLength
typedef HOOK_ACTION(*HOOK_FUNCTION)(NET_LUID InterfaceLuid, FILTER_PONIT FilterPoint, ULONG BufferLength, BYTE* Buffer, ULONG* pDataLength);

// HOOK_ACTION values
// Drop the packet 
#define HOOK_ACTION_DROP			0
// Accept the packet 
#define HOOK_ACTION_ACCEPT			1
// The hook function modified the data in buffer
#define HOOK_ACTION_MODIFIED		2
// Accept the packet and truncate this Winpfilter hook processing chain
#define HOOK_ACTION_TRUNCATE_CHAIN	3

typedef union _HOOK_RESULT {
	ULONG	Result;
	struct MyStruct
	{
		BYTE	Accept;
		BYTE	Modified;
		BYTE	Reserve0;
		BYTE	Reserve1;
	};
}HOOK_RESULT, * PHOOK_RESULT;

typedef struct _HOOK_ENTRY {
	LIST_ENTRY		HookLink;
	ULONG			Priority;
	HOOK_FUNCTION	HookFunction;
	ULONG	FilterPoint;
}HOOK_ENTRY, * PHOOK_ENTRY;

NTSTATUS InitializeFilterHookManager(NDIS_HANDLE Handle);
NTSTATUS RegisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, FILTER_PONIT FilterPoint);
VOID UnregisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, FILTER_PONIT FilterPoint);
VOID UnregisterAllHooks(FILTER_PONIT FilterPoint);
VOID FreeFilterHookManager();
HOOK_RESULT FilterEthernetPacket(BYTE* EthernetBuffer, ULONG* DataLength, ULONG BufferLength, FILTER_PONIT FilterPoint, NET_LUID InterfaceIndex, UCHAR DispatchLevel);

#ifdef DBG
VOID PrintHookTable();
#endif
