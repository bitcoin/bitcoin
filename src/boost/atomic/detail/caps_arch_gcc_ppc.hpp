/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_arch_gcc_ppc.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_PPC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_PPC_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__POWERPC__) || defined(__PPC__)

#if defined(_ARCH_PWR8)
// Power8 and later architectures have 8 and 16-bit instructions
#define BOOST_ATOMIC_DETAIL_PPC_HAS_LBARX_STBCX
#define BOOST_ATOMIC_DETAIL_PPC_HAS_LHARX_STHCX
#endif

#if defined(__powerpc64__) || defined(__PPC64__)
// Power7 and later architectures in 64-bit mode have 64-bit instructions
#define BOOST_ATOMIC_DETAIL_PPC_HAS_LDARX_STDCX
#if defined(_ARCH_PWR8)
// Power8 also has 128-bit instructions
#define BOOST_ATOMIC_DETAIL_PPC_HAS_LQARX_STQCX
#endif
#endif

#endif // defined(__POWERPC__) || defined(__PPC__)

#define BOOST_ATOMIC_INT8_LOCK_FREE 2
#define BOOST_ATOMIC_INT16_LOCK_FREE 2
#define BOOST_ATOMIC_INT32_LOCK_FREE 2
#if defined(BOOST_ATOMIC_DETAIL_PPC_HAS_LDARX_STDCX)
#define BOOST_ATOMIC_INT64_LOCK_FREE 2
#endif
#define BOOST_ATOMIC_POINTER_LOCK_FREE 2

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_ARCH_GCC_PPC_HPP_INCLUDED_
