#include "winpfilter.h"

NDIS_HANDLE FilterDriverHandle = NULL;
NDIS_HANDLE FilterDriverObject = NULL;

NDIS_SPIN_LOCK FilterListLock;
LIST_ENTRY FilterModuleList;

DEVICE_OBJECT* WPFilterCommunicationDevice;

BOOLEAN PassModifiedRxPacketWithBadIPChecksum = TRUE;
BOOLEAN PassModifiedRxPacketWithBadTCPChecksum = TRUE;
BOOLEAN PassModifiedRxPacketWithBadUDPChecksum = TRUE;

BOOLEAN AllowIPForwarding = FALSE;