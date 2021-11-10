//
// execution/set_done.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_SET_DONE_HPP
#define BOOST_ASIO_EXECUTION_SET_DONE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/traits/set_done_member.hpp>
#include <boost/asio/traits/set_done_free.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {
namespace execution {

/// A customisation point that delivers a done notification to a receiver.
/**
 * The name <tt>execution::set_done</tt> denotes a customisation point object.
 * The expression <tt>execution::set_done(R)</tt> for some subexpression
 * <tt>R</tt> is expression-equivalent to:
 *
 * @li <tt>R.set_done()</tt>, if that expression is valid. If the function
 *   selected does not signal the receiver <tt>R</tt>'s done channel, the
 *   program is ill-formed with no diagnostic required.
 *
 * @li Otherwise, <tt>set_done(R)</tt>, if that expression is valid, with
 * overload resolution performed in a context that includes the declaration
 * <tt>void set_done();</tt> and that does not include a declaration of
 * <tt>execution::set_done</tt>. If the function selected by overload
 * resolution does not signal the receiver <tt>R</tt>'s done channel, the
 * program is ill-formed with no diagnostic required.
 *
 * @li Otherwise, <tt>execution::set_done(R)</tt> is ill-formed.
 */
inline constexpr unspecified set_done = unspecified;

/// A type trait that determines whether a @c set_done expression is
/// well-formed.
/**
 * Class template @c can_set_done is a trait that is derived from
 * @c true_type if the expression <tt>execution::set_done(std::declval<R>(),
 * std::declval<E>())</tt> is well formed; otherwise @c false_type.
 */
template <typename R>
struct can_set_done :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio
} // namespace boost

#else // defined(GENERATING_DOCUMENTATION)

namespace boost_asio_execution_set_done_fn {

using boost::asio::decay;
using boost::asio::declval;
using boost::asio::enable_if;
using boost::asio::traits::set_done_free;
using boost::asio::traits::set_done_member;

void set_done();

enum overload_type
{
  call_member,
  call_free,
  ill_formed
};

template <typename R, typename = void, typename = void>
struct call_traits
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename R>
struct call_traits<R,
  typename enable_if<
    set_done_member<R>::is_valid
  >::type> :
  set_done_member<R>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename R>
struct call_traits<R,
  typename enable_if<
    !set_done_member<R>::is_valid
  >::type,
  typename enable_if<
    set_done_free<R>::is_valid
  >::type> :
  set_done_free<R>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

struct impl
{
#if defined(BOOST_ASIO_HAS_MOVE)
  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R>::overload == call_member,
    typename call_traits<R>::result_type
  >::type
  operator()(R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(R)(r).set_done();
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R>::overload == call_free,
    typename call_traits<R>::result_type
  >::type
  operator()(R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R>::is_noexcept))
  {
    return set_done(BOOST_ASIO_MOVE_CAST(R)(r));
  }
#else // defined(BOOST_ASIO_HAS_MOVE)
  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R&>::overload == call_member,
    typename call_traits<R&>::result_type
  >::type
  operator()(R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R&>::is_noexcept))
  {
    return r.set_done();
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&>::overload == call_member,
    typename call_traits<const R&>::result_type
  >::type
  operator()(const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const R&>::is_noexcept))
  {
    return r.set_done();
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R&>::overload == call_free,
    typename call_traits<R&>::result_type
  >::type
  operator()(R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R&>::is_noexcept))
  {
    return set_done(r);
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&>::overload == call_free,
    typename call_traits<const R&>::result_type
  >::type
  operator()(const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const R&>::is_noexcept))
  {
    return set_done(r);
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)
};

template <typename T = impl>
struct static_instance
{
  static const T instance;
};

template <typename T>
const T static_instance<T>::instance = {};

} // namespace boost_asio_execution_set_done_fn
namespace boost {
namespace asio {
namespace execution {
namespace {

static BOOST_ASIO_CONSTEXPR const boost_asio_execution_set_done_fn::impl&
  set_done = boost_asio_execution_set_done_fn::static_instance<>::instance;

} // namespace

template <typename R>
struct can_set_done :
  integral_constant<bool,
    boost_asio_execution_set_done_fn::call_traits<R>::overload !=
      boost_asio_execution_set_done_fn::ill_formed>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R>
constexpr bool can_set_done_v = can_set_done<R>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R>
struct is_nothrow_set_done :
  integral_constant<bool,
    boost_asio_execution_set_done_fn::call_traits<R>::is_noexcept>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R>
constexpr bool is_nothrow_set_done_v
  = is_nothrow_set_done<R>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace execution
} // namespace asio
} // namespace boost

#endif // defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_SET_DONE_HPP
