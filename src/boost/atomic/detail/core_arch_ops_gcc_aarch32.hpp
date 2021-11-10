/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/core_arch_ops_gcc_aarch32.hpp
 *
 * This header contains implementation of the \c core_arch_operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_CORE_ARCH_OPS_GCC_AARCH32_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CORE_ARCH_OPS_GCC_AARCH32_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/core_arch_operations_fwd.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/ops_gcc_aarch32_common.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

// ARMv8 (AArch32) instruction set is similar to ARMv7, but adds
// lda(b/h) and ldaex(b/h/d) instructions for acquire loads and
// stl(b/h) and stlex(b/h/d) instructions for release stores. This
// makes explicit memory fences unnecessary for implementation of
// the majority of the atomic operations.
//
// ARMv8 deprecates applying "it" hints to some instructions, including
// strex. It also deprecates "it" hints applying to more than one
// of the following conditional instructions. This means we have to
// use conditional jumps instead of making other instructions conditional.

struct core_arch_operations_gcc_aarch32_base
{
    static BOOST_CONSTEXPR_OR_CONST bool full_cas_based = false;
    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;
};

template< bool Signed, bool Interprocess >
struct core_arch_operations< 1u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch32_base
{
    typedef typename storage_traits< 1u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 1u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 1u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
        {
            __asm__ __volatile__
            (
                "stlb %[value], %[storage]\n\t"
                : [storage] "=Q" (storage)
                : [value] "r" (v)
                : "memory"
            );
        }
        else
        {
            storage = v;
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v;
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_consume) | static_cast< unsigned int >(memory_order_acquire))) != 0u)
        {
            __asm__ __volatile__
            (
                "ldab %[value], %[storage]\n\t"
                : [value] "=r" (v)
                : [storage] "Q" (storage)
                : "memory"
            );
        }
        else
        {
            v = storage;
        }

        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "st" st_mo "exb %[tmp], %[value], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [tmp] "=&r" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)\
            : [value] "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxtb %[expected], %[expected]\n\t"\
            "mov %[success], #0\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "cmp %[original], %[expected]\n\t"\
            "bne 1f\n\t"\
            "st" st_mo "exb %[success], %[desired], %[storage]\n\t"\
            "eor %[success], %[success], #1\n\t"\
            "1:\n\t"\
            : [original] "=&r" (original), [success] "=&r" (success), [storage] "+Q" (storage)\
            : [expected] "r" (expected), [desired] "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxtb %[expected], %[expected]\n\t"\
            "mov %[success], #0\n\t"\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "cmp %[original], %[expected]\n\t"\
            "bne 2f\n\t"\
            "st" st_mo "exb %[success], %[desired], %[storage]\n\t"\
            "eors %[success], %[success], #1\n\t"\
            "beq 1b\n\t"\
            "2:\n\t"\
            : [original] "=&r" (original), [success] "=&r" (success), [storage] "+Q" (storage)\
            : [expected] "r" (expected), [desired] "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "add %[result], %[original], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "sub %[result], %[original], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "and %[result], %[original], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "orr %[result], %[original], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "eor %[result], %[original], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

template< bool Signed, bool Interprocess >
struct core_arch_operations< 2u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch32_base
{
    typedef typename storage_traits< 2u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 2u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 2u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
        {
            __asm__ __volatile__
            (
                "stlh %[value], %[storage]\n\t"
                : [storage] "=Q" (storage)
                : [value] "r" (v)
                : "memory"
            );
        }
        else
        {
            storage = v;
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v;
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_consume) | static_cast< unsigned int >(memory_order_acquire))) != 0u)
        {
            __asm__ __volatile__
            (
                "ldah %[value], %[storage]\n\t"
                : [value] "=r" (v)
                : [storage] "Q" (storage)
                : "memory"
            );
        }
        else
        {
            v = storage;
        }

        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "st" st_mo "exh %[tmp], %[value], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [tmp] "=&r" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)\
            : [value] "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxth %[expected], %[expected]\n\t"\
            "mov %[success], #0\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "cmp %[original], %[expected]\n\t"\
            "bne 1f\n\t"\
            "st" st_mo "exh %[success], %[desired], %[storage]\n\t"\
            "eor %[success], %[success], #1\n\t"\
            "1:\n\t"\
            : [original] "=&r" (original), [success] "=&r" (success), [storage] "+Q" (storage)\
            : [expected] "r" (expected), [desired] "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxth %[expected], %[expected]\n\t"\
            "mov %[success], #0\n\t"\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "cmp %[original], %[expected]\n\t"\
            "bne 2f\n\t"\
            "st" st_mo "exh %[success], %[desired], %[storage]\n\t"\
            "eors %[success], %[success], #1\n\t"\
            "beq 1b\n\t"\
            "2:\n\t"\
            : [original] "=&r" (original), [success] "=&r" (success), [storage] "+Q" (storage)\
            : [expected] "r" (expected), [desired] "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "add %[result], %[original], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "sub %[result], %[original], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "and %[result], %[original], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "orr %[result], %[original], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "eor %[result], %[original], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

