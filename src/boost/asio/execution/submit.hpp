//
// execution/submit.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_SUBMIT_HPP
#define BOOST_ASIO_EXECUTION_SUBMIT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/detail/submit_receiver.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/receiver.hpp>
#include <boost/asio/execution/sender.hpp>
#include <boost/asio/execution/start.hpp>
#include <boost/asio/traits/submit_member.hpp>
#include <boost/asio/traits/submit_free.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {
namespace execution {

/// A customisation point that submits a sender to a receiver.
/**
 * The name <tt>execution::submit</tt> denotes a customisation point object. For
 * some subexpressions <tt>s</tt> and <tt>r</tt>, let <tt>S</tt> be a type such
 * that <tt>decltype((s))</tt> is <tt>S</tt> and let <tt>R</tt> be a type such
 * that <tt>decltype((r))</tt> is <tt>R</tt>. The expression
 * <tt>execution::submit(s, r)</tt> is ill-formed if <tt>sender_to<S, R></tt> is
 * not <tt>true</tt>. Otherwise, it is expression-equivalent to:
 *
 * @li <tt>s.submit(r)</tt>, if that expression is valid and <tt>S</tt> models
 *   <tt>sender</tt>. If the function selected does not submit the receiver
 *   object <tt>r</tt> via the sender <tt>s</tt>, the program is ill-formed with
 *   no diagnostic required.
 *
 * @li Otherwise, <tt>submit(s, r)</tt>, if that expression is valid and
 *   <tt>S</tt> models <tt>sender</tt>, with overload resolution performed in a
 *   context that includes the declaration <tt>void submit();</tt> and that does
 *   not include a declaration of <tt>execution::submit</tt>. If the function
 *   selected by overload resolution does not submit the receiver object
 *   <tt>r</tt> via the sender <tt>s</tt>, the program is ill-formed with no
 *   diagnostic required.
 *
 * @li Otherwise, <tt>execution::start((new submit_receiver<S,
 *   R>{s,r})->state_)</tt>, where <tt>submit_receiver</tt> is an
 *   implementation-defined class template equivalent to:
 *   @code template<class S, class R>
 *   struct submit_receiver {
 *     struct wrap {
 *       submit_receiver * p_;
 *       template<class...As>
 *         requires receiver_of<R, As...>
 *       void set_value(As&&... as) &&
 *         noexcept(is_nothrow_receiver_of_v<R, As...>) {
 *         execution::set_value(std::move(p_->r_), (As&&) as...);
 *         delete p_;
 *       }
 *       template<class E>
 *         requires receiver<R, E>
 *       void set_error(E&& e) && noexcept {
 *         execution::set_error(std::move(p_->r_), (E&&) e);
 *         delete p_;
 *       }
 *       void set_done() && noexcept {
 *         execution::set_done(std::move(p_->r_));
 *         delete p_;
 *       }
 *     };
 *     remove_cvref_t<R> r_;
 *     connect_result_t<S, wrap> state_;
 *     submit_receiver(S&& s, R&& r)
 *       : r_((R&&) r)
 *       , state_(execution::connect((S&&) s, wrap{this})) {}
 *   };
 *   @endcode
 */
inline constexpr unspecified submit = unspecified;

/// A type trait that determines whether a @c submit expression is
/// well-formed.
/**
 * Class template @c can_submit is a trait that is derived from
 * @c true_type if the expression <tt>execution::submit(std::declval<R>(),
 * std::declval<E>())</tt> is well formed; otherwise @c false_type.
 */
template <typename S, typename R>
struct can_submit :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio
} // namespace boost

#else // defined(GENERATING_DOCUMENTATION)

namespace boost_asio_execution_submit_fn {

using boost::asio::declval;
using boost::asio::enable_if;
using boost::asio::execution::is_sender_to;
using boost::asio::traits::submit_free;
using boost::asio::traits::submit_member;

void submit();

enum overload_type
{
  call_member,
  call_free,
  adapter,
  ill_formed
};

template <typename S, typename R, typename = void,
    typename = void, typename = void>
struct call_traits
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename S, typename R>
struct call_traits<S, void(R),
  typename enable_if<
    submit_member<S, R>::is_valid
  >::type,
  typename enable_if<
    is_sender_to<S, R>::value
  >::type> :
  submit_member<S, R>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename S, typename R>
struct call_traits<S, void(R),
  typename enable_if<
    !submit_member<S, R>::is_valid
  >::type,
  typename enable_if<
    submit_free<S, R>::is_valid
  >::type,
  typename enable_if<
    is_sender_to<S, R>::value
  >::type> :
  submit_free<S, R>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

template <typename S, typename R>
struct call_traits<S, void(R),
  typename enable_if<
    !submit_member<S, R>::is_valid
  >::type,
  typename enable_if<
    !submit_free<S, R>::is_valid
  >::type,
  typename enable_if<
    is_sender_to<S, R>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = adapter);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

