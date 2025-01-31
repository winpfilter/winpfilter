#include "route_table.h"
#include "net/ip.h"
#include "global_variables.h"

#if DBG

VOID PrintRouteTable(PROUTE_TABLE RouteTable) {
    PLIST_ENTRY listHead = &RouteTable->IPv4ListHead;
    PLIST_ENTRY entry;
    PLIST_ENTRY prevEntry;
    for (entry = listHead->Flink; entry != listHead; entry = entry->Flink) {
        PROUTE_ENTRY routeEntry = CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry);
        DbgPrint("%d.%d.%d.%d\/%d\t\t%d.%d.%d.%d\t\t%d\t\t%d\n",
            routeEntry->Destination.IPV4Address.AddressBytes[0],
            routeEntry->Destination.IPV4Address.AddressBytes[1],
            routeEntry->Destination.IPV4Address.AddressBytes[2],
            routeEntry->Destination.IPV4Address.AddressBytes[3],
            routeEntry->Prefix,
            routeEntry->Gateway.IPV4Address.AddressBytes[0],
            routeEntry->Gateway.IPV4Address.AddressBytes[1],
            routeEntry->Gateway.IPV4Address.AddressBytes[2],
            routeEntry->Gateway.IPV4Address.AddressBytes[3],
            routeEntry->InterfaceLuid,
            routeEntry->Metric
        );
        prevEntry = entry;
    }
    listHead = &RouteTable->IPv6ListHead;
    for (entry = listHead->Flink; entry != listHead; entry = entry->Flink) {
		PROUTE_ENTRY routeEntry = CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry);
		DbgPrint("%X:%X:%X:%X:%X:%X:%X:%X\/%d\t\t%X:%X:%X:%X:%X:%X:%X:%X\t\t%d\t\t%d\n",
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[0]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[1]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[2]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[3]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[4]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[5]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[6]),
			CONVERT_NETE_16(routeEntry->Destination.IPV6Address.AddressWords[7]),
			routeEntry->Prefix,
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[0]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[1]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[2]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[3]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[4]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[5]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[6]),
            CONVERT_NETE_16(routeEntry->Gateway.IPV6Address.AddressWords[7]),
            routeEntry->InterfaceLuid,
            routeEntry->Metric
        );
        prevEntry = entry;
    }
}
#endif

VOID InitializeRouteTable(PROUTE_TABLE RouteTable) {
    InitializeListHead(&RouteTable->IPv4ListHead);
    InitializeListHead(&RouteTable->IPv6ListHead);
    KeInitializeSpinLock(&RouteTable->Lock);
}

VOID InitializeAllRouteTable() {
    for (LONG i = 0; i < ROUTE_TABLE_TOTAL_COUNT; i++) {
        InitializeRouteTable(&WinPFilterRouteTable[i]);
    }
}

VOID CleanupRouteTable(PROUTE_TABLE RouteTable, BOOLEAN AcquireLock) {
    PLIST_ENTRY entry;
    PLIST_ENTRY nextEntry;
    KIRQL oldIrql;
    if (AcquireLock) {
        KeAcquireSpinLock(&RouteTable->Lock, &oldIrql);
    }
    

    for (entry = RouteTable->IPv4ListHead.Flink; entry != &RouteTable->IPv4ListHead; entry = nextEntry) {
        nextEntry = entry->Flink;
        RemoveEntryList(entry);
        ExFreePoolWithTag(CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry), ROUTE_TABLE_ALLOC_TAG);
    }

    for (entry = RouteTable->IPv6ListHead.Flink; entry != &RouteTable->IPv6ListHead; entry = nextEntry) {
        nextEntry = entry->Flink;
        RemoveEntryList(entry);
        ExFreePoolWithTag(CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry), ROUTE_TABLE_ALLOC_TAG);
    }

    if (AcquireLock) {
        KeReleaseSpinLock(&RouteTable->Lock, oldIrql);
    }
}

