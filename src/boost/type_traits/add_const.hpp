
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_ADD_CONST_HPP_INCLUDED
#define BOOST_TT_ADD_CONST_HPP_INCLUDED

#include <boost/type_traits/detail/config.hpp>

namespace boost {

// * convert a type T to const type - add_const<T>
// this is not required since the result is always
// the same as "T const", but it does suppress warnings
// from some compilers:

#if defined(BOOST_MSVC)
// This bogus warning will appear when add_const is applied to a
// const volatile reference because we can't detect const volatile
// references with MSVC6.
#   pragma warning(push)
#   pragma warning(disable:4181) // warning C4181: qualifier applied to reference type ignored
#endif 

   template <class T> struct add_const
   {
      typedef T const type;
   };

#if defined(BOOST_MSVC)
#   pragma warning(pop)
#endif 

   template <class T> struct add_const<T&>
   {
      typedef T& type;
   };

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using add_const_t = typename add_const<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_ADD_CONST_HPP_INCLUDED
