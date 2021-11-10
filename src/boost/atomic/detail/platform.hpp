/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2014-2018, 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/platform.hpp
 *
 * This header defines macros for the target platform detection
 */

#ifndef BOOST_ATOMIC_DETAIL_PLATFORM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_PLATFORM_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__GNUC__) && defined(__arm__)

// Newer gcc versions define __ARM_ARCH. Older ones don't, so we have to deduce ARM arch version from a bunch of version-specific macros.
#if defined(__ARM_ARCH)
#define BOOST_ATOMIC_DETAIL_ARM_ARCH __ARM_ARCH
#elif defined(__ARM_ARCH_8A__)
#define BOOST_ATOMIC_DETAIL_ARM_ARCH 8
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) ||\
    defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) ||\
    defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7S__)
#define BOOST_ATOMIC_DETAIL_ARM_ARCH 7
#elif defined(__ARM_ARCH_6__)  || defined(__ARM_ARCH_6J__) ||\
    defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) ||\
    defined(__ARM_ARCH_6ZK__)
#define BOOST_ATOMIC_DETAIL_ARM_ARCH 6
#else
// We are not interested in older versions - they don't support atomic ops
#define BOOST_ATOMIC_DETAIL_ARM_ARCH 0
#endif

#endif // defined(__GNUC__) && defined(__arm__)

#if !defined(BOOST_ATOMIC_FORCE_FALLBACK)

// Determine the target platform.
// The target platform describes the compiler and target architecture. It can be used by more generic backends, such as the ones
// based on compiler intrinsics, to implement specialized operations in a non-generic way.

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_x86
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND gcc_x86

#elif defined(__GNUC__) && defined(__aarch64__)

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_aarch64
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND gcc_aarch64

#elif defined(__GNUC__) && defined(__arm__) && (BOOST_ATOMIC_DETAIL_ARM_ARCH >= 6)

#if (BOOST_ATOMIC_DETAIL_ARM_ARCH >= 8)
#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_aarch32
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND gcc_aarch32
#else
#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_arm
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND gcc_arm
#endif

#elif defined(__GNUC__) && (defined(__POWERPC__) || defined(__PPC__))

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_ppc
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND gcc_ppc

#elif (defined(__GNUC__) || defined(__SUNPRO_CC)) && (defined(__sparcv8plus) || defined(__sparc_v9__))

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_sparc

#elif defined(__GNUC__) && defined(__alpha__)

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND gcc_alpha

#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND msvc_x86

#elif defined(_MSC_VER) && _MSC_VER >= 1700 && (defined(_M_ARM) || defined(_M_ARM64))

#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND msvc_arm

#endif

// Compiler-based backends

// IBM XL C++ Compiler has to be checked before GCC/Clang as it pretends to be one but does not support __atomic* intrinsics.
// It does support GCC inline assembler though.
#if !(defined(__ibmxl__) || defined(__IBMCPP__)) &&\
    ((defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 407)) ||\
        (defined(BOOST_CLANG) && ((__clang_major__ * 100 + __clang_minor__) >= 302))) &&\
    (\
        (__GCC_ATOMIC_BOOL_LOCK_FREE + 0) == 2 ||\
        (__GCC_ATOMIC_CHAR_LOCK_FREE + 0) == 2 ||\
        (__GCC_ATOMIC_SHORT_LOCK_FREE + 0) == 2 ||\
        (__GCC_ATOMIC_INT_LOCK_FREE + 0) == 2 ||\
        (__GCC_ATOMIC_LONG_LOCK_FREE + 0) == 2 ||\
        (__GCC_ATOMIC_LLONG_LOCK_FREE + 0) == 2\
    )

#define BOOST_ATOMIC_DETAIL_CORE_BACKEND gcc_atomic

// GCC __sync* instrinsics backend is less efficient than asm-based backends, so use it only when nothing better is available.
#elif !defined(BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND) &&\
    defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 401) &&\
    (\
        defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1) ||\
        defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2) ||\
        defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) ||\
        defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8) ||\
        defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)\
    )

