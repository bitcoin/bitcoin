/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/extra_ops_gcc_aarch64.hpp
 *
 * This header contains implementation of the extra atomic operations for AArch64.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_AARCH64_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_AARCH64_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/extra_operations_fwd.hpp>
#include <boost/atomic/detail/extra_ops_generic.hpp>
#include <boost/atomic/detail/ops_gcc_aarch64_common.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename Base >
struct extra_operations_gcc_aarch64_common :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    // Note: For opaque operations prefer operations returning the resulting values instead of the original values
    //       as these operations require less registers. That is unless LSE is available, in which case
    //       it is better to use the dedicated atomic instructions. The LSE check is done in the base_type,
    //       where needed (e.g. for 128-bit operations there are no LSE instructions).
    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::negate(storage, order);
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::bitwise_complement(storage, order);
    }

    static BOOST_FORCEINLINE void opaque_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::add(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::sub(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::bitwise_and(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::bitwise_or(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::bitwise_xor(storage, v, order);
    }

    static BOOST_FORCEINLINE bool negate_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::negate(storage, order);
    }

    static BOOST_FORCEINLINE bool add_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::add(storage, v, order);
    }

    static BOOST_FORCEINLINE bool sub_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::sub(storage, v, order);
    }

    static BOOST_FORCEINLINE bool and_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::bitwise_and(storage, v, order);
    }

    static BOOST_FORCEINLINE bool or_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::bitwise_or(storage, v, order);
    }

    static BOOST_FORCEINLINE bool xor_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::bitwise_xor(storage, v, order);
    }

    static BOOST_FORCEINLINE bool complement_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!base_type::bitwise_complement(storage, order);
    }
};

template< typename Base, std::size_t Size, bool Signed >
struct extra_operations_gcc_aarch64;

template< typename Base, bool Signed >
struct extra_operations_gcc_aarch64< Base, 1u, Signed > :
    public extra_operations_generic< Base, 1u, Signed >
{
    typedef extra_operations_generic< Base, 1u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "neg %w[result], %w[original]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "neg %w[result], %w[result]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#if !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "add %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "sub %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "and %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "orr %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "eor %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[original], %[storage]\n\t"\
            "mvn %w[result], %w[original]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrb %w[result], %[storage]\n\t"\
            "mvn %w[result], %w[result]\n\t"\
            "st" st_mo "xrb %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#endif // !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
};

template< typename Base, bool Signed >
struct extra_operations< Base, 1u, Signed, true > :
    public extra_operations_gcc_aarch64_common< extra_operations_gcc_aarch64< Base, 1u, Signed > >
{
};


template< typename Base, bool Signed >
struct extra_operations_gcc_aarch64< Base, 2u, Signed > :
    public extra_operations_generic< Base, 2u, Signed >
{
    typedef extra_operations_generic< Base, 2u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "neg %w[result], %w[original]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "neg %w[result], %w[result]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#if !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "add %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "sub %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "and %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "orr %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "eor %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[original], %[storage]\n\t"\
            "mvn %w[result], %w[original]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xrh %w[result], %[storage]\n\t"\
            "mvn %w[result], %w[result]\n\t"\
            "st" st_mo "xrh %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#endif // !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
};

template< typename Base, bool Signed >
struct extra_operations< Base, 2u, Signed, true > :
    public extra_operations_gcc_aarch64_common< extra_operations_gcc_aarch64< Base, 2u, Signed > >
{
};


template< typename Base, bool Signed >
struct extra_operations_gcc_aarch64< Base, 4u, Signed > :
    public extra_operations_generic< Base, 4u, Signed >
{
    typedef extra_operations_generic< Base, 4u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "neg %w[result], %w[original]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "neg %w[result], %w[result]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#if !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "add %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "sub %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "and %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "orr %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "eor %w[result], %w[result], %w[value]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Kr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[original], %[storage]\n\t"\
            "mvn %w[result], %w[original]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %w[result], %[storage]\n\t"\
            "mvn %w[result], %w[result]\n\t"\
            "st" st_mo "xr %w[tmp], %w[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#endif // !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
};

template< typename Base, bool Signed >
struct extra_operations< Base, 4u, Signed, true > :
    public extra_operations_gcc_aarch64_common< extra_operations_gcc_aarch64< Base, 4u, Signed > >
{
};


