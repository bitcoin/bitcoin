/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_arch_operations.hpp
 *
 * This header defines architecture-specific fence atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPERATIONS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPERATIONS_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>

#if defined(BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND_HEADER)
#include BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND_HEADER(boost/atomic/detail/fence_arch_ops_)
#else
#include <boost/atomic/detail/fence_operations_emulated.hpp>

namespace boost {
namespace atomics {
namespace detail {

typedef fence_operations_emulated fence_arch_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#endif // BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPERATIONS_HPP_INCLUDED_
