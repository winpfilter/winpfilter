#pragma once
#include "winpfilter.h"
NTSTATUS StartMonitorSystemRouteTableChange();
NTSTATUS StopMonitorSystemRouteTableChange();
BOOLEAN IsValidForwardAddress(USHORT Protocol, NET_LUID InterfaceId, BYTE* IpAddress);