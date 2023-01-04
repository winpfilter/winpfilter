#include "route.h"
#include "net/ip.h"

BOOLEAN IsSystemRouteTableChangeMonitorActive = FALSE;
HANDLE WPFilterRouteTableChangeNotifyHandle = NULL;

VOID RouteTableChangeNotifyCallback(PVOID CallerContext, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType);


NTSTATUS StartMonitorSystemRouteTableChange() {
	NTSTATUS Status;

	TRACE_ENTER();

	if (!IsSystemRouteTableChangeMonitorActive) {
		Status = NotifyRouteChange2(AF_UNSPEC, (PIPFORWARD_CHANGE_CALLBACK)RouteTableChangeNotifyCallback, NULL, TRUE, &WPFilterRouteTableChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsSystemRouteTableChangeMonitorActive = TRUE;
		}

		return Status;
	}

	TRACE_EXIT();

	return STATUS_SUCCESS;
}


NTSTATUS StopMonitorSystemRouteTableChange() {
	NTSTATUS Status;

	TRACE_ENTER();
	if (IsSystemRouteTableChangeMonitorActive) {
		Status = CancelMibChangeNotify2(WPFilterRouteTableChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsSystemRouteTableChangeMonitorActive = FALSE;
		}

		return Status;
	}

	TRACE_EXIT();

	return STATUS_SUCCESS;
}


VOID RouteTableChangeNotifyCallback(PVOID CallerContext, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType) {

	PMIB_IPFORWARD_TABLE2 SystemRouteTable;
	ULONG SystemRouteTableItemCounts;
	PMIB_IPFORWARD_ROW2 Item;
	IP_ADDRESS RouteDestination;
	IP_ADDRESS NextHop;
	ULONG InterfaceID;
	ULONG Metric;

	NTSTATUS Status = GetIpForwardTable2(AF_UNSPEC, &SystemRouteTable);

	if (!NT_SUCCESS(Status)) {
		return;
	}

	SystemRouteTableItemCounts = SystemRouteTable->NumEntries;

	for (ULONG i = 0; i < SystemRouteTableItemCounts; i++) {
		Item = &SystemRouteTable->Table[i];
		
		InitializeIPAddressBySockAddrINet(&RouteDestination, &Item->DestinationPrefix.Prefix, Item->DestinationPrefix.PrefixLength);
		InitializeIPAddressBySockAddrINet(&NextHop, &Item->NextHop, BYTE_MAX_VALUE);
		InterfaceID = Item->InterfaceIndex;
		Metric = Item->Metric;

	}

	FreeMibTable(SystemRouteTable);

}
