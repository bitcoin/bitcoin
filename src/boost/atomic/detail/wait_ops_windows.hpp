/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/wait_ops_windows.hpp
 *
 * This header contains implementation of the waiting/notifying atomic operations on Windows.
 */

#ifndef BOOST_ATOMIC_DETAIL_WAIT_OPS_WINDOWS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_WAIT_OPS_WINDOWS_HPP_INCLUDED_

#include <cstddef>
#include <boost/static_assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/wait_constants.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/link.hpp>
#include <boost/atomic/detail/once_flag.hpp>
#include <boost/atomic/detail/wait_operations_fwd.hpp>
#include <boost/atomic/detail/wait_ops_generic.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

typedef boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
wait_on_address_t(
    volatile boost::winapi::VOID_* addr,
    boost::winapi::PVOID_ compare_addr,
    boost::winapi::SIZE_T_ size,
    boost::winapi::DWORD_ timeout_ms);

typedef boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
wake_by_address_t(boost::winapi::PVOID_ addr);

extern BOOST_ATOMIC_DECL wait_on_address_t* wait_on_address;
extern BOOST_ATOMIC_DECL wake_by_address_t* wake_by_address_single;
extern BOOST_ATOMIC_DECL wake_by_address_t* wake_by_address_all;

extern BOOST_ATOMIC_DECL once_flag wait_functions_once_flag;
BOOST_ATOMIC_DECL void initialize_wait_functions() BOOST_NOEXCEPT;

BOOST_FORCEINLINE void ensure_wait_functions_initialized() BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT_MSG(once_flag_operations::is_always_lock_free, "Boost.Atomic unsupported target platform: native atomic operations not implemented for bytes");
    if (BOOST_LIKELY(once_flag_operations::load(wait_functions_once_flag.m_flag, boost::memory_order_acquire) == 0u))
        return;

    initialize_wait_functions();
}

template< typename Base, std::size_t Size >
struct wait_operations_windows :
    public atomics::detail::wait_operations_generic< Base, false >
{
    typedef atomics::detail::wait_operations_generic< Base, false > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_CONSTEXPR_OR_CONST bool always_has_native_wait_notify = false;

    static BOOST_FORCEINLINE bool has_native_wait_notify(storage_type const volatile&) BOOST_NOEXCEPT
    {
        ensure_wait_functions_initialized();
        return atomics::detail::wait_on_address != NULL;
    }

    static BOOST_FORCEINLINE storage_type wait(storage_type const volatile& storage, storage_type old_val, memory_order order) BOOST_NOEXCEPT
    {
        ensure_wait_functions_initialized();

        if (BOOST_LIKELY(atomics::detail::wait_on_address != NULL))
        {
            storage_type new_val = base_type::load(storage, order);
            while (new_val == old_val)
            {
                atomics::detail::wait_on_address(const_cast< storage_type* >(&storage), &old_val, Size, boost::winapi::infinite);
                new_val = base_type::load(storage, order);
            }

            return new_val;
        }
        else
        {
            return base_type::wait(storage, old_val, order);
        }
    }

    static BOOST_FORCEINLINE void notify_one(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        ensure_wait_functions_initialized();

        if (BOOST_LIKELY(atomics::detail::wake_by_address_single != NULL))
            atomics::detail::wake_by_address_single(const_cast< storage_type* >(&storage));
        else
            base_type::notify_one(storage);
    }

    static BOOST_FORCEINLINE void notify_all(storage_type volatile& storage) BOOST_NOEXCEPT
    {
        ensure_wait_functions_initialized();

        if (BOOST_LIKELY(atomics::detail::wake_by_address_all != NULL))
            atomics::detail::wake_by_address_all(const_cast< storage_type* >(&storage));
        else
            base_type::notify_all(storage);
    }
};

template< typename Base >
struct wait_operations< Base, 1u, true, false > :
    public wait_operations_windows< Base, 1u >
{
};

template< typename Base >
struct wait_operations< Base, 2u, true, false > :
    public wait_operations_windows< Base, 2u >
{
};

template< typename Base >
struct wait_operations< Base, 4u, true, false > :
    public wait_operations_windows< Base, 4u >
{
};

template< typename Base >
struct wait_operations< Base, 8u, true, false > :
    public wait_operations_windows< Base, 8u >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_WAIT_OPS_WINDOWS_HPP_INCLUDED_
