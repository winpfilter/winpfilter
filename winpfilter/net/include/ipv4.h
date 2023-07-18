#pragma once

#include "ip.h"

#pragma pack (1)

typedef struct _IPV4_ADDRESS {
	union 
	{
		ULONG	AddressInt32;
		BYTE	AddressBytes[4];
	};
}IPV4_ADDRESS,*PIPV4_ADDRESS;

#define IPV4_ADDRESS_BYTE_LENGTH 4
#define IPV4_ADDRESS_BIT_LENGTH 32

typedef struct _IPV4_HEADER {
#if  BIG_ENDIAN
	BYTE	Version : 4;
	BYTE	HeaderLength : 4;
#else 
	BYTE	HeaderLength : 4;
	BYTE	Version : 4;
#endif
	BYTE	TOS;
	USHORT	TotalLength;
	USHORT	Identification;
#if  BIG_ENDIAN
	BYTE : 1;
	BYTE	DF : 1;
	BYTE	MF : 1;
	BYTE	OffsetH : 5;
#else
	BYTE	OffsetH : 5;
	BYTE	MF : 1;
	BYTE	DF : 1;
	BYTE : 1;
#endif
	BYTE	OffsetL;
	BYTE	TTL;
	BYTE	Protocol;
	USHORT	Checksum;
	IPV4_ADDRESS	SrcAddress;
	IPV4_ADDRESS	DestAddress;
}IPV4_HEADER, * PIPV4_HEADER;

#pragma pack ()

inline BYTE GetIPv4HeaderLength(PIPV4_HEADER header) {
	return header->HeaderLength;
}

inline VOID SetIPv4HeaderLength(PIPV4_HEADER header,BYTE length) {
	header->HeaderLength = length;
}


inline BYTE GetIPv4HeaderByteLength(PIPV4_HEADER header) {
	return ((header->HeaderLength) << 2);
}

inline VOID SetIPv4HeaderByteLength(PIPV4_HEADER header, BYTE byte_length) {
	header->HeaderLength = byte_length >> 2;
}


inline BYTE GetIPv4HeaderTOS(PIPV4_HEADER header) {
	return header->TOS;
}

inline VOID SetIPv4HeaderTOS(PIPV4_HEADER header, BYTE tos) {
	header->TOS = tos;
}


inline USHORT GetIPv4HeaderTotalLength(PIPV4_HEADER header) {
	return CONVERT_NETE_16((header)->TotalLength);
}

inline VOID SetIPv4HeaderTotalLength(PIPV4_HEADER header, USHORT total_length) {
	header->TotalLength = CONVERT_NETE_16(total_length);
}

inline USHORT GetIPv4HeaderIdentification(PIPV4_HEADER header) {
	return CONVERT_NETE_16 (header->Identification);
}

inline VOID SetIPv4HeaderIdentification(PIPV4_HEADER header, USHORT id) {
	header->Identification = CONVERT_NETE_16(id);
}


inline BYTE GetIPv4HeaderDF(PIPV4_HEADER header) {
	return header->DF;
}

inline VOID SetIPv4HeaderDF(PIPV4_HEADER header, BYTE df) {
	header->DF = df;
}


inline BYTE GetIPv4HeaderMF(PIPV4_HEADER header) {
	return header->MF;
}

inline VOID SetIPv4HeaderMF(PIPV4_HEADER header, BYTE mf) {
	header->MF = mf;
}


inline USHORT GetIPv4HeaderOffset(PIPV4_HEADER header) {
	return ((((USHORT)(header->OffsetH)) & 0x1F) << 8) | (((USHORT)(header->OffsetL)) & 0xFF);
}

inline VOID SetIPv4HeaderOffset(PIPV4_HEADER header, USHORT offset) {
	header->OffsetL = ((USHORT)offset) & 0xFF; 
	header->OffsetH = (((USHORT)offset) >> 8) & 0x1F;
}


inline BYTE GetIPv4HeaderTTL(PIPV4_HEADER header) {
	return header->TTL;
}

inline VOID SetIPv4HeaderTTL(PIPV4_HEADER header, BYTE ttl) {
	header->TTL = ttl;
}


inline BYTE GetIPv4HeaderProtocol(PIPV4_HEADER header) {
	return header->Protocol;
}

inline VOID SetIPv4HeaderProtocol(PIPV4_HEADER header, BYTE protocol) {
	header->Protocol = protocol;
}


inline USHORT GetIPv4HeaderChecksum(PIPV4_HEADER header) {
	return CONVERT_NETE_16(header->Checksum);
}

inline VOID SetIPv4HeaderChecksum(PIPV4_HEADER header, USHORT checksum) {
	header->Checksum = CONVERT_NETE_16(checksum);
}


inline IPV4_ADDRESS GetIPv4HeaderSrcAddr(PIPV4_HEADER header) {
	return header->SrcAddress;
}

inline VOID SetIPv4HeaderSrcAddr(PIPV4_HEADER header, IPV4_ADDRESS src_addr) {
	header->SrcAddress = src_addr;
}


inline IPV4_ADDRESS GetIPv4HeaderDestAddr(PIPV4_HEADER header) {
	return header->DestAddress;
}

inline VOID SetIPv4HeaderDestAddr(PIPV4_HEADER header, IPV4_ADDRESS dest_addr) {
	header->DestAddress = dest_addr;
}

inline PVOID GetTransportLayerHeaderFromIPv4Header(PIPV4_HEADER header) {
	return (PVOID)(((BYTE*)header) + GetIPv4HeaderByteLength(header));
}