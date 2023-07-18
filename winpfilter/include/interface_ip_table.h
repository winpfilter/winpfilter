#pragma once
#include "winpfilter.h"
#include "ip.h"

#define INTERFACE_MAX_IP_COUNT 65536

typedef struct _INTERFACE_IP_INFO {
    PNDIS_RW_LOCK_EX InterfaceInfoRwLock;
    LIST_ENTRY IPv4LinkList;
    LIST_ENTRY IPv6LinkList;
}INTERFACE_IP_INFO, * PINTERFACE_IP_INFO;

NTSTATUS InitializeInterfaceIPCacheManager(NDIS_HANDLE Handle);


VOID FreeInterfaceIPCacheManager();

VOID InsertIntoInterfaceIPCache(NET_LUID InterfaceLuid, IP_ADDRESS Address);

VOID DeleteAllInterfaceIPCache();

BOOLEAN IsLocalIP(BYTE* Address, IP_PROTOCOLS EtherNetProtocol);