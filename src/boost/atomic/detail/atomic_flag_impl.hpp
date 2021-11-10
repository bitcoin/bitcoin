/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014, 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/atomic_flag_impl.hpp
 *
 * This header contains implementation of \c atomic_flag.
 */

#ifndef BOOST_ATOMIC_DETAIL_ATOMIC_FLAG_IMPL_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ATOMIC_FLAG_IMPL_HPP_INCLUDED_

#include <boost/assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/core_operations.hpp>
#include <boost/atomic/detail/wait_operations.hpp>
#include <boost/atomic/detail/aligned_variable.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

/*
 * IMPLEMENTATION NOTE: All interface functions MUST be declared with BOOST_FORCEINLINE,
 *                      see comment for convert_memory_order_to_gcc in gcc_atomic_memory_order_utils.hpp.
 */

namespace boost {
namespace atomics {
namespace detail {

#if defined(BOOST_ATOMIC_DETAIL_NO_CXX11_CONSTEXPR_UNION_INIT) || defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
#define BOOST_ATOMIC_NO_ATOMIC_FLAG_INIT
#else
#define BOOST_ATOMIC_FLAG_INIT {}
#endif

//! Atomic flag implementation
template< bool IsInterprocess >
struct atomic_flag_impl
{
    // Prefer 4-byte storage as most platforms support waiting/notifying operations without a lock pool for 32-bit integers
    typedef atomics::detail::core_operations< 4u, false, IsInterprocess > core_operations;
    typedef atomics::detail::wait_operations< core_operations > wait_operations;
    typedef typename core_operations::storage_type storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool is_always_lock_free = core_operations::is_always_lock_free;
    static BOOST_CONSTEXPR_OR_CONST bool always_has_native_wait_notify = wait_operations::always_has_native_wait_notify;

    BOOST_ATOMIC_DETAIL_ALIGNED_VAR_TPL(core_operations::storage_alignment, storage_type, m_storage);

    BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_UNION_INIT atomic_flag_impl() BOOST_NOEXCEPT : m_storage(0u)
    {
    }

    BOOST_FORCEINLINE bool is_lock_free() const volatile BOOST_NOEXCEPT
    {
        return is_always_lock_free;
    }

    BOOST_FORCEINLINE bool has_native_wait_notify() const volatile BOOST_NOEXCEPT
    {
        return wait_operations::has_native_wait_notify(m_storage);
    }

    BOOST_FORCEINLINE bool test(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        BOOST_ASSERT(order != memory_order_release);
        BOOST_ASSERT(order != memory_order_acq_rel);
        return !!core_operations::load(m_storage, order);
    }

    BOOST_FORCEINLINE bool test_and_set(memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        return core_operations::test_and_set(m_storage, order);
    }

    BOOST_FORCEINLINE void clear(memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        BOOST_ASSERT(order != memory_order_consume);
        BOOST_ASSERT(order != memory_order_acquire);
        BOOST_ASSERT(order != memory_order_acq_rel);
        core_operations::clear(m_storage, order);
    }

    BOOST_FORCEINLINE bool wait(bool old_val, memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        BOOST_ASSERT(order != memory_order_release);
        BOOST_ASSERT(order != memory_order_acq_rel);

        return !!wait_operations::wait(m_storage, static_cast< storage_type >(old_val), order);
    }

    BOOST_FORCEINLINE void notify_one() volatile BOOST_NOEXCEPT
    {
        wait_operations::notify_one(m_storage);
    }

    BOOST_FORCEINLINE void notify_all() volatile BOOST_NOEXCEPT
    {
        wait_operations::notify_all(m_storage);
    }

    BOOST_DELETED_FUNCTION(atomic_flag_impl(atomic_flag_impl const&))
    BOOST_DELETED_FUNCTION(atomic_flag_impl& operator= (atomic_flag_impl const&))
};

#if defined(BOOST_NO_CXX17_INLINE_VARIABLES)
template< bool IsInterprocess >
BOOST_CONSTEXPR_OR_CONST bool atomic_flag_impl< IsInterprocess >::is_always_lock_free;
template< bool IsInterprocess >
BOOST_CONSTEXPR_OR_CONST bool atomic_flag_impl< IsInterprocess >::always_has_native_wait_notify;
#endif

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_ATOMIC_FLAG_IMPL_HPP_INCLUDED_
