/* Copyright 2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_TT_DETAIL_IS_LIKELY_STATELESS_LAMBDA_HPP
#define BOOST_TT_DETAIL_IS_LIKELY_STATELESS_LAMBDA_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/type_traits/detail/config.hpp>
#include <boost/type_traits/integral_constant.hpp>

#if defined(BOOST_TT_HAS_ACCURATE_BINARY_OPERATOR_DETECTION)
//
// We don't need or use this, just define a dummy class:
//
namespace boost{ namespace type_traits_detail{

template<typename T>
struct is_likely_stateless_lambda : public false_type {};

}}

#elif !defined(BOOST_NO_CXX11_LAMBDAS) && !defined(BOOST_NO_CXX11_DECLTYPE) && !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) && !BOOST_WORKAROUND(BOOST_MSVC, < 1900)\
         && !(BOOST_WORKAROUND(BOOST_MSVC, == 1900) && defined(_MANAGED))

#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/enable_if.hpp>

namespace boost{

namespace type_traits_detail{

/* Stateless lambda expressions have one (and only one) call operator and are
 * convertible to a function pointer with the same signature. Non-lambda types
 * could satisfy this too, hence the "likely" qualifier.
 */

template<typename T>
struct has_one_operator_call_helper
{
  template<typename Q> static boost::true_type  test(decltype(&Q::operator())*);
  template<typename>   static boost::false_type test(...);

  using type=decltype(test<T>(nullptr));
};

template<typename T>
using has_one_operator_call=typename has_one_operator_call_helper<T>::type;

template<typename T>
struct equivalent_function_pointer
{
  template<typename Q,typename R,typename... Args>
  static auto helper(R (Q::*)(Args...)const)->R(*)(Args...);
  template<typename Q,typename R,typename... Args>
  static auto helper(R (Q::*)(Args...))->R(*)(Args...);

  using type=decltype(helper(&T::operator()));
};

template<typename T,typename=void>
struct is_likely_stateless_lambda : false_type{};

template<typename T>
struct is_likely_stateless_lambda<
  T,
  typename boost::enable_if_<has_one_operator_call<T>::value>::type> :
     boost::is_convertible<T, typename equivalent_function_pointer<T>::type
>{};

} /* namespace type_traits_detail */

} /* namespace boost */

#else
 //
 // Can't implement this:
 //
namespace boost {
   namespace type_traits_detail {

      template<typename T>
      struct is_likely_stateless_lambda : public boost::integral_constant<bool, false> {};
}}

#endif
#endif