struct impl
{
#if defined(BOOST_ASIO_HAS_MOVE)
  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(R)>::overload == call_member,
    typename call_traits<S, void(R)>::result_type
  >::type
  operator()(S&& s, R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(R)>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(S)(s).submit(BOOST_ASIO_MOVE_CAST(R)(r));
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(R)>::overload == call_free,
    typename call_traits<S, void(R)>::result_type
  >::type
  operator()(S&& s, R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(R)>::is_noexcept))
  {
    return submit(BOOST_ASIO_MOVE_CAST(S)(s), BOOST_ASIO_MOVE_CAST(R)(r));
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S, void(R)>::overload == adapter,
    typename call_traits<S, void(R)>::result_type
  >::type
  operator()(S&& s, R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S, void(R)>::is_noexcept))
  {
    return boost::asio::execution::start(
        (new boost::asio::execution::detail::submit_receiver<S, R>(
          BOOST_ASIO_MOVE_CAST(S)(s), BOOST_ASIO_MOVE_CAST(R)(r)))->state_);
  }
#else // defined(BOOST_ASIO_HAS_MOVE)
  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&, void(R&)>::overload == call_member,
    typename call_traits<S&, void(R&)>::result_type
  >::type
  operator()(S& s, R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&, void(R&)>::is_noexcept))
  {
    return s.submit(r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&, void(R&)>::overload == call_member,
    typename call_traits<const S&, void(R&)>::result_type
  >::type
  operator()(const S& s, R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&, void(R&)>::is_noexcept))
  {
    return s.submit(r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&, void(R&)>::overload == call_free,
    typename call_traits<S&, void(R&)>::result_type
  >::type
  operator()(S& s, R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&, void(R&)>::is_noexcept))
  {
    return submit(s, r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&, void(R&)>::overload == call_free,
    typename call_traits<const S&, void(R&)>::result_type
  >::type
  operator()(const S& s, R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&, void(R&)>::is_noexcept))
  {
    return submit(s, r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&, void(R&)>::overload == adapter,
    typename call_traits<S&, void(R&)>::result_type
  >::type
  operator()(S& s, R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&, void(R&)>::is_noexcept))
  {
    return boost::asio::execution::start(
        (new boost::asio::execution::detail::submit_receiver<
          S&, R&>(s, r))->state_);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&, void(R&)>::overload == adapter,
    typename call_traits<const S&, void(R&)>::result_type
  >::type
  operator()(const S& s, R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&, void(R&)>::is_noexcept))
  {
    boost::asio::execution::start(
        (new boost::asio::execution::detail::submit_receiver<
          const S&, R&>(s, r))->state_);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&, void(const R&)>::overload == call_member,
    typename call_traits<S&, void(const R&)>::result_type
  >::type
  operator()(S& s, const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&, void(const R&)>::is_noexcept))
  {
    return s.submit(r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&, void(const R&)>::overload == call_member,
    typename call_traits<const S&, void(const R&)>::result_type
  >::type
  operator()(const S& s, const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&, void(const R&)>::is_noexcept))
  {
    return s.submit(r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&, void(const R&)>::overload == call_free,
    typename call_traits<S&, void(const R&)>::result_type
  >::type
  operator()(S& s, const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&, void(const R&)>::is_noexcept))
  {
    return submit(s, r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&, void(const R&)>::overload == call_free,
    typename call_traits<const S&, void(const R&)>::result_type
  >::type
  operator()(const S& s, const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&, void(const R&)>::is_noexcept))
  {
    return submit(s, r);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<S&, void(const R&)>::overload == adapter,
    typename call_traits<S&, void(const R&)>::result_type
  >::type
  operator()(S& s, const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<S&, void(const R&)>::is_noexcept))
  {
    boost::asio::execution::start(
        (new boost::asio::execution::detail::submit_receiver<
          S&, const R&>(s, r))->state_);
  }

  template <typename S, typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&, void(const R&)>::overload == adapter,
    typename call_traits<const S&, void(const R&)>::result_type
  >::type
  operator()(const S& s, const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const S&, void(const R&)>::is_noexcept))
  {
    boost::asio::execution::start(
        (new boost::asio::execution::detail::submit_receiver<
          const S&, const R&>(s, r))->state_);
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

} // namespace boost_asio_execution_submit_fn
namespace boost {
namespace asio {
namespace execution {
namespace {

static BOOST_ASIO_CONSTEXPR const boost_asio_execution_submit_fn::impl&
  submit = boost_asio_execution_submit_fn::static_instance<>::instance;

} // namespace

template <typename S, typename R>
struct can_submit :
  integral_constant<bool,
    boost_asio_execution_submit_fn::call_traits<S, void(R)>::overload !=
      boost_asio_execution_submit_fn::ill_formed>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename R>
constexpr bool can_submit_v = can_submit<S, R>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename R>
struct is_nothrow_submit :
  integral_constant<bool,
    boost_asio_execution_submit_fn::call_traits<S, void(R)>::is_noexcept>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename R>
constexpr bool is_nothrow_submit_v
  = is_nothrow_submit<S, R>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S, typename R>
struct submit_result
{
  typedef typename boost_asio_execution_submit_fn::call_traits<
      S, void(R)>::result_type type;
};

namespace detail {

template <typename S, typename R>
void submit_helper(BOOST_ASIO_MOVE_ARG(S) s, BOOST_ASIO_MOVE_ARG(R) r)
{
  execution::submit(BOOST_ASIO_MOVE_CAST(S)(s), BOOST_ASIO_MOVE_CAST(R)(r));
}

} // namespace detail
} // namespace execution
} // namespace asio
} // namespace boost

#endif // defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_SUBMIT_HPP
