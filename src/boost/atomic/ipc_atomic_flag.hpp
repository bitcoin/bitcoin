/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/ipc_atomic_flag.hpp
 *
 * This header contains definition of \c ipc_atomic_flag.
 */

#ifndef BOOST_ATOMIC_IPC_ATOMIC_FLAG_HPP_INCLUDED_
#define BOOST_ATOMIC_IPC_ATOMIC_FLAG_HPP_INCLUDED_

#include <boost/atomic/capabilities.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/atomic_flag_impl.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {

//! Atomic flag for inter-process communication
typedef atomics::detail::atomic_flag_impl< true > ipc_atomic_flag;

} // namespace atomics

using atomics::ipc_atomic_flag;

} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_IPC_ATOMIC_FLAG_HPP_INCLUDED_
