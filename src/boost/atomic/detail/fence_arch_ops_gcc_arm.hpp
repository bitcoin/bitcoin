/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/fence_arch_ops_gcc_arm.hpp
 *
 * This header contains implementation of the \c fence_arch_operations struct.
 */

#ifndef BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_ARM_HPP_INCLUDED_

#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/gcc_arm_asm_common.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Fence operations for legacy ARM
struct fence_arch_operations_gcc_arm
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
        // A memory barrier is effected using a "co-processor 15" instruction,
        // though a separate assembler mnemonic is available for it in v7.

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_DMB)
        // Older binutils (supposedly, older than 2.21.1) didn't support symbolic or numeric arguments of the "dmb" instruction such as "ish" or "#11".
        // As a workaround we have to inject encoded bytes of the instruction. There are two encodings for the instruction: ARM and Thumb. See ARM Architecture Reference Manual, A8.8.43.
        // Since we cannot detect binutils version at compile time, we'll have to always use this hack.
        __asm__ __volatile__
        (
#if defined(__thumb2__)
            ".short 0xF3BF, 0x8F5B\n\t" // dmb ish
#else
            ".word 0xF57FF05B\n\t" // dmb ish
#endif
            :
            :
            : "memory"
        );
#else
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "mcr p15, 0, r0, c7, c10, 5\n\t"
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : "=&l" (tmp)
            :
            : "memory"
        );
#endif
    }
};

typedef fence_arch_operations_gcc_arm fence_arch_operations;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FENCE_ARCH_OPS_GCC_ARM_HPP_INCLUDED_
