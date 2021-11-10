
//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_ALL_EXTENTS_HPP_INCLUDED
#define BOOST_TT_REMOVE_ALL_EXTENTS_HPP_INCLUDED

#include <boost/config.hpp>
#include <cstddef> // size_t
#include <boost/detail/workaround.hpp>

namespace boost {

template <class T> struct remove_all_extents{ typedef T type; };

#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template <class T, std::size_t N> struct remove_all_extents<T[N]> : public remove_all_extents<T>{};
template <class T, std::size_t N> struct remove_all_extents<T const[N]> : public remove_all_extents<T const>{};
template <class T, std::size_t N> struct remove_all_extents<T volatile[N]> : public remove_all_extents<T volatile>{};
template <class T, std::size_t N> struct remove_all_extents<T const volatile[N]> : public remove_all_extents<T const volatile>{};
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610)) && !defined(__IBMCPP__) &&  !BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840))
template <class T> struct remove_all_extents<T[]> : public remove_all_extents<T>{};
template <class T> struct remove_all_extents<T const[]> : public remove_all_extents<T const>{};
template <class T> struct remove_all_extents<T volatile[]> : public remove_all_extents<T volatile>{};
template <class T> struct remove_all_extents<T const volatile[]> : public remove_all_extents<T const volatile>{};
#endif
#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using remove_all_extents_t = typename remove_all_extents<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_REMOVE_BOUNDS_HPP_INCLUDED
