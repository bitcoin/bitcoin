/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/core_arch_ops_gcc_aarch64.hpp
 *
 * This header contains implementation of the \c core_arch_operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_CORE_ARCH_OPS_GCC_AARCH64_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CORE_ARCH_OPS_GCC_AARCH64_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/core_arch_operations_fwd.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/ops_gcc_aarch64_common.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

struct core_arch_operations_gcc_aarch64_base
{
    static BOOST_CONSTEXPR_OR_CONST bool full_cas_based = false;
    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;
};

// Due to bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63359 we have to explicitly specify size of the registers
// to use in the asm blocks below. Use %w prefix for the 32-bit registers and %x for 64-bit ones.

// A note about compare_exchange implementations. Since failure_order must never include release semantics and
// must not be stronger than success_order, we can always use success_order to select instructions. Thus, when
// CAS fails, only the acquire semantics of success_order is applied, which may be stronger than failure_order.

template< bool Signed, bool Interprocess >
struct core_arch_operations< 1u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch64_base
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
                "stlrb %w[value], %[storage]\n\t"
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_RCPC)
            if (order == memory_order_consume || order == memory_order_acquire)
            {
                __asm__ __volatile__
                (
                    "ldaprb %w[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
            else
#endif
            {
                __asm__ __volatile__
                (
                    "ldarb %w[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "swp" ld_mo st_mo "b %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[value], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo "b %w[original], %w[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxtb %w[expected], %w[expected]\n\t"\
            "mov %w[success], #0\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "cmp %w[original], %w[expected]\n\t"\
            "b.ne 1f\n\t"\
            "st" st_mo "xrb %w[success], %w[desired], %[storage]\n\t"\
            "eor %w[success], %w[success], #1\n\t"\
            "1:\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "r" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo "b %w[original], %w[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxtb %w[expected], %w[expected]\n\t"\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "cmp %w[original], %w[expected]\n\t"\
            "b.ne 2f\n\t"\
            "st" st_mo "xrb %w[success], %w[desired], %[storage]\n\t"\
            "cbnz %w[success], 1b\n\t"\
            "2:\n\t"\
            "cset %w[success], eq\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "r" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo "b %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "add %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = -v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo "b %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );

#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "sub %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = ~v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldclr" ld_mo st_mo "b %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "and %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldset" ld_mo st_mo "b %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "orr %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldeor" ld_mo st_mo "b %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "eor %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

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
    public core_arch_operations_gcc_aarch64_base
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
                "stlrh %w[value], %[storage]\n\t"
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_RCPC)
            if (order == memory_order_consume || order == memory_order_acquire)
            {
                __asm__ __volatile__
                (
                    "ldaprh %w[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
            else
#endif
            {
                __asm__ __volatile__
                (
                    "ldarh %w[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "swp" ld_mo st_mo "h %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[value], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo "h %w[original], %w[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxth %w[expected], %w[expected]\n\t"\
            "mov %w[success], #0\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "cmp %w[original], %w[expected]\n\t"\
            "b.ne 1f\n\t"\
            "st" st_mo "xrh %w[success], %w[desired], %[storage]\n\t"\
            "eor %w[success], %w[success], #1\n\t"\
            "1:\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "r" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo "h %w[original], %w[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "uxth %w[expected], %w[expected]\n\t"\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "cmp %w[original], %w[expected]\n\t"\
            "b.ne 2f\n\t"\
            "st" st_mo "xrh %w[success], %w[desired], %[storage]\n\t"\
            "cbnz %w[success], 1b\n\t"\
            "2:\n\t"\
            "cset %w[success], eq\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "r" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo "h %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "add %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = -v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo "h %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );

#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "sub %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = ~v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldclr" ld_mo st_mo "h %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "and %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldset" ld_mo st_mo "h %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "orr %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldeor" ld_mo st_mo "h %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "eor %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

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
    public core_arch_operations_gcc_aarch64_base
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
                "stlr %w[value], %[storage]\n\t"
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_RCPC)
            if (order == memory_order_consume || order == memory_order_acquire)
            {
                __asm__ __volatile__
                (
                    "ldapr %w[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
            else
#endif
            {
                __asm__ __volatile__
                (
                    "ldar %w[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "swp" ld_mo st_mo " %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "st" st_mo "xr %w[tmp], %w[value], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo " %w[original], %w[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %w[success], #0\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "cmp %w[original], %w[expected]\n\t"\
            "b.ne 1f\n\t"\
            "st" st_mo "xr %w[success], %w[desired], %[storage]\n\t"\
            "eor %w[success], %w[success], #1\n\t"\
            "1:\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "Ir" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo " %w[original], %w[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "cmp %w[original], %w[expected]\n\t"\
            "b.ne 2f\n\t"\
            "st" st_mo "xr %w[success], %w[desired], %[storage]\n\t"\
            "cbnz %w[success], 1b\n\t"\
            "2:\n\t"\
            "cset %w[success], eq\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "Ir" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo " %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "add %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = -v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo " %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );

#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "sub %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = ~v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldclr" ld_mo st_mo " %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "and %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldset" ld_mo st_mo " %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "orr %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldeor" ld_mo st_mo " %w[value], %w[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "eor %w[result], %w[original], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Kr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

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
struct core_arch_operations< 8u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch64_base
{
    typedef typename storage_traits< 8u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 8u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 8u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
        {
            __asm__ __volatile__
            (
                "stlr %x[value], %[storage]\n\t"
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_RCPC)
            if (order == memory_order_consume || order == memory_order_acquire)
            {
                __asm__ __volatile__
                (
                    "ldapr %x[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
            else
#endif
            {
                __asm__ __volatile__
                (
                    "ldar %x[value], %[storage]\n\t"
                    : [value] "=r" (v)
                    : [storage] "Q" (storage)
                    : "memory"
                );
            }
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
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "swp" ld_mo st_mo " %x[value], %x[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "st" st_mo "xr %w[tmp], %x[value], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo " %x[original], %x[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %w[success], #0\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "cmp %x[original], %x[expected]\n\t"\
            "b.ne 1f\n\t"\
            "st" st_mo "xr %w[success], %x[desired], %[storage]\n\t"\
            "eor %w[success], %w[success], #1\n\t"\
            "1:\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "Ir" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        original = expected;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "cas" ld_mo st_mo " %x[original], %x[desired], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "+r" (original)\
            : [desired] "r" (desired)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
        bool success = original == expected;
#else
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "cmp %x[original], %x[expected]\n\t"\
            "b.ne 2f\n\t"\
            "st" st_mo "xr %w[success], %x[desired], %[storage]\n\t"\
            "cbnz %w[success], 1b\n\t"\
            "2:\n\t"\
            "cset %w[success], eq\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [desired] "r" (desired), [expected] "Ir" (expected)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#endif
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo " %x[value], %x[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "add %x[result], %x[original], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = -v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldadd" ld_mo st_mo " %x[value], %x[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );

#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "sub %x[result], %x[original], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Ir" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
        v = ~v;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldclr" ld_mo st_mo " %x[value], %x[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "and %x[result], %x[original], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Lr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldset" ld_mo st_mo " %x[value], %x[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "orr %x[result], %x[original], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Lr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
#if defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "ldeor" ld_mo st_mo " %x[value], %x[original], %[storage]\n\t"\
            : [storage] "+Q" (storage), [original] "=r" (original)\
            : [value] "r" (v)\
            : "memory"\
        );
#else
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "eor %x[result], %x[original], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : [value] "Lr" (v)\
            : "memory"\
        );
#endif

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

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

// For 128-bit atomic operations we always have to use ldxp+stxp (optionally, with acquire/release semantics), even in load and store operations.
// ARM Architecture Reference Manual Armv8, for Armv8-A architecture profile, Section B2.2.1 "Requirements for single-copy atomicity"
// specifies that ldxp does not guarantee an atomic load, and we have to perform ldxp+stxp loop to ensure that the loaded value
// is consistent with a previous atomic store.
//
// The ldxp and stxp instructions operate on pairs of registers, meaning that each load loads two integers from memory in
// successive address order, to the first and second registers in the pair, respectively, and store similarly stores two integers.
// The order of these integers does not depend on the active endianness mode (although the byte order in the integers themselves
// obviously does depend on endianness). This means we need to account for the current endianness mode ourselves, where it matters.
//
// Unlike AArch32/A32 or ARMv7, ldxp/stxp do not require adjacent even+odd registers in the pair and accept any two different
// registers. Still, it may be more preferable to select the adjacent registers as 128-bit objects are represented by two adjacent
// registers in the ABI. Unfortunately, clang 10 and probably older doesn't seem to support allocating register pairs in the asm blocks,
// like in ARMv7. For now we use a union to convert between a pair of 64-bit elements and 128-bit storage.

template< bool Signed, bool Interprocess >
struct core_arch_operations< 16u, Signed, Interprocess > :
    public core_arch_operations_gcc_aarch64_base
{
    typedef typename storage_traits< 16u >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = 16u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = 16u;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    // Union to convert between two 64-bit registers and a 128-bit storage
    union storage_union
    {
        storage_type as_storage;
        uint64_t as_uint64[2u];
    };

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        exchange(storage, v, order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_union v;
        uint32_t tmp;
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_consume) | static_cast< unsigned int >(memory_order_acquire))) != 0u)
        {
            __asm__ __volatile__
            (
                "1:\n\t"
                "ldaxp %x[value_0], %x[value_1], %[storage]\n\t"
                "stxp %w[tmp], %x[value_0], %x[value_1], %[storage]\n\t"
                "cbnz %w[tmp], 1b\n\t"
                : [tmp] "=&r" (tmp), [value_0] "=&r" (v.as_uint64[0u]), [value_1] "=&r" (v.as_uint64[1u])
                : [storage] "Q" (storage)
                : "memory"
            );
        }
        else
        {
            __asm__ __volatile__
            (
                "1:\n\t"
                "ldxp %x[value_0], %x[value_1], %[storage]\n\t"
                "stxp %w[tmp], %x[value_0], %x[value_1], %[storage]\n\t"
                "cbnz %w[tmp], 1b\n\t"
                : [tmp] "=&r" (tmp), [value_0] "=&r" (v.as_uint64[0u]), [value_1] "=&r" (v.as_uint64[1u])
                : [storage] "Q" (storage)
                : "memory"
            );
        }

        return v.as_storage;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union value = { v };
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "st" st_mo "xp %w[tmp], %x[value_0], %x[value_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u])\
            : [value_0] "r" (value.as_uint64[0u]), [value_1] "r" (value.as_uint64[1u])\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union e = { expected };
        storage_union d = { desired };
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "mov %w[success], #0\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "cmp %x[original_0], %x[expected_0]\n\t"\
            "ccmp %x[original_1], %x[expected_1], #0, eq\n\t"\
            "b.ne 1f\n\t"\
            "st" st_mo "xp %w[success], %x[desired_0], %x[desired_1], %[storage]\n\t"\
            "eor %w[success], %w[success], #1\n\t"\
            "1:\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u])\
            : [desired_0] "r" (d.as_uint64[0u]), [desired_1] "r" (d.as_uint64[1u]), [expected_0] "r" (e.as_uint64[0u]), [expected_1] "r" (e.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original.as_storage;
        return success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union e = { expected };
        storage_union d = { desired };
        bool success;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "cmp %x[original_0], %x[expected_0]\n\t"\
            "ccmp %x[original_1], %x[expected_1], #0, eq\n\t"\
            "b.ne 2f\n\t"\
            "st" st_mo "xp %w[success], %x[desired_0], %x[desired_1], %[storage]\n\t"\
            "cbnz %w[success], 1b\n\t"\
            "2:\n\t"\
            "cset %w[success], eq\n\t"\
            : [success] "=&r" (success), [storage] "+Q" (storage), [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u])\
            : [desired_0] "r" (d.as_uint64[0u]), [desired_1] "r" (d.as_uint64[1u]), [expected_0] "r" (e.as_uint64[0u]), [expected_1] "r" (e.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(success_order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        expected = original.as_storage;
        return success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union value = { v };
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "adds %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[original_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "]\n\t"\
            "adc %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[original_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "r" (value.as_uint64[0u]), [value_1] "r" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union value = { v };
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "subs %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[original_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "]\n\t"\
            "sbc %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[original_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "r" (value.as_uint64[0u]), [value_1] "r" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union value = { v };
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "and %x[result_0], %x[original_0], %x[value_0]\n\t"\
            "and %x[result_1], %x[original_1], %x[value_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "Lr" (value.as_uint64[0u]), [value_1] "Lr" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union value = { v };
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "orr %x[result_0], %x[original_0], %x[value_0]\n\t"\
            "orr %x[result_1], %x[original_1], %x[value_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "Lr" (value.as_uint64[0u]), [value_1] "Lr" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union value = { v };
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "eor %x[result_0], %x[original_0], %x[value_0]\n\t"\
            "eor %x[result_1], %x[original_1], %x[value_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "Lr" (value.as_uint64[0u]), [value_1] "Lr" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
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

#endif // BOOST_ATOMIC_DETAIL_CORE_ARCH_OPS_GCC_AARCH64_HPP_INCLUDED_
