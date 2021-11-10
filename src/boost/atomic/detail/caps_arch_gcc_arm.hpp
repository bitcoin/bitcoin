/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2009 Phil Endecott
 * Copyright (c) 2013 Tim Blechmann
 * ARM Code by Phil Endecott, based on other architectures.
 * Copyright (c) 2014, 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_arch_gcc_arm.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_ARM_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__ARMEL__) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
    (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || \
    defined(BOOST_WINDOWS)
#define BOOST_ATOMIC_DETAIL_ARM_LITTLE_ENDIAN
#elif defined(__ARMEB__) || \
    defined(__ARM_BIG_ENDIAN) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
    (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__))
#define BOOST_ATOMIC_DETAIL_ARM_BIG_ENDIAN
#else
#error "Boost.Atomic: Failed to determine ARM endianness, the target platform is not supported. Please, report to the developers (patches are welcome)."
#endif

#if defined(__GNUC__) && defined(__arm__) && (BOOST_ATOMIC_DETAIL_ARM_ARCH >= 6)

#if BOOST_ATOMIC_DETAIL_ARM_ARCH > 6
// ARMv7 and later have dmb instruction
#define BOOST_ATOMIC_DETAIL_ARM_HAS_DMB 1
#endif

#if defined(__ARM_FEATURE_LDREX)

#if (__ARM_FEATURE_LDREX & 1)
#define BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB 1
#endif
#if (__ARM_FEATURE_LDREX & 2)
#define BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH 1
#endif
#if (__ARM_FEATURE_LDREX & 8)
#define BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD 1
#endif

#else // defined(__ARM_FEATURE_LDREX)

#if !(defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6Z__))

// ARMv6k and ARMv7 have 8 and 16-bit ldrex/strex variants, but at least GCC 4.7 fails to compile them. GCC 4.9 is known to work.
#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 409
#define BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB 1
#define BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH 1
#endif

#if !(((defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6ZK__)) && defined(__thumb__)) || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7M__))
// ARMv6k and ARMv7 except ARMv7-M have 64-bit ldrex/strex variants.
// Unfortunately, GCC (at least 4.7.3 on Ubuntu) does not allocate register pairs properly when targeting ARMv6k Thumb,
// which is required for ldrexd/strexd instructions, so we disable 64-bit support. When targeting ARMv6k ARM
// or ARMv7 (both ARM and Thumb 2) it works as expected.
#define BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD 1
#endif

#endif // !(defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6Z__))

#endif // defined(__ARM_FEATURE_LDREX)

#endif // defined(__GNUC__) && defined(__arm__) && (BOOST_ATOMIC_DETAIL_ARM_ARCH >= 6)

#define BOOST_ATOMIC_INT8_LOCK_FREE 2
#define BOOST_ATOMIC_INT16_LOCK_FREE 2
#define BOOST_ATOMIC_INT32_LOCK_FREE 2
#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD)
#define BOOST_ATOMIC_INT64_LOCK_FREE 2
#endif
#define BOOST_ATOMIC_POINTER_LOCK_FREE 2

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_ARM_HPP_INCLUDED_
