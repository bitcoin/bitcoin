//
// associated_cancellation_slot.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_ASSOCIATED_CANCELLATION_SLOT_HPP
#define BOOST_ASIO_ASSOCIATED_CANCELLATION_SLOT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/associator.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename T, typename CancellationSlot>
struct associated_cancellation_slot;

namespace detail {

template <typename T, typename = void>
struct has_cancellation_slot_type : false_type
{
};

template <typename T>
struct has_cancellation_slot_type<T,
  typename void_type<typename T::cancellation_slot_type>::type>
    : true_type
{
};

template <typename T, typename S, typename = void, typename = void>
struct associated_cancellation_slot_impl
{
  typedef void asio_associated_cancellation_slot_is_unspecialised;

  typedef S type;

  static type get(const T&, const S& s = S()) BOOST_ASIO_NOEXCEPT
  {
    return s;
  }
};

template <typename T, typename S>
struct associated_cancellation_slot_impl<T, S,
  typename void_type<typename T::cancellation_slot_type>::type>
{
  typedef typename T::cancellation_slot_type type;

  static type get(const T& t, const S& = S()) BOOST_ASIO_NOEXCEPT
  {
    return t.get_cancellation_slot();
  }
};

template <typename T, typename S>
struct associated_cancellation_slot_impl<T, S,
  typename enable_if<
    !has_cancellation_slot_type<T>::value
  >::type,
  typename void_type<
    typename associator<associated_cancellation_slot, T, S>::type
  >::type> : associator<associated_cancellation_slot, T, S>
{
};

} // namespace detail

/// Traits type used to obtain the cancellation_slot associated with an object.
/**
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type. The template parameter @c
 * CancellationSlot shall be a type meeting the CancellationSlot requirements.
 *
 * Specialisations shall meet the following requirements, where @c t is a const
 * reference to an object of type @c T, and @c s is an object of type @c
 * CancellationSlot.
 *
 * @li Provide a nested typedef @c type that identifies a type meeting the
 * CancellationSlot requirements.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t) and with return type @c type.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t,s) and with return type @c type.
 */
template <typename T, typename CancellationSlot = cancellation_slot>
struct associated_cancellation_slot
#if !defined(GENERATING_DOCUMENTATION)
  : detail::associated_cancellation_slot_impl<T, CancellationSlot>
#endif // !defined(GENERATING_DOCUMENTATION)
{
#if defined(GENERATING_DOCUMENTATION)
  /// If @c T has a nested type @c cancellation_slot_type,
  /// <tt>T::cancellation_slot_type</tt>. Otherwise
  /// @c CancellationSlot.
  typedef see_below type;

  /// If @c T has a nested type @c cancellation_slot_type, returns
  /// <tt>t.get_cancellation_slot()</tt>. Otherwise returns @c s.
  static type get(const T& t,
      const CancellationSlot& s = CancellationSlot()) BOOST_ASIO_NOEXCEPT;
#endif // defined(GENERATING_DOCUMENTATION)
};

/// Helper function to obtain an object's associated cancellation_slot.
/**
 * @returns <tt>associated_cancellation_slot<T>::get(t)</tt>
 */
template <typename T>
inline typename associated_cancellation_slot<T>::type
get_associated_cancellation_slot(const T& t) BOOST_ASIO_NOEXCEPT
{
  return associated_cancellation_slot<T>::get(t);
}

/// Helper function to obtain an object's associated cancellation_slot.
/**
 * @returns <tt>associated_cancellation_slot<T,
 * CancellationSlot>::get(t, st)</tt>
 */
template <typename T, typename CancellationSlot>
inline typename associated_cancellation_slot<T, CancellationSlot>::type
get_associated_cancellation_slot(const T& t,
    const CancellationSlot& st) BOOST_ASIO_NOEXCEPT
{
  return associated_cancellation_slot<T, CancellationSlot>::get(t, st);
}

#if defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

template <typename T, typename CancellationSlot = cancellation_slot>
using associated_cancellation_slot_t =
  typename associated_cancellation_slot<T, CancellationSlot>::type;

#endif // defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

namespace detail {

template <typename T, typename S, typename = void>
struct associated_cancellation_slot_forwarding_base
{
};

template <typename T, typename S>
struct associated_cancellation_slot_forwarding_base<T, S,
    typename enable_if<
      is_same<
        typename associated_cancellation_slot<T,
          S>::asio_associated_cancellation_slot_is_unspecialised,
        void
      >::value
    >::type>
{
  typedef void asio_associated_cancellation_slot_is_unspecialised;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_ASSOCIATED_CANCELLATION_SLOT_HPP
