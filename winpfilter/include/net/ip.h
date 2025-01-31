#pragma once


#include "winpfilter.h"
#include "converter.h"

typedef enum _IP_PROTOCOLS
{
	IP = 0,
	IPV4 = 0,
	IPV6_EXT_HOPOPT = 0,
	ICMP = 1,
	IGMP = 2,
	TCP = 6,
	UDP = 17,
	IPV6 = 41,
	IPV6_EXT_ROUTE = 43,
	IPV6_EXT_FRAG = 44,
	IPV6_EXT_ESP = 50,
	IPV6_EXT_AH = 51,
	ICMPV6 = 58,
	IPV6_EXT_NONXT = 59,
	IPV6_EXT_OPTS = 60
}IP_PROTOCOLS;

#include "ipv4.h"
#include "ipv6.h"

typedef union _UNI_IPADDRESS
{
	IPV4_ADDRESS	IPV4Address;
	IPV6_ADDRESS	IPV6Address;
}UNI_IPADDRESS,*PUNI_IPADDRESS;

typedef struct _IP_ADDRESS {
	BYTE Family;
	BYTE PrefixLength;
	UNI_IPADDRESS IPAddress;
}IP_ADDRESS, *PIP_ADDRESS;



inline BYTE GetIPHeaderVersion(PVOID header) {
	return ((PIPV4_HEADER)header)->Version;
}

inline VOID SetIPHeaderVersion(PVOID header,BYTE version) {
	((PIPV4_HEADER)header)->Version = version;
}