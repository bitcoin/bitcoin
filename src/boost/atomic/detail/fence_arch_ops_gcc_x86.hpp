/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_arch_ops_gcc_x86.hpp
 *
 * This header contains implementation of the \c fence_arch_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_X86_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_X86_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations for x86
struct fence_arch_operations_gcc_x86
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
        {
            // We could generate mfence for a seq_cst fence here, but a dummy lock-prefixed instruction is enough
            // and is faster than mfence on most modern x86 CPUs (as of 2020).
            // Note that we want to apply the atomic operation on any location so that:
            // - It is not shared with other threads. A variable on the stack suits this well.
            // - It is likely in cache. Being close to the top of the stack fits this well.
            // - It does not alias existing data on the stack, so that we don't introduce a false data dependency.
            // See some performance data here: https://shipilev.net/blog/2014/on-the-fence-with-dependencies/
            // Unfortunately, to make tools like valgrind happy, we have to initialize the dummy, which is
            // otherwise not needed.
            unsigned char dummy = 0u;
            __asm__ __volatile__ ("lock; notb %0" : "+m" (dummy) : : "memory");
        }
        else if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_acquire) | static_cast< unsigned int >(memory_order_release))) != 0u)
        {
            __asm__ __volatile__ ("" ::: "memory");
        }
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
            __asm__ __volatile__ ("" ::: "memory");
    }
};

typedef fence_arch_operations_gcc_x86 fence_arch_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_X86_HPP_INCLUDED_
