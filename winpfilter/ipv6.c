#include "net/ipv6.h"

BYTE* GetTransportLayerHeaderFromIPv6Header(PIPV6_HEADER IPv6Header) {
	BYTE ProtocolId = IPV6_HEADER_NEXT_HEADER(IPv6Header);
	BYTE* TransLayerPrt = (BYTE*)(IPv6Header + 1);
	ULONG NxHdrOffset;

	while (ProtocolId == IPV6_EXT_HOPOPT || ProtocolId == IPV6_EXT_ROUTE ||
		ProtocolId == IPV6_EXT_FRAG || ProtocolId == IPV6_EXT_ESP || ProtocolId == IPV6_EXT_AH ||
		ProtocolId == IPV6_EXT_NONXT || ProtocolId == IPV6_EXT_OPTS) {

		NxHdrOffset = ((*(TransLayerPrt + 1)) + 1) << 3;
		TransLayerPrt += NxHdrOffset;
		ProtocolId = *TransLayerPrt;
	}
	return TransLayerPrt;
}

BYTE GetTransportLayerProtocolFromIPv6Header(PIPV6_HEADER IPv6Header) {
	BYTE ProtocolId = IPV6_HEADER_NEXT_HEADER(IPv6Header);
	BYTE* TransLayerPrt = (BYTE*)(IPv6Header + 1);
	ULONG NxHdrOffset;

	while (ProtocolId == IPV6_EXT_HOPOPT || ProtocolId == IPV6_EXT_ROUTE ||
		ProtocolId == IPV6_EXT_FRAG || ProtocolId == IPV6_EXT_ESP || ProtocolId == IPV6_EXT_AH ||
		ProtocolId == IPV6_EXT_NONXT || ProtocolId == IPV6_EXT_OPTS) {

		NxHdrOffset = ((*(TransLayerPrt + 1)) + 1) << 3;
		TransLayerPrt += NxHdrOffset;
		ProtocolId = *TransLayerPrt;
	}
	return ProtocolId;
}