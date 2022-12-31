#pragma once


#if  BIG_ENDIAN
#define CONVERT_NETE_16(u16_val) ((USHORT)u16_val)
#else
#define CONVERT_NETE_16(u16_val) ((USHORT)((((USHORT)u16_val)&0xff)<<8)|(((USHORT)u16_val)>>8))
#endif
#if  BIG_ENDIAN
#define CONVERT_NETE_32(u32_val) ((ULONG)u32_val)
#else
#define CONVERT_NETE_32(u32_val) ((ULONG)((((ULONG)u32_val)&0xff)<<24)|((((ULONG)u32_val)&0xff00)<<8)|((((ULONG)u32_val)&0xff0000)>>8)|(((ULONG)u32_val)>>24))
#endif
