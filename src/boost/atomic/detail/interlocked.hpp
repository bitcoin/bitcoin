#ifndef BOOST_ATOMIC_DETAIL_INTERLOCKED_HPP
#define BOOST_ATOMIC_DETAIL_INTERLOCKED_HPP

//  Copyright (c) 2009 Helge Bahmann
//  Copyright (c) 2012 - 2014, 2017 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(_WIN32_WCE)

#if _WIN32_WCE >= 0x600

extern "C" long __cdecl _InterlockedCompareExchange(long volatile*, long, long);
extern "C" long __cdecl _InterlockedExchangeAdd(long volatile*, long);
extern "C" long __cdecl _InterlockedExchange(long volatile*, long);
extern "C" long __cdecl _InterlockedIncrement(long volatile*);
extern "C" long __cdecl _InterlockedDecrement(long volatile*);

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare) _InterlockedCompareExchange((long*)(dest), exchange, compare)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, addend) _InterlockedExchangeAdd((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval) _InterlockedExchange((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_INCREMENT(dest) _InterlockedIncrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_DECREMENT(dest) _InterlockedDecrement((long*)(dest))

#else // _WIN32_WCE >= 0x600

extern "C" long __cdecl InterlockedCompareExchange(long*, long, long);
extern "C" long __cdecl InterlockedExchangeAdd(long*, long);
extern "C" long __cdecl InterlockedExchange(long*, long);
extern "C" long __cdecl InterlockedIncrement(long*);
extern "C" long __cdecl InterlockedDecrement(long*);

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare) InterlockedCompareExchange((long*)(dest), exchange, compare)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, addend) InterlockedExchangeAdd((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval) InterlockedExchange((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_INCREMENT(dest) InterlockedIncrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_DECREMENT(dest) InterlockedDecrement((long*)(dest))

#endif // _WIN32_WCE >= 0x600

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) ((void*)BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE((long*)(dest), (long)(exchange), (long)(compare)))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, exchange) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE((long*)(dest), (long)(exchange)))

#elif defined(_MSC_VER) && _MSC_VER >= 1310

#if _MSC_VER < 1400

extern "C" long __cdecl _InterlockedCompareExchange(long volatile*, long, long);
extern "C" long __cdecl _InterlockedExchangeAdd(long volatile*, long);
extern "C" long __cdecl _InterlockedExchange(long volatile*, long);
extern "C" long __cdecl _InterlockedIncrement(long volatile*);
extern "C" long __cdecl _InterlockedDecrement(long volatile*);

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#endif

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare) _InterlockedCompareExchange((long*)(dest), exchange, compare)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, addend) _InterlockedExchangeAdd((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval) _InterlockedExchange((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_INCREMENT(dest) _InterlockedIncrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_DECREMENT(dest) _InterlockedDecrement((long*)(dest))

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) ((void*)BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE((long*)(dest), (long)(exchange), (long)(compare)))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, exchange) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE((long*)(dest), (long)(exchange)))

#else // _MSC_VER < 1400

