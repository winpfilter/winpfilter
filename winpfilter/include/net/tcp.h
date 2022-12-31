#pragma once

#include "winpfilter.h"
#include "converter.h"

#pragma pack (1)
typedef struct _TCP_FLAGS {
#if  BIG_ENDIAN
	BYTE CWR : 1;
	BYTE ECE : 1;
	BYTE URG : 1;
	BYTE ACK : 1;
	BYTE PSH : 1;
	BYTE RST : 1;
	BYTE SYN : 1;
	BYTE FIN : 1;
#else 
	BYTE FIN : 1;
	BYTE SYN : 1;
	BYTE RST : 1;
	BYTE PSH : 1;
	BYTE ACK : 1;
	BYTE URG : 1;
	BYTE ECE : 1;
	BYTE CWR : 1;
#endif
}TCP_FLAGS,*PTCP_FLAGS;

typedef struct _TCP_HEADER {
	USHORT		SrcPort;
	USHORT		DestPort;
	ULONG		Sequence;
	ULONG		Acknowledgment;
#if  BIG_ENDIAN
	BYTE		HeaderLength : 4;
	BYTE : 4;
#else 
	BYTE : 4;
	BYTE		HeaderLength : 4;
#endif
	TCP_FLAGS	Flags;
	USHORT		Window;
	USHORT		Checksum;
	USHORT		UrgentPointer;
}TCP_HEADER, * PTCP_HEADER;
#pragma pack ()

#define TCP_HEADER_SRC_PORT(header)						(CONVERT_NETE_16((header)->SrcPort))
#define TCP_HEADER_DEST_PORT(header)					(CONVERT_NETE_16((header)->DestPort))
#define TCP_HEADER_SEQ_NUMBER(header)					(CONVERT_NETE_32((header)->Sequence))
#define TCP_HEADER_ACK_NUMBER(header)					(CONVERT_NETE_32((header)->Acknowledgment))
#define TCP_HEADER_LENGTH(header)						((header)->HeaderLength)
#define TCP_HEADER_LENGTH_BYTES(header)					(TCP_HEADER_LENGTH(header)<<2)
#define TCP_HEADER_FLAGS(header)						((header)->Flags)
#define TCP_HEADER_FLAGS_CWR(header)					((header)->Flags.CWR)
#define TCP_HEADER_FLAGS_ECE(header)					((header)->Flags.ECE)
#define TCP_HEADER_FLAGS_URG(header)					((header)->Flags.URG)
#define TCP_HEADER_FLAGS_ACK(header)					((header)->Flags.ACK)
#define TCP_HEADER_FLAGS_PSH(header)					((header)->Flags.PSH)
#define TCP_HEADER_FLAGS_RST(header)					((header)->Flags.RST)
#define TCP_HEADER_FLAGS_SYN(header)					((header)->Flags.SYN)
#define TCP_HEADER_FLAGS_FIN(header)					((header)->Flags.FIN)
#define TCP_HEADER_WINDOW(header)						(CONVERT_NETE_16((header)->Window))
#define TCP_HEADER_CHECKSUM(header)						(CONVERT_NETE_16((header)->Checksum))
#define TCP_HEADER_URG_PTR(header)						(CONVERT_NETE_16((header)->UrgentPointer))

#define SET_TCP_HEADER_SRC_PORT(header,sport)			((header)->SrcPort = CONVERT_NETE_16(sport))
#define SET_TCP_HEADER_DEST_PORT(header,dport)			((header)->DestPort = CONVERT_NETE_16(dport))
#define SET_TCP_HEADER_SEQ_NUMBER(header,seq_num)		((header)->Sequence = CONVERT_NETE_32(seq_num))
#define SET_TCP_HEADER_ACK_NUMBER(header,ack_num)		((header)->Acknowledgment = CONVERT_NETE_32(ack_num))
#define SET_TCP_HEADER_LENGTH(header,length)			((header)->HeaderLength = (BYTE)length)
#define SET_TCP_HEADER_LENGTH_BYTES(header,length_byte)	(SET_TCP_HEADER_LENGTH(header,length_byte>>2))
#define SET_TCP_HEADER_FLAGS(header,flags)				((header)->Flags = flags)
#define SET_TCP_HEADER_FLAGS_CWR(header,flag_cwr)		((header)->Flags.CWR = flag_cwr)
#define SET_TCP_HEADER_FLAGS_ECE(header,flag_ece)		((header)->Flags.ECE = flag_ece)
#define SET_TCP_HEADER_FLAGS_URG(header,flag_urg)		((header)->Flags.URG = flag_urg)
#define SET_TCP_HEADER_FLAGS_ACK(header,flag_ack)		((header)->Flags.ACK = flag_ack)
#define SET_TCP_HEADER_FLAGS_PSH(header,flag_psh)		((header)->Flags.PSH = flag_psh)
#define SET_TCP_HEADER_FLAGS_RST(header,flag_rst)		((header)->Flags.RST = flag_rst)
#define SET_TCP_HEADER_FLAGS_SYN(header,flag_syn)		((header)->Flags.SYN = flag_syn)
#define SET_TCP_HEADER_FLAGS_FIN(header,flag_fin)		((header)->Flags.FIN = flag_fin)
#define SET_TCP_HEADER_WINDOW(header,window)			((header)->Window = CONVERT_NETE_16(window))
#define SET_TCP_HEADER_CHECKSUM(header,checksum)		((header)->Checksum = CONVERT_NETE_16(checksum))
#define SET_TCP_HEADER_URG_PTR(header,urg_ptr)			((header)->UrgentPointer = CONVERT_NETE_16(urg_ptr))
