#include "route_info.h"
#include "global_variables.h"
#include "filter_subroutines.h"
#include "net/ip.h"
#include "net/ether.h"
#include "route_table.h"

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
	NET_LUID InterfaceLuid;
	ULONG Metric;

	NTSTATUS Status = GetIpForwardTable2(AF_UNSPEC, &SystemRouteTable);

	if (!NT_SUCCESS(Status)) {
		return;
	}

	SystemRouteTableItemCounts = SystemRouteTable->NumEntries;

	KIRQL oldIrql;
	PROUTE_TABLE SystemMirrorRouteTable = &WinPFilterRouteTable[ROUTE_TABLE_SYSTEM_MIRROR];
	KeAcquireSpinLock(&SystemMirrorRouteTable->Lock, &oldIrql);
	CleanupRouteTable(SystemMirrorRouteTable,FALSE);

	for (ULONG i = 0; i < SystemRouteTableItemCounts; i++) {
		Item = &SystemRouteTable->Table[i];
		
		UNI_IPADDRESS DestIP;
		UNI_IPADDRESS GwIP;
		IP_PROTOCOLS Protocol = Item->DestinationPrefix.Prefix.si_family == AF_INET ? IPV4 : IPV6;

		if (Protocol == IPV4) {
			DestIP.IPV4Address.AddressInt32 = Item->DestinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_addr;
			GwIP.IPV4Address.AddressInt32 = Item->NextHop.Ipv4.sin_addr.S_un.S_addr;
		}
		else {
			memcpy(DestIP.IPV6Address.AddressBytes, Item->DestinationPrefix.Prefix.Ipv6.sin6_addr.u.Byte, IPV6_ADDRESS_BYTE_LENGTH);
			memcpy(GwIP.IPV6Address.AddressBytes, Item->NextHop.Ipv6.sin6_addr.u.Byte, IPV6_ADDRESS_BYTE_LENGTH);
		}

		AddRouteEntry(SystemMirrorRouteTable, Protocol, DestIP, GwIP, Item->DestinationPrefix.PrefixLength, Item->InterfaceLuid, Item->Metric, FALSE);
	}
	KeReleaseSpinLock(&SystemMirrorRouteTable->Lock, oldIrql);

#if DBG
	TRACE_DBG("SYSTEM ROUTE TABLE (MIRROR):");
	PrintRouteTable(SystemMirrorRouteTable);
	TRACE_DBG("SYSTEM ROUTE TABLE (MIRROR) END");
#endif	


	FreeMibTable(SystemRouteTable);

}


VOID UnicastIpAddressChangeNotifyCallback(PVOID CallerContext, PMIB_UNICASTIPADDRESS_ROW Row, MIB_NOTIFICATION_TYPE NotificationType) {

	PMIB_UNICASTIPADDRESS_TABLE UnicastIPTable;
	ULONG UnicastIPTableItemCounts;
	PMIB_UNICASTIPADDRESS_ROW UnicastIPTableItem;
	IP_ADDRESS Address;

	NTSTATUS Status = GetUnicastIpAddressTable(AF_UNSPEC, &UnicastIPTable);

	if (!NT_SUCCESS(Status)) {
		return;
	}

	DeleteAllInterfaceIPCache();

	UnicastIPTableItemCounts = UnicastIPTable->NumEntries;
	for (ULONG i = 0; i < UnicastIPTableItemCounts; i++) {
		UnicastIPTableItem = &UnicastIPTable->Table[i];

		Address.Family = UnicastIPTableItem->Address.si_family == AF_INET ? IPV4 : IPV6;
		Address.PrefixLength = UnicastIPTableItem->OnLinkPrefixLength;
		UnicastIPTableItem->Address.si_family == AF_INET ? 
			RtlCopyMemory(Address.IPAddress.IPV4Address.AddressBytes, &UnicastIPTableItem->Address.Ipv4.sin_addr.s_addr, IPV4_ADDRESS_BYTE_LENGTH):
			RtlCopyMemory(Address.IPAddress.IPV6Address.AddressBytes, &UnicastIPTableItem->Address.Ipv6.sin6_addr.s6_bytes, IPV6_ADDRESS_BYTE_LENGTH);
		InsertIntoInterfaceIPCache(UnicastIPTableItem->InterfaceLuid, Address);

	}

	FreeMibTable(UnicastIPTable);

}

BYTE IsValidForwardAddress(USHORT Protocol, NET_LUID InterfaceLuid, BYTE* IpAddress) {

	TRACE_DBG("IPPacket info: Protocol: %d; LuId: %lld; Addr(First4bytes): %d,%d,%d,%d\n", Protocol, InterfaceLuid.Value, IpAddress[0], IpAddress[1], IpAddress[2], IpAddress[3]);
	
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
		if (IsLocalIP(IpAddress, IP)) {
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

		if (IsLocalIP(IpAddress, IPV6)) {
			return FALSE;
		}

		return TRUE;
	}

	TRACE_DBG("Not a valid IP packet");
	return FALSE;
}