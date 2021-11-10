//
// execution/bulk_execute.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_BULK_EXECUTE_HPP
#define BOOST_ASIO_EXECUTION_BULK_EXECUTE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/bulk_guarantee.hpp>
#include <boost/asio/execution/detail/bulk_sender.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/sender.hpp>
#include <boost/asio/traits/bulk_execute_member.hpp>
#include <boost/asio/traits/bulk_execute_free.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {
namespace execution {

/// A customisation point that creates a bulk sender.
/**
 * The name <tt>execution::bulk_execute</tt> denotes a customisation point
 * object. If <tt>is_convertible_v<N, size_t></tt> is true, then the expression
 * <tt>execution::bulk_execute(S, F, N)</tt> for some subexpressions
 * <tt>S</tt>, <tt>F</tt>, and <tt>N</tt> is expression-equivalent to:
 *
 * @li <tt>S.bulk_execute(F, N)</tt>, if that expression is valid. If the
 *   function selected does not execute <tt>N</tt> invocations of the function
 *   object <tt>F</tt> on the executor <tt>S</tt> in bulk with forward progress
 *   guarantee <tt>boost::asio::query(S, execution::bulk_guarantee)</tt>, and
 *   the result of that function does not model <tt>sender<void></tt>, the
 *   program is ill-formed with no diagnostic required.
 *
 * @li Otherwise, <tt>bulk_execute(S, F, N)</tt>, if that expression is valid,
 *   with overload resolution performed in a context that includes the
 *   declaration <tt>void bulk_execute();</tt> and that does not include a
 *   declaration of <tt>execution::bulk_execute</tt>. If the function selected
 *   by overload resolution does not execute <tt>N</tt> invocations of the
 *   function object <tt>F</tt> on the executor <tt>S</tt> in bulk with forward
 *   progress guarantee <tt>boost::asio::query(E,
 *   execution::bulk_guarantee)</tt>, and the result of that function does not
 *   model <tt>sender<void></tt>, the program is ill-formed with no diagnostic
 *   required.
 *
 * @li Otherwise, if the types <tt>F</tt> and
 *   <tt>executor_index_t<remove_cvref_t<S>></tt> model <tt>invocable</tt> and
 *   if <tt>boost::asio::query(S, execution::bulk_guarantee)</tt> equals
 *   <tt>execution::bulk_guarantee.unsequenced</tt>, then
 *
 *    - Evaluates <tt>DECAY_COPY(std::forward<decltype(F)>(F))</tt> on the
 *      calling thread to create a function object <tt>cf</tt>. [Note:
 *      Additional copies of <tt>cf</tt> may subsequently be created. --end
 *      note.]
 *
 *    - For each value of <tt>i</tt> in <tt>N</tt>, <tt>cf(i)</tt> (or copy of
 *      <tt>cf</tt>)) will be invoked at most once by an execution agent that is
 *      unique for each value of <tt>i</tt>.
 *
 *    - May block pending completion of one or more invocations of <tt>cf</tt>.
 *
 *    - Synchronizes with (C++Std [intro.multithread]) the invocations of
 *      <tt>cf</tt>.
 *
 * @li Otherwise, <tt>execution::bulk_execute(S, F, N)</tt> is ill-formed.
 */
inline constexpr unspecified bulk_execute = unspecified;

/// A type trait that determines whether a @c bulk_execute expression is
/// well-formed.
/**
 * Class template @c can_bulk_execute is a trait that is derived from @c
 * true_type if the expression <tt>execution::bulk_execute(std::declval<S>(),
 * std::declval<F>(), std::declval<N>)</tt> is well formed; otherwise @c
 * false_type.
 */
template <typename S, typename F, typename N>
struct can_bulk_execute :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio
} // namespace boost

#else // defined(GENERATING_DOCUMENTATION)

namespace boost_asio_execution_bulk_execute_fn {

using boost::asio::declval;
using boost::asio::enable_if;
using boost::asio::execution::bulk_guarantee_t;
using boost::asio::execution::detail::bulk_sender;
using boost::asio::execution::executor_index;
using boost::asio::execution::is_sender;
using boost::asio::is_convertible;
using boost::asio::is_same;
using boost::asio::remove_cvref;
using boost::asio::result_of;
using boost::asio::traits::bulk_execute_free;
using boost::asio::traits::bulk_execute_member;
using boost::asio::traits::static_require;

void bulk_execute();

enum overload_type
{
  call_member,
  call_free,
  adapter,
  ill_formed
};

template <typename S, typename Args, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct call_traits
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename S, typename F, typename N>
struct call_traits<S, void(F, N),
  typename enable_if<
    is_convertible<N, std::size_t>::value
  >::type,
  typename enable_if<
    bulk_execute_member<S, F, N>::is_valid
  >::type,
  typename enable_if<
    is_sender<
      typename bulk_execute_member<S, F, N>::result_type
    >::value
  >::type> :
  bulk_execute_member<S, F, N>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename S, typename F, typename N>
struct call_traits<S, void(F, N),
  typename enable_if<
    is_convertible<N, std::size_t>::value
  >::type,
  typename enable_if<
    !bulk_execute_member<S, F, N>::is_valid
  >::type,
  typename enable_if<
    bulk_execute_free<S, F, N>::is_valid
  >::type,
  typename enable_if<
    is_sender<
      typename bulk_execute_free<S, F, N>::result_type
    >::value
  >::type> :
  bulk_execute_free<S, F, N>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

template <typename S, typename F, typename N>
struct call_traits<S, void(F, N),
  typename enable_if<
    is_convertible<N, std::size_t>::value
  >::type,
  typename enable_if<
    !bulk_execute_member<S, F, N>::is_valid
  >::type,
  typename enable_if<
    !bulk_execute_free<S, F, N>::is_valid
  >::type,
  typename enable_if<
    is_sender<S>::value
  >::type,
  typename enable_if<
    is_same<
      typename result_of<
        F(typename executor_index<typename remove_cvref<S>::type>::type)
      >::type,
      typename result_of<
        F(typename executor_index<typename remove_cvref<S>::type>::type)
      >::type
    >::value
  >::type,
  typename enable_if<
    static_require<S, bulk_guarantee_t::unsequenced_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = adapter);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef bulk_sender<S, F, N> result_type;
};

