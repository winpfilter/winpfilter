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


#include "net/ip.h"

#define IPV6_HEADER_TRAFFIC_CLASS(header)					(((ULONG)((header)->TrafficClassH) << 4 )| (ULONG)(((header)->TrafficClassL)))
#define IPV6_HEADER_FLOW_LABEL(header)						(((ULONG)((header)->FlowLabelH) << 16 )| (ULONG)(CONVERT_NETE_16((header)->FlowLabelL)))
#define IPV6_HEADER_PAYLOAD_LENGTH(header)					(CONVERT_NETE_16((header)->PayloadLength))
#define IPV6_HEADER_NEXT_HEADER(header)						((header)->NextHeader)
#define IPV6_HEADER_HOP_LIMIT(header)						((header)->HopLimit)
#define IPV6_HEADER_SRC_ADDR(header)						(((header)->SrcAddress))
#define IPV6_HEADER_DEST_ADDR(header)						(((header)->DestAddress))

#define SET_IPV6_HEADER_PAYLOAD_LENGTH(header,length)		((header)->NextHeader = CONVERT_NETE_16(length))
#define SET_IPV6_HEADER_NEXT_HEADER(header,next_header)		((header)->NextHeader = next_header)
#define SET_IPV6_HEADER_HOP_LIMIT(header,hop_limit)			((header)->HopLimit = hop_limit)
#define SET_IPV6_HEADER_SRC_ADDR(header,src_addr)			((header)->SrcAddress = (src_addr))
#define SET_IPV6_HEADER_DEST_ADDR(header,dest_addr)			((header)->DestAddress = (dest_addr))


BYTE* GetTransportLayerHeaderFromIPv6Header(PIPV6_HEADER IPv6Header);

BYTE GetTransportLayerProtocolFromIPv6Header(PIPV6_HEADER IPv6Header);