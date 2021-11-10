/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_caps_freebsd_umtx.hpp
 *
 * This header defines waiting/notifying operations capabilities macros.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_CAPS_FREEBSD_UMTX_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_CAPS_FREEBSD_UMTX_HPP_INCLUDED_

#include <sys/umtx.h>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/int_sizes.hpp>
#include <boost/atomic/detail/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

// FreeBSD _umtx_op uses physical address to the atomic object as a key, which means it should support address-free operations.
// https://www.freebsd.org/cgi/man.cgi?query=_umtx_op&apropos=0&sektion=2&manpath=FreeBSD+11-current&format=html

#if (defined(UMTX_OP_WAIT_UINT) && BOOST_ATOMIC_DETAIL_SIZEOF_INT == 4) ||\
    (defined(UMTX_OP_WAIT) && BOOST_ATOMIC_DETAIL_SIZEOF_LONG == 4)
#define BOOST_ATOMIC_HAS_NATIVE_INT32_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT32_IPC_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE
#endif

#if defined(UMTX_OP_WAIT) && BOOST_ATOMIC_DETAIL_SIZEOF_LONG == 8
#define BOOST_ATOMIC_HAS_NATIVE_INT64_WAIT_NOTIFY BOOST_ATOMIC_INT64_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT64_IPC_WAIT_NOTIFY BOOST_ATOMIC_INT64_LOCK_FREE
#endif

#endif // BOOST_ATOMIC_DETAIL_WAIT_CAPS_FREEBSD_UMTX_HPP_INCLUDED_
