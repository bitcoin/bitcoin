
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000-2005.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_EXTENT_HPP_INCLUDED
#define BOOST_TT_REMOVE_EXTENT_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <cstddef> // size_t

namespace boost {

template <class T> struct remove_extent{ typedef T type; };

#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template <typename T, std::size_t N> struct remove_extent<T[N]> { typedef T type; };
template <typename T, std::size_t N> struct remove_extent<T const[N]> { typedef T const type; };
template <typename T, std::size_t N> struct remove_extent<T volatile [N]> { typedef T volatile type; };
template <typename T, std::size_t N> struct remove_extent<T const volatile [N]> { typedef T const volatile type; };
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610)) && !defined(__IBMCPP__) &&  !BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840))
template <typename T> struct remove_extent<T[]> { typedef T type; };
template <typename T> struct remove_extent<T const[]> { typedef T const type; };
template <typename T> struct remove_extent<T volatile[]> { typedef T volatile type; };
template <typename T> struct remove_extent<T const volatile[]> { typedef T const volatile type; };
#endif
#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using remove_extent_t = typename remove_extent<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_REMOVE_BOUNDS_HPP_INCLUDED
