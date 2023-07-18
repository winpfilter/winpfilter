#pragma once
#include "winpfilter.h"
#include "converter.h"

#define MAC_LENGTH	6					

#define ETH_PROTOCOL_IP		0x0800U		
#define ETH_PROTOCOL_ARP	0x0806U		
#define ETH_PROTOCOL_8021Q	0x8100U    
#define ETH_PROTOCOL_IPV6	0x86DDU	

#pragma pack (1)
typedef struct _ETH_HEADER
{
	BYTE DestMAC[MAC_LENGTH];			
	BYTE SrcMac[MAC_LENGTH];	
	USHORT Protocol;				
}ETH_HEADER,*PETH_HEADER;
#pragma pack ()

inline USHORT GetEtherHeaderProtocol(PETH_HEADER header) {
	return CONVERT_NETE_16(header->Protocol);
}

inline VOID SetEtherHeaderProtocol(PETH_HEADER header, USHORT protocol) {
	header->Protocol = CONVERT_NETE_16(protocol);
}

inline PVOID GetNetworkLayerHeaderFromEtherHeader(PETH_HEADER header) {
	return (PVOID)((BYTE*)header + sizeof(ETH_HEADER));
}