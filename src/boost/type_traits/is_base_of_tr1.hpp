
//  (C) Copyright Rani Sharoni 2003-2005.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.
 
#ifndef BOOST_TT_IS_BASE_OF_TR1_HPP_INCLUDED
#define BOOST_TT_IS_BASE_OF_TR1_HPP_INCLUDED

#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_class.hpp>

namespace boost { namespace tr1{

   namespace detail{
      template <class B, class D>
      struct is_base_of_imp
      {
          typedef typename remove_cv<B>::type ncvB;
          typedef typename remove_cv<D>::type ncvD;
          BOOST_STATIC_CONSTANT(bool, value = ((::boost::detail::is_base_and_derived_impl<ncvB,ncvD>::value) || (::boost::is_same<ncvB,ncvD>::value)));
      };
   }

   template <class Base, class Derived> struct is_base_of
      : public integral_constant<bool, (::boost::tr1::detail::is_base_of_imp<Base, Derived>::value)>{};

   template <class Base, class Derived> struct is_base_of<Base, Derived&> : public false_type{};
   template <class Base, class Derived> struct is_base_of<Base&, Derived&> : public false_type{};
   template <class Base, class Derived> struct is_base_of<Base&, Derived> : public false_type{};

} } // namespace boost

#endif // BOOST_TT_IS_BASE_OF_TR1_HPP_INCLUDED
