#pragma once
/**
	@file
	@brief atomic operation

	@author MITSUNARI Shigeo(@herumi)
	@author MITSUNARI Shigeo
*/
#include <cybozu/inttype.hpp>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#else
#include <emmintrin.h>
#endif

namespace cybozu {

namespace atomic_local {

template<size_t S>
struct Tag {};

template<>
struct Tag<4> {
	template<class T>
	static inline T AtomicAddSub(T *p, T y)
	{
#ifdef _WIN32
		return (T)_InterlockedExchangeAdd((long*)p, (long)y);
#else
		return static_cast<T>(__sync_fetch_and_add(p, y));
#endif
	}

	template<class T>
	static inline T AtomicCompareExchangeSub(T *p, T newValue, T oldValue)
	{
#ifdef _WIN32
		return (T)_InterlockedCompareExchange((long*)p, (long)newValue, (long)oldValue);
#else
		return static_cast<T>(__sync_val_compare_and_swap(p, oldValue, newValue));
#endif
	}

	template<class T>
	static inline T AtomicExchangeSub(T *p, T newValue)
	{
#ifdef _WIN32
		return (T)_InterlockedExchange((long*)p, (long)newValue);
#else
		return static_cast<T>(__sync_lock_test_and_set(p, newValue));
#endif
	}
};

template<>
struct Tag<8> {
#if (CYBOZU_OS_BIT == 64)
	template<class T>
	static inline T AtomicAddSub(T *p, T y)
	{
#ifdef _WIN32
		return (T)_InterlockedExchangeAdd64((int64_t*)p, (int64_t)y);
#else
		return static_cast<T>(__sync_fetch_and_add(p, y));
#endif
	}
#endif

	template<class T>
	static inline T AtomicCompareExchangeSub(T *p, T newValue, T oldValue)
	{
#ifdef _WIN32
		return (T)_InterlockedCompareExchange64((int64_t*)p, (int64_t)newValue, (int64_t)oldValue);
#else
		return static_cast<T>(__sync_val_compare_and_swap(p, oldValue, newValue));
#endif
	}

#if (CYBOZU_OS_BIT == 64)
	template<class T>
	static inline T AtomicExchangeSub(T *p, T newValue)
	{
#ifdef _WIN32
		return (T)_InterlockedExchange64((int64_t*)p, (int64_t)newValue);
#else
		return static_cast<T>(__sync_lock_test_and_set(p, newValue));
#endif
	}
#endif
};

} // atomic_local

/**
	atomic operation
	see http://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Atomic-Builtins.html
	http://msdn.microsoft.com/en-us/library/ms683504(VS.85).aspx
*/
/**
	tmp = *p;
	*p += y;
	return tmp;
*/
template<class T>
T AtomicAdd(T *p, T y)
{
	return atomic_local::Tag<sizeof(T)>::AtomicAddSub(p, y);
}

/**
	tmp = *p;
	if (*p == oldValue) *p = newValue;
	return tmp;
*/
template<class T>
T AtomicCompareExchange(T *p, T newValue, T oldValue)
{
	return atomic_local::Tag<sizeof(T)>::AtomicCompareExchangeSub(p, newValue, oldValue);
}

/**
	tmp = *p;
	*p = newValue;
	return tmp;
*/
template<class T>
T AtomicExchange(T *p, T newValue)
{
	return atomic_local::Tag<sizeof(T)>::AtomicExchangeSub(p, newValue);
}

inline void mfence()
{
#ifdef _MSC_VER
	MemoryBarrier();
#else
	_mm_mfence();
#endif
}

} // cybozu
