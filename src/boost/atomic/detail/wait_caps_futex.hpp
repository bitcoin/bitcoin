/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_caps_futex.hpp
 *
 * This header defines waiting/notifying operations capabilities macros.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_CAPS_FUTEX_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_CAPS_FUTEX_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/futex.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_ATOMIC_DETAIL_HAS_FUTEX)
// futexes are always 32-bit and they always supported address-free operations
#define BOOST_ATOMIC_HAS_NATIVE_INT32_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT32_IPC_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE
#endif // defined(BOOST_ATOMIC_DETAIL_HAS_FUTEX)

#endif // BOOST_ATOMIC_DETAIL_WAIT_CAPS_FUTEX_HPP_INCLUDED_