template< typename Base, bool Signed >
struct extra_operations_gcc_aarch64< Base, 8u, Signed > :
    public extra_operations_generic< Base, 8u, Signed >
{
    typedef extra_operations_generic< Base, 8u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "neg %x[result], %x[original]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "neg %x[result], %x[result]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#if !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "add %x[result], %x[result], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "sub %x[result], %x[result], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Ir" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "and %x[result], %x[result], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Lr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "orr %x[result], %x[result], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Lr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "eor %x[result], %x[result], %x[value]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : [value] "Lr" (v)\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[original], %[storage]\n\t"\
            "mvn %x[result], %x[original]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [result] "=&r" (result), [storage] "+Q" (storage), [original] "=&r" (original)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xr %x[result], %[storage]\n\t"\
            "mvn %x[result], %x[result]\n\t"\
            "st" st_mo "xr %w[tmp], %x[result], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage), [result] "=&r" (result)\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result;
    }

#endif // !defined(BOOST_ATOMIC_DETAIL_AARCH64_HAS_LSE)
};

template< typename Base, bool Signed >
struct extra_operations< Base, 8u, Signed, true > :
    public extra_operations_gcc_aarch64_common< extra_operations_gcc_aarch64< Base, 8u, Signed > >
{
};


template< typename Base, bool Signed >
struct extra_operations_gcc_aarch64< Base, 16u, Signed > :
    public extra_operations_generic< Base, 16u, Signed >
{
    typedef extra_operations_generic< Base, 16u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;
    typedef typename base_type::storage_union storage_union;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "mvn %x[result_0], %x[original_0]\n\t"\
            "mvn %x[result_1], %x[original_1]\n\t"\
            "adds %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], #1\n\t"\
            "adc %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], xzr\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "mvn %x[result_0], %x[result_0]\n\t"\
            "mvn %x[result_1], %x[result_1]\n\t"\
            "adds %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], #1\n\t"\
            "adc %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], xzr\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        storage_union value = { v };
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "adds %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "]\n\t"\
            "adc %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "r" (value.as_uint64[0u]), [value_1] "r" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        storage_union value = { v };
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "subs %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "]\n\t"\
            "sbc %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[result_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "], %x[value_" BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "r" (value.as_uint64[0u]), [value_1] "r" (value.as_uint64[1u])\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        storage_union value = { v };
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "and %x[result_0], %x[result_0], %x[value_0]\n\t"\
            "and %x[result_1], %x[result_1], %x[value_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "Lr" (value.as_uint64[0u]), [value_1] "Lr" (value.as_uint64[1u])\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        storage_union value = { v };
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "orr %x[result_0], %x[result_0], %x[value_0]\n\t"\
            "orr %x[result_1], %x[result_1], %x[value_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "Lr" (value.as_uint64[0u]), [value_1] "Lr" (value.as_uint64[1u])\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        storage_union value = { v };
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "eor %x[result_0], %x[result_0], %x[value_0]\n\t"\
            "eor %x[result_1], %x[result_1], %x[value_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : [value_0] "Lr" (value.as_uint64[0u]), [value_1] "Lr" (value.as_uint64[1u])\
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_union original;
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[original_0], %x[original_1], %[storage]\n\t"\
            "mvn %x[result_0], %x[original_0]\n\t"\
            "mvn %x[result_1], %x[original_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [original_0] "=&r" (original.as_uint64[0u]), [original_1] "=&r" (original.as_uint64[1u]),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return original.as_storage;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_union result;
        uint32_t tmp;

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "xp %x[result_0], %x[result_1], %[storage]\n\t"\
            "mvn %x[result_0], %x[result_0]\n\t"\
            "mvn %x[result_1], %x[result_1]\n\t"\
            "st" st_mo "xp %w[tmp], %x[result_0], %x[result_1], %[storage]\n\t"\
            "cbnz %w[tmp], 1b\n\t"\
            : [tmp] "=&r" (tmp), [storage] "+Q" (storage),\
              [result_0] "=&r" (result.as_uint64[0u]), [result_1] "=&r" (result.as_uint64[1u])\
            : \
            : "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN

        return result.as_storage;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 16u, Signed, true > :
    public extra_operations_gcc_aarch64_common< extra_operations_gcc_aarch64< Base, 16u, Signed > >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_AARCH64_HPP_INCLUDED_
