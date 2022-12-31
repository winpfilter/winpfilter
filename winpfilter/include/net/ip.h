#pragma once


#include "winpfilter.h"
#include "converter.h"

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