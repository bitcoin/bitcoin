#pragma once

/**
	@file
	@brief deal with big and little endian

	@author MITSUNARI Shigeo(@herumi)
*/
#include <cybozu/inttype.hpp>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace cybozu {

#ifdef _MSC_VER
inline uint16_t byteSwap(uint16_t x) { return _byteswap_ushort(x); }
inline uint32_t byteSwap(uint32_t x) { return _byteswap_ulong(x); }
inline uint64_t byteSwap(uint64_t x) { return _byteswap_uint64(x); }
#else
#if (((__GNUC__) << 16) + (__GNUC_MINOR__)) >= ((4 << 16) + 8)
inline uint16_t byteSwap(uint16_t x) { return __builtin_bswap16(x); }
#else
inline uint16_t byteSwap(uint16_t x) { return (x >> 8) | (x << 8); }
#endif
inline uint32_t byteSwap(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t byteSwap(uint64_t x) { return __builtin_bswap64(x); }
#endif

/**
	get 16bit integer as little endian
	@param src [in] pointer
*/
inline uint16_t Get16bitAsLE(const void *src)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	uint16_t x;
	memcpy(&x, src, sizeof(x));
	return x;
#else
	const uint8_t *p = static_cast<const uint8_t *>(src);
	return p[0] | (p[1] << 8);
#endif
}

/**
	get 32bit integer as little endian
	@param src [in] pointer
*/
inline uint32_t Get32bitAsLE(const void *src)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	uint32_t x;
	memcpy(&x, src, sizeof(x));
	return x;
#else
	const uint8_t *p = static_cast<const uint8_t *>(src);
	return Get16bitAsLE(p) | (static_cast<uint32_t>(Get16bitAsLE(p + 2)) << 16);
#endif
}

/**
	get 64bit integer as little endian
	@param src [in] pointer
*/
inline uint64_t Get64bitAsLE(const void *src)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	uint64_t x;
	memcpy(&x, src, sizeof(x));
	return x;
#else
	const uint8_t *p = static_cast<const uint8_t *>(src);
	return Get32bitAsLE(p) | (static_cast<uint64_t>(Get32bitAsLE(p + 4)) << 32);
#endif
}

/**
	get 16bit integer as bit endian
	@param src [in] pointer
*/
inline uint16_t Get16bitAsBE(const void *src)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	uint16_t x;
	memcpy(&x, src, sizeof(x));
	return byteSwap(x);
#else
	const uint8_t *p = static_cast<const uint8_t *>(src);
	return p[1] | (p[0] << 8);
#endif
}

/**
	get 32bit integer as bit endian
	@param src [in] pointer
*/
inline uint32_t Get32bitAsBE(const void *src)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	uint32_t x;
	memcpy(&x, src, sizeof(x));
	return byteSwap(x);
#else
	const uint8_t *p = static_cast<const uint8_t *>(src);
	return Get16bitAsBE(p + 2) | (static_cast<uint32_t>(Get16bitAsBE(p)) << 16);
#endif
}

/**
	get 64bit integer as big endian
	@param src [in] pointer
*/
inline uint64_t Get64bitAsBE(const void *src)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	uint64_t x;
	memcpy(&x, src, sizeof(x));
	return byteSwap(x);
#else
	const uint8_t *p = static_cast<const uint8_t *>(src);
	return Get32bitAsBE(p + 4) | (static_cast<uint64_t>(Get32bitAsBE(p)) << 32);
#endif
}

/**
	set 16bit integer as little endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Set16bitAsLE(void *src, uint16_t x)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	memcpy(src, &x, sizeof(x));
#else
	uint8_t *p = static_cast<uint8_t *>(src);
	p[0] = static_cast<uint8_t>(x);
	p[1] = static_cast<uint8_t>(x >> 8);
#endif
}
/**
	set 32bit integer as little endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Set32bitAsLE(void *src, uint32_t x)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	memcpy(src, &x, sizeof(x));
#else
	uint8_t *p = static_cast<uint8_t *>(src);
	p[0] = static_cast<uint8_t>(x);
	p[1] = static_cast<uint8_t>(x >> 8);
	p[2] = static_cast<uint8_t>(x >> 16);
	p[3] = static_cast<uint8_t>(x >> 24);
#endif
}
/**
	set 64bit integer as little endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Set64bitAsLE(void *src, uint64_t x)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	memcpy(src, &x, sizeof(x));
#else
	uint8_t *p = static_cast<uint8_t *>(src);
	Set32bitAsLE(p, static_cast<uint32_t>(x));
	Set32bitAsLE(p + 4, static_cast<uint32_t>(x >> 32));
#endif
}
/**
	set 16bit integer as big endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Set16bitAsBE(void *src, uint16_t x)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	x = byteSwap(x);
	memcpy(src, &x, sizeof(x));
#else
	uint8_t *p = static_cast<uint8_t *>(src);
	p[0] = static_cast<uint8_t>(x >> 8);
	p[1] = static_cast<uint8_t>(x);
#endif
}
/**
	set 32bit integer as big endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Set32bitAsBE(void *src, uint32_t x)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	x = byteSwap(x);
	memcpy(src, &x, sizeof(x));
#else
	uint8_t *p = static_cast<uint8_t *>(src);
	p[0] = static_cast<uint8_t>(x >> 24);
	p[1] = static_cast<uint8_t>(x >> 16);
	p[2] = static_cast<uint8_t>(x >> 8);
	p[3] = static_cast<uint8_t>(x);
#endif
}
/**
	set 64bit integer as big endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Set64bitAsBE(void *src, uint64_t x)
{
#if CYBOZU_ENDIAN == CYBOZU_ENDIAN_LITTLE
	x = byteSwap(x);
	memcpy(src, &x, sizeof(x));
#else
	uint8_t *p = static_cast<uint8_t *>(src);
	Set32bitAsBE(p, static_cast<uint32_t>(x >> 32));
	Set32bitAsBE(p + 4, static_cast<uint32_t>(x));
#endif
}

} // cybozu
