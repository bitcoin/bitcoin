///////////////////////////////////////////////////////////////////////////////
//  Copyright 2015 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_IS_BYTE_CONTAINER_HPP
#define BOOST_IS_BYTE_CONTAINER_HPP

#include <iterator>
#include <type_traits>

namespace boost { namespace multiprecision { namespace detail {

template <class T>
struct has_member_const_iterator
{
   template <class U>
   static double         check(U*, typename U::const_iterator* = 0);
   static char           check(...);
   static T*             get();
   static constexpr bool value = sizeof(check(get())) == sizeof(double);
};


template <class C, bool b>
struct is_byte_container_imp
{
   // Note: Don't use C::value_type as this is a rather widespread typedef, even for non-range types
   using container_value_type = typename std::remove_cv<typename std::iterator_traits<typename C::const_iterator>::value_type>::type;
   static constexpr const bool                                                                                  value = boost::multiprecision::detail::is_integral<container_value_type>::value && (sizeof(container_value_type) == 1);
};

template <class C>
struct is_byte_container_imp<C, false> : public boost::false_type
{};

template <class C>
struct is_byte_container : public is_byte_container_imp<C, has_member_const_iterator<C>::value>
{};

}}} // namespace boost::multiprecision::detail

#endif // BOOST_IS_BYTE_CONTAINER_HPP
