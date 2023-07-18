#pragma once

#include "winpfilter.h"
#include "converter.h"

#pragma pack (1)
typedef struct _UDP_HEADER {
	USHORT	SrcPort;
	USHORT  DestPort;
	USHORT	Length;
	USHORT	Checksum;
}UDP_HEADER,*PUDP_HEADER;
#pragma pack ()

inline USHORT GetUDPHeaderSrcPort(PUDP_HEADER header) {
	return CONVERT_NETE_16(header->SrcPort);
}

inline VOID SetUDPHeaderSrcPort(PUDP_HEADER header, USHORT sport) {
	header->SrcPort = CONVERT_NETE_16(sport);
}


inline USHORT GetUDPHeaderDestPort(PUDP_HEADER header) {
	return CONVERT_NETE_16(header->DestPort);
}

inline VOID SetUDPHeaderDestPort(PUDP_HEADER header, USHORT dport) {
	header->DestPort = CONVERT_NETE_16(dport);
}


inline USHORT GetUDPHeaderLength(PUDP_HEADER header) {
	return CONVERT_NETE_16(header->Length);
}

inline VOID SetUDPHeaderLength(PUDP_HEADER header, USHORT length) {
	header->Length = CONVERT_NETE_16(length);
}


inline USHORT GetUDPHeaderChecksum(PUDP_HEADER header) {
	return CONVERT_NETE_16(header->Checksum);
}

inline VOID SetUDPHeaderChecksum(PUDP_HEADER header, USHORT checksum) {
	header->Checksum = CONVERT_NETE_16(checksum);
}


inline PVOID GetApplicationLayerHeaderFromUDPHeader(PUDP_HEADER header) {
	return (PVOID)((BYTE*)header + sizeof(UDP_HEADER));
}

