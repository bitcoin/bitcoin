/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_ops_emulated.hpp
 *
 * This header contains emulated (lock-based) implementation of the waiting and notifying atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPS_EMULATED_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPS_EMULATED_HPP_INCLUDED_

#include <cstddef>
#include <boost/static_assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/lock_pool.hpp>
#include <boost/atomic/detail/wait_operations_fwd.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Emulated implementation of waiting and notifying operations
template< typename Base >
struct wait_operations_emulated :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;
    typedef lock_pool::scoped_lock< base_type::storage_alignment, true > scoped_lock;
    typedef lock_pool::scoped_wait_state< base_type::storage_alignment > scoped_wait_state;

    static BOOST_CONSTEXPR_OR_CONST bool always_has_native_wait_notify = false;

    static BOOST_FORCEINLINE bool has_native_wait_notify(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return false;
    }

    static
#if defined(BOOST_MSVC) && BOOST_MSVC < 1500
    // In some cases, when this function is inlined, MSVC-8 (VS2005) x64 generates broken code that returns a bogus value from this function.
    BOOST_NOINLINE
#endif
    storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order) BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT_MSG(!base_type::is_interprocess, "Boost.Atomic: operation invoked on a non-lock-free inter-process atomic object");
        storage_type const& s = const_cast< storage_type const& >(storage);
        scoped_wait_state wait_state(&storage);
        storage_type new_val = s;
        while (new_val == old_val)
        {
            wait_state.wait();
            new_val = s;
        }

        return new_val;
    }

    static void notify_one(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT_MSG(!base_type::is_interprocess, "Boost.Atomic: operation invoked on a non-lock-free inter-process atomic object");
        scoped_lock lock(&storage);
        lock_pool::notify_one(lock.get_lock_state(), &storage);
    }

    static void notify_all(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT_MSG(!base_type::is_interprocess, "Boost.Atomic: operation invoked on a non-lock-free inter-process atomic object");
        scoped_lock lock(&storage);
        lock_pool::notify_all(lock.get_lock_state(), &storage);
    }
};

template< typename Base, std::size_t Size, bool Interprocess >
struct wait_operations< Base, Size, false, Interprocess > :
    public wait_operations_emulated< Base >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPS_EMULATED_HPP_INCLUDED_