template< bool Signed, bool Interprocess >
struct core_arch_operations< 4u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch32_base
{
    typedef typename storage_traits< 4u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 4u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 4u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
        {
            __asm__ __volatile__
            (
                "stl %[value], %[storage]\n\t"
                : [storage] "=Q" (storage)
                : [value] "r" (v)
                : "memory"
            );
        }
        else
        {
            storage = v;
        }
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v;
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_consume) | static_cast< unsigned int >(memory_order_acquire))) != 0u)
        {
            __asm__ __volatile__
            (
                "lda %[value], %[storage]\n\t"
                : [value] "=r" (v)
                : [storage] "Q" (storage)
                : "memory"
            );
        }
        else
        {
            v = storage;
        }

        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "st" st_mo "ex %[tmp], %[value], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [tmp] "=&r" (tmp), [original] "=&r" (original), [storage] "+Q" (storage)\
            : [value] "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %[success], #0\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "cmp %[original], %[expected]\n\t"\
            "bne 1f\n\t"\
            "st" st_mo "ex %[success], %[desired], %[storage]\n\t"\
            "eor %[success], %[success], #1\n\t"\
            "1:\n\t"\
            : [original] "=&r" (original), [success] "=&r" (success), [storage] "+Q" (storage)\
            : [expected] "Ir" (expected), [desired] "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %[success], #0\n\t"\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "cmp %[original], %[expected]\n\t"\
            "bne 2f\n\t"\
            "st" st_mo "ex %[success], %[desired], %[storage]\n\t"\
            "eors %[success], %[success], #1\n\t"\
            "beq 1b\n\t"\
            "2:\n\t"\
            : [original] "=&r" (original), [success] "=&r" (success), [storage] "+Q" (storage)\
            : [expected] "Ir" (expected), [desired] "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "add %[result], %[original], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "sub %[result], %[original], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "and %[result], %[original], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "orr %[result], %[original], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "eor %[result], %[original], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};


// Unlike 32-bit operations, for 64-bit loads and stores we must use ldrexd/strexd.
// Other instructions result in a non-atomic sequence of 32-bit or more fine-grained accesses.
// See "ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile", Section E2.2 "Atomicity in the ARM architecture".
// Section E2.3.7 "Memory barriers", subsection "Load-Acquire, Store-Release" extends atomicity guarantees given for ldrexd/strexd
// to the new ldaexd/stlexd instructions with acquire/release semantics.
//
// In the asm blocks below we have to use 32-bit register pairs to compose 64-bit values. In order to pass the 64-bit operands
// to/from asm blocks, we use undocumented gcc feature: the lower half (Rt) of the operand is accessible normally, via the numbered
// placeholder (e.g. %0), and the upper half (Rt2) - via the same placeholder with an 'H' after the '%' sign (e.g. %H0).
// See: http://hardwarebug.org/2010/07/06/arm-inline-asm-secrets/
//
// The ldrexd and strexd instructions operate on pairs of registers, meaning that each load loads two integers from memory in
// successive address order, to the first and second registers in the pair, respectively, and store similarly stores two integers.
// The order of these integers does not depend on the active endianness mode (although the byte order in the integers themselves
// obviously does depend on endianness). This means we need to account for the current endianness mode ourselves, where it matters.

template< bool Signed, bool Interprocess >
struct core_arch_operations< 8u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch32_base
{
    typedef typename storage_traits< 8u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 8u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 8u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        exchange(storage, v, order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_consume) | static_cast< unsigned int >(memory_order_acquire))) != 0u)
        {
            __asm__ __volatile__
            (
                "ldaexd %0, %H0, %1\n\t"
                : "=&r" (original)   // %0
                : "Q" (storage)      // %1
            );
        }
        else
        {
            __asm__ __volatile__
            (
                "ldrexd %0, %H0, %1\n\t"
                : "=&r" (original)   // %0
                : "Q" (storage)      // %1
            );
        }

        return original;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %1, %H1, %2\n\t"\
            "st" st_mo "exd %0, %3, %H3, %2\n\t"\
            "teq %0, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (tmp), "=&r" (original), "+Q" (storage)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %1, #0\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "cmp %0, %3\n\t"\
            "it eq\n\t"\
            "cmpeq %H0, %H3\n\t"\
            "bne 1f\n\t"\
            "st" st_mo "exd %1, %4, %H4, %2\n\t"\
            "eor %1, %1, #1\n\t"\
            "1:\n\t"\
            : "=&r" (original), "=&r" (success), "+Q" (storage)\
            : "r" (expected), "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;

        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
        bool success;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %1, #0\n\t"\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "cmp %0, %3\n\t"\
            "it eq\n\t"\
            "cmpeq %H0, %H3\n\t"\
            "bne 2f\n\t"\
            "st" st_mo "exd %1, %4, %H4, %2\n\t"\
            "eors %1, %1, #1\n\t"\
            "beq 1b\n\t"\
            "2:\n\t"\
            : "=&r" (original), "=&r" (success), "+Q" (storage)\
            : "r" (expected), "r" (desired)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        expected = original;

        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "adds " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(3) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(4) "\n\t"\
            "adc " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(3) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(4) "\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "subs " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(3) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(4) "\n\t"\
            "sbc " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(3) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(4) "\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "and %3, %0, %4\n\t"\
            "and %H3, %H0, %H4\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "orr %3, %0, %4\n\t"\
            "orr %H3, %H0, %H4\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "eor %3, %0, %4\n\t"\
            "eor %H3, %H0, %H4\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, (storage_type)0, order);
    }
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_CORE_ARCH_OPS_GCC_AARCH32_HPP_INCLUDED_
