/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_ops_freebsd_umtx.hpp
 *
 * This header contains implementation of the waiting/notifying atomic operations based on FreeBSD _umtx_op syscall.
 * https://www.freebsd.org/cgi/man.cgi?query=_umtx_op&apropos=0&sektion=2&manpath=FreeBSD+11-current&format=html
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPS_FREEBSD_UMTX_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPS_FREEBSD_UMTX_HPP_INCLUDED_

#include <sys/types.h>
#include <sys/umtx.h>
#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/int_sizes.hpp>
#include <boost/atomic/detail/wait_operations_fwd.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

#if defined(UMTX_OP_WAIT_UINT) || defined(UMTX_OP_WAIT)

template< typename Base >
struct wait_operations_freebsd_umtx_common :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool always_has_native_wait_notify = true;

    static BOOST_FORCEINLINE bool has_native_wait_notify(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }

    static BOOST_FORCEINLINE void notify_one(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        ::_umtx_op(const_cast< storage_type* >(&storage), UMTX_OP_WAKE, 1u, NULL, NULL);
    }

    static BOOST_FORCEINLINE void notify_all(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        ::_umtx_op(const_cast< storage_type* >(&storage), UMTX_OP_WAKE, (~static_cast< unsigned int >(0u)) >> 1, NULL, NULL);
    }
};

#endif // defined(UMTX_OP_WAIT_UINT) || defined(UMTX_OP_WAIT)

// UMTX_OP_WAIT_UINT only appeared in FreeBSD 8.0
#if defined(UMTX_OP_WAIT_UINT) && BOOST_ATOMIC_DETAIL_SIZEOF_INT < BOOST_ATOMIC_DETAIL_SIZEOF_LONG

template< typename Base, bool Interprocess >
struct wait_operations< Base, sizeof(unsigned int), true, Interprocess > :
    public wait_operations_freebsd_umtx_common< Base >
{
    typedef wait_operations_freebsd_umtx_common< Base > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        storage_type new_val = base_type::load(storage, order);
        while (new_val == old_val)
        {
            ::_umtx_op(const_cast< storage_type* >(&storage), UMTX_OP_WAIT_UINT, old_val, NULL, NULL);
            new_val = base_type::load(storage, order);
        }

        return new_val;
    }
};

#endif // defined(UMTX_OP_WAIT_UINT) && BOOST_ATOMIC_DETAIL_SIZEOF_INT < BOOST_ATOMIC_DETAIL_SIZEOF_LONG

#if defined(UMTX_OP_WAIT)

template< typename Base, bool Interprocess >
struct wait_operations< Base, sizeof(unsigned long), true, Interprocess > :
    public wait_operations_freebsd_umtx_common< Base >
{
    typedef wait_operations_freebsd_umtx_common< Base > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        storage_type new_val = base_type::load(storage, order);
        while (new_val == old_val)
        {
            ::_umtx_op(const_cast< storage_type* >(&storage), UMTX_OP_WAIT, old_val, NULL, NULL);
            new_val = base_type::load(storage, order);
        }

        return new_val;
    }
};

#endif // defined(UMTX_OP_WAIT)

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPS_FREEBSD_UMTX_HPP_INCLUDED_
