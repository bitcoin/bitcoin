/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/gcc_atomic_memory_order_utils.hpp
 *
 * This header contains utilities for working with gcc atomic memory order constants.
 */

#ifndef BOOST_ATOMIC_DETAIL_GCC_ATOMIC_MEMORY_ORDER_UTILS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_GCC_ATOMIC_MEMORY_ORDER_UTILS_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

/*!
 * The function converts \c boost::memory_order values to the compiler-specific constants.
 *
 * NOTE: The intention is that the function is optimized away by the compiler, and the
 *       compiler-specific constants are passed to the intrinsics. Unfortunately, constexpr doesn't
 *       work in this case because the standard atomics interface require memory ordering
 *       constants to be passed as function arguments, at which point they stop being constexpr.
 *       However, it is crucial that the compiler sees constants and not runtime values,
 *       because otherwise it just ignores the ordering value and always uses seq_cst.
 *       This is the case with Intel C++ Compiler 14.0.3 (Composer XE 2013 SP1, update 3) and
 *       gcc 4.8.2. Intel Compiler issues a warning in this case:
 *
 *       warning #32013: Invalid memory order specified. Defaulting to seq_cst memory order.
 *
 *       while gcc acts silently.
 *
 *       To mitigate the problem ALL functions, including the atomic<> members must be
 *       declared with BOOST_FORCEINLINE. In this case the compilers are able to see that
 *       all functions are called with constant orderings and call intrinstcts properly.
 *
 *       Unfortunately, this still doesn't work in debug mode as the compiler doesn't
 *       propagate constants even when functions are marked with BOOST_FORCEINLINE. In this case
 *       all atomic operaions will be executed with seq_cst semantics.
 */
BOOST_FORCEINLINE BOOST_CONSTEXPR int convert_memory_order_to_gcc(memory_order order) BOOST_NOEXCEPT
{
    return (order == memory_order_relaxed ? __ATOMIC_RELAXED : (order == memory_order_consume ? __ATOMIC_CONSUME :
        (order == memory_order_acquire ? __ATOMIC_ACQUIRE : (order == memory_order_release ? __ATOMIC_RELEASE :
        (order == memory_order_acq_rel ? __ATOMIC_ACQ_REL : __ATOMIC_SEQ_CST)))));
}

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_GCC_ATOMIC_MEMORY_ORDER_UTILS_HPP_INCLUDED_
