/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_ops_dragonfly_umtx.hpp
 *
 * This header contains implementation of the waiting/notifying atomic operations based on DragonFly BSD umtx.
 * https://man.dragonflybsd.org/?command=umtx&section=2
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPS_DRAGONFLY_UMTX_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPS_DRAGONFLY_UMTX_HPP_INCLUDED_

#include <unistd.h>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/wait_operations_fwd.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename Base, bool Interprocess >
struct wait_operations< Base, sizeof(int), true, Interprocess > :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool always_has_native_wait_notify = true;

    static BOOST_FORCEINLINE bool has_native_wait_notify(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        storage_type new_val = base_type::load(storage, order);
        while (new_val == old_val)
        {
            ::umtx_sleep(reinterpret_cast< int* >(const_cast< storage_type* >(&storage)), static_cast< int >(old_val), 0);
            new_val = base_type::load(storage, order);
        }

        return new_val;
    }

    static BOOST_FORCEINLINE void notify_one(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        ::umtx_wakeup(reinterpret_cast< int* >(const_cast< storage_type* >(&storage)), 1);
    }

    static BOOST_FORCEINLINE void notify_all(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        ::umtx_wakeup(reinterpret_cast< int* >(const_cast< storage_type* >(&storage)), 0);
    }
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPS_DRAGONFLY_UMTX_HPP_INCLUDED_
