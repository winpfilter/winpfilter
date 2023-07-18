#pragma once


#if  BIG_ENDIAN
inline USHORT CONVERT_NETE_16(USHORT u16_val) {
	return u16_val;
}
#else
inline USHORT CONVERT_NETE_16(USHORT u16_val) {
	return (USHORT)(((u16_val & 0xff) << 8) | (u16_val >> 8));
}
#endif
#if  BIG_ENDIAN
inline ULONG CONVERT_NETE_16(ULONG u32_val) {
	return u32_val;
}
#else
inline ULONG CONVERT_NETE_32(ULONG u32_val) {
	return (ULONG)(((u32_val & 0xff) << 24) | ((u32_val & 0xff00) << 8) | ((u32_val & 0xff0000) >> 8) | (u32_val >> 24));
}
#endif
