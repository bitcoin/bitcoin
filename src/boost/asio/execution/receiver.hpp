//
// execution/receiver.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_RECEIVER_HPP
#define BOOST_ASIO_EXECUTION_RECEIVER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/variadic_templates.hpp>
#include <boost/asio/execution/set_done.hpp>
#include <boost/asio/execution/set_error.hpp>
#include <boost/asio/execution/set_value.hpp>

#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
# include <exception>
#else // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
# include <boost/system/error_code.hpp>
#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)

#if defined(BOOST_ASIO_HAS_DEDUCED_SET_DONE_FREE_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_SET_DONE_MEMBER_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_FREE_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_FREE_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_RECEIVER_OF_FREE_TRAIT) \
  && defined(BOOST_ASIO_HAS_DEDUCED_RECEIVER_OF_MEMBER_TRAIT)
# define BOOST_ASIO_HAS_DEDUCED_EXECUTION_IS_RECEIVER_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_DEDUCED_SET_DONE_FREE_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_SET_DONE_MEMBER_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_FREE_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_FREE_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_RECEIVER_OF_FREE_TRAIT)
       //   && defined(BOOST_ASIO_HAS_DEDUCED_RECEIVER_OF_MEMBER_TRAIT)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {
namespace detail {

template <typename T, typename E>
struct is_receiver_base :
  integral_constant<bool,
    is_move_constructible<typename remove_cvref<T>::type>::value
      && is_constructible<typename remove_cvref<T>::type, T>::value
  >
{
};

} // namespace detail

#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
# define BOOST_ASIO_EXECUTION_RECEIVER_ERROR_DEFAULT = std::exception_ptr
#else // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
# define BOOST_ASIO_EXECUTION_RECEIVER_ERROR_DEFAULT \
  = ::boost::system::error_code
#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)

/// The is_receiver trait detects whether a type T satisfies the
/// execution::receiver concept.
/**
 * Class template @c is_receiver is a type trait that is derived from @c
 * true_type if the type @c T meets the concept definition for a receiver for
 * error type @c E, otherwise @c false_type.
 */
template <typename T, typename E BOOST_ASIO_EXECUTION_RECEIVER_ERROR_DEFAULT>
struct is_receiver :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  conditional<
    can_set_done<typename remove_cvref<T>::type>::value
      && is_nothrow_set_done<typename remove_cvref<T>::type>::value
      && can_set_error<typename remove_cvref<T>::type, E>::value
      && is_nothrow_set_error<typename remove_cvref<T>::type, E>::value,
    detail::is_receiver_base<T, E>,
    false_type
  >::type
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename E BOOST_ASIO_EXECUTION_RECEIVER_ERROR_DEFAULT>
BOOST_ASIO_CONSTEXPR const bool is_receiver_v = is_receiver<T, E>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename T, typename E BOOST_ASIO_EXECUTION_RECEIVER_ERROR_DEFAULT>
BOOST_ASIO_CONCEPT receiver = is_receiver<T, E>::value;

#define BOOST_ASIO_EXECUTION_RECEIVER ::boost::asio::execution::receiver

#else // defined(BOOST_ASIO_HAS_CONCEPTS)

#define BOOST_ASIO_EXECUTION_RECEIVER typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES) \
  || defined(GENERATING_DOCUMENTATION)

/// The is_receiver_of trait detects whether a type T satisfies the
/// execution::receiver_of concept for some set of value arguments.
/**
 * Class template @c is_receiver_of is a type trait that is derived from @c
 * true_type if the type @c T meets the concept definition for a receiver for
 * value arguments @c Vs, otherwise @c false_type.
 */
template <typename T, typename... Vs>
struct is_receiver_of :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  conditional<
    is_receiver<T>::value,
    can_set_value<typename remove_cvref<T>::type, Vs...>,
    false_type
  >::type
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename... Vs>
BOOST_ASIO_CONSTEXPR const bool is_receiver_of_v =
  is_receiver_of<T, Vs...>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename T, typename... Vs>
