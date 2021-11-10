
//  (C) Copyright John Maddock 2017.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.
 
#ifndef BOOST_TT_IS_COMPLETE_HPP_INCLUDED
#define BOOST_TT_IS_COMPLETE_HPP_INCLUDED

#include <boost/type_traits/declval.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/config/workaround.hpp>

/*
 * CAUTION:
 * ~~~~~~~~
 *
 * THIS TRAIT EXISTS SOLELY TO GENERATE HARD ERRORS WHEN A ANOTHER TRAIT
 * WHICH REQUIRES COMPLETE TYPES AS ARGUMENTS IS PASSED AN INCOMPLETE TYPE
 *
 * DO NOT MAKE GENERAL USE OF THIS TRAIT, AS THE COMPLETENESS OF A TYPE
 * VARIES ACROSS TRANSLATION UNITS AS WELL AS WITHIN A SINGLE UNIT.
 *
*/

namespace boost {


//
// We will undef this if the trait isn't fully functional:
//
#define BOOST_TT_HAS_WORKING_IS_COMPLETE

#if !defined(BOOST_NO_SFINAE_EXPR) && !BOOST_WORKAROUND(BOOST_MSVC, <= 1900) && !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40600)

   namespace detail{

      template <unsigned N>
      struct ok_tag { double d; char c[N]; };

      template <class T>
      ok_tag<sizeof(T)> check_is_complete(int);
      template <class T>
      char check_is_complete(...);
   }

   template <class T> struct is_complete
      : public integral_constant<bool, ::boost::is_function<typename boost::remove_reference<T>::type>::value || (sizeof(boost::detail::check_is_complete<T>(0)) != sizeof(char))> {};

#elif !defined(BOOST_NO_SFINAE) && !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS) && !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40500)

   namespace detail
   {

      template <class T>
      struct is_complete_imp
      {
         template <class U, class = decltype(sizeof(boost::declval< U >())) >
         static type_traits::yes_type check(U*);

         template <class U>
         static type_traits::no_type check(...);

         static const bool value = sizeof(check<T>(0)) == sizeof(type_traits::yes_type);
      };

} // namespace detail


   template <class T>
   struct is_complete : boost::integral_constant<bool, ::boost::is_function<typename boost::remove_reference<T>::type>::value || ::boost::detail::is_complete_imp<T>::value>
   {};
   template <class T>
   struct is_complete<T&> : boost::is_complete<T> {};
   
#else

      template <class T> struct is_complete
         : public boost::integral_constant<bool, true> {};

#undef BOOST_TT_HAS_WORKING_IS_COMPLETE

#endif

} // namespace boost

#endif // BOOST_TT_IS_COMPLETE_HPP_INCLUDED
