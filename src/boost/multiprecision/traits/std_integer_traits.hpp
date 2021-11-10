///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MP_STD_INTEGER_TRAITS_HPP
#define BOOST_MP_STD_INTEGER_TRAITS_HPP

#include <type_traits>

namespace boost {
namespace multiprecision {
namespace detail {

template <class T>
struct is_signed : public std::is_signed<T> {};
template <class T>
struct is_unsigned : public std::is_unsigned<T> {};
template <class T>
struct is_integral : public std::is_integral<T> {};
template <class T>
struct is_arithmetic : public std::is_arithmetic<T> {};
template <class T>
struct make_unsigned : public std::make_unsigned<T> {};
template <class T>
struct make_signed : public std::make_signed<T> {};

#ifdef BOOST_HAS_INT128

template <>
struct is_signed<__int128> : public std::integral_constant<bool, true> {};
template <>
struct is_signed<unsigned __int128> : public std::integral_constant<bool, false> {};
template <>
struct is_unsigned<__int128> : public std::integral_constant<bool, false> {};
template <>
struct is_unsigned<unsigned __int128> : public std::integral_constant<bool, true> {};
template <>
struct is_integral<__int128> : public std::integral_constant<bool, true> {};
template <>
struct is_integral<unsigned __int128> : public std::integral_constant<bool, true> {};
template <>
struct is_arithmetic<__int128> : public std::integral_constant<bool, true> {};
template <>
struct is_arithmetic<unsigned __int128> : public std::integral_constant<bool, true> {};
template <>
struct make_unsigned<__int128>
{
   using type = unsigned __int128;
};
template <>
struct make_unsigned<unsigned __int128>
{
   using type = unsigned __int128;
};
template <>
struct make_signed<__int128>
{
   using type = __int128;
};
template <>
struct make_signed<unsigned __int128>
{
   using type = __int128;
};

#endif

}}} // namespace boost::multiprecision::detail

#endif
