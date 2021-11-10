
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_REMOVE_VOLATILE_HPP_INCLUDED
#define BOOST_TT_REMOVE_VOLATILE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <cstddef> // size_t

namespace boost {

   //  convert a type T to a non-cv-qualified type - remove_volatile<T>
   template <class T> struct remove_volatile{ typedef T type; };
   template <class T> struct remove_volatile<T volatile>{ typedef T type; };

#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
   template <class T, std::size_t N> struct remove_volatile<T volatile[N]>{ typedef T type[N]; };
#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600) && !defined(__IBMCPP__) &&  !BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840))
   template <class T> struct remove_volatile<T volatile[]>{ typedef T type[]; };
#endif
#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using remove_volatile_t = typename remove_volatile<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_REMOVE_VOLATILE_HPP_INCLUDED
