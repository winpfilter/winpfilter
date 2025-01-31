#pragma once
#include "winpfilter.h"
#include "net\ip.h"

#define ROUTE_TABLE_TOTAL_COUNT 64
#define ROUTE_TABLE_SYSTEM_MIRROR 31
#define ROUTE_TABLE_WINPFILTER_RESERVED 0

typedef struct _ROUTE_ENTRY {
    IP_PROTOCOLS Type;
    UNI_IPADDRESS Destination;
    UNI_IPADDRESS Gateway;
    ULONG Prefix;
    NET_LUID InterfaceLuid;
    ULONG Metric;
    LIST_ENTRY ListEntry;
} ROUTE_ENTRY, * PROUTE_ENTRY;

typedef struct _ROUTE_TABLE {
    LIST_ENTRY IPv4ListHead;
    LIST_ENTRY IPv6ListHead;
    KSPIN_LOCK Lock;       
} ROUTE_TABLE, * PROUTE_TABLE;


#if DBG
VOID PrintRouteTable(PROUTE_TABLE RouteTable);
#endif

VOID InitializeAllRouteTable();
VOID CleanupAllRouteTable();

VOID CleanupRouteTable(PROUTE_TABLE RouteTable, BOOLEAN AcquireLock);
NTSTATUS AddRouteEntry(PROUTE_TABLE RouteTable, IP_PROTOCOLS Type, UNI_IPADDRESS Destination, UNI_IPADDRESS Gateway, ULONG Prefix, NET_LUID Interface, ULONG Metric, BOOLEAN AcquireLock);