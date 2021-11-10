/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_operations.hpp
 *
 * This header defines waiting/notifying atomic operations, including the generic version.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPERATIONS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPERATIONS_HPP_INCLUDED_

#include <boost/atomic/detail/wait_ops_generic.hpp>
#include <boost/atomic/detail/wait_ops_emulated.hpp>

#if !defined(BOOST_ATOMIC_DETAIL_WAIT_BACKEND_GENERIC)
#include BOOST_ATOMIC_DETAIL_WAIT_BACKEND_HEADER(boost/atomic/detail/wait_ops_)
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPERATIONS_HPP_INCLUDED_
