#include "net/ip.h"

VOID InitializeIPAddressBySockAddrINet(PIP_ADDRESS IPAddress,PSOCKADDR_INET SockAddress,BYTE PrefixLength) {
	if (SockAddress->si_family == AF_INET) {
		IPAddress->Family = IP;
		if (PrefixLength > IPV4_ADDRESS_BIT_LENGTH)
			PrefixLength = IPV4_ADDRESS_BIT_LENGTH;
		RtlCopyMemory(IPAddress->IPV4Address.AddressBytes, &SockAddress->Ipv4.sin_addr.s_addr, IPV4_ADDRESS_BYTE_LENGTH);
		RtlCopyMemory(IPAddress->IPV4NetworkSegment.AddressBytes, &SockAddress->Ipv4.sin_addr.s_addr, IPV4_ADDRESS_BYTE_LENGTH);
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
		IPAddress->IPV6Address.AddressBytes[PrefixByteCount] &= PrefixByteRemainMask;
	}
	for (BYTE i = PrefixByteCount +1; i < IPV6_ADDRESS_BYTE_LENGTH; i++) {
		IPAddress->IPV6Address.AddressBytes[i] = 0;
	}
}