VOID CleanupAllRouteTable() {
    for (LONG i = 0; i < ROUTE_TABLE_TOTAL_COUNT; i++) {
        CleanupRouteTable(&WinPFilterRouteTable[i],TRUE);
    }
}

BOOLEAN CompareRoutes(PROUTE_ENTRY a, PROUTE_ENTRY b)
{
    if (a->Metric != b->Metric) {
        return (a->Metric < b->Metric);
    }

    if (a->Prefix != b->Prefix) {
        return (a->Prefix > b->Prefix);
    }

    return 0;
}

NTSTATUS AddRouteEntry(PROUTE_TABLE RouteTable,IP_PROTOCOLS Type,UNI_IPADDRESS Destination,UNI_IPADDRESS Gateway,ULONG Prefix,NET_LUID Interface,ULONG Metric,BOOLEAN AcquireLock) {
    PROUTE_ENTRY newEntry;
    PLIST_ENTRY entry;
    PLIST_ENTRY prevEntry;
    PROUTE_ENTRY routeEntry;
    KIRQL oldIrql;
    LIST_ENTRY* listHead;

    newEntry = (PROUTE_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(ROUTE_ENTRY), ROUTE_TABLE_ALLOC_TAG);
    if (newEntry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    newEntry->Type = Type;
    newEntry->Prefix = Prefix;
    newEntry->InterfaceLuid = Interface;
    newEntry->Metric = Metric;

    newEntry->Destination = Destination;
    newEntry->Gateway = Gateway;
    if (Type == IPV4) {
        listHead = &RouteTable->IPv4ListHead;
    }
    else if (Type == IPV6) {
        listHead = &RouteTable->IPv6ListHead;
    }
    else {
        ExFreePoolWithTag(newEntry, ROUTE_TABLE_ALLOC_TAG);
        return STATUS_INVALID_PARAMETER;
    }
    if(AcquireLock){ 
        KeAcquireSpinLock(&RouteTable->Lock, &oldIrql); 
    }
        

    prevEntry = listHead;
    for (entry = listHead->Flink; entry != listHead; entry = entry->Flink) {
        routeEntry = CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry);
        if (CompareRoutes(newEntry, routeEntry) > 0) {
            InsertHeadList(prevEntry, &newEntry->ListEntry);
            if (AcquireLock) { KeReleaseSpinLock(&RouteTable->Lock, oldIrql); }
            return STATUS_SUCCESS;
        }
        prevEntry = entry;
    }
    InsertHeadList(prevEntry, &newEntry->ListEntry);
    if (AcquireLock) { KeReleaseSpinLock(&RouteTable->Lock, oldIrql); }
    
    return STATUS_SUCCESS;
}


