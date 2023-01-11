#pragma once


#include "winpfilter.h"
#include "converter.h"
#include "net/ipv4.h"
#include "net/ipv6.h"

typedef struct _IP_ADDRESS {
	BYTE Family;
	BYTE PrefixLength;
	union
	{
		IPV4_ADDRESS	IPV4Address;
		IPV6_ADDRESS	IPV6Address;
	};
	union
	{
		IPV4_ADDRESS	IPV4NetworkSegment;
		IPV6_ADDRESS	IPV6NetworkSegment;
	};
	IPV4_ADDRESS	IPV4BroadcastAddress;
}IP_ADDRESS, *PIP_ADDRESS;


typedef enum _IP_PROTOCOLS
{
	IP					= 0,
	IPV6_EXT_HOPOPT		= 0,
	ICMP				= 1,
	IGMP				= 2,
	TCP					= 6,
	UDP					= 17,
	IPV6				= 41,
	IPV6_EXT_ROUTE		= 43,
	IPV6_EXT_FRAG		= 44,
	IPV6_EXT_ESP		= 50,
	IPV6_EXT_AH			= 51,
	ICMPV6		  		= 58,
	IPV6_EXT_NONXT		= 59,
	IPV6_EXT_OPTS		= 60
}IP_PROTOCOLS;



#define IP_HEADER_VERSION(header)						((header)->Version)
#define SET_IP_HEADER_VERSION(header,version)			((header)->Version = (BYTE)version)

VOID InitializeIPAddressBySockAddrINet(PIP_ADDRESS IPAddress, PSOCKADDR_INET SockAddress, BYTE PrefixLength);