#pragma once
#include <cybozu/inttype.hpp>

namespace cybozu {

template<class Iter>
uint32_t hash32(Iter begin, Iter end, uint32_t v = 0)
{
	if (v == 0) v = 2166136261U;
	while (begin != end) {
		v ^= *begin++;
		v *= 16777619;
	}
	return v;
}
template<class Iter>
uint64_t hash64(Iter begin, Iter end, uint64_t v = 0)
{
	if (v == 0) v = 14695981039346656037ULL;
	while (begin != end) {
		v ^= *begin++;
		v *= 1099511628211ULL;
	}
	v ^= v >> 32;
	return v;
}
template<class T>
uint32_t hash32(const T *x, size_t n, uint32_t v = 0)
{
	return hash32(x, x + n, v);
}
template<class T>
uint64_t hash64(const T *x, size_t n, uint64_t v = 0)
{
	return hash64(x, x + n, v);
}

} // cybozu

namespace boost {

template<class T>
struct hash;

} // boost

#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
#include <functional>
#else

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4099) // missmatch class and struct
#endif
#if !(defined(__APPLE__) && defined(__clang__))
template<class T>
struct hash;
#endif
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

CYBOZU_NAMESPACE_TR1_END } // std

#endif
