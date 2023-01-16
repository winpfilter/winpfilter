#include "route.h"
#include "global_variables.h"
#include "filter_subroutines.h"
#include "net/ip.h"
#include "net/ether.h"

BOOLEAN IsSystemRouteTableChangeMonitorActive = FALSE;
HANDLE WPFilterRouteTableChangeNotifyHandle = NULL;

BOOLEAN IsUnicastIpChangeMonitorActive = FALSE;
HANDLE WPFilterUnicastIpChangeNotifyHandle = NULL;

VOID RouteTableChangeNotifyCallback(PVOID CallerContext, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType);
VOID UnicastIpAddressChangeNotifyCallback(PVOID CallerContext, PMIB_UNICASTIPADDRESS_ROW Row, MIB_NOTIFICATION_TYPE NotificationType);


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
		Status = NotifyUnicastIpAddressChange(AF_UNSPEC, (PUNICAST_IPADDRESS_CHANGE_CALLBACK)UnicastIpAddressChangeNotifyCallback, NULL, TRUE, &WPFilterUnicastIpChangeNotifyHandle);
		if (NT_SUCCESS(Status)) {
			IsUnicastIpChangeMonitorActive = TRUE;
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

	PMIB_UNICASTIPADDRESS_TABLE UnicastIPTable;
	ULONG UnicastIPTableItemCounts;
	PMIB_UNICASTIPADDRESS_ROW UnicastIPTableItem;
	IF_INDEX InterfaceIndex;
	
	NTSTATUS Status = GetUnicastIpAddressTable(AF_UNSPEC, &UnicastIPTable);

	if (!NT_SUCCESS(Status)) {
		return;
	}


	UnicastIPTableItemCounts = UnicastIPTable->NumEntries;

	for (ULONG i = 0; i < UnicastIPTableItemCounts; i++) {
		UnicastIPTableItem = &UnicastIPTable->Table[i];
		InterfaceIndex = UnicastIPTableItem->InterfaceIndex;


		
	}

	FreeMibTable(UnicastIPTable);

}

BYTE IsValidForwardAddress(USHORT Protocol, IF_INDEX InterfaceIndex, BYTE* IpAddress) {

	TRACE_DBG("IPPacket info: Protocol: %d; IfId: %d; Addr(First4bytes): %d,%d,%d,%d\n", Protocol, InterfaceIndex, IpAddress[0], IpAddress[1], IpAddress[2], IpAddress[3]);
	
	if (Protocol == ETH_PROTOCOL_IP) {
		switch (IpAddress[0])
		{
		case 0:
			// 0.x.x.x
			return FALSE;
		case 127:
			//127.x.x.x
			return FALSE;
		case 169:
			if (IpAddress[1] == 254) {
				//169.254.x.x
				return FALSE;
			}
			break;
		default:
			if (IpAddress[0] >= 224) {
				// 224.0.0.0 - 255.255.255.255
				return FALSE;
			}
			break;
		}

		// Look for local IP
		if (RtlEqualMemory(IpAddress, IpAddress, IPV4_ADDRESS_BYTE_LENGTH)) {
			return FALSE;
		}


		return TRUE;
	}
	else if (Protocol == ETH_PROTOCOL_IPV6) {
		// Not 2000::/3
		if ((IpAddress[0] & 0xe0) != 0x20) {
			return FALSE;
		}

		// Look for local IP

		if (RtlEqualMemory(IpAddress, IpAddress, IPV6_ADDRESS_BYTE_LENGTH)) {
			return FALSE;
		}

		return TRUE;
	}

	TRACE_DBG("Not a valid IP packet");
	return FALSE;
}