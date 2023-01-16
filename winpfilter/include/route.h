#pragma once
#include "winpfilter.h"


NTSTATUS StartMonitorSystemRouteTableChange();
NTSTATUS StopMonitorSystemRouteTableChange();
NTSTATUS StartMonitorUnicastIpChange();
NTSTATUS StopMonitorUnicastIpChange();
BYTE IsValidForwardAddress(USHORT Protocol, IF_INDEX InterfaceId, BYTE* IpAddress);
VOID UnicastIpAddressChangeNotifyCallback(PVOID CallerContext, PMIB_UNICASTIPADDRESS_ROW Row, MIB_NOTIFICATION_TYPE NotificationType);