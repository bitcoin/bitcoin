/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_arch_ops_msvc_arm.hpp
 *
 * This header contains implementation of the \c fence_arch_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_MSVC_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_MSVC_ARM_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/ops_msvc_common.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

extern "C" void __dmb(unsigned int);
#if defined(BOOST_MSVC)
#pragma intrinsic(__dmb)
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations for ARM
struct fence_arch_operations_msvc_arm
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
    {
        BOOST_ATOMIC_DETAIL_COMPILER_BARRIER();
        if (order != memory_order_relaxed)
            hardware_full_fence();
        BOOST_ATOMIC_DETAIL_COMPILER_BARRIER();
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
            BOOST_ATOMIC_DETAIL_COMPILER_BARRIER();
    }

    static BOOST_FORCEINLINE void hardware_full_fence() BOOST_NOEXCEPT
    {
        __dmb(0xB); // _ARM_BARRIER_ISH, see armintr.h from MSVC 11 and later
    }
};

typedef fence_arch_operations_msvc_arm fence_arch_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_MSVC_ARM_HPP_INCLUDED_