#include <intrin.h>

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_interlockedbittestandset)
#pragma intrinsic(_interlockedbittestandreset)
#endif

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare) _InterlockedCompareExchange((long*)(dest), (long)(exchange), (long)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, addend) _InterlockedExchangeAdd((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval) _InterlockedExchange((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_INCREMENT(dest) _InterlockedIncrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_DECREMENT(dest) _InterlockedDecrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_AND(dest, arg) _InterlockedAnd((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR(dest, arg) _InterlockedOr((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR(dest, arg) _InterlockedXor((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTS(dest, arg) _interlockedbittestandset((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTR(dest, arg) _interlockedbittestandreset((long*)(dest), (long)(arg))

#if defined(_M_AMD64)
#if defined(BOOST_MSVC)
#pragma intrinsic(_interlockedbittestandset64)
#pragma intrinsic(_interlockedbittestandreset64)
#endif

#define BOOST_ATOMIC_INTERLOCKED_BTS64(dest, arg) _interlockedbittestandset64((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTR64(dest, arg) _interlockedbittestandreset64((__int64*)(dest), (__int64)(arg))
#endif // defined(_M_AMD64)

#if (defined(_M_IX86) && _M_IX86 >= 500) || defined(_M_AMD64) || defined(_M_IA64)
#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange64)
#endif
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64(dest, exchange, compare) _InterlockedCompareExchange64((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#endif

#if _MSC_VER >= 1500 && defined(_M_AMD64)
#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange128)
#endif
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE128(dest, exchange, compare) _InterlockedCompareExchange128((__int64*)(dest), ((const __int64*)(&exchange))[1], ((const __int64*)(&exchange))[0], (__int64*)(compare))
#endif

#if _MSC_VER >= 1600

// MSVC 2010 and later provide intrinsics for 8 and 16 bit integers.
// Note that for each bit count these macros must be either all defined or all not defined.
// Otherwise atomic<> operations will be implemented inconsistently.

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange8)
#pragma intrinsic(_InterlockedExchangeAdd8)
#pragma intrinsic(_InterlockedExchange8)
#pragma intrinsic(_InterlockedAnd8)
#pragma intrinsic(_InterlockedOr8)
#pragma intrinsic(_InterlockedXor8)
#endif

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE8(dest, exchange, compare) _InterlockedCompareExchange8((char*)(dest), (char)(exchange), (char)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD8(dest, addend) _InterlockedExchangeAdd8((char*)(dest), (char)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE8(dest, newval) _InterlockedExchange8((char*)(dest), (char)(newval))
#define BOOST_ATOMIC_INTERLOCKED_AND8(dest, arg) _InterlockedAnd8((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR8(dest, arg) _InterlockedOr8((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR8(dest, arg) _InterlockedXor8((char*)(dest), (char)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange16)
#pragma intrinsic(_InterlockedExchangeAdd16)
#pragma intrinsic(_InterlockedExchange16)
#pragma intrinsic(_InterlockedAnd16)
#pragma intrinsic(_InterlockedOr16)
#pragma intrinsic(_InterlockedXor16)
#endif

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE16(dest, exchange, compare) _InterlockedCompareExchange16((short*)(dest), (short)(exchange), (short)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD16(dest, addend) _InterlockedExchangeAdd16((short*)(dest), (short)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE16(dest, newval) _InterlockedExchange16((short*)(dest), (short)(newval))
#define BOOST_ATOMIC_INTERLOCKED_AND16(dest, arg) _InterlockedAnd16((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR16(dest, arg) _InterlockedOr16((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR16(dest, arg) _InterlockedXor16((short*)(dest), (short)(arg))

#endif // _MSC_VER >= 1600

#if defined(_M_AMD64) || defined(_M_IA64)

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#endif

#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64(dest, addend) _InterlockedExchangeAdd64((__int64*)(dest), (__int64)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64(dest, newval) _InterlockedExchange64((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_AND64(dest, arg) _InterlockedAnd64((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR64(dest, arg) _InterlockedOr64((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR64(dest, arg) _InterlockedXor64((__int64*)(dest), (__int64)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedExchangePointer)
#endif

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) _InterlockedCompareExchangePointer((void**)(dest), (void*)(exchange), (void*)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) _InterlockedExchangePointer((void**)(dest), (void*)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64((long*)(dest), byte_offset))

#elif defined(_M_IX86)

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) ((void*)_InterlockedCompareExchange((long*)(dest), (long)(exchange), (long)(compare)))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) ((void*)_InterlockedExchange((long*)(dest), (long)(newval)))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD((long*)(dest), byte_offset))

#endif

#if _MSC_VER >= 1700 && (defined(_M_ARM) || defined(_M_ARM64))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#endif

#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64(dest, addend) _InterlockedExchangeAdd64((__int64*)(dest), (__int64)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64(dest, newval) _InterlockedExchange64((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_AND64(dest, arg) _InterlockedAnd64((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR64(dest, arg) _InterlockedOr64((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR64(dest, arg) _InterlockedXor64((__int64*)(dest), (__int64)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedCompareExchange8_nf)
#pragma intrinsic(_InterlockedCompareExchange8_acq)
#pragma intrinsic(_InterlockedCompareExchange8_rel)
#pragma intrinsic(_InterlockedCompareExchange16_nf)
#pragma intrinsic(_InterlockedCompareExchange16_acq)
#pragma intrinsic(_InterlockedCompareExchange16_rel)
#pragma intrinsic(_InterlockedCompareExchange_nf)
#pragma intrinsic(_InterlockedCompareExchange_acq)
#pragma intrinsic(_InterlockedCompareExchange_rel)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedCompareExchange64_nf)
#pragma intrinsic(_InterlockedCompareExchange64_acq)
#pragma intrinsic(_InterlockedCompareExchange64_rel)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer_nf)
#pragma intrinsic(_InterlockedCompareExchangePointer_acq)
#pragma intrinsic(_InterlockedCompareExchangePointer_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE8_RELAXED(dest, exchange, compare) _InterlockedCompareExchange8_nf((char*)(dest), (char)(exchange), (char)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE8_ACQUIRE(dest, exchange, compare) _InterlockedCompareExchange8_acq((char*)(dest), (char)(exchange), (char)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE8_RELEASE(dest, exchange, compare) _InterlockedCompareExchange8_rel((char*)(dest), (char)(exchange), (char)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE16_RELAXED(dest, exchange, compare) _InterlockedCompareExchange16_nf((short*)(dest), (short)(exchange), (short)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE16_ACQUIRE(dest, exchange, compare) _InterlockedCompareExchange16_acq((short*)(dest), (short)(exchange), (short)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE16_RELEASE(dest, exchange, compare) _InterlockedCompareExchange16_rel((short*)(dest), (short)(exchange), (short)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_RELAXED(dest, exchange, compare) _InterlockedCompareExchange_nf((long*)(dest), (long)(exchange), (long)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_ACQUIRE(dest, exchange, compare) _InterlockedCompareExchange_acq((long*)(dest), (long)(exchange), (long)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_RELEASE(dest, exchange, compare) _InterlockedCompareExchange_rel((long*)(dest), (long)(exchange), (long)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64(dest, exchange, compare) _InterlockedCompareExchange64((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64_RELAXED(dest, exchange, compare) _InterlockedCompareExchange64_nf((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64_ACQUIRE(dest, exchange, compare) _InterlockedCompareExchange64_acq((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64_RELEASE(dest, exchange, compare) _InterlockedCompareExchange64_rel((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) _InterlockedCompareExchangePointer((void**)(dest), (void*)(exchange), (void*)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER_RELAXED(dest, exchange, compare) _InterlockedCompareExchangePointer_nf((void**)(dest), (void*)(exchange), (void*)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER_ACQUIRE(dest, exchange, compare) _InterlockedCompareExchangePointer_acq((void**)(dest), (void*)(exchange), (void*)(compare))
#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER_RELEASE(dest, exchange, compare) _InterlockedCompareExchangePointer_rel((void**)(dest), (void*)(exchange), (void*)(compare))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedExchangeAdd8_nf)
#pragma intrinsic(_InterlockedExchangeAdd8_acq)
#pragma intrinsic(_InterlockedExchangeAdd8_rel)
#pragma intrinsic(_InterlockedExchangeAdd16_nf)
#pragma intrinsic(_InterlockedExchangeAdd16_acq)
#pragma intrinsic(_InterlockedExchangeAdd16_rel)
#pragma intrinsic(_InterlockedExchangeAdd_nf)
#pragma intrinsic(_InterlockedExchangeAdd_acq)
#pragma intrinsic(_InterlockedExchangeAdd_rel)
#pragma intrinsic(_InterlockedExchangeAdd64_nf)
#pragma intrinsic(_InterlockedExchangeAdd64_acq)
#pragma intrinsic(_InterlockedExchangeAdd64_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD8_RELAXED(dest, addend) _InterlockedExchangeAdd8_nf((char*)(dest), (char)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD8_ACQUIRE(dest, addend) _InterlockedExchangeAdd8_acq((char*)(dest), (char)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD8_RELEASE(dest, addend) _InterlockedExchangeAdd8_rel((char*)(dest), (char)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD16_RELAXED(dest, addend) _InterlockedExchangeAdd16_nf((short*)(dest), (short)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD16_ACQUIRE(dest, addend) _InterlockedExchangeAdd16_acq((short*)(dest), (short)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD16_RELEASE(dest, addend) _InterlockedExchangeAdd16_rel((short*)(dest), (short)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_RELAXED(dest, addend) _InterlockedExchangeAdd_nf((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_ACQUIRE(dest, addend) _InterlockedExchangeAdd_acq((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_RELEASE(dest, addend) _InterlockedExchangeAdd_rel((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64_RELAXED(dest, addend) _InterlockedExchangeAdd64_nf((__int64*)(dest), (__int64)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64_ACQUIRE(dest, addend) _InterlockedExchangeAdd64_acq((__int64*)(dest), (__int64)(addend))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64_RELEASE(dest, addend) _InterlockedExchangeAdd64_rel((__int64*)(dest), (__int64)(addend))

#if defined(_M_ARM64)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64((__int64*)(dest), byte_offset))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER_RELAXED(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64_RELAXED((__int64*)(dest), byte_offset))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER_ACQUIRE(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64_ACQUIRE((__int64*)(dest), byte_offset))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER_RELEASE(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64_RELEASE((__int64*)(dest), byte_offset))
#else
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD((long*)(dest), byte_offset))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER_RELAXED(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_RELAXED((long*)(dest), byte_offset))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER_ACQUIRE(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_ACQUIRE((long*)(dest), byte_offset))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER_RELEASE(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_RELEASE((long*)(dest), byte_offset))
#endif

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedExchange8_nf)
#pragma intrinsic(_InterlockedExchange8_acq)
#pragma intrinsic(_InterlockedExchange16_nf)
#pragma intrinsic(_InterlockedExchange16_acq)
#pragma intrinsic(_InterlockedExchange_nf)
#pragma intrinsic(_InterlockedExchange_acq)
#pragma intrinsic(_InterlockedExchange64_nf)
#pragma intrinsic(_InterlockedExchange64_acq)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedExchangePointer_nf)
#pragma intrinsic(_InterlockedExchangePointer_acq)
#if _MSC_VER >= 1800
#pragma intrinsic(_InterlockedExchange8_rel)
#pragma intrinsic(_InterlockedExchange16_rel)
#pragma intrinsic(_InterlockedExchange_rel)
#pragma intrinsic(_InterlockedExchange64_rel)
#pragma intrinsic(_InterlockedExchangePointer_rel)
#endif
#endif

#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE8_RELAXED(dest, newval) _InterlockedExchange8_nf((char*)(dest), (char)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE8_ACQUIRE(dest, newval) _InterlockedExchange8_acq((char*)(dest), (char)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE16_RELAXED(dest, newval) _InterlockedExchange16_nf((short*)(dest), (short)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE16_ACQUIRE(dest, newval) _InterlockedExchange16_acq((short*)(dest), (short)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_RELAXED(dest, newval) _InterlockedExchange_nf((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ACQUIRE(dest, newval) _InterlockedExchange_acq((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64_RELAXED(dest, newval) _InterlockedExchange64_nf((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64_ACQUIRE(dest, newval) _InterlockedExchange64_acq((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) _InterlockedExchangePointer((void**)(dest), (void*)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER_RELAXED(dest, newval) _InterlockedExchangePointer_nf((void**)(dest), (void*)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER_ACQUIRE(dest, newval) _InterlockedExchangePointer_acq((void**)(dest), (void*)(newval))

#if _MSC_VER >= 1800
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE8_RELEASE(dest, newval) _InterlockedExchange8_rel((char*)(dest), (char)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE16_RELEASE(dest, newval) _InterlockedExchange16_rel((short*)(dest), (short)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_RELEASE(dest, newval) _InterlockedExchange_rel((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64_RELEASE(dest, newval) _InterlockedExchange64_rel((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER_RELEASE(dest, newval) _InterlockedExchangePointer_rel((void**)(dest), (void*)(newval))
#else
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE8_RELEASE(dest, newval) BOOST_ATOMIC_INTERLOCKED_EXCHANGE8(dest, newval)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE16_RELEASE(dest, newval) BOOST_ATOMIC_INTERLOCKED_EXCHANGE16(dest, newval)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_RELEASE(dest, newval) BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64_RELEASE(dest, newval) BOOST_ATOMIC_INTERLOCKED_EXCHANGE64(dest, newval)
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER_RELEASE(dest, newval) BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval)
#endif

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedAnd8_nf)
#pragma intrinsic(_InterlockedAnd8_acq)
#pragma intrinsic(_InterlockedAnd8_rel)
#pragma intrinsic(_InterlockedAnd16_nf)
#pragma intrinsic(_InterlockedAnd16_acq)
#pragma intrinsic(_InterlockedAnd16_rel)
#pragma intrinsic(_InterlockedAnd_nf)
#pragma intrinsic(_InterlockedAnd_acq)
#pragma intrinsic(_InterlockedAnd_rel)
#pragma intrinsic(_InterlockedAnd64_nf)
#pragma intrinsic(_InterlockedAnd64_acq)
#pragma intrinsic(_InterlockedAnd64_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_AND8_RELAXED(dest, arg) _InterlockedAnd8_nf((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND8_ACQUIRE(dest, arg) _InterlockedAnd8_acq((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND8_RELEASE(dest, arg) _InterlockedAnd8_rel((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND16_RELAXED(dest, arg) _InterlockedAnd16_nf((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND16_ACQUIRE(dest, arg) _InterlockedAnd16_acq((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND16_RELEASE(dest, arg) _InterlockedAnd16_rel((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND_RELAXED(dest, arg) _InterlockedAnd_nf((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND_ACQUIRE(dest, arg) _InterlockedAnd_acq((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND_RELEASE(dest, arg) _InterlockedAnd_rel((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND64_RELAXED(dest, arg) _InterlockedAnd64_nf((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND64_ACQUIRE(dest, arg) _InterlockedAnd64_acq((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_AND64_RELEASE(dest, arg) _InterlockedAnd64_rel((__int64*)(dest), (__int64)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedOr8_nf)
#pragma intrinsic(_InterlockedOr8_acq)
#pragma intrinsic(_InterlockedOr8_rel)
#pragma intrinsic(_InterlockedOr16_nf)
#pragma intrinsic(_InterlockedOr16_acq)
#pragma intrinsic(_InterlockedOr16_rel)
#pragma intrinsic(_InterlockedOr_nf)
#pragma intrinsic(_InterlockedOr_acq)
#pragma intrinsic(_InterlockedOr_rel)
#pragma intrinsic(_InterlockedOr64_nf)
#pragma intrinsic(_InterlockedOr64_acq)
#pragma intrinsic(_InterlockedOr64_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_OR8_RELAXED(dest, arg) _InterlockedOr8_nf((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR8_ACQUIRE(dest, arg) _InterlockedOr8_acq((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR8_RELEASE(dest, arg) _InterlockedOr8_rel((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR16_RELAXED(dest, arg) _InterlockedOr16_nf((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR16_ACQUIRE(dest, arg) _InterlockedOr16_acq((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR16_RELEASE(dest, arg) _InterlockedOr16_rel((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR_RELAXED(dest, arg) _InterlockedOr_nf((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR_ACQUIRE(dest, arg) _InterlockedOr_acq((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR_RELEASE(dest, arg) _InterlockedOr_rel((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR64_RELAXED(dest, arg) _InterlockedOr64_nf((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR64_ACQUIRE(dest, arg) _InterlockedOr64_acq((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_OR64_RELEASE(dest, arg) _InterlockedOr64_rel((__int64*)(dest), (__int64)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_InterlockedXor8_nf)
#pragma intrinsic(_InterlockedXor8_acq)
#pragma intrinsic(_InterlockedXor8_rel)
#pragma intrinsic(_InterlockedXor16_nf)
#pragma intrinsic(_InterlockedXor16_acq)
#pragma intrinsic(_InterlockedXor16_rel)
#pragma intrinsic(_InterlockedXor_nf)
#pragma intrinsic(_InterlockedXor_acq)
#pragma intrinsic(_InterlockedXor_rel)
#pragma intrinsic(_InterlockedXor64_nf)
#pragma intrinsic(_InterlockedXor64_acq)
#pragma intrinsic(_InterlockedXor64_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_XOR8_RELAXED(dest, arg) _InterlockedXor8_nf((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR8_ACQUIRE(dest, arg) _InterlockedXor8_acq((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR8_RELEASE(dest, arg) _InterlockedXor8_rel((char*)(dest), (char)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR16_RELAXED(dest, arg) _InterlockedXor16_nf((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR16_ACQUIRE(dest, arg) _InterlockedXor16_acq((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR16_RELEASE(dest, arg) _InterlockedXor16_rel((short*)(dest), (short)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR_RELAXED(dest, arg) _InterlockedXor_nf((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR_ACQUIRE(dest, arg) _InterlockedXor_acq((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR_RELEASE(dest, arg) _InterlockedXor_rel((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR64_RELAXED(dest, arg) _InterlockedXor64_nf((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR64_ACQUIRE(dest, arg) _InterlockedXor64_acq((__int64*)(dest), (__int64)(arg))
#define BOOST_ATOMIC_INTERLOCKED_XOR64_RELEASE(dest, arg) _InterlockedXor64_rel((__int64*)(dest), (__int64)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_interlockedbittestandset_nf)
#pragma intrinsic(_interlockedbittestandset_acq)
#pragma intrinsic(_interlockedbittestandset_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_BTS_RELAXED(dest, arg) _interlockedbittestandset_nf((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTS_ACQUIRE(dest, arg) _interlockedbittestandset_acq((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTS_RELEASE(dest, arg) _interlockedbittestandset_rel((long*)(dest), (long)(arg))

#if defined(BOOST_MSVC)
#pragma intrinsic(_interlockedbittestandreset_nf)
#pragma intrinsic(_interlockedbittestandreset_acq)
#pragma intrinsic(_interlockedbittestandreset_rel)
#endif

#define BOOST_ATOMIC_INTERLOCKED_BTR_RELAXED(dest, arg) _interlockedbittestandreset_nf((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTR_ACQUIRE(dest, arg) _interlockedbittestandreset_acq((long*)(dest), (long)(arg))
#define BOOST_ATOMIC_INTERLOCKED_BTR_RELEASE(dest, arg) _interlockedbittestandreset_rel((long*)(dest), (long)(arg))

#endif // _MSC_VER >= 1700 && defined(_M_ARM)

#endif // _MSC_VER < 1400

#else // defined(_MSC_VER) && _MSC_VER >= 1310

#if defined(BOOST_USE_WINDOWS_H)

#include <windows.h>

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare) InterlockedCompareExchange((long*)(dest), (long)(exchange), (long)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval) InterlockedExchange((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, addend) InterlockedExchangeAdd((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_INCREMENT(dest) InterlockedIncrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_DECREMENT(dest) InterlockedDecrement((long*)(dest))

#if defined(_WIN64)

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64(dest, exchange, compare) InterlockedCompareExchange64((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64(dest, newval) InterlockedExchange64((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64(dest, addend) InterlockedExchangeAdd64((__int64*)(dest), (__int64)(addend))

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) InterlockedCompareExchangePointer((void**)(dest), (void*)(exchange), (void*)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) InterlockedExchangePointer((void**)(dest), (void*)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64(dest, byte_offset))

#else // defined(_WIN64)

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) ((void*)BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, byte_offset))

#endif // defined(_WIN64)

#else // defined(BOOST_USE_WINDOWS_H)

#if defined(__MINGW64__)
#define BOOST_ATOMIC_INTERLOCKED_IMPORT
#else
#define BOOST_ATOMIC_INTERLOCKED_IMPORT __declspec(dllimport)
#endif

namespace boost {
namespace atomics {
namespace detail {

extern "C" {

BOOST_ATOMIC_INTERLOCKED_IMPORT long __stdcall InterlockedCompareExchange(long volatile*, long, long);
BOOST_ATOMIC_INTERLOCKED_IMPORT long __stdcall InterlockedExchange(long volatile*, long);
BOOST_ATOMIC_INTERLOCKED_IMPORT long __stdcall InterlockedExchangeAdd(long volatile*, long);
BOOST_ATOMIC_INTERLOCKED_IMPORT long __stdcall InterlockedIncrement(long volatile*, long);
BOOST_ATOMIC_INTERLOCKED_IMPORT long __stdcall InterlockedDecrement(long volatile*, long);

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare) boost::atomics::detail::InterlockedCompareExchange((long*)(dest), (long)(exchange), (long)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval) boost::atomics::detail::InterlockedExchange((long*)(dest), (long)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, addend) boost::atomics::detail::InterlockedExchangeAdd((long*)(dest), (long)(addend))
#define BOOST_ATOMIC_INTERLOCKED_INCREMENT(dest) boost::atomics::detail::InterlockedIncrement((long*)(dest))
#define BOOST_ATOMIC_INTERLOCKED_DECREMENT(dest) boost::atomics::detail::InterlockedDecrement((long*)(dest))

#if defined(_WIN64)

BOOST_ATOMIC_INTERLOCKED_IMPORT __int64 __stdcall InterlockedCompareExchange64(__int64 volatile*, __int64, __int64);
BOOST_ATOMIC_INTERLOCKED_IMPORT __int64 __stdcall InterlockedExchange64(__int64 volatile*, __int64);
BOOST_ATOMIC_INTERLOCKED_IMPORT __int64 __stdcall InterlockedExchangeAdd64(__int64 volatile*, __int64);

BOOST_ATOMIC_INTERLOCKED_IMPORT void* __stdcall InterlockedCompareExchangePointer(void* volatile*, void*, void*);
BOOST_ATOMIC_INTERLOCKED_IMPORT void* __stdcall InterlockedExchangePointer(void* volatile*, void*);

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE64(dest, exchange, compare) boost::atomics::detail::InterlockedCompareExchange64((__int64*)(dest), (__int64)(exchange), (__int64)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE64(dest, newval) boost::atomics::detail::InterlockedExchange64((__int64*)(dest), (__int64)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64(dest, addend) boost::atomics::detail::InterlockedExchangeAdd64((__int64*)(dest), (__int64)(addend))

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) boost::atomics::detail::InterlockedCompareExchangePointer((void**)(dest), (void*)(exchange), (void*)(compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) boost::atomics::detail::InterlockedExchangePointer((void**)(dest), (void*)(newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD64(dest, byte_offset))

#else // defined(_WIN64)

#define BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE_POINTER(dest, exchange, compare) ((void*)BOOST_ATOMIC_INTERLOCKED_COMPARE_EXCHANGE(dest, exchange, compare))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_POINTER(dest, newval) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE(dest, newval))
#define BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD_POINTER(dest, byte_offset) ((void*)BOOST_ATOMIC_INTERLOCKED_EXCHANGE_ADD(dest, byte_offset))

#endif // defined(_WIN64)

} // extern "C"

} // namespace detail
} // namespace atomics
} // namespace boost

#undef BOOST_ATOMIC_INTERLOCKED_IMPORT

#endif // defined(BOOST_USE_WINDOWS_H)

#endif // defined(_MSC_VER)

#endif
