/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2018 Andrey Semashev
 */
/*!
 * \file   atomic/detail/type_traits/is_trivially_copyable.hpp
 *
 * This header defines \c is_trivially_copyable type trait
 */

#ifndef BOOST_ATOMIC_DETAIL_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <type_traits>
#else
// For std::is_trivially_copyable we require a genuine support from the compiler.
// Fallback to is_pod or a false negative result in Boost.TypeTraits is not acceptable
// as this trait will be used in a static assert and may deny valid uses of boost::atomic/atomic_ref.
#define BOOST_ATOMIC_DETAIL_NO_CXX11_IS_TRIVIALLY_COPYABLE
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_IS_TRIVIALLY_COPYABLE)

namespace boost {
namespace atomics {
namespace detail {

using std::is_trivially_copyable;

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_IS_TRIVIALLY_COPYABLE)

#endif // BOOST_ATOMIC_DETAIL_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_HPP_INCLUDED_
