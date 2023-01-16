#include "global_variables.h"
NDIS_HANDLE FilterDriverHandle = NULL;
NDIS_HANDLE FilterDriverObject = NULL;

NDIS_SPIN_LOCK FilterListLock;
LIST_ENTRY FilterModuleList;

DEVICE_OBJECT* WPFilterCommunicationDevice;

BOOLEAN IndicateModifiedRxPacketWithBadIPChecksumAsGood = TRUE;
BOOLEAN IndicateModifiedRxPacketWithBadTCPChecksumAsGood = TRUE;
BOOLEAN IndicateModifiedRxPacketWithBadUDPChecksumAsGood = TRUE;

BOOLEAN TryCalcModifiedTxPacketIPChecksumByNIC = TRUE;
BOOLEAN TryCalcModifiedTxPacketTCPChecksumByNIC = TRUE;
BOOLEAN TryCalcModifiedTxPacketUDPChecksumByNIC = TRUE;

BOOLEAN AllowIPForwarding = FALSE;