struct impl
{
#if defined(BOOST_ASIO_HAS_MOVE)
  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(F, N)>::overload == call_member,
    typename call_traits<S, void(F, N)>::result_type
  >::type
  operator()(S&& s, F&& f, N&& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(F, N)>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(S)(s).bulk_execute(
        BOOST_ASIO_MOVE_CAST(F)(f), BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(F, N)>::overload == call_free,
    typename call_traits<S, void(F, N)>::result_type
  >::type
  operator()(S&& s, F&& f, N&& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(F, N)>::is_noexcept))
  {
    return bulk_execute(BOOST_ASIO_MOVE_CAST(S)(s),
        BOOST_ASIO_MOVE_CAST(F)(f), BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(F, N)>::overload == adapter,
    typename call_traits<S, void(F, N)>::result_type
  >::type
  operator()(S&& s, F&& f, N&& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(F, N)>::is_noexcept))
  {
    return typename call_traits<S, void(F, N)>::result_type(
        BOOST_ASIO_MOVE_CAST(S)(s), BOOST_ASIO_MOVE_CAST(F)(f),
        BOOST_ASIO_MOVE_CAST(N)(n));
  }
#else // defined(BOOST_ASIO_HAS_MOVE)
  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(const F&, const N&)>::overload == call_member,
    typename call_traits<S, void(const F&, const N&)>::result_type
  >::type
  operator()(S& s, const F& f, const N& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(const F&, const N&)>::is_noexcept))
  {
    return s.bulk_execute(BOOST_ASIO_MOVE_CAST(F)(f),
        BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(const F&, const N&)>::overload == call_member,
    typename call_traits<S, void(const F&, const N&)>::result_type
  >::type
  operator()(const S& s, const F& f, const N& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(const F&, const N&)>::is_noexcept))
  {
    return s.bulk_execute(BOOST_ASIO_MOVE_CAST(F)(f),
        BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(const F&, const N&)>::overload == call_free,
    typename call_traits<S, void(const F&, const N&)>::result_type
  >::type
  operator()(S& s, const F& f, const N& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(const F&, const N&)>::is_noexcept))
  {
    return bulk_execute(s, BOOST_ASIO_MOVE_CAST(F)(f),
        BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(const F&, const N&)>::overload == call_free,
    typename call_traits<S, void(const F&, const N&)>::result_type
  >::type
  operator()(const S& s, const F& f, const N& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(const F&, const N&)>::is_noexcept))
  {
    return bulk_execute(s, BOOST_ASIO_MOVE_CAST(F)(f),
        BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(const F&, const N&)>::overload == adapter,
    typename call_traits<S, void(const F&, const N&)>::result_type
  >::type
  operator()(S& s, const F& f, const N& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(const F&, const N&)>::is_noexcept))
  {
    return typename call_traits<S, void(const F&, const N&)>::result_type(
        s, BOOST_ASIO_MOVE_CAST(F)(f), BOOST_ASIO_MOVE_CAST(N)(n));
  }

  template <typename S, typename F, typename N>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(const F&, const N&)>::overload == adapter,
    typename call_traits<S, void(const F&, const N&)>::result_type
  >::type
  operator()(const S& s, const F& f, const N& n) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(const F&, const N&)>::is_noexcept))
  {
    return typename call_traits<S, void(const F&, const N&)>::result_type(
        s, BOOST_ASIO_MOVE_CAST(F)(f), BOOST_ASIO_MOVE_CAST(N)(n));
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

} // namespace boost_asio_execution_bulk_execute_fn
namespace boost {
namespace asio {
namespace execution {
namespace {

static BOOST_ASIO_CONSTEXPR
  const boost_asio_execution_bulk_execute_fn::impl& bulk_execute =
    boost_asio_execution_bulk_execute_fn::static_instance<>::instance;

} // namespace

template <typename S, typename F, typename N>
struct can_bulk_execute :
  integral_constant<bool,
    boost_asio_execution_bulk_execute_fn::call_traits<
      S, void(F, N)>::overload !=
        boost_asio_execution_bulk_execute_fn::ill_formed>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename F, typename N>
constexpr bool can_bulk_execute_v = can_bulk_execute<S, F, N>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename F, typename N>
struct is_nothrow_bulk_execute :
  integral_constant<bool,
    boost_asio_execution_bulk_execute_fn::call_traits<
      S, void(F, N)>::is_noexcept>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename F, typename N>
constexpr bool is_nothrow_bulk_execute_v
  = is_nothrow_bulk_execute<S, F, N>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename F, typename N>
struct bulk_execute_result
{
  typedef typename boost_asio_execution_bulk_execute_fn::call_traits<
      S, void(F, N)>::result_type type;
};

} // namespace execution
} // namespace asio
} // namespace boost

#endif // defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_BULK_EXECUTE_HPP
