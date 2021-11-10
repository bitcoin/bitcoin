/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2018 Andrey Semashev
 */
/*!
 * \file   atomic/detail/addressof.hpp
 *
 * This header defines \c addressof helper function. It is similar to \c boost::addressof but it is more
 * lightweight and also contains a workaround for some compiler warnings.
 */

#ifndef BOOST_ATOMIC_DETAIL_ADDRESSOF_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ADDRESSOF_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

// Detection logic is based on boost/core/addressof.hpp
#if defined(BOOST_MSVC_FULL_VER) && BOOST_MSVC_FULL_VER >= 190024215
#define BOOST_ATOMIC_DETAIL_HAS_BUILTIN_ADDRESSOF
#elif defined(BOOST_GCC) && BOOST_GCC >= 70000
#define BOOST_ATOMIC_DETAIL_HAS_BUILTIN_ADDRESSOF
#elif defined(__has_builtin)
#if __has_builtin(__builtin_addressof)
#define BOOST_ATOMIC_DETAIL_HAS_BUILTIN_ADDRESSOF
#endif
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename T >
BOOST_FORCEINLINE T* addressof(T& value) BOOST_NOEXCEPT
{
#if defined(BOOST_ATOMIC_DETAIL_HAS_BUILTIN_ADDRESSOF)
    return __builtin_addressof(value);
#else
    // Note: The point of using a local struct as the intermediate type instead of char is to avoid gcc warnings
    // if T is a const volatile char*:
    // warning: casting 'const volatile char* const' to 'const volatile char&' does not dereference pointer
    // The local struct makes sure T is not related to the cast target type.
    struct opaque_type;
    return reinterpret_cast< T* >(&const_cast< opaque_type& >(reinterpret_cast< const volatile opaque_type& >(value)));
#endif
}

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_ADDRESSOF_HPP_INCLUDED_