#define BOOST_ATOMIC_DETAIL_CORE_BACKEND gcc_sync

#endif

// OS-based backends

#if !defined(BOOST_ATOMIC_DETAIL_CORE_BACKEND) && !defined(BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND)

#if defined(__linux__) && defined(__arm__)

#define BOOST_ATOMIC_DETAIL_CORE_BACKEND linux_arm

#elif defined(BOOST_WINDOWS) || defined(_WIN32_CE)

#define BOOST_ATOMIC_DETAIL_CORE_BACKEND windows

#endif

#endif // !defined(BOOST_ATOMIC_DETAIL_CORE_BACKEND)

// Waiting and notifying operations backends
#if defined(BOOST_WINDOWS)

#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND windows

#else // defined(BOOST_WINDOWS)

#include <boost/atomic/detail/futex.hpp>

#if defined(BOOST_ATOMIC_DETAIL_HAS_FUTEX)
#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND futex
#elif defined(__FreeBSD__)
#include <sys/param.h>
// FreeBSD prior to 7.0 had _umtx_op with a different signature
#if defined(__FreeBSD_version) && __FreeBSD_version >= 700000
#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND freebsd_umtx
#endif // defined(__FreeBSD_version) && __FreeBSD_version >= 700000
#elif defined(__DragonFly__)
#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND dragonfly_umtx
#endif

#endif // defined(BOOST_WINDOWS)

#endif // !defined(BOOST_ATOMIC_FORCE_FALLBACK)

#if !defined(BOOST_ATOMIC_DETAIL_FP_BACKEND)
#define BOOST_ATOMIC_DETAIL_FP_BACKEND generic
#define BOOST_ATOMIC_DETAIL_FP_BACKEND_GENERIC
#endif

#if !defined(BOOST_ATOMIC_DETAIL_EXTRA_BACKEND)
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND generic
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND_GENERIC
#endif

#if !defined(BOOST_ATOMIC_DETAIL_EXTRA_FP_BACKEND)
#define BOOST_ATOMIC_DETAIL_EXTRA_FP_BACKEND generic
#define BOOST_ATOMIC_DETAIL_EXTRA_FP_BACKEND_GENERIC
#endif

#if !defined(BOOST_ATOMIC_DETAIL_WAIT_BACKEND)
#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND generic
#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND_GENERIC
#endif

#if defined(BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND)
#define BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND_HEADER(prefix) <BOOST_JOIN(prefix, BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND).hpp>
#endif
#if defined(BOOST_ATOMIC_DETAIL_CORE_BACKEND)
#define BOOST_ATOMIC_DETAIL_CORE_BACKEND_HEADER(prefix) <BOOST_JOIN(prefix, BOOST_ATOMIC_DETAIL_CORE_BACKEND).hpp>
#endif
#define BOOST_ATOMIC_DETAIL_FP_BACKEND_HEADER(prefix) <BOOST_JOIN(prefix, BOOST_ATOMIC_DETAIL_FP_BACKEND).hpp>
#define BOOST_ATOMIC_DETAIL_EXTRA_BACKEND_HEADER(prefix) <BOOST_JOIN(prefix, BOOST_ATOMIC_DETAIL_EXTRA_BACKEND).hpp>
#define BOOST_ATOMIC_DETAIL_EXTRA_FP_BACKEND_HEADER(prefix) <BOOST_JOIN(prefix, BOOST_ATOMIC_DETAIL_EXTRA_FP_BACKEND).hpp>
#define BOOST_ATOMIC_DETAIL_WAIT_BACKEND_HEADER(prefix) <BOOST_JOIN(prefix, BOOST_ATOMIC_DETAIL_WAIT_BACKEND).hpp>

#endif // BOOST_ATOMIC_DETAIL_PLATFORM_HPP_INCLUDED_
