
//  (C) Copyright John Maddock 2015.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_DEFAULT_CONSTRUCTIBLE_HPP_INCLUDED
#define BOOST_TT_IS_DEFAULT_CONSTRUCTIBLE_HPP_INCLUDED

#include <cstddef> // size_t
#include <boost/type_traits/integral_constant.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/type_traits/is_complete.hpp>
#include <boost/static_assert.hpp>

#if BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40700)
#include <boost/type_traits/is_abstract.hpp>
#endif
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ <= 5)) || (defined(BOOST_MSVC) && (BOOST_MSVC == 1800))
#include <utility> // std::pair
#endif

#if !defined(BOOST_NO_CXX11_DECLTYPE) && !BOOST_WORKAROUND(BOOST_MSVC, < 1800) && !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40500)

#include <boost/type_traits/detail/yes_no_type.hpp>

namespace boost{

   namespace detail{

      struct is_default_constructible_imp
      {
         template<typename _Tp, typename = decltype(_Tp())>
         static boost::type_traits::yes_type test(int);

         template<typename>
         static boost::type_traits::no_type test(...);
      };
#if BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40700)
      template<class T, bool b> 
      struct is_default_constructible_abstract_filter
      {
          static const bool value = sizeof(is_default_constructible_imp::test<T>(0)) == sizeof(boost::type_traits::yes_type);
      };
      template<class T> 
      struct is_default_constructible_abstract_filter<T, true>
      {
          static const bool value = false;
      };
#endif
   }

#if BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40700)
   template <class T> struct is_default_constructible : public integral_constant<bool, detail::is_default_constructible_abstract_filter<T, boost::is_abstract<T>::value>::value>
   {
      BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_default_constructible must be complete types");
   };
#else
   template <class T> struct is_default_constructible : public integral_constant<bool, sizeof(boost::detail::is_default_constructible_imp::test<T>(0)) == sizeof(boost::type_traits::yes_type)>
   {
      BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_default_constructible must be complete types");
   };
#endif
   template <class T, std::size_t N> struct is_default_constructible<T[N]> : public is_default_constructible<T>{};
   template <class T> struct is_default_constructible<T[]> : public is_default_constructible<T>{};
   template <class T> struct is_default_constructible<T&> : public integral_constant<bool, false>{};
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ <= 5))|| (defined(BOOST_MSVC) && (BOOST_MSVC == 1800))
   template <class T, class U> struct is_default_constructible<std::pair<T,U> > : public integral_constant<bool, is_default_constructible<T>::value && is_default_constructible<U>::value>{};
#endif
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) 
   template <class T> struct is_default_constructible<T&&> : public integral_constant<bool, false>{};
#endif
   template <> struct is_default_constructible<void> : public integral_constant<bool, false>{};
   template <> struct is_default_constructible<void const> : public integral_constant<bool, false>{};
   template <> struct is_default_constructible<void volatile> : public integral_constant<bool, false>{};
   template <> struct is_default_constructible<void const volatile> : public integral_constant<bool, false>{};

#else

#include <boost/type_traits/is_pod.hpp>

namespace boost{

   // We don't know how to implement this, note we can not use has_trivial_constructor here
   // because the correct implementation of that trait requires this one:
   template <class T> struct is_default_constructible : public is_pod<T>{};
   template <> struct is_default_constructible<void> : public integral_constant<bool, false>{};
   template <> struct is_default_constructible<void const> : public integral_constant<bool, false>{};
   template <> struct is_default_constructible<void volatile> : public integral_constant<bool, false>{};
   template <> struct is_default_constructible<void const volatile> : public integral_constant<bool, false>{};

#endif

} // namespace boost

#endif // BOOST_TT_IS_DEFAULT_CONSTRUCTIBLE_HPP_INCLUDED
