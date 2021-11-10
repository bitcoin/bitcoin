
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


// Some fixes for is_array are based on a newsgroup posting by Jonathan Lundquist.


#ifndef BOOST_TT_IS_ARRAY_HPP_INCLUDED
#define BOOST_TT_IS_ARRAY_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>
#include <cstddef> // size_t

namespace boost {

#if defined( BOOST_CODEGEARC )
   template <class T> struct is_array : public integral_constant<bool, __is_array(T)> {};
#else
   template <class T> struct is_array : public false_type {};
#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
   template <class T, std::size_t N> struct is_array<T[N]> : public true_type {};
   template <class T, std::size_t N> struct is_array<T const[N]> : public true_type{};
   template <class T, std::size_t N> struct is_array<T volatile[N]> : public true_type{};
   template <class T, std::size_t N> struct is_array<T const volatile[N]> : public true_type{};
#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600) && !defined(__IBMCPP__) &&  !BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840))
   template <class T> struct is_array<T[]> : public true_type{};
   template <class T> struct is_array<T const[]> : public true_type{};
   template <class T> struct is_array<T const volatile[]> : public true_type{};
   template <class T> struct is_array<T volatile[]> : public true_type{};
#endif
#endif

#endif

} // namespace boost

#endif // BOOST_TT_IS_ARRAY_HPP_INCLUDED
