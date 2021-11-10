/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_arch_gcc_aarch32.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_AARCH32_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_AARCH32_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__ARMEL__) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
    (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || \
    defined(BOOST_WINDOWS)
#define BOOST_ATOMIC_DETAIL_AARCH32_LITTLE_ENDIAN
#elif defined(__ARMEB__) || \
    defined(__ARM_BIG_ENDIAN) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
    (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__))
#define BOOST_ATOMIC_DETAIL_AARCH32_BIG_ENDIAN
#else
#error "Boost.Atomic: Failed to determine AArch32 endianness, the target platform is not supported. Please, report to the developers (patches are welcome)."
#endif

#define BOOST_ATOMIC_INT8_LOCK_FREE 2
#define BOOST_ATOMIC_INT16_LOCK_FREE 2
#define BOOST_ATOMIC_INT32_LOCK_FREE 2
#define BOOST_ATOMIC_INT64_LOCK_FREE 2
#define BOOST_ATOMIC_POINTER_LOCK_FREE 2

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_AARCH32_HPP_INCLUDED_
