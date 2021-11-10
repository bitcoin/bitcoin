/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_ops_gcc_atomic.hpp
 *
 * This header contains implementation of the \c fence_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_OPS_GCC_ATOMIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_OPS_GCC_ATOMIC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/fence_arch_operations.hpp>
#include <boost/atomic/detail/gcc_atomic_memory_order_utils.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__INTEL_COMPILER)
// This is used to suppress warning #32013 described in gcc_atomic_memory_order_utils.hpp
// for Intel Compiler.
// In debug builds the compiler does not inline any functions, so basically
// every atomic function call results in this warning. I don't know any other
// way to selectively disable just this one warning.
#pragma system_header
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations based on gcc __atomic* intrinsics
struct fence_operations_gcc_atomic
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
    {
#if defined(__x86_64__) || defined(__i386__)
        if (order != memory_order_seq_cst)
        {
            __atomic_thread_fence(atomics::detail::convert_memory_order_to_gcc(order));
        }
        else
        {
            // gcc, clang, icc and probably other compilers generate mfence for a seq_cst fence,
            // while a dummy lock-prefixed instruction would be enough and faster. See the comment in fence_ops_gcc_x86.hpp.
            fence_arch_operations::thread_fence(order);
        }
#else
        __atomic_thread_fence(atomics::detail::convert_memory_order_to_gcc(order));
#endif
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
    {
        __atomic_signal_fence(atomics::detail::convert_memory_order_to_gcc(order));
    }
};

typedef fence_operations_gcc_atomic fence_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_OPS_GCC_ATOMIC_HPP_INCLUDED_
