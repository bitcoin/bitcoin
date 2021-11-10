/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2017 - 2018 Andrey Semashev
 */
/*!
 * \file   atomic/detail/extra_ops_gcc_arm.hpp
 *
 * This header contains implementation of the extra atomic operations for ARM.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_ARM_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/extra_operations_fwd.hpp>
#include <boost/atomic/detail/extra_ops_generic.hpp>
#include <boost/atomic/detail/ops_gcc_arm_common.hpp>
#include <boost/atomic/detail/gcc_arm_asm_common.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename Base >
struct extra_operations_gcc_arm_common :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fetch_negate(storage, order);
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fetch_complement(storage, order);
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
struct extra_operations_gcc_arm;

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB)

template< typename Base, bool Signed >
struct extra_operations_gcc_arm< Base, 1u, Signed > :
    public extra_operations_generic< Base, 1u, Signed >
{
    typedef extra_operations_generic< Base, 1u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;
    typedef typename storage_traits< 4u >::type extended_storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "rsb      %[result], %[original], #0\n\t"        // result = 0 - original
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "rsb      %[result], %[original], #0\n\t"        // result = 0 - original
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "add      %[result], %[original], %[value]\n\t"  // result = original + value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "sub      %[result], %[original], %[value]\n\t"  // result = original - value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "and      %[result], %[original], %[value]\n\t"  // result = original & value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "orr      %[result], %[original], %[value]\n\t"  // result = original | value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "eor      %[result], %[original], %[value]\n\t"  // result = original ^ value
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "mvn      %[result], %[original]\n\t"            // result = NOT original
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexb   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "mvn      %[result], %[original]\n\t"            // result = NOT original
            "strexb   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 1u, Signed, true > :
    public extra_operations_gcc_arm_common< extra_operations_gcc_arm< Base, 1u, Signed > >
{
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXB_STREXB)

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH)

template< typename Base, bool Signed >
struct extra_operations_gcc_arm< Base, 2u, Signed > :
    public extra_operations_generic< Base, 2u, Signed >
{
    typedef extra_operations_generic< Base, 2u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;
    typedef typename storage_traits< 4u >::type extended_storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "rsb      %[result], %[original], #0\n\t"        // result = 0 - original
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "rsb      %[result], %[original], #0\n\t"        // result = 0 - original
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "add      %[result], %[original], %[value]\n\t"  // result = original + value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "sub      %[result], %[original], %[value]\n\t"  // result = original - value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "and      %[result], %[original], %[value]\n\t"  // result = original & value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "orr      %[result], %[original], %[value]\n\t"  // result = original | value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "eor      %[result], %[original], %[value]\n\t"  // result = original ^ value
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "mvn      %[result], %[original]\n\t"            // result = NOT original
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(original);
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        extended_storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrexh   %[original], %[storage]\n\t"           // original = zero_extend(*(&storage))
            "mvn      %[result], %[original]\n\t"            // result = NOT original
            "strexh   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return static_cast< storage_type >(result);
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 2u, Signed, true > :
    public extra_operations_gcc_arm_common< extra_operations_gcc_arm< Base, 2u, Signed > >
{
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXH_STREXH)

template< typename Base, bool Signed >
struct extra_operations_gcc_arm< Base, 4u, Signed > :
    public extra_operations_generic< Base, 4u, Signed >
{
    typedef extra_operations_generic< Base, 4u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex    %[original], %[storage]\n\t"           // original = *(&storage)
            "rsb      %[result], %[original], #0\n\t"        // result = 0 - original
            "strex    %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex    %[original], %[storage]\n\t"           // original = *(&storage)
            "rsb      %[result], %[original], #0\n\t"        // result = 0 - original
            "strex    %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "add     %[result], %[original], %[value]\n\t"  // result = original + value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "sub     %[result], %[original], %[value]\n\t"  // result = original - value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "and     %[result], %[original], %[value]\n\t"  // result = original & value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "orr     %[result], %[original], %[value]\n\t"  // result = original | value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex   %[original], %[storage]\n\t"           // original = *(&storage)
            "eor     %[result], %[original], %[value]\n\t"  // result = original ^ value
            "strex   %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq     %[tmp], #0\n\t"                        // flags = tmp==0
            "bne     1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            : [value] "Ir" (v)              // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex    %[original], %[storage]\n\t"           // original = *(&storage)
            "mvn      %[result], %[original]\n\t"            // result = NOT original
            "strex    %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        uint32_t tmp;
        storage_type original, result;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%[tmp])
            "1:\n\t"
            "ldrex    %[original], %[storage]\n\t"           // original = *(&storage)
            "mvn      %[result], %[original]\n\t"            // result = NOT original
            "strex    %[tmp], %[result], %[storage]\n\t"     // *(&storage) = result, tmp = store failed
            "teq      %[tmp], #0\n\t"                        // flags = tmp==0
            "bne      1b\n\t"                                // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%[tmp])
            : [original] "=&r" (original),  // %0
              [result] "=&r" (result),      // %1
              [tmp] "=&l" (tmp),            // %2
              [storage] "+Q" (storage)      // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 4u, Signed, true > :
    public extra_operations_gcc_arm_common< extra_operations_gcc_arm< Base, 4u, Signed > >
{
};

#if defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD)

template< typename Base, bool Signed >
struct extra_operations_gcc_arm< Base, 8u, Signed > :
    public extra_operations_generic< Base, 8u, Signed >
{
    typedef extra_operations_generic< Base, 8u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "mvn     %2, %1\n\t"                      // result = NOT original
            "mvn     %H2, %H1\n\t"
            "adds   " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(2) ", #1\n\t" // result = result + 1
            "adc    " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(2) ", #0\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "mvn     %2, %1\n\t"                      // result = NOT original
            "mvn     %H2, %H1\n\t"
            "adds   " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(2) ", #1\n\t" // result = result + 1
            "adc    " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(2) ", #0\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "adds   " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(1) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(4) "\n\t" // result = original + value
            "adc    " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(1) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(4) "\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "subs   " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(1) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(4) "\n\t" // result = original - value
            "sbc    " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(2) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(1) ", " BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(4) "\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "and     %2, %1, %4\n\t"                  // result = original & value
            "and     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "orr     %2, %1, %4\n\t"                  // result = original | value
            "orr     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "eor     %2, %1, %4\n\t"                  // result = original ^ value
            "eor     %H2, %H1, %H4\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            : "r" (v)            // %4
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "mvn     %2, %1\n\t"                      // result = NOT original
            "mvn     %H2, %H1\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        core_arch_operations_gcc_arm_base::fence_before(order);
        storage_type original, result;
        uint32_t tmp;
        __asm__ __volatile__
        (
            BOOST_ATOMIC_DETAIL_ARM_ASM_START(%0)
            "1:\n\t"
            "ldrexd  %1, %H1, %3\n\t"                 // original = *(&storage)
            "mvn     %2, %1\n\t"                      // result = NOT original
            "mvn     %H2, %H1\n\t"
            "strexd  %0, %2, %H2, %3\n\t"             // *(&storage) = result, tmp = store failed
            "teq     %0, #0\n\t"                      // flags = tmp==0
            "bne     1b\n\t"                          // if (!flags.equal) goto retry
            BOOST_ATOMIC_DETAIL_ARM_ASM_END(%0)
            : BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(tmp), // %0
              "=&r" (original),  // %1
              "=&r" (result),    // %2
              "+Q" (storage)     // %3
            :
            : BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
        );
        core_arch_operations_gcc_arm_base::fence_after(order);
        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 8u, Signed, true > :
    public extra_operations_gcc_arm_common< extra_operations_gcc_arm< Base, 8u, Signed > >
{
};

#endif // defined(BOOST_ATOMIC_DETAIL_ARM_HAS_LDREXD_STREXD)

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_EXTRA_OPS_GCC_ARM_HPP_INCLUDED_
