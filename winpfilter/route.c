#include "route.h"
#include "net/ip.h"
#include "net/ether.h"

BOOLEAN IsSystemRouteTableChangeMonitorActive = FALSE;
HANDLE WPFilterRouteTableChangeNotifyHandle = NULL;

BOOLEAN IsUnicastIpChangeMonitorActive = FALSE;
HANDLE WPFilterUnicastIpChangeNotifyHandle = NULL;

VOID RouteTableChangeNotifyCallback(PVOID CallerContext, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType);
VOID UnicastIpAddressChangeNotifyCallback(PVOID CallerContext,PMIB_UNICASTIPADDRESS_ROW Row,MIB_NOTIFICATION_TYPE NotificationType);


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

NTSTATUS StartMonitorUnicastIpChange() {
	NTSTATUS Status;

	TRACE_ENTER();

	if (!IsUnicastIpChangeMonitorActive) {
		Status = NotifyUnicastIpAddressChange(AF_UNSPEC, (PIPFORWARD_CHANGE_CALLBACK)UnicastIpAddressChangeNotifyCallback, NULL, TRUE, &WPFilterUnicastIpChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsUnicastIpChangeMonitorActive = TRUE;
		}

		return Status;
	}

	TRACE_EXIT();

	return STATUS_SUCCESS;
}


NTSTATUS StopMonitorUnicastIpChange() {
	NTSTATUS Status;

	TRACE_ENTER();
	if (IsUnicastIpChangeMonitorActive) {
		Status = CancelMibChangeNotify2(WPFilterUnicastIpChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsUnicastIpChangeMonitorActive = FALSE;
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


VOID UnicastIpAddressChangeNotifyCallback(PVOID CallerContext, PMIB_UNICASTIPADDRESS_ROW Row, MIB_NOTIFICATION_TYPE NotificationType) {

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

BOOLEAN IsValidForwardAddress(USHORT Protocol, NET_LUID InterfaceId,BYTE* IpAddress) {
	if (Protocol != ETH_PROTOCOL_IP && Protocol != ETH_PROTOCOL_IPV6) {
		TRACE_DBG("Not a valid IP packet");
		return FALSE;
	}

	TRACE_DBG("IPPacket info: Protocol: %d; IfId: %lld; Addr(First4bytes): %d,%d,%d,%d\n", Protocol, InterfaceId.Value, IpAddress[0], IpAddress[1], IpAddress[2], IpAddress[3]);


	return FALSE;
}