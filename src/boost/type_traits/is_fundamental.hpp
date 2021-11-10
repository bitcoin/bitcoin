
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_FUNDAMENTAL_HPP_INCLUDED
#define BOOST_TT_IS_FUNDAMENTAL_HPP_INCLUDED

#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_void.hpp>

namespace boost {

//* is a type T a fundamental type described in the standard (3.9.1)
#if defined( BOOST_CODEGEARC )
template <class T> struct is_fundamental : public integral_constant<bool, __is_fundamental(T)> {};
#else
template <class T> struct is_fundamental : public integral_constant<bool, ::boost::is_arithmetic<T>::value || ::boost::is_void<T>::value> {};
#endif

} // namespace boost

#endif // BOOST_TT_IS_FUNDAMENTAL_HPP_INCLUDED
