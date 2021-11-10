/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/ipc_atomic.hpp
 *
 * This header contains definition of \c ipc_atomic template.
 */

#ifndef BOOST_ATOMIC_IPC_ATOMIC_HPP_INCLUDED_
#define BOOST_ATOMIC_IPC_ATOMIC_HPP_INCLUDED_

#include <cstddef>
#include <boost/static_assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/capabilities.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/classify.hpp>
#include <boost/atomic/detail/atomic_impl.hpp>
#include <boost/atomic/detail/type_traits/is_trivially_copyable.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {

//! Atomic object for inter-process communication
template< typename T >
class ipc_atomic :
    public atomics::detail::base_atomic< T, typename atomics::detail::classify< T >::type, true >
{
private:
    typedef atomics::detail::base_atomic< T, typename atomics::detail::classify< T >::type, true > base_type;
    typedef typename base_type::value_arg_type value_arg_type;

public:
    typedef typename base_type::value_type value_type;

    BOOST_STATIC_ASSERT_MSG(sizeof(value_type) > 0u, "boost::ipc_atomic<T> requires T to be a complete type");
#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_IS_TRIVIALLY_COPYABLE)
    BOOST_STATIC_ASSERT_MSG(atomics::detail::is_trivially_copyable< value_type >::value, "boost::ipc_atomic<T> requires T to be a trivially copyable type");
#endif

public:
    BOOST_DEFAULTED_FUNCTION(ipc_atomic() BOOST_ATOMIC_DETAIL_DEF_NOEXCEPT_DECL, BOOST_ATOMIC_DETAIL_DEF_NOEXCEPT_IMPL {})
    BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_UNION_INIT ipc_atomic(value_arg_type v) BOOST_NOEXCEPT : base_type(v) {}

    BOOST_FORCEINLINE value_type operator= (value_arg_type v) BOOST_NOEXCEPT
    {
        this->store(v);
        return v;
    }

    BOOST_FORCEINLINE value_type operator= (value_arg_type v) volatile BOOST_NOEXCEPT
    {
        this->store(v);
        return v;
    }

    BOOST_FORCEINLINE operator value_type() const volatile BOOST_NOEXCEPT
    {
        return this->load();
    }

    BOOST_DELETED_FUNCTION(ipc_atomic(ipc_atomic const&))
    BOOST_DELETED_FUNCTION(ipc_atomic& operator= (ipc_atomic const&))
    BOOST_DELETED_FUNCTION(ipc_atomic& operator= (ipc_atomic const&) volatile)
};

} // namespace atomics

using atomics::ipc_atomic;

} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_IPC_ATOMIC_HPP_INCLUDED_
