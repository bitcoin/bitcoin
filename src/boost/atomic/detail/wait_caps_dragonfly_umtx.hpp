/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_caps_dragonfly_umtx.hpp
 *
 * This header defines waiting/notifying operations capabilities macros.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_CAPS_DRAGONFLY_UMTX_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_CAPS_DRAGONFLY_UMTX_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

// DragonFly BSD umtx_sleep/umtx_wakeup use physical address to the atomic object as a key, which means it should support address-free operations.
// https://man.dragonflybsd.org/?command=umtx&section=2

#define BOOST_ATOMIC_HAS_NATIVE_INT32_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE
#define BOOST_ATOMIC_HAS_NATIVE_INT32_IPC_WAIT_NOTIFY BOOST_ATOMIC_INT32_LOCK_FREE

#endif // BOOST_ATOMIC_DETAIL_WAIT_CAPS_DRAGONFLY_UMTX_HPP_INCLUDED_