BOOST_ASIO_CONCEPT receiver_of = is_receiver_of<T, Vs...>::value;

#define BOOST_ASIO_EXECUTION_RECEIVER_OF_0 \
  ::boost::asio::execution::receiver_of

#define BOOST_ASIO_EXECUTION_RECEIVER_OF_1(v) \
  ::boost::asio::execution::receiver_of<v>

#else // defined(BOOST_ASIO_HAS_CONCEPTS)

#define BOOST_ASIO_EXECUTION_RECEIVER_OF_0 typename
#define BOOST_ASIO_EXECUTION_RECEIVER_OF_1(v) typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
      //   || defined(GENERATING_DOCUMENTATION)

template <typename T, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct is_receiver_of;

template <typename T>
struct is_receiver_of<T> :
  conditional<
    is_receiver<T>::value,
    can_set_value<typename remove_cvref<T>::type>,
    false_type
  >::type
{
};

#define BOOST_ASIO_PRIVATE_RECEIVER_OF_TRAITS_DEF(n) \
  template <typename T, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct is_receiver_of<T, BOOST_ASIO_VARIADIC_TARGS(n)> : \
    conditional< \
      conditional<true, is_receiver<T>, void>::type::value, \
      can_set_value< \
        typename remove_cvref<T>::type, \
        BOOST_ASIO_VARIADIC_TARGS(n)>, \
      false_type \
    >::type \
  { \
  }; \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_RECEIVER_OF_TRAITS_DEF)
#undef BOOST_ASIO_PRIVATE_RECEIVER_OF_TRAITS_DEF

#define BOOST_ASIO_EXECUTION_RECEIVER_OF_0 typename
#define BOOST_ASIO_EXECUTION_RECEIVER_OF_1(v) typename

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
       //   || defined(GENERATING_DOCUMENTATION)

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES) \
  || defined(GENERATING_DOCUMENTATION)

/// The is_nothrow_receiver_of trait detects whether a type T satisfies the
/// execution::receiver_of concept for some set of value arguments, with a
/// noexcept @c set_value operation.
/**
 * Class template @c is_nothrow_receiver_of is a type trait that is derived
 * from @c true_type if the type @c T meets the concept definition for a
 * receiver for value arguments @c Vs, and the expression
 * <tt>execution::set_value(declval<T>(), declval<Ts>()...)</tt> is noexcept,
 * otherwise @c false_type.
 */
template <typename T, typename... Vs>
struct is_nothrow_receiver_of :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  integral_constant<bool,
    is_receiver_of<T, Vs...>::value
      && is_nothrow_set_value<typename remove_cvref<T>::type, Vs...>::value
  >
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename... Vs>
BOOST_ASIO_CONSTEXPR const bool is_nothrow_receiver_of_v =
  is_nothrow_receiver_of<T, Vs...>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
      //   || defined(GENERATING_DOCUMENTATION)

template <typename T, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct is_nothrow_receiver_of;

template <typename T>
struct is_nothrow_receiver_of<T> :
  integral_constant<bool,
    is_receiver_of<T>::value
      && is_nothrow_set_value<typename remove_cvref<T>::type>::value
  >
{
};

#define BOOST_ASIO_PRIVATE_NOTHROW_RECEIVER_OF_TRAITS_DEF(n) \
  template <typename T, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct is_nothrow_receiver_of<T, BOOST_ASIO_VARIADIC_TARGS(n)> : \
    integral_constant<bool, \
      is_receiver_of<T, BOOST_ASIO_VARIADIC_TARGS(n)>::value \
        && is_nothrow_set_value<typename remove_cvref<T>::type, \
          BOOST_ASIO_VARIADIC_TARGS(n)>::value \
    > \
  { \
  }; \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_NOTHROW_RECEIVER_OF_TRAITS_DEF)
#undef BOOST_ASIO_PRIVATE_NOTHROW_RECEIVER_OF_TRAITS_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace execution
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_RECEIVER_HPP
