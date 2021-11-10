/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_ops_generic.hpp
 *
 * This header contains generic (lock-based) implementation of the waiting/notifying atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPS_GENERIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPS_GENERIC_HPP_INCLUDED_

#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/pause.hpp>
#include <boost/atomic/detail/lock_pool.hpp>
#include <boost/atomic/detail/wait_operations_fwd.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Generic implementation of waiting/notifying operations
template< typename Base, bool Interprocess >
struct wait_operations_generic;

template< typename Base >
struct wait_operations_generic< Base, false > :
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

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        storage_type new_val = base_type::load(storage, order);
        if (new_val == old_val)
        {
            scoped_wait_state wait_state(&storage);
            new_val = base_type::load(storage, order);
            while (new_val == old_val)
            {
                wait_state.wait();
                new_val = base_type::load(storage, order);
            }
        }

        return new_val;
    }

    static BOOST_FORCEINLINE void notify_one(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        scoped_lock lock(&storage);
        lock_pool::notify_one(lock.get_lock_state(), &storage);
    }

    static BOOST_FORCEINLINE void notify_all(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        scoped_lock lock(&storage);
        lock_pool::notify_all(lock.get_lock_state(), &storage);
    }
};

template< typename Base >
struct wait_operations_generic< Base, true > :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool always_has_native_wait_notify = false;

    static BOOST_FORCEINLINE bool has_native_wait_notify(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return false;
    }

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        storage_type new_val = base_type::load(storage, order);
        if (new_val == old_val)
        {
            for (unsigned int i = 0u; i < 16u; ++i)
            {
                atomics::detail::pause();
                new_val = base_type::load(storage, order);
                if (new_val != old_val)
                    goto finish;
            }

            do
            {
                atomics::detail::wait_some();
                new_val = base_type::load(storage, order);
            }
            while (new_val == old_val);
        }

    finish:
        return new_val;
    }

    static BOOST_FORCEINLINE void notify_one(storage_type volatile&) BOOST_NOEXCEPT
    {
    }

    static BOOST_FORCEINLINE void notify_all(storage_type volatile&) BOOST_NOEXCEPT
    {
    }
};

template< typename Base, std::size_t Size, bool Interprocess >
struct wait_operations< Base, Size, true, Interprocess > :
    public wait_operations_generic< Base, Interprocess >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPS_GENERIC_HPP_INCLUDED_
