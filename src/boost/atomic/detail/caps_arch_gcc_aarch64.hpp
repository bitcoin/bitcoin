/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_arch_gcc_aarch64.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_AARCH64_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_AARCH64_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__AARCH64EL__) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
    (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || \
    defined(BOOST_WINDOWS)
#define BOOST_ATOMIC_DETAIL_AARCH64_LITTLE_ENDIAN
#elif defined(__AARCH64EB__) || \
    defined(__ARM_BIG_ENDIAN) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
    (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__))
#define BOOST_ATOMIC_DETAIL_AARCH64_BIG_ENDIAN
#else
#error "Boost.Atomic: Failed to determine AArch64 endianness, the target platform is not supported. Please, report to the developers (patches are welcome)."
#endif

#if defined(__ARM_FEATURE_ATOMICS)
// ARMv8.1 added Large System Extensions, which includes cas, swp, and a number of other read-modify-write instructions
#define BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE
#endif

#if defined(__ARM_FEATURE_COMPLEX)
// ARMv8.3 added Release Consistency processor consistent (RCpc) memory model, which includes ldapr and similar instructions.
// Unfortunately, there seems to be no dedicated __ARM_FEATURE macro for this, so we use __ARM_FEATURE_COMPLEX, which is also defined starting ARMv8.3.
#define BOOST_ATOMIC_DETAIL_AARCH64_HAS_RCPC
#endif

#define BOOST_ATOMIC_INT8_LOCK_FREE 2
#define BOOST_ATOMIC_INT16_LOCK_FREE 2
#define BOOST_ATOMIC_INT32_LOCK_FREE 2
#define BOOST_ATOMIC_INT64_LOCK_FREE 2
#define BOOST_ATOMIC_INT128_LOCK_FREE 2
#define BOOST_ATOMIC_POINTER_LOCK_FREE 2

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_AARCH64_HPP_INCLUDED_
