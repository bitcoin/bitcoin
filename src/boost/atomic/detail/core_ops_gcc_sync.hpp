/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/core_ops_gcc_sync.hpp
 *
 * This header contains implementation of the \c core_operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_CORE_OPS_GCC_SYNC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CORE_OPS_GCC_SYNC_HPP_INCLUDED_

#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/core_operations_fwd.hpp>
#include <boost/atomic/detail/extending_cas_based_arithmetic.hpp>
#include <boost/atomic/detail/type_traits/integral_constant.hpp>
#include <boost/atomic/detail/capabilities.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

struct core_operations_gcc_sync_base
{
    static BOOST_CONSTEXPR_OR_CONST bool full_cas_based = false;
    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = true;

    static BOOST_FORCEINLINE void fence_before_store(memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
            __sync_synchronize();
    }

    static BOOST_FORCEINLINE void fence_after_store(memory_order order) BOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            __sync_synchronize();
    }

    static BOOST_FORCEINLINE void fence_after_load(memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & (static_cast< unsigned int >(memory_order_acquire) | static_cast< unsigned int >(memory_order_consume))) != 0u)
            __sync_synchronize();
    }
};

template< std::size_t Size, bool Signed, bool Interprocess >
struct core_operations_gcc_sync :
    public core_operations_gcc_sync_base
{
    typedef typename storage_traits< Size >::type storage_type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_size = Size;
    static BOOST_CONSTEXPR_OR_CONST std::size_t storage_alignment = storage_traits< storage_size >::alignment;
    static BOOST_CONSTEXPR_OR_CONST bool is_signed = Signed;
    static BOOST_CONSTEXPR_OR_CONST bool is_interprocess = Interprocess;

    // In general, we cannot guarantee atomicity of plain loads and stores of anything larger than a single byte on
    // an arbitrary CPU architecture. However, all modern architectures seem to guarantee atomic loads and stores of
    // suitably aligned objects of up to a pointer size. For larger objects we should probably use intrinsics to guarantee
    // atomicity. If there appears an architecture where this doesn't hold, this threshold needs to be updated (patches are welcome).
    typedef atomics::detail::integral_constant< bool, storage_size <= sizeof(void*) > plain_stores_loads_are_atomic;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, v, order, plain_stores_loads_are_atomic());
    }

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order, atomics::detail::true_type) BOOST_NOEXCEPT
    {
        fence_before_store(order);
        storage = v;
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order, atomics::detail::false_type) BOOST_NOEXCEPT
    {
        exchange(storage, v, order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return load(storage, order, plain_stores_loads_are_atomic());
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order, atomics::detail::true_type) BOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after_load(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order, atomics::detail::false_type) BOOST_NOEXCEPT
    {
        // Note: don't use fetch_add or other arithmetics here since storage_type may not be an arithmetic type.
        storage_type expected = storage_type();
        storage_type desired = expected;
        // We don't care if CAS succeeds or not. If it does, it will just write the same value there was before.
        return __sync_val_compare_and_swap(const_cast< storage_type volatile* >(&storage), expected, desired);
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_add(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_sub(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        // GCC docs mention that not all architectures may support full exchange semantics for this intrinsic. However, GCC's implementation of
        // std::atomic<> uses this intrinsic unconditionally. We do so as well. In case if some architectures actually don't support this, we can always
        // add a check here and fall back to a CAS loop.
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
            __sync_synchronize();
        return __sync_lock_test_and_set(&storage, v);
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order, memory_order) BOOST_NOEXCEPT
    {
        storage_type expected2 = expected;
        storage_type old_val = __sync_val_compare_and_swap(&storage, expected2, desired);

        if (old_val == expected2)
        {
            return true;
        }
        else
        {
            expected = old_val;
            return false;
        }
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_and(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_or(&storage, v);
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order) BOOST_NOEXCEPT
    {
        return __sync_fetch_and_xor(&storage, v);
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        if ((static_cast< unsigned int >(order) & static_cast< unsigned int >(memory_order_release)) != 0u)
            __sync_synchronize();
        return !!__sync_lock_test_and_set(&storage, 1);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        __sync_lock_release(&storage);
        if (order == memory_order_seq_cst)
            __sync_synchronize();
    }
};

#if BOOST_ATOMIC_INT8_LOCK_FREE > 0
template< bool Signed, bool Interprocess >
struct core_operations< 1u, Signed, Interprocess > :
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1)
    public core_operations_gcc_sync< 1u, Signed, Interprocess >
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 2u, Signed, Interprocess >, 1u, Signed >
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 4u, Signed, Interprocess >, 1u, Signed >
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 8u, Signed, Interprocess >, 1u, Signed >
#else
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 16u, Signed, Interprocess >, 1u, Signed >
#endif
{
};
#endif

#if BOOST_ATOMIC_INT16_LOCK_FREE > 0
template< bool Signed, bool Interprocess >
struct core_operations< 2u, Signed, Interprocess > :
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)
    public core_operations_gcc_sync< 2u, Signed, Interprocess >
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 4u, Signed, Interprocess >, 2u, Signed >
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 8u, Signed, Interprocess >, 2u, Signed >
#else
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 16u, Signed, Interprocess >, 2u, Signed >
#endif
{
};
#endif

#if BOOST_ATOMIC_INT32_LOCK_FREE > 0
template< bool Signed, bool Interprocess >
struct core_operations< 4u, Signed, Interprocess > :
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    public core_operations_gcc_sync< 4u, Signed, Interprocess >
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 8u, Signed, Interprocess >, 4u, Signed >
#else
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 16u, Signed, Interprocess >, 4u, Signed >
#endif
{
};
#endif

#if BOOST_ATOMIC_INT64_LOCK_FREE > 0
template< bool Signed, bool Interprocess >
struct core_operations< 8u, Signed, Interprocess > :
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    public core_operations_gcc_sync< 8u, Signed, Interprocess >
#else
    public extending_cas_based_arithmetic< core_operations_gcc_sync< 16u, Signed, Interprocess >, 8u, Signed >
#endif
{
};
#endif

#if BOOST_ATOMIC_INT128_LOCK_FREE > 0
template< bool Signed, bool Interprocess >
struct core_operations< 16u, Signed, Interprocess > :
    public core_operations_gcc_sync< 16u, Signed, Interprocess >
{
};
#endif

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_CORE_OPS_GCC_SYNC_HPP_INCLUDED_
