/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/caps_gcc_sync.hpp
 *
 * This header defines feature capabilities macros
 */

#ifndef BOOST_ATOMIC_DETAIL_CAPS_GCC_SYNC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CAPS_GCC_SYNC_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define BOOST_ATOMIC_INT8_LOCK_FREE 2
#endif
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define BOOST_ATOMIC_INT16_LOCK_FREE 2
#endif
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define BOOST_ATOMIC_INT32_LOCK_FREE 2
#endif
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)\
    || defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define BOOST_ATOMIC_INT64_LOCK_FREE 2
#endif
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define BOOST_ATOMIC_INT128_LOCK_FREE 2
#endif

#define BOOST_ATOMIC_THREAD_FENCE 2
#define BOOST_ATOMIC_SIGNAL_FENCE 2

#endif // BOOST_ATOMIC_DETAIL_CAPS_GCC_SYNC_HPP_INCLUDED_
