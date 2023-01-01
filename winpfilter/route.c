#include "route.h"
#include "net/ip.h"

BOOLEAN IsSystemRouteTableChangeMonitorActive = FALSE;
HANDLE WPFilterRouteTableChangeNotifyHandle = NULL;

VOID RouteTableChangeNotifyCallback(PVOID CallerContext, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType);

typedef struct  _ROUTE_ENTRY {
	IP_ADDRESS Destination;
	IP_ADDRESS Gateway;
	ULONG InterfaceID;
	ULONG Metric;
}ROUTE_ENTRY, * PROUTE_ENTRY;

typedef struct  _ROUTE_ENTRY_LIST {
	struct ROUTE_ENTRY_LIST*	Prev;
	struct ROUTE_ENTRY_LIST*	Next;
	ROUTE_ENTRY					Route;
}ROUTE_ENTRY_LIST, * PROUTE_ENTRY_LIST;

typedef struct  _ROUTE_TABLE {
	UNICODE_STRING		TableName;
	PROUTE_ENTRY_LIST	IPv4RouteList;
	PROUTE_ENTRY_LIST	IPv6RouteList;
}ROUTE_TABLE, * PROUTE_TABLE;

PROUTE_TABLE InitializeRouteTable(PUNICODE_STRING RouteTableName) {
	PROUTE_TABLE RouteTable = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ROUTE_TABLE), ROUTE_TABLE_ALLOC_TAG);
	if (RouteTable == NULL) {
		return NULL;
	}
	RtlZeroMemory(RouteTable, sizeof(ROUTE_TABLE));
	RtlCopyUnicodeString(&RouteTable->TableName, RouteTableName);
	RouteTable->IPv4RouteList = NULL;
	RouteTable->IPv6RouteList = NULL;
}

FreeRouteEntryList(PROUTE_ENTRY_LIST RouteEntryListHead) {
	PROUTE_ENTRY_LIST CurrentLink = RouteEntryListHead;
	while (CurrentLink != NULL) {
		PROUTE_ENTRY_LIST ToBeFreeLink = CurrentLink;
		CurrentLink = CurrentLink->Next;
		ExFreePoolWithTag(ToBeFreeLink, ROUTE_TABLE_ALLOC_TAG);
	}
}

VOID FreeRouteTable(PROUTE_TABLE RouteTable) {
	RtlFreeUnicodeString(&RouteTable->TableName);
	FreeRouteEntryList(RouteTable->IPv4RouteList);
	FreeRouteEntryList(RouteTable->IPv6RouteList);
	ExFreePoolWithTag(RouteTable, ROUTE_TABLE_ALLOC_TAG);
}

NTSTATUS StartMonitorSystemRouteTableChange() {
	if (!IsSystemRouteTableChangeMonitorActive) {
		NTSTATUS Status = NotifyRouteChange2(AF_UNSPEC, (PIPFORWARD_CHANGE_CALLBACK)RouteTableChangeNotifyCallback, NULL, TRUE, &WPFilterRouteTableChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsSystemRouteTableChangeMonitorActive = TRUE;
		}
		return Status;
	}
	return STATUS_SUCCESS;
}


NTSTATUS StopMonitorSystemRouteTableChange() {
	if (IsSystemRouteTableChangeMonitorActive) {
		NTSTATUS Status = CancelMibChangeNotify2(WPFilterRouteTableChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsSystemRouteTableChangeMonitorActive = FALSE;
		}
		return Status;
	}
	return STATUS_SUCCESS;
}


VOID PrintIP(PIP_ADDRESS addr) {
	DbgPrint("Family:%d\n", addr->Family);
	DbgPrint("PrefixLength:%d\nIPAddress", addr->PrefixLength);
	int a = (addr->Family == IP) ? 4 : 16;
	for (int i = 0; i < a; i++) {
		DbgPrint("%d.", addr->IPV6Address.AddressBytes[i]);
	}
	DbgPrint("\nIPAddressSegment");
	for (int i = 0; i < a; i++) {
		DbgPrint("%d.", addr->IPV6NetworkSegment.AddressBytes[i]);
	}
	DbgPrint("\n");
}


VOID RouteTableChangeNotifyCallback(PVOID CallerContext, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType) {
	TRACE_ENTER();

	PMIB_IPFORWARD_TABLE2 SystemRouteTable;
	NTSTATUS Status = GetIpForwardTable2(AF_UNSPEC, &SystemRouteTable);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("Status%d\n", Status);
		return;
	}

	ULONG SystemRouteTableItemCounts = SystemRouteTable->NumEntries;
	DbgPrint("SystemRouteTableItemCounts%d\n", SystemRouteTableItemCounts);
	for (ULONG i = 0; i < SystemRouteTableItemCounts; i++) {
		PMIB_IPFORWARD_ROW2 Item = &SystemRouteTable->Table[i];

		IP_ADDRESS RouteDestination;
		InitializeIPAddressBySockAddrINet(&RouteDestination, &Item->DestinationPrefix.Prefix, Item->DestinationPrefix.PrefixLength);
		IP_ADDRESS NextHop;
		InitializeIPAddressBySockAddrINet(&NextHop, &Item->NextHop, BYTE_MAX_VALUE);
		ULONG InterfaceID = Item->InterfaceIndex;
		ULONG Metric = Item->Metric;
		DbgPrint("DEST\n");
		PrintIP(&RouteDestination);
		DbgPrint("NEXTHOP\n");
		PrintIP(&NextHop);
	}
	FreeMibTable(SystemRouteTable);
	TRACE_EXIT();
}