BOOLEAN IsAddressInSubnet(UNI_IPADDRESS Address,UNI_IPADDRESS Destination,ULONG PrefixLength,IP_PROTOCOLS Type) {
    ULONG i;
    ULONG byteCount = PrefixLength / 8;
    ULONG bitCount = PrefixLength % 8;
    CHAR* AddrBuf = (Type == IPV4) ? Address.IPV4Address.AddressBytes : Address.IPV6Address.AddressBytes;
    CHAR* DestBuf = (Type == IPV4) ? Destination.IPV4Address.AddressBytes : Destination.IPV6Address.AddressBytes;
    for (i = 0; i < byteCount; i++) {
        if (AddrBuf[i] != DestBuf[i]) {
            return FALSE;
        }
    }

    if (bitCount > 0) {
        UCHAR mask = (UCHAR)(0xFF00 >> bitCount);
        if ((AddrBuf[byteCount] & mask) != (DestBuf[byteCount] & mask)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN IsAddressEqual(UNI_IPADDRESS Address,ULONG AddressPrefixLength,UNI_IPADDRESS Destination,ULONG DestPrefixLength,IP_PROTOCOLS Type) {
    ULONG i;
    if (DestPrefixLength != AddressPrefixLength) {
        return FALSE;
    }
    CHAR* AddrBuf = (Type == IPV4) ? Address.IPV4Address.AddressBytes : Address.IPV6Address.AddressBytes;
    CHAR* DestBuf = (Type == IPV4) ? Destination.IPV4Address.AddressBytes : Destination.IPV6Address.AddressBytes;
    ULONG Length = ((Type == IPV4) ? IPV4_ADDRESS_BYTE_LENGTH : IPV6_ADDRESS_BYTE_LENGTH);
    for (i = 0; i < Length; i++) {
        if (AddrBuf[i] != DestBuf[i]) {
            return FALSE;
        }
    }
    return TRUE;
}


NTSTATUS FindRouteEntry(PROUTE_TABLE RouteTable,ULONG Type,UNI_IPADDRESS Address,PROUTE_ENTRY OutEntry,BOOLEAN AcquireLock) {
    PLIST_ENTRY entry;
    PROUTE_ENTRY routeEntry;
    KIRQL oldIrql;
    LIST_ENTRY* listHead;

    if (AcquireLock) {
        KeAcquireSpinLock(&RouteTable->Lock, &oldIrql);
    }

    if (Type == IPV4) {
        listHead = &RouteTable->IPv4ListHead;
    }
    else if (Type == IPV6) {
        listHead = &RouteTable->IPv6ListHead;
    }
    else {
        if (AcquireLock) { 
            KeReleaseSpinLock(&RouteTable->Lock, oldIrql); 
        }

        return STATUS_INVALID_PARAMETER;
    }

    for (entry = listHead->Flink; entry != listHead; entry = entry->Flink) {
        routeEntry = CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry);

        if (routeEntry->Type == Type) {
                if (IsAddressInSubnet(Address, routeEntry->Destination, routeEntry->Prefix, Type)) {
                    *OutEntry = *routeEntry;
                    if (AcquireLock) { 
                        KeReleaseSpinLock(&RouteTable->Lock, oldIrql); 
                    }
                    return STATUS_SUCCESS;
                }
           }
    }
    if(AcquireLock){ 
        KeReleaseSpinLock(&RouteTable->Lock, oldIrql);
    }
    
    return STATUS_NOT_FOUND;
}

NTSTATUS RemoveRouteEntry(PROUTE_TABLE RouteTable,ULONG Type,UNI_IPADDRESS Address,ULONG PrefixLength, BOOLEAN AcquireLock) {
    PLIST_ENTRY entry;
    PLIST_ENTRY nextEntry;
    PROUTE_ENTRY routeEntry;
    KIRQL oldIrql;
    LIST_ENTRY* listHead;
    NTSTATUS status = STATUS_NOT_FOUND;
    if (AcquireLock) {
        KeAcquireSpinLock(&RouteTable->Lock, &oldIrql);
    }

    if (Type == IPV4) {
        listHead = &RouteTable->IPv4ListHead;
    }
    else if (Type == IPV6) {
        listHead = &RouteTable->IPv6ListHead;
    }
    else {
        if (AcquireLock) {
            KeReleaseSpinLock(&RouteTable->Lock, oldIrql);
        }
        return STATUS_INVALID_PARAMETER;
    }


    for (entry = listHead->Flink; entry != listHead; entry = nextEntry) {
        nextEntry = entry->Flink;
        routeEntry = CONTAINING_RECORD(entry, ROUTE_ENTRY, ListEntry);

        if (routeEntry->Type == Type) {
            if (IsAddressEqual(Address, PrefixLength, routeEntry->Destination, routeEntry->Prefix, Type)) {
                RemoveEntryList(&routeEntry->ListEntry);
                ExFreePoolWithTag(routeEntry, ROUTE_TABLE_ALLOC_TAG);
                status = STATUS_SUCCESS;
            }
        }
    }

    if (AcquireLock) {
        KeReleaseSpinLock(&RouteTable->Lock, oldIrql);
    }
    return status;
}
