#include "ip.h"

#ifdef DBG
VOID PrintIPAddress(PIP_ADDRESS addr) {
	TRACE_DBG("Family:%d\n", addr->Family);
	TRACE_DBG("PrefixLength:%d\nIPAddress", addr->PrefixLength);
	int a = (addr->Family == IP) ? IPV4_ADDRESS_BYTE_LENGTH : IPV6_ADDRESS_BYTE_LENGTH;
	for (int i = 0; i < a; i++) {
		TRACE_DBG("%d.", addr->IPV6Address.AddressBytes[i]);
	}
	TRACE_DBG("\nIPAddressSegment");
	for (int i = 0; i < a; i++) {
		TRACE_DBG("%d.", addr->IPV6NetworkSegment.AddressBytes[i]);
	}
	TRACE_DBG("\n");
}
#endif 


VOID InitializeIPAddressBySockAddrINet(PIP_ADDRESS IPAddress,PSOCKADDR_INET SockAddress,BYTE PrefixLength) {
	RtlZeroMemory(IPAddress, sizeof(IP_ADDRESS));
	if (SockAddress->si_family == AF_INET) {
		IPAddress->Family = IP;
		if (PrefixLength > IPV4_ADDRESS_BIT_LENGTH)
			PrefixLength = IPV4_ADDRESS_BIT_LENGTH;
		RtlCopyMemory(IPAddress->IPV4Address.AddressBytes, &SockAddress->Ipv4.sin_addr.s_addr, IPV4_ADDRESS_BYTE_LENGTH);
		RtlCopyMemory(IPAddress->IPV4NetworkSegment.AddressBytes, &SockAddress->Ipv4.sin_addr.s_addr, IPV4_ADDRESS_BYTE_LENGTH);
		IPAddress->IPV4BroadcastAddress.AddressInt32 = CONVERT_NETE_32(CONVERT_NETE_32(IPAddress->IPV4BroadcastAddress.AddressInt32) | (0xffffffff) >> PrefixLength);
	}
	else {
		IPAddress->Family = IPV6;
		if (PrefixLength > IPV6_ADDRESS_BIT_LENGTH)
			PrefixLength = IPV6_ADDRESS_BIT_LENGTH;
		RtlCopyMemory(IPAddress->IPV6Address.AddressBytes, &SockAddress->Ipv6.sin6_addr.s6_bytes, IPV6_ADDRESS_BYTE_LENGTH);
		RtlCopyMemory(IPAddress->IPV6NetworkSegment.AddressBytes, &SockAddress->Ipv6.sin6_addr.s6_bytes, IPV6_ADDRESS_BYTE_LENGTH);
	}
	IPAddress->PrefixLength = PrefixLength;
	BYTE PrefixByteCount = PrefixLength / BYTE_BIT_LENGTH;
	BYTE PrefixByteRemain = PrefixLength % BYTE_BIT_LENGTH;
	BYTE PrefixByteRemainMask = (((BYTE)0xff) << (BYTE_BIT_LENGTH- PrefixByteRemain));
	if (PrefixByteCount < IPV6_ADDRESS_BYTE_LENGTH) {
		IPAddress->IPV6NetworkSegment.AddressBytes[PrefixByteCount] &= PrefixByteRemainMask;
	}
	for (BYTE i = PrefixByteCount +1; i < IPV6_ADDRESS_BYTE_LENGTH; i++) {
		IPAddress->IPV6NetworkSegment.AddressBytes[i] = 0;
	}
}