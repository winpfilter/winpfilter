#pragma once
#include "winpfilter.h"
#include "interface_ip_table.h"

extern NDIS_HANDLE FilterDriverHandle;
extern NDIS_HANDLE FilterDriverObject;

extern NDIS_SPIN_LOCK FilterListLock;
extern LIST_ENTRY FilterModuleList;

extern DEVICE_OBJECT* WPFilterCommunicationDevice;

extern BOOLEAN IndicateModifiedRxPacketWithBadIPChecksumAsGood;
extern BOOLEAN IndicateModifiedRxPacketWithBadTCPChecksumAsGood;
extern BOOLEAN IndicateModifiedRxPacketWithBadUDPChecksumAsGood;
extern BOOLEAN TryCalcModifiedTxPacketIPChecksumByNIC;
extern BOOLEAN TryCalcModifiedTxPacketTCPChecksumByNIC;
extern BOOLEAN TryCalcModifiedTxPacketUDPChecksumByNIC;

extern BOOLEAN AllowIPForwarding;
