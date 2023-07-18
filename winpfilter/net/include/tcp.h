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


inline USHORT GetTCPHeaderSrcPort(PTCP_HEADER header) {
	return CONVERT_NETE_16(header->SrcPort);
}

inline VOID SetTCPHeaderSrcPort(PTCP_HEADER header, USHORT src_port) {
	header->SrcPort = CONVERT_NETE_16(src_port);
}


inline USHORT GetTCPHeaderDestPort(PTCP_HEADER header) {
	return CONVERT_NETE_16(header->DestPort);
}

inline VOID SetTCPHeaderDestPort(PTCP_HEADER header, USHORT dst_port) {
	header->DestPort = CONVERT_NETE_16(dst_port);
}


inline ULONG GetTCPHeaderSequenceNumber(PTCP_HEADER header) {
	return CONVERT_NETE_32((header)->Sequence);
}

inline VOID SetTCPHeaderSequenceNumber(PTCP_HEADER header, ULONG seq_num) {
	header->Sequence = CONVERT_NETE_32(seq_num);
}


inline ULONG GetTCPHeaderAcknowledgementNumber(PTCP_HEADER header) {
	return CONVERT_NETE_32((header)->Acknowledgment);
}

inline VOID SetTCPHeaderAcknowledgementNumber(PTCP_HEADER header, ULONG ack_num) {
	header->Acknowledgment = CONVERT_NETE_32(ack_num);
}


inline ULONG GetTCPHeaderAcknowledgementNumber(PTCP_HEADER header) {
	return CONVERT_NETE_32((header)->Acknowledgment);
}

inline VOID SetTCPHeaderAcknowledgementNumber(PTCP_HEADER header, ULONG ack_num) {
	header->Acknowledgment = CONVERT_NETE_32(ack_num);
}


inline BYTE GetTCPHeaderLength(PTCP_HEADER header) {
	return header->HeaderLength;
}

inline VOID SetTCPHeaderLength(PTCP_HEADER header, BYTE length) {
	header->HeaderLength = length;
}


inline BYTE GetTCPHeaderByteLength(PTCP_HEADER header) {
	return ((header->HeaderLength) << 2);
}

inline VOID SetTCPHeaderByteLength(PTCP_HEADER header, BYTE byte_length) {
	header->HeaderLength = (byte_length >> 2);
}


inline TCP_FLAGS GetTCPHeaderFlags(PTCP_HEADER header) {
	return header->Flags;
}

inline VOID SetTCPHeaderFlags(PTCP_HEADER header, TCP_FLAGS flags) {
	header->Flags = flags;
}


inline BYTE GetTCPHeaderFlagsCWR(PTCP_HEADER header) {
	return header->Flags.CWR;
}

inline VOID SetTCPHeaderFlagsCWR(PTCP_HEADER header, BYTE value) {
	header->Flags.CWR = value;
}


inline BYTE GetTCPHeaderFlagsECE(PTCP_HEADER header) {
	return header->Flags.ECE;
}

inline VOID SetTCPHeaderFlagsECE(PTCP_HEADER header, BYTE value) {
	header->Flags.ECE = value;
}


inline BYTE GetTCPHeaderFlagsURG(PTCP_HEADER header) {
	return header->Flags.URG;
}

inline VOID SetTCPHeaderFlagsURG(PTCP_HEADER header, BYTE value) {
	header->Flags.URG = value;
}


inline BYTE GetTCPHeaderFlagsACK(PTCP_HEADER header) {
	return header->Flags.ACK;
}

inline VOID SetTCPHeaderFlagsACK(PTCP_HEADER header, BYTE value) {
	header->Flags.ACK = value;
}


inline BYTE GetTCPHeaderFlagsPSH(PTCP_HEADER header) {
	return header->Flags.PSH;
}

inline VOID SetTCPHeaderFlagsPSH(PTCP_HEADER header, BYTE value) {
	header->Flags.PSH = value;
}


inline BYTE GetTCPHeaderFlagsRST(PTCP_HEADER header) {
	return header->Flags.RST;
}

inline VOID SetTCPHeaderFlagsRST(PTCP_HEADER header, BYTE value) {
	header->Flags.RST = value;
}


inline BYTE GetTCPHeaderFlagsSYN(PTCP_HEADER header) {
	return header->Flags.SYN;
}

inline VOID SetTCPHeaderFlagsSYN(PTCP_HEADER header, BYTE value) {
	header->Flags.SYN = value;
}


inline BYTE GetTCPHeaderFlagsFIN(PTCP_HEADER header) {
	return header->Flags.FIN;
}

inline VOID SetTCPHeaderFlagsFIN(PTCP_HEADER header, BYTE value) {
	header->Flags.FIN = value;
}

inline USHORT GetTCPHeaderWindow(PTCP_HEADER header) {
	return CONVERT_NETE_16(header->Window);
}

inline VOID SetTCPHeaderWindow(PTCP_HEADER header, USHORT window) {
	header->Window = CONVERT_NETE_16(window);
}

inline USHORT GetTCPHeaderChecksum(PTCP_HEADER header) {
	return CONVERT_NETE_16(header->Checksum);
}

inline VOID SetTCPHeaderChecksum(PTCP_HEADER header, USHORT checksum) {
	header->Checksum = CONVERT_NETE_16(checksum);
}

inline USHORT GetTCPHeaderUrgentPointer(PTCP_HEADER header) {
	return CONVERT_NETE_16(header->UrgentPointer);
}

inline VOID SetTCPHeaderSrcPort(PTCP_HEADER header, USHORT urg_ptr) {
	header->UrgentPointer = CONVERT_NETE_16(urg_ptr);
}


inline PVOID GetApplicationLayerHeaderFromTCPHeader(PTCP_HEADER header) {
	return (PVOID)(((BYTE*)header) + GetTCPHeaderByteLength(header));
}
