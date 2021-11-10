
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_ADD_VOLATILE_HPP_INCLUDED
#define BOOST_TT_ADD_VOLATILE_HPP_INCLUDED

#include <boost/config.hpp>

namespace boost {

// * convert a type T to volatile type - add_volatile<T>
// this is not required since the result is always
// the same as "T volatile", but it does suppress warnings
// from some compilers:

#if defined(BOOST_MSVC)
// This bogus warning will appear when add_volatile is applied to a
// const volatile reference because we can't detect const volatile
// references with MSVC6.
#   pragma warning(push)
#   pragma warning(disable:4181) // warning C4181: qualifier applied to reference type ignored
#endif 

template <class T> struct add_volatile{ typedef T volatile type; };

#if defined(BOOST_MSVC)
#   pragma warning(pop)
#endif 

template <class T> struct add_volatile<T&>{ typedef T& type; };

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using add_volatile_t = typename add_volatile<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_ADD_VOLATILE_HPP_INCLUDED
