//
// associated_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_ASSOCIATED_EXECUTOR_HPP
#define BOOST_ASIO_ASSOCIATED_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/associator.hpp>
#include <boost/asio/detail/functional.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/is_executor.hpp>
#include <boost/asio/system_executor.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename T, typename Executor>
struct associated_executor;

namespace detail {

template <typename T, typename = void>
struct has_executor_type : false_type
{
};

template <typename T>
struct has_executor_type<T,
  typename void_type<typename T::executor_type>::type>
    : true_type
{
};

template <typename T, typename E, typename = void, typename = void>
struct associated_executor_impl
{
  typedef void asio_associated_executor_is_unspecialised;

  typedef E type;

  static type get(const T&, const E& e = E()) BOOST_ASIO_NOEXCEPT
  {
    return e;
  }
};

template <typename T, typename E>
struct associated_executor_impl<T, E,
  typename void_type<typename T::executor_type>::type>
{
  typedef typename T::executor_type type;

  static type get(const T& t, const E& = E()) BOOST_ASIO_NOEXCEPT
  {
    return t.get_executor();
  }
};

template <typename T, typename E>
struct associated_executor_impl<T, E,
  typename enable_if<
    !has_executor_type<T>::value
  >::type,
  typename void_type<
    typename associator<associated_executor, T, E>::type
  >::type> : associator<associated_executor, T, E>
{
};

} // namespace detail

/// Traits type used to obtain the executor associated with an object.
/**
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type. The template parameter @c
 * Executor shall be a type meeting the Executor requirements.
 *
 * Specialisations shall meet the following requirements, where @c t is a const
 * reference to an object of type @c T, and @c e is an object of type @c
 * Executor.
 *
 * @li Provide a nested typedef @c type that identifies a type meeting the
 * Executor requirements.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t) and with return type @c type.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t,e) and with return type @c type.
 */
template <typename T, typename Executor = system_executor>
struct associated_executor
#if !defined(GENERATING_DOCUMENTATION)
  : detail::associated_executor_impl<T, Executor>
#endif // !defined(GENERATING_DOCUMENTATION)
{
#if defined(GENERATING_DOCUMENTATION)
  /// If @c T has a nested type @c executor_type, <tt>T::executor_type</tt>.
  /// Otherwise @c Executor.
  typedef see_below type;

  /// If @c T has a nested type @c executor_type, returns
  /// <tt>t.get_executor()</tt>. Otherwise returns @c ex.
  static type get(const T& t,
      const Executor& ex = Executor()) BOOST_ASIO_NOEXCEPT;
#endif // defined(GENERATING_DOCUMENTATION)
};

/// Helper function to obtain an object's associated executor.
/**
 * @returns <tt>associated_executor<T>::get(t)</tt>
 */
template <typename T>
inline typename associated_executor<T>::type
get_associated_executor(const T& t) BOOST_ASIO_NOEXCEPT
{
  return associated_executor<T>::get(t);
}

/// Helper function to obtain an object's associated executor.
/**
 * @returns <tt>associated_executor<T, Executor>::get(t, ex)</tt>
 */
template <typename T, typename Executor>
inline typename associated_executor<T, Executor>::type
get_associated_executor(const T& t, const Executor& ex,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return associated_executor<T, Executor>::get(t, ex);
}

/// Helper function to obtain an object's associated executor.
/**
 * @returns <tt>associated_executor<T, typename
 * ExecutionContext::executor_type>::get(t, ctx.get_executor())</tt>
 */
template <typename T, typename ExecutionContext>
inline typename associated_executor<T,
  typename ExecutionContext::executor_type>::type
get_associated_executor(const T& t, ExecutionContext& ctx,
    typename constraint<is_convertible<ExecutionContext&,
      execution_context&>::value>::type = 0) BOOST_ASIO_NOEXCEPT
{
  return associated_executor<T,
    typename ExecutionContext::executor_type>::get(t, ctx.get_executor());
}

#if defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

template <typename T, typename Executor = system_executor>
using associated_executor_t = typename associated_executor<T, Executor>::type;

#endif // defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

namespace detail {

template <typename T, typename E, typename = void>
struct associated_executor_forwarding_base
{
};

template <typename T, typename E>
struct associated_executor_forwarding_base<T, E,
    typename enable_if<
      is_same<
        typename associated_executor<T,
          E>::asio_associated_executor_is_unspecialised,
        void
      >::value
    >::type>
{
  typedef void asio_associated_executor_is_unspecialised;
};

} // namespace detail

#if defined(BOOST_ASIO_HAS_STD_REFERENCE_WRAPPER) \
  || defined(GENERATING_DOCUMENTATION)

/// Specialisation of associated_executor for @c std::reference_wrapper.
template <typename T, typename Executor>
struct associated_executor<reference_wrapper<T>, Executor>
#if !defined(GENERATING_DOCUMENTATION)
  : detail::associated_executor_forwarding_base<T, Executor>
#endif // !defined(GENERATING_DOCUMENTATION)
{
  /// Forwards @c type to the associator specialisation for the unwrapped type
  /// @c T.
  typedef typename associated_executor<T, Executor>::type type;

  /// Forwards the request to get the executor to the associator specialisation
  /// for the unwrapped type @c T.
  static type get(reference_wrapper<T> t,
      const Executor& ex = Executor()) BOOST_ASIO_NOEXCEPT
  {
    return associated_executor<T, Executor>::get(t.get(), ex);
  }
};

#endif // defined(BOOST_ASIO_HAS_STD_REFERENCE_WRAPPER)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_ASSOCIATED_EXECUTOR_HPP
