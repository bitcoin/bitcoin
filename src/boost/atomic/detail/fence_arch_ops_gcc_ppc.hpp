/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_arch_ops_gcc_ppc.hpp
 *
 * This header contains implementation of the \c fence_arch_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_PPC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_PPC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations for PowerPC
struct fence_arch_operations_gcc_ppc
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
        {
#if defined(__powerpc64__) || defined(__PPC64__)
            if (order != memory_order_seq_cst)
                __asm__ __volatile__ ("lwsync" ::: "memory");
            else
                __asm__ __volatile__ ("sync" ::: "memory");
#else
            __asm__ __volatile__ ("sync" ::: "memory");
#endif
        }
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
        {
#if defined(__ibmxl__) || defined(__IBMCPP__)
            __fence();
#else
            __asm__ __volatile__ ("" ::: "memory");
#endif
        }
    }
};

typedef fence_arch_operations_gcc_ppc fence_arch_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_PPC_HPP_INCLUDED_
