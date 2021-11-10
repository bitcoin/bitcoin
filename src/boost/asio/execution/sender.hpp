//
// execution/sender.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_SENDER_HPP
#define BOOST_ASIO_EXECUTION_SENDER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/detail/as_invocable.hpp>
#include <boost/asio/execution/detail/void_receiver.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/receiver.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES) \
  && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES) \
  && defined(BOOST_ASIO_HAS_DECLTYPE) \
  && !defined(BOOST_ASIO_MSVC) || (_MSC_VER >= 1910)
# define BOOST_ASIO_HAS_DEDUCED_EXECUTION_IS_TYPED_SENDER_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)
       //   && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
       //   && defined(BOOST_ASIO_HAS_DECLTYPE)
       //   && !defined(BOOST_ASIO_MSVC) || (_MSC_VER >= 1910)

namespace boost {
namespace asio {
namespace execution {
namespace detail {

namespace sender_base_ns { struct sender_base {}; }

template <typename S, typename = void>
struct sender_traits_base
{
  typedef void asio_execution_sender_traits_base_is_unspecialised;
};

template <typename S>
struct sender_traits_base<S,
    typename enable_if<
      is_base_of<sender_base_ns::sender_base, S>::value
    >::type>
{
};

template <typename S, typename = void, typename = void, typename = void>
struct has_sender_types : false_type
{
};

#if defined(BOOST_ASIO_HAS_DEDUCED_EXECUTION_IS_TYPED_SENDER_TRAIT)

template <
    template <
      template <typename...> class Tuple,
      template <typename...> class Variant
    > class>
struct has_value_types
{
  typedef void type;
};

template <
  template <
    template <typename...> class Variant
  > class>
struct has_error_types
{
  typedef void type;
};

template <typename S>
struct has_sender_types<S,
    typename has_value_types<S::template value_types>::type,
    typename has_error_types<S::template error_types>::type,
    typename conditional<S::sends_done, void, void>::type> : true_type
{
};

template <typename S>
struct sender_traits_base<S,
    typename enable_if<
      has_sender_types<S>::value
    >::type>
{
  template <
      template <typename...> class Tuple,
      template <typename...> class Variant>
  using value_types = typename S::template value_types<Tuple, Variant>;

  template <template <typename...> class Variant>
  using error_types = typename S::template error_types<Variant>;

  BOOST_ASIO_STATIC_CONSTEXPR(bool, sends_done = S::sends_done);
};

#endif // defined(BOOST_ASIO_HAS_DEDUCED_EXECUTION_IS_TYPED_SENDER_TRAIT)

template <typename S>
struct sender_traits_base<S,
    typename enable_if<
      !has_sender_types<S>::value
        && detail::is_executor_of_impl<S,
          as_invocable<void_receiver, S> >::value
    >::type>
{
#if defined(BOOST_ASIO_HAS_DEDUCED_EXECUTION_IS_TYPED_SENDER_TRAIT) \
  && defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)

  template <
      template <typename...> class Tuple,
      template <typename...> class Variant>
  using value_types = Variant<Tuple<>>;

  template <template <typename...> class Variant>
  using error_types = Variant<std::exception_ptr>;

  BOOST_ASIO_STATIC_CONSTEXPR(bool, sends_done = true);

#endif // defined(BOOST_ASIO_HAS_DEDUCED_EXECUTION_IS_TYPED_SENDER_TRAIT)
       //   && defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
};

} // namespace detail

/// Base class used for tagging senders.
#if defined(GENERATING_DOCUMENTATION)
typedef unspecified sender_base;
#else // defined(GENERATING_DOCUMENTATION)
typedef detail::sender_base_ns::sender_base sender_base;
#endif // defined(GENERATING_DOCUMENTATION)

/// Traits for senders.
template <typename S>
struct sender_traits
#if !defined(GENERATING_DOCUMENTATION)
  : detail::sender_traits_base<S>
#endif // !defined(GENERATING_DOCUMENTATION)
{
};

namespace detail {

template <typename S, typename = void>
struct has_sender_traits : true_type
{
};

template <typename S>
struct has_sender_traits<S,
    typename enable_if<
      is_same<
        typename boost::asio::execution::sender_traits<
          S>::asio_execution_sender_traits_base_is_unspecialised,
        void
      >::value
    >::type> : false_type
{
};

} // namespace detail

/// The is_sender trait detects whether a type T satisfies the
/// execution::sender concept.

/**
 * Class template @c is_sender is a type trait that is derived from @c
 * true_type if the type @c T meets the concept definition for a sender,
 * otherwise @c false_type.
 */
template <typename T>
struct is_sender :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  conditional<
    detail::has_sender_traits<typename remove_cvref<T>::type>::value,
    is_move_constructible<typename remove_cvref<T>::type>,
    false_type
  >::type
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
BOOST_ASIO_CONSTEXPR const bool is_sender_v = is_sender<T>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename T>
BOOST_ASIO_CONCEPT sender = is_sender<T>::value;

#define BOOST_ASIO_EXECUTION_SENDER ::boost::asio::execution::sender

#else // defined(BOOST_ASIO_HAS_CONCEPTS)

#define BOOST_ASIO_EXECUTION_SENDER typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename S, typename R>
struct can_connect;

/// The is_sender_to trait detects whether a type T satisfies the
/// execution::sender_to concept for some receiver.
/**
 * Class template @c is_sender_to is a type trait that is derived from @c
 * true_type if the type @c T meets the concept definition for a sender
 * for some receiver type R, otherwise @c false.
 */
template <typename T, typename R>
struct is_sender_to :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  integral_constant<bool,
    is_sender<T>::value
      && is_receiver<R>::value
      && can_connect<T, R>::value
  >
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename R>
BOOST_ASIO_CONSTEXPR const bool is_sender_to_v =
  is_sender_to<T, R>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename T, typename R>
BOOST_ASIO_CONCEPT sender_to = is_sender_to<T, R>::value;

#define BOOST_ASIO_EXECUTION_SENDER_TO(r) \
  ::boost::asio::execution::sender_to<r>

#else // defined(BOOST_ASIO_HAS_CONCEPTS)

#define BOOST_ASIO_EXECUTION_SENDER_TO(r) typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)

/// The is_typed_sender trait detects whether a type T satisfies the
/// execution::typed_sender concept.
/**
 * Class template @c is_typed_sender is a type trait that is derived from @c
 * true_type if the type @c T meets the concept definition for a typed sender,
 * otherwise @c false.
 */
template <typename T>
struct is_typed_sender :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  integral_constant<bool,
    is_sender<T>::value
      && detail::has_sender_types<
        sender_traits<typename remove_cvref<T>::type> >::value
  >
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
BOOST_ASIO_CONSTEXPR const bool is_typed_sender_v = is_typed_sender<T>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename T>
BOOST_ASIO_CONCEPT typed_sender = is_typed_sender<T>::value;

#define BOOST_ASIO_EXECUTION_TYPED_SENDER \
  ::boost::asio::execution::typed_sender

#else // defined(BOOST_ASIO_HAS_CONCEPTS)

#define BOOST_ASIO_EXECUTION_TYPED_SENDER typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)

} // namespace execution
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/execution/connect.hpp>

#endif // BOOST_ASIO_EXECUTION_SENDER_HPP
