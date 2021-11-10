/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_arch_ops_msvc_x86.hpp
 *
 * This header contains implementation of the \c fence_arch_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_MSVC_X86_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_MSVC_X86_HPP_INCLUDED_

#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/interlocked.hpp>
#include <boost/atomic/detail/ops_msvc_common.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations for x86
struct fence_arch_operations_msvc_x86
{
    static BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
        {
            // See the comment in fence_ops_gcc_x86.hpp as to why we're not using mfence here.
            // We're not using __faststorefence() here because it generates an atomic operation
            // on [rsp]/[esp] location, which may alias valid data and cause false data dependency.
            boost::uint32_t dummy;
            BOOST_ATOMIC_INTERLOCKED_INCREMENT(&dummy);
        }
        else if (order != memory_order_relaxed)
        {
            BOOST_ATOMIC_DETAIL_COMPILER_BARRIER();
        }
    }

    static BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
    {
        if (order != memory_order_relaxed)
            BOOST_ATOMIC_DETAIL_COMPILER_BARRIER();
    }
};

typedef fence_arch_operations_msvc_x86 fence_arch_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_MSVC_X86_HPP_INCLUDED_
