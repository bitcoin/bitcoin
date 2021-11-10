//
// associated_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_ASSOCIATED_ALLOCATOR_HPP
#define BOOST_ASIO_ASSOCIATED_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <memory>
#include <boost/asio/associator.hpp>
#include <boost/asio/detail/functional.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename T, typename Allocator>
struct associated_allocator;

namespace detail {

template <typename T, typename = void>
struct has_allocator_type : false_type
{
};

template <typename T>
struct has_allocator_type<T,
  typename void_type<typename T::executor_type>::type>
    : true_type
{
};

template <typename T, typename E, typename = void, typename = void>
struct associated_allocator_impl
{
  typedef E type;

  static type get(const T&, const E& e) BOOST_ASIO_NOEXCEPT
  {
    return e;
  }
};

template <typename T, typename E>
struct associated_allocator_impl<T, E,
  typename void_type<typename T::allocator_type>::type>
{
  typedef typename T::allocator_type type;

  static type get(const T& t, const E&) BOOST_ASIO_NOEXCEPT
  {
    return t.get_allocator();
  }
};

template <typename T, typename E>
struct associated_allocator_impl<T, E,
  typename enable_if<
    !has_allocator_type<T>::value
  >::type,
  typename void_type<
    typename associator<associated_allocator, T, E>::type
  >::type> : associator<associated_allocator, T, E>
{
};

} // namespace detail

/// Traits type used to obtain the allocator associated with an object.
/**
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type. The template parameter @c
 * Allocator shall be a type meeting the Allocator requirements.
 *
 * Specialisations shall meet the following requirements, where @c t is a const
 * reference to an object of type @c T, and @c a is an object of type @c
 * Allocator.
 *
 * @li Provide a nested typedef @c type that identifies a type meeting the
 * Allocator requirements.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t) and with return type @c type.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t,a) and with return type @c type.
 */
template <typename T, typename Allocator = std::allocator<void> >
struct associated_allocator
{
  /// If @c T has a nested type @c allocator_type, <tt>T::allocator_type</tt>.
  /// Otherwise @c Allocator.
#if defined(GENERATING_DOCUMENTATION)
  typedef see_below type;
#else // defined(GENERATING_DOCUMENTATION)
  typedef typename detail::associated_allocator_impl<T, Allocator>::type type;
#endif // defined(GENERATING_DOCUMENTATION)

  /// If @c T has a nested type @c allocator_type, returns
  /// <tt>t.get_allocator()</tt>. Otherwise returns @c a.
  static type get(const T& t,
      const Allocator& a = Allocator()) BOOST_ASIO_NOEXCEPT
  {
    return detail::associated_allocator_impl<T, Allocator>::get(t, a);
  }
};

/// Helper function to obtain an object's associated allocator.
/**
 * @returns <tt>associated_allocator<T>::get(t)</tt>
 */
template <typename T>
inline typename associated_allocator<T>::type
get_associated_allocator(const T& t) BOOST_ASIO_NOEXCEPT
{
  return associated_allocator<T>::get(t);
}

/// Helper function to obtain an object's associated allocator.
/**
 * @returns <tt>associated_allocator<T, Allocator>::get(t, a)</tt>
 */
template <typename T, typename Allocator>
inline typename associated_allocator<T, Allocator>::type
get_associated_allocator(const T& t, const Allocator& a) BOOST_ASIO_NOEXCEPT
{
  return associated_allocator<T, Allocator>::get(t, a);
}

#if defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

template <typename T, typename Allocator = std::allocator<void> >
using associated_allocator_t
  = typename associated_allocator<T, Allocator>::type;

#endif // defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

#if defined(BOOST_ASIO_HAS_STD_REFERENCE_WRAPPER) \
  || defined(GENERATING_DOCUMENTATION)

/// Specialisation of associated_allocator for @c std::reference_wrapper.
template <typename T, typename Allocator>
struct associated_allocator<reference_wrapper<T>, Allocator>
{
  /// Forwards @c type to the associator specialisation for the unwrapped type
  /// @c T.
  typedef typename associated_allocator<T, Allocator>::type type;

  /// Forwards the request to get the allocator to the associator specialisation
  /// for the unwrapped type @c T.
  static type get(reference_wrapper<T> t,
      const Allocator& a = Allocator()) BOOST_ASIO_NOEXCEPT
  {
    return associated_allocator<T, Allocator>::get(t.get(), a);
  }
};

#endif // defined(BOOST_ASIO_HAS_STD_REFERENCE_WRAPPER)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_ASSOCIATED_ALLOCATOR_HPP
