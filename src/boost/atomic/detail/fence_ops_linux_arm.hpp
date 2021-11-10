/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009, 2011 Helge Bahmann
 * Copyright (c) 2009 Phil Endecott
 * Copyright (c) 2013 Tim Blechmann
 * Linux-specific code by Phil Endecott
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_ops_linux_arm.hpp
 *
 * This header contains implementation of the \c fence_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_OPS_LINUX_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_OPS_LINUX_ARM_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations based on Linux-specific system routines
struct fence_operations_linux_arm
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
            hardware_full_fence();
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
            __asm__ __volatile__ ("" ::: "memory");
    }

    static BOOST_FORCEINLINE void hardware_full_fence() BOOST_NOEXCEPT
    {
        // See the comment in core_ops_linux_arm.hpp regarding the function pointer below
        typedef void (*kernel_dmb_t)(void);
        ((kernel_dmb_t)0xffff0fa0)();
    }
};

typedef fence_operations_linux_arm fence_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_OPS_LINUX_ARM_HPP_INCLUDED_
