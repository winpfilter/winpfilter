#pragma once

extern NDIS_HANDLE FilterDriverHandle;
extern NDIS_HANDLE FilterDriverObject;

extern NDIS_SPIN_LOCK FilterListLock;
extern LIST_ENTRY FilterModuleList;

extern DEVICE_OBJECT* WPFilterCommunicationDevice;

extern BOOLEAN PassModifiedRxPacketWithBadIPChecksum;
extern BOOLEAN PassModifiedRxPacketWithBadTCPChecksum;
extern BOOLEAN PassModifiedRxPacketWithBadUDPChecksum;

extern BOOLEAN AllowIPForwarding;