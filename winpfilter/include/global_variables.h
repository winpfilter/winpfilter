#pragma once
#include "winpfilter.h"
#include "interface_ip_table.h"
#include "route_table.h"

extern NDIS_HANDLE FilterDriverHandle;
extern NDIS_HANDLE FilterDriverObject;

extern NDIS_SPIN_LOCK FilterListLock;
extern LIST_ENTRY FilterModuleList;

extern PDEVICE_OBJECT WPFilterR0HookCommunicationDevice;
extern PDEVICE_OBJECT WPFilterR3CommandCommunicationDevice;

extern BYTE IndicateModifiedRxPacketWithBadIPChecksumAsGood;
extern BYTE IndicateModifiedRxPacketWithBadTCPChecksumAsGood;
extern BYTE IndicateModifiedRxPacketWithBadUDPChecksumAsGood;
extern BYTE TryCalcModifiedTxPacketIPChecksumByNIC;
extern BYTE TryCalcModifiedTxPacketTCPChecksumByNIC;
extern BYTE TryCalcModifiedTxPacketUDPChecksumByNIC;

#define IP_FORWARDING_MODE_SYSTEM		0
#define IP_FORWARDING_MODE_WINPFILTER	1
#define IP_FORWARDING_MODE_DISABLE		2
extern BYTE IPForwardingMode;

extern ROUTE_TABLE WinPFilterRouteTable[ROUTE_TABLE_TOTAL_COUNT];
