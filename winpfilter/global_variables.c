#include "global_variables.h"

NDIS_HANDLE FilterDriverHandle = NULL;
NDIS_HANDLE FilterDriverObject = NULL;

NDIS_SPIN_LOCK FilterListLock;
LIST_ENTRY FilterModuleList;

PDEVICE_OBJECT WPFilterR0HookCommunicationDevice;
PDEVICE_OBJECT WPFilterR3CommandCommunicationDevice;

BYTE IndicateModifiedRxPacketWithBadIPChecksumAsGood = TRUE;
BYTE IndicateModifiedRxPacketWithBadTCPChecksumAsGood = TRUE;
BYTE IndicateModifiedRxPacketWithBadUDPChecksumAsGood = TRUE;

BYTE TryCalcModifiedTxPacketIPChecksumByNIC = TRUE;
BYTE TryCalcModifiedTxPacketTCPChecksumByNIC = TRUE;
BYTE TryCalcModifiedTxPacketUDPChecksumByNIC = TRUE;

BYTE IPForwardingMode = IP_FORWARDING_MODE_SYSTEM;
