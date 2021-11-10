
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_POINTER_HPP_INCLUDED
#define BOOST_TT_REMOVE_POINTER_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/config/workaround.hpp>

#if defined(BOOST_MSVC)
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_pointer.hpp>
#endif

namespace boost {

#if BOOST_WORKAROUND(BOOST_MSVC, < 1900)

namespace detail{

   //
   // We need all this crazy indirection because a type such as:
   //
   // T (*const)(U)
   //
   // Does not bind to a <T*> or <T*const> partial specialization with VC10 and earlier
   //
   template <class T> 
   struct remove_pointer_imp
   {
      typedef T type;
   };

   template <class T> 
   struct remove_pointer_imp<T*>
   {
      typedef T type;
   };

   template <class T, bool b> 
   struct remove_pointer_imp3
   {
      typedef typename remove_pointer_imp<typename boost::remove_cv<T>::type>::type type;
   };

   template <class T> 
   struct remove_pointer_imp3<T, false>
   {
      typedef T type;
   };

   template <class T> 
   struct remove_pointer_imp2
   {
      typedef typename remove_pointer_imp3<T, ::boost::is_pointer<T>::value>::type type;
   };
}

template <class T> struct remove_pointer{ typedef typename boost::detail::remove_pointer_imp2<T>::type type; };

#else

template <class T> struct remove_pointer{ typedef T type; };
template <class T> struct remove_pointer<T*>{ typedef T type; };
template <class T> struct remove_pointer<T*const>{ typedef T type; };
template <class T> struct remove_pointer<T*volatile>{ typedef T type; };
template <class T> struct remove_pointer<T*const volatile>{ typedef T type; };

#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using remove_pointer_t = typename remove_pointer<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_REMOVE_POINTER_HPP_INCLUDED
