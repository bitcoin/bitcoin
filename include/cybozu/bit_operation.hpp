#pragma once
/**
	@file
	@brief bit operation
*/
#include <assert.h>
#include <cybozu/inttype.hpp>

#if (CYBOZU_HOST == CYBOZU_HOST_INTEL)
	#if defined(_WIN32)
		#include <intrin.h>
	#elif defined(__linux__) || defined(__CYGWIN__) || defined(__clang__)
		#include <x86intrin.h>
	#elif defined(__GNUC__)
		#include <emmintrin.h>
	#endif
#endif

namespace cybozu {

namespace bit_op_local {

template<bool equalTo8>
struct Tag {};

// sizeof(T) < 8
template<>
struct Tag<false> {
	template<class T>
	static inline int bsf(T x)
	{
#if defined(_MSC_VER)
		unsigned long out;
		_BitScanForward(&out, x);
#pragma warning(suppress: 6102)
		return out;
#else
		return __builtin_ctz(x);
#endif
	}
	template<class T>
	static inline int bsr(T x)
	{
#if defined(_MSC_VER)
		unsigned long out;
		_BitScanReverse(&out, x);
#pragma warning(suppress: 6102)
		return out;
#else
		return __builtin_clz(x) ^ 0x1f;
#endif
	}
};

// sizeof(T) == 8
template<>
struct Tag<true> {
	template<class T>
	static inline int bsf(T x)
	{
#if defined(_MSC_VER) && defined(_WIN64)
		unsigned long out;
		_BitScanForward64(&out, x);
#pragma warning(suppress: 6102)
		return out;
#elif defined(__x86_64__)
		return __builtin_ctzll(x);
#else
		const uint32_t L = uint32_t(x);
		if (L) return Tag<false>::bsf(L);
		const uint32_t H = uint32_t(x >> 32);
		return Tag<false>::bsf(H) + 32;
#endif
	}
	template<class T>
	static inline int bsr(T x)
	{
#if defined(_MSC_VER) && defined(_WIN64)
		unsigned long out;
		_BitScanReverse64(&out, x);
#pragma warning(suppress: 6102)
		return out;
#elif defined(__x86_64__)
		return __builtin_clzll(x) ^ 0x3f;
#else
		const uint32_t H = uint32_t(x >> 32);
		if (H) return Tag<false>::bsr(H) + 32;
		const uint32_t L = uint32_t(x);
		return Tag<false>::bsr(L);
#endif
	}
};

} // bit_op_local

template<class T>
int bsf(T x)
{
	return bit_op_local::Tag<sizeof(T) == 8>::bsf(x);
}
template<class T>
int bsr(T x)
{
	return bit_op_local::Tag<sizeof(T) == 8>::bsr(x);
}

template<class T>
uint64_t makeBitMask64(T x)
{
	assert(x < 64);
	return (uint64_t(1) << x) - 1;
}

template<class T>
uint32_t popcnt(T x);

template<>
inline uint32_t popcnt<uint32_t>(uint32_t x)
{
#if defined(_MSC_VER)
	return static_cast<uint32_t>(_mm_popcnt_u32(x));
#else
	return static_cast<uint32_t>(__builtin_popcount(x));
#endif
}

template<>
inline uint32_t popcnt<uint64_t>(uint64_t x)
{
#if defined(__x86_64__)
	return static_cast<uint32_t>(__builtin_popcountll(x));
#elif defined(_WIN64)
	return static_cast<uint32_t>(_mm_popcnt_u64(x));
#else
	return popcnt<uint32_t>(static_cast<uint32_t>(x)) + popcnt<uint32_t>(static_cast<uint32_t>(x >> 32));
#endif
}

} // cybozu
