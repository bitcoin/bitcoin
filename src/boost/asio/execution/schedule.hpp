//
// execution/schedule.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_SCHEDULE_HPP
#define BOOST_ASIO_EXECUTION_SCHEDULE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/traits/schedule_member.hpp>
#include <boost/asio/traits/schedule_free.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {
namespace execution {

/// A customisation point that is used to obtain a sender from a scheduler.
/**
 * The name <tt>execution::schedule</tt> denotes a customisation point object.
 * For some subexpression <tt>s</tt>, let <tt>S</tt> be a type such that
 * <tt>decltype((s))</tt> is <tt>S</tt>. The expression
 * <tt>execution::schedule(s)</tt> is expression-equivalent to:
 *
 * @li <tt>s.schedule()</tt>, if that expression is valid and its type models
 *   <tt>sender</tt>.
 *
 * @li Otherwise, <tt>schedule(s)</tt>, if that expression is valid and its
 *   type models <tt>sender</tt> with overload resolution performed in a context
 *   that includes the declaration <tt>void schedule();</tt> and that does not
 *   include a declaration of <tt>execution::schedule</tt>.
 *
 * @li Otherwise, <tt>S</tt> if <tt>S</tt> satisfies <tt>executor</tt>.
 *
 * @li Otherwise, <tt>execution::schedule(s)</tt> is ill-formed.
 */
inline constexpr unspecified schedule = unspecified;

/// A type trait that determines whether a @c schedule expression is
/// well-formed.
/**
 * Class template @c can_schedule is a trait that is derived from @c true_type
 * if the expression <tt>execution::schedule(std::declval<S>())</tt> is well
 * formed; otherwise @c false_type.
 */
template <typename S>
struct can_schedule :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio
} // namespace boost

#else // defined(GENERATING_DOCUMENTATION)

namespace boost_asio_execution_schedule_fn {

using boost::asio::decay;
using boost::asio::declval;
using boost::asio::enable_if;
using boost::asio::execution::is_executor;
using boost::asio::traits::schedule_free;
using boost::asio::traits::schedule_member;

void schedule();

enum overload_type
{
  identity,
  call_member,
  call_free,
  ill_formed
};

template <typename S, typename = void, typename = void, typename = void>
struct call_traits
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename S>
struct call_traits<S,
  typename enable_if<
    schedule_member<S>::is_valid
  >::type> :
  schedule_member<S>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename S>
struct call_traits<S,
  typename enable_if<
    !schedule_member<S>::is_valid
  >::type,
  typename enable_if<
    schedule_free<S>::is_valid
  >::type> :
  schedule_free<S>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

template <typename S>
struct call_traits<S,
  typename enable_if<
    !schedule_member<S>::is_valid
  >::type,
  typename enable_if<
    !schedule_free<S>::is_valid
  >::type,
  typename enable_if<
    is_executor<typename decay<S>::type>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = identity);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

#if defined(BOOST_ASIO_HAS_MOVE)
  typedef BOOST_ASIO_MOVE_ARG(S) result_type;
#else // defined(BOOST_ASIO_HAS_MOVE)
  typedef BOOST_ASIO_MOVE_ARG(typename decay<S>::type) result_type;
#endif // defined(BOOST_ASIO_HAS_MOVE)
};

struct impl
{
  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S>::overload == identity,
    typename call_traits<S>::result_type
  >::type
  operator()(BOOST_ASIO_MOVE_ARG(S) s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(S)(s);
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S>::overload == call_member,
    typename call_traits<S>::result_type
  >::type
  operator()(S&& s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(S)(s).schedule();
  }

  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S>::overload == call_free,
    typename call_traits<S>::result_type
  >::type
  operator()(S&& s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S>::is_noexcept))
  {
    return schedule(BOOST_ASIO_MOVE_CAST(S)(s));
  }
#else // defined(BOOST_ASIO_HAS_MOVE)
  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&>::overload == call_member,
    typename call_traits<S&>::result_type
  >::type
  operator()(S& s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&>::is_noexcept))
  {
    return s.schedule();
  }

  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&>::overload == call_member,
    typename call_traits<const S&>::result_type
  >::type
  operator()(const S& s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&>::is_noexcept))
  {
    return s.schedule();
  }

  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&>::overload == call_free,
    typename call_traits<S&>::result_type
  >::type
  operator()(S& s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&>::is_noexcept))
  {
    return schedule(s);
  }

  template <typename S>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&>::overload == call_free,
    typename call_traits<const S&>::result_type
  >::type
  operator()(const S& s) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&>::is_noexcept))
  {
    return schedule(s);
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

} // namespace boost_asio_execution_schedule_fn
namespace boost {
namespace asio {
namespace execution {
namespace {

static BOOST_ASIO_CONSTEXPR const boost_asio_execution_schedule_fn::impl&
  schedule = boost_asio_execution_schedule_fn::static_instance<>::instance;

} // namespace

template <typename S>
struct can_schedule :
  integral_constant<bool,
    boost_asio_execution_schedule_fn::call_traits<S>::overload !=
      boost_asio_execution_schedule_fn::ill_formed>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S>
constexpr bool can_schedule_v = can_schedule<S>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S>
struct is_nothrow_schedule :
  integral_constant<bool,
    boost_asio_execution_schedule_fn::call_traits<S>::is_noexcept>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S>
constexpr bool is_nothrow_schedule_v
  = is_nothrow_schedule<S>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace execution
} // namespace asio
} // namespace boost

#endif // defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_SCHEDULE_HPP
