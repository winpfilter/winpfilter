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

#define UDP_HEADER_SRC_PORT(header)						(CONVERT_NETE_16((header)->SrcPort))
#define UDP_HEADER_DEST_PORT(header)					(CONVERT_NETE_16((header)->DestPort))
#define UDP_HEADER_LENGTH(header)						(CONVERT_NETE_16((header)->Length))
#define UDP_HEADER_CHECKSUM(header)						(CONVERT_NETE_16((header)->Checksum))

#define SET_UDP_HEADER_SRC_PORT(header,sport)			((header)->SrcPort = CONVERT_NETE_16(sport))
#define SET_UDP_HEADER_DEST_PORT(header,dport)			((header)->DestPort = CONVERT_NETE_16(dport))
#define SET_UDP_HEADER_LENGTH(header,length)			((header)->Length = CONVERT_NETE_16(length))
#define SET_UDP_HEADER_CHECKSUM(header,checksum)		((header)->Checksum = CONVERT_NETE_16(checksum))