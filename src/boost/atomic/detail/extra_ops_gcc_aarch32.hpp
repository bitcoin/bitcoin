/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/extra_ops_gcc_aarch32.hpp
 *
 * This header contains implementation of the extra atomic operations for AArch32.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_AARCH32_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_AARCH32_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/extra_operations_fwd.hpp>
#include <boost/atomic/detail/extra_ops_generic.hpp>
#include <boost/atomic/detail/ops_gcc_aarch32_common.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename Base >
struct extra_operations_gcc_aarch32_common :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    // Note: For opaque operations prefer operations returning the resulting values instead of the original values
    //       as these operations require less registers.
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
struct extra_operations_gcc_aarch32;

template< typename Base, bool Signed >
struct extra_operations_gcc_aarch32< Base, 1u, Signed > :
    public extra_operations_generic< Base, 1u, Signed >
{
    typedef extra_operations_generic< Base, 1u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "rsb %[result], %[original], #0\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "rsb %[result], #0\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "add %[result], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "sub %[result], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "and %[result], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "orr %[result], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "eor %[result], %[value]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[original], %[storage]\n\t"\
            "mvn %[result], %[original]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exb %[result], %[storage]\n\t"\
            "mvn %[result], %[result]\n\t"\
            "st" st_mo "exb %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 1u, Signed, true > :
    public extra_operations_gcc_aarch32_common< extra_operations_gcc_aarch32< Base, 1u, Signed > >
{
};


template< typename Base, bool Signed >
struct extra_operations_gcc_aarch32< Base, 2u, Signed > :
    public extra_operations_generic< Base, 2u, Signed >
{
    typedef extra_operations_generic< Base, 2u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "rsb %[result], %[original], #0\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "rsb %[result], #0\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "add %[result], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "sub %[result], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "and %[result], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "orr %[result], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "eor %[result], %[value]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[original], %[storage]\n\t"\
            "mvn %[result], %[original]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exh %[result], %[storage]\n\t"\
            "mvn %[result], %[result]\n\t"\
            "st" st_mo "exh %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 2u, Signed, true > :
    public extra_operations_gcc_aarch32_common< extra_operations_gcc_aarch32< Base, 2u, Signed > >
{
};

template< typename Base, bool Signed >
struct extra_operations_gcc_aarch32< Base, 4u, Signed > :
    public extra_operations_generic< Base, 4u, Signed >
{
    typedef extra_operations_generic< Base, 4u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "rsb %[result], %[original], #0\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "rsb %[result], #0\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "add %[result], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "sub %[result], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "and %[result], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "orr %[result], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "eor %[result], %[value]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : [value] "Ir" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[original], %[storage]\n\t"\
            "mvn %[result], %[original]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [original] "=&r" (original), [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "ex %[result], %[storage]\n\t"\
            "mvn %[result], %[result]\n\t"\
            "st" st_mo "ex %[tmp], %[result], %[storage]\n\t"\
            "teq %[tmp], #0\n\t"\
            "bne 1b\n\t"\
            : [result] "=&r" (result), [tmp] "=&r" (tmp), [storage] "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 4u, Signed, true > :
    public extra_operations_gcc_aarch32_common< extra_operations_gcc_aarch32< Base, 4u, Signed > >
{
};

template< typename Base, bool Signed >
struct extra_operations_gcc_aarch32< Base, 8u, Signed > :
    public extra_operations_generic< Base, 8u, Signed >
{
    typedef extra_operations_generic< Base, 8u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "mvn %3, %0\n\t"\
            "mvn %H3, %H0\n\t"\
            "adds " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(3) ", #1\n\t"\
            "adc " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(3) ", #0\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "mvn %0, %0\n\t"\
            "mvn %H0, %H0\n\t"\
            "adds " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(0) ", #1\n\t"\
            "adc " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(0) ", #0\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "adds " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(3) "\n\t"\
            "adc " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(3) "\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "subs " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_LO(3) "\n\t"\
            "sbc " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(0) ", " BOOST_ATOMIC_DETAIL_AARCH32_ASM_ARG_HI(3) "\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "and %0, %3\n\t"\
            "and %H0, %H3\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "orr %0, %3\n\t"\
            "orr %H0, %H3\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "eor %0, %3\n\t"\
            "eor %H0, %H3\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : "r" (v)\
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "mvn %3, %0\n\t"\
            "mvn %H3, %H0\n\t"\
            "st" st_mo "exd %1, %3, %H3, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (original), "=&r" (tmp), "+Q" (storage), "=&r" (result)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type result;
        uint32_t tmp;
#define BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN(ld_mo, st_mo)\
        __asm__ __volatile__\
        (\
            "1:\n\t"\
            "ld" ld_mo "exd %0, %H0, %2\n\t"\
            "mvn %0, %0\n\t"\
            "mvn %H0, %H0\n\t"\
            "st" st_mo "exd %1, %0, %H0, %2\n\t"\
            "teq %1, #0\n\t"\
            "bne 1b\n\t"\
            : "=&r" (result), "=&r" (tmp), "+Q" (storage)\
            : \
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "memory"\
        );

        BOOST_ATOMIC_DETAIL_AARCH32_MO_SWITCH(order)
#undef BOOST_ATOMIC_DETAIL_AARCH32_MO_INSN

        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 8u, Signed, true > :
    public extra_operations_gcc_aarch32_common< extra_operations_gcc_aarch32< Base, 8u, Signed > >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_AARCH32_HPP_INCLUDED_
