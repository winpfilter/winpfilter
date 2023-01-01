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

#pragma pack ()
