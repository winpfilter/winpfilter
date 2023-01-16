#pragma once
#include "winpfilter.h"

#define INTERFACE_MAX_IP_COUNT 65536

typedef struct _INTERFACE_IP_INFO {
    PNDIS_RW_LOCK_EX InterfaceInfoRwLock;
    LIST_ENTRY IPv4LinkList;
    LIST_ENTRY IPv6LinkList;
}INTERFACE_IP_INFO, * PINTERFACE_IP_INFO;

NTSTATUS InitializeInterfaceIPCache(NDIS_HANDLE Handle);


VOID FreeInterfaceIPCache();

VOID InsertIntoInterfaceIPCache(IF_INDEX InterfaceIndex, PFILTER_CONTEXT Context);

VOID DeleteFromInterfaceIPCache(IF_INDEX InterfaceIndex);