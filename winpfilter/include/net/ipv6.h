#pragma once
#include "winpfilter.h"
#pragma pack (1)

typedef struct _IPV6_ADDRESS {
	union
	{
		BYTE		AddressBytes[16];
		USHORT		AddressWords[8];
		ULONG		AddressDwords[4];
		ULONGLONG	AddressQwords[2];
	};
}IPV6_ADDRESS, *PIPV6_ADDRESS;
#define IPV6_ADDRESS_BYTE_LENGTH 16
#define IPV6_ADDRESS_BIT_LENGTH 128

typedef struct _IPV6_HEADER {
#if  BIG_ENDIAN
	BYTE	Version : 4;
	BYTE	TrafficClassH : 4;
#else 
	BYTE	TrafficClassH : 4;
	BYTE	Version : 4;
#endif
#if  BIG_ENDIAN
	BYTE	TrafficClassL : 4;
	BYTE	FlowLabelH : 4;
#else 
	BYTE	FlowLabelH : 4;
	BYTE	TrafficClassL : 4;
#endif
	USHORT	FlowLabelL;
	USHORT	PayloadLength;
	BYTE	NextHeader;
	BYTE	HopLimit;
	IPV6_ADDRESS	SrcAddress;
	IPV6_ADDRESS	DestAddress;
}IPV6_HEADER, * PIPV6_HEADER;

#pragma pack ()


#include "ip.h"

#define IPV6_HEADER_TRAFFIC_CLASS(header)					(((ULONG)((header)->TrafficClassH) << 4 )| (ULONG)(((header)->TrafficClassL)))
#define IPV6_HEADER_FLOW_LABEL(header)						(((ULONG)((header)->FlowLabelH) << 16 )| (ULONG)(CONVERT_NETE_16((header)->FlowLabelL)))

inline USHORT GetIPv6HeaderPayloadLength(PIPV6_HEADER header) {
	return CONVERT_NETE_16((header)->PayloadLength);
}

inline VOID SetIPv6HeaderPayloadLength(PIPV6_HEADER header, USHORT payload_length) {
	header->PayloadLength = CONVERT_NETE_16(payload_length);
}


inline BYTE GetIPv6HeaderNextHeader(PIPV6_HEADER header) {
	return header->NextHeader;
}

inline VOID SetIPv6HeaderNextHeader(PIPV6_HEADER header, BYTE next_header) {
	header->NextHeader = next_header;
}


inline BYTE GetIPv6HeaderHopLimit(PIPV6_HEADER header) {
	return header->HopLimit;
}

inline VOID SetIPv6HeaderHopLimit(PIPV6_HEADER header, BYTE hop_limit) {
	header->HopLimit = hop_limit;
}

inline IPV6_ADDRESS GetIPv6HeaderSrcAddr(PIPV6_HEADER header) {
	return header->SrcAddress;
}

inline VOID SetIPv6HeaderSrcAddr(PIPV6_HEADER header, IPV6_ADDRESS src_addr) {
	header->SrcAddress = src_addr;
}


inline IPV6_ADDRESS GetIPv6HeaderDestAddr(PIPV6_HEADER header) {
	return header->DestAddress;
}

inline VOID SetIPv6HeaderDestAddr(PIPV6_HEADER header, IPV6_ADDRESS dest_addr) {
	header->DestAddress = dest_addr;
}


inline PVOID GetTransportLayerHeaderFromIPv6Header(PIPV6_HEADER IPv6Header) {
	BYTE ProtocolId = GetIPv6HeaderNextHeader(IPv6Header);
	BYTE* TransLayerPrt = (BYTE*)(IPv6Header + 1);
	ULONG NxHdrOffset;

	while (ProtocolId == IPV6_EXT_HOPOPT || ProtocolId == IPV6_EXT_ROUTE ||
		ProtocolId == IPV6_EXT_FRAG || ProtocolId == IPV6_EXT_ESP || ProtocolId == IPV6_EXT_AH ||
		ProtocolId == IPV6_EXT_NONXT || ProtocolId == IPV6_EXT_OPTS) {

		NxHdrOffset = ((*(TransLayerPrt + 1)) + 1) << 3;
		TransLayerPrt += NxHdrOffset;
		ProtocolId = *TransLayerPrt;
	}
	return (PVOID)TransLayerPrt;
}

inline BYTE GetTransportLayerProtocolFromIPv6Header(PIPV6_HEADER IPv6Header) {
	BYTE ProtocolId = GetIPv6HeaderNextHeader(IPv6Header);
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