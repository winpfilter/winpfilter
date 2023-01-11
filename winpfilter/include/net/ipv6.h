#pragma once

#include "net/ip.h"


#pragma pack (1)

typedef struct _IPV6_ADDRESS {
	union
	{
		BYTE	AddressBytes[16];
		USHORT	AddressWords[8];
	};
}IPV6_ADDRESS, PIPV6_ADDRESS;
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
