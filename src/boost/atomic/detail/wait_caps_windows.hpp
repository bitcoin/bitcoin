/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_caps_windows.hpp
 *
 * This header defines waiting/notifying operations capabilities macros.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_CAPS_WINDOWS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_CAPS_WINDOWS_HPP_INCLUDED_

#include <boost/winapi/config.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

// MSDN says WaitOnAddress, WakeByAddressSingle and WakeByAddressAll only support notifications between threads of the same process, so no address-free operations.
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddressall

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8 && (BOOST_WINAPI_PARTITION_APP || BOOST_WINAPI_PARTITION_SYSTEM)

#define BOOST_ATOMIC_HAS_NATIVE_INT8_WAIT_NOTIFY BOOST_ATOMIC_INT8_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT16_WAIT_NOTIFY BOOST_ATOMIC_INT16_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT32_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT64_WAIT_NOTIFY BOOST_ATOMIC_INT64_LOCK_FREE

#else // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8 && (BOOST_WINAPI_PARTITION_APP || BOOST_WINAPI_PARTITION_SYSTEM)

// Since will detect availability of WaitOnAddress etc. at run time, we have to define capability macros to 1 instead of 2
#if BOOST_ATOMIC_INT8_LOCK_FREE > 0
#define BOOST_ATOMIC_HAS_NATIVE_INT8_WAIT_NOTIFY 1
#endif
#if BOOST_ATOMIC_INT16_LOCK_FREE > 0
#define BOOST_ATOMIC_HAS_NATIVE_INT16_WAIT_NOTIFY 1
#endif
#if BOOST_ATOMIC_INT32_LOCK_FREE > 0
#define BOOST_ATOMIC_HAS_NATIVE_INT32_WAIT_NOTIFY 1
#endif
#if BOOST_ATOMIC_INT64_LOCK_FREE > 0
#define BOOST_ATOMIC_HAS_NATIVE_INT64_WAIT_NOTIFY 1
#endif

#endif // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8 && (BOOST_WINAPI_PARTITION_APP || BOOST_WINAPI_PARTITION_SYSTEM)

#endif // BOOST_ATOMIC_DETAIL_WAIT_CAPS_WINDOWS_HPP_INCLUDED_
