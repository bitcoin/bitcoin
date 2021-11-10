//
// execution/set_value.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_SET_VALUE_HPP
#define BOOST_ASIO_EXECUTION_SET_VALUE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/variadic_templates.hpp>
#include <boost/asio/traits/set_value_member.hpp>
#include <boost/asio/traits/set_value_free.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {
namespace execution {

/// A customisation point that delivers a value to a receiver.
/**
 * The name <tt>execution::set_value</tt> denotes a customisation point object.
 * The expression <tt>execution::set_value(R, Vs...)</tt> for some
 * subexpressions <tt>R</tt> and <tt>Vs...</tt> is expression-equivalent to:
 *
 * @li <tt>R.set_value(Vs...)</tt>, if that expression is valid. If the
 *   function selected does not send the value(s) <tt>Vs...</tt> to the receiver
 *   <tt>R</tt>'s value channel, the program is ill-formed with no diagnostic
 *   required.
 *
 * @li Otherwise, <tt>set_value(R, Vs...)</tt>, if that expression is valid,
 *   with overload resolution performed in a context that includes the
 *   declaration <tt>void set_value();</tt> and that does not include a
 *   declaration of <tt>execution::set_value</tt>. If the function selected by
 *   overload resolution does not send the value(s) <tt>Vs...</tt> to the
 *   receiver <tt>R</tt>'s value channel, the program is ill-formed with no
 *   diagnostic required.
 *
 * @li Otherwise, <tt>execution::set_value(R, Vs...)</tt> is ill-formed.
 */
inline constexpr unspecified set_value = unspecified;

/// A type trait that determines whether a @c set_value expression is
/// well-formed.
/**
 * Class template @c can_set_value is a trait that is derived from
 * @c true_type if the expression <tt>execution::set_value(std::declval<R>(),
 * std::declval<Vs>()...)</tt> is well formed; otherwise @c false_type.
 */
template <typename R, typename... Vs>
struct can_set_value :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio
} // namespace boost

#else // defined(GENERATING_DOCUMENTATION)

namespace boost_asio_execution_set_value_fn {

using boost::asio::decay;
using boost::asio::declval;
using boost::asio::enable_if;
using boost::asio::traits::set_value_free;
using boost::asio::traits::set_value_member;

void set_value();

enum overload_type
{
  call_member,
  call_free,
  ill_formed
};

template <typename R, typename Vs, typename = void, typename = void>
struct call_traits
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename R, typename Vs>
struct call_traits<R, Vs,
  typename enable_if<
    set_value_member<R, Vs>::is_valid
  >::type> :
  set_value_member<R, Vs>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename R, typename Vs>
struct call_traits<R, Vs,
  typename enable_if<
    !set_value_member<R, Vs>::is_valid
  >::type,
  typename enable_if<
    set_value_free<R, Vs>::is_valid
  >::type> :
  set_value_free<R, Vs>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

struct impl
{
#if defined(BOOST_ASIO_HAS_MOVE)

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R, typename... Vs>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void(Vs...)>::overload == call_member,
    typename call_traits<R, void(Vs...)>::result_type
  >::type
  operator()(R&& r, Vs&&... v) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R, void(Vs...)>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(R)(r).set_value(BOOST_ASIO_MOVE_CAST(Vs)(v)...);
  }

  template <typename R, typename... Vs>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void(Vs...)>::overload == call_free,
    typename call_traits<R, void(Vs...)>::result_type
  >::type
  operator()(R&& r, Vs&&... v) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R, void(Vs...)>::is_noexcept))
  {
    return set_value(BOOST_ASIO_MOVE_CAST(R)(r),
        BOOST_ASIO_MOVE_CAST(Vs)(v)...);
  }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void()>::overload == call_member,
    typename call_traits<R, void()>::result_type
  >::type
  operator()(R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R, void()>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(R)(r).set_value();
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void()>::overload == call_free,
    typename call_traits<R, void()>::result_type
  >::type
  operator()(R&& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R, void()>::is_noexcept))
  {
    return set_value(BOOST_ASIO_MOVE_CAST(R)(r));
  }

#define BOOST_ASIO_PRIVATE_SET_VALUE_CALL_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  BOOST_ASIO_CONSTEXPR typename enable_if< \
    call_traits<R, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload == call_member, \
    typename call_traits<R, void(BOOST_ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R&& r, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    BOOST_ASIO_NOEXCEPT_IF(( \
      call_traits<R, void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return BOOST_ASIO_MOVE_CAST(R)(r).set_value( \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  BOOST_ASIO_CONSTEXPR typename enable_if< \
    call_traits<R, void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload == call_free, \
    typename call_traits<R, void(BOOST_ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R&& r, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    BOOST_ASIO_NOEXCEPT_IF(( \
      call_traits<R, void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return set_value(BOOST_ASIO_MOVE_CAST(R)(r), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SET_VALUE_CALL_DEF)
#undef BOOST_ASIO_PRIVATE_SET_VALUE_CALL_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#else // defined(BOOST_ASIO_HAS_MOVE)

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R, typename... Vs>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void(const Vs&...)>::overload == call_member,
    typename call_traits<R&, void(const Vs&...)>::result_type
  >::type
  operator()(R& r, const Vs&... v) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R&, void(const Vs&...)>::is_noexcept))
  {
    return r.set_value(v...);
  }

  template <typename R, typename... Vs>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void(const Vs&...)>::overload == call_member,
    typename call_traits<const R&, void(const Vs&...)>::result_type
  >::type
  operator()(const R& r, const Vs&... v) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const R&, void(const Vs&...)>::is_noexcept))
  {
    return r.set_value(v...);
  }

  template <typename R, typename... Vs>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void(const Vs&...)>::overload == call_free,
    typename call_traits<R&, void(const Vs&...)>::result_type
  >::type
  operator()(R& r, const Vs&... v) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R&, void(const Vs&...)>::is_noexcept))
  {
    return set_value(r, v...);
  }

  template <typename R, typename... Vs>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void(const Vs&...)>::overload == call_free,
    typename call_traits<const R&, void(const Vs&...)>::result_type
  >::type
  operator()(const R& r, const Vs&... v) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const R&, void(const Vs&...)>::is_noexcept))
  {
    return set_value(r, v...);
  }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void()>::overload == call_member,
    typename call_traits<R&, void()>::result_type
  >::type
  operator()(R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R&, void()>::is_noexcept))
  {
    return r.set_value();
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void()>::overload == call_member,
    typename call_traits<const R&, void()>::result_type
  >::type
  operator()(const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const R&, void()>::is_noexcept))
  {
    return r.set_value();
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void()>::overload == call_free,
    typename call_traits<R&, void()>::result_type
  >::type
  operator()(R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<R&, void()>::is_noexcept))
  {
    return set_value(r);
  }

  template <typename R>
  BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void()>::overload == call_free,
    typename call_traits<const R&, void()>::result_type
  >::type
  operator()(const R& r) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<const R&, void()>::is_noexcept))
  {
    return set_value(r);
  }

#define BOOST_ASIO_PRIVATE_SET_VALUE_CALL_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  BOOST_ASIO_CONSTEXPR typename enable_if< \
    call_traits<R&, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload == call_member, \
    typename call_traits<R&, void(BOOST_ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R& r, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    BOOST_ASIO_NOEXCEPT_IF(( \
      call_traits<R&, void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return r.set_value(BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  BOOST_ASIO_CONSTEXPR typename enable_if< \
    call_traits<const R&, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload == call_member, \
    typename call_traits<const R&, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(const R& r, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    BOOST_ASIO_NOEXCEPT_IF(( \
      call_traits<const R&, void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return r.set_value(BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  BOOST_ASIO_CONSTEXPR typename enable_if< \
    call_traits<R&, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload == call_free, \
    typename call_traits<R&, void(BOOST_ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R& r, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    BOOST_ASIO_NOEXCEPT_IF(( \
      call_traits<R&, void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return set_value(r, BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  BOOST_ASIO_CONSTEXPR typename enable_if< \
    call_traits<const R&, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload == call_free, \
    typename call_traits<const R&, \
      void(BOOST_ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(const R& r, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    BOOST_ASIO_NOEXCEPT_IF(( \
      call_traits<const R&, void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return set_value(r, BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SET_VALUE_CALL_DEF)
#undef BOOST_ASIO_PRIVATE_SET_VALUE_CALL_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#endif // defined(BOOST_ASIO_HAS_MOVE)
};

template <typename T = impl>
struct static_instance
{
  static const T instance;
};

template <typename T>
const T static_instance<T>::instance = {};

} // namespace boost_asio_execution_set_value_fn
namespace boost {
namespace asio {
namespace execution {
namespace {

static BOOST_ASIO_CONSTEXPR const boost_asio_execution_set_value_fn::impl&
  set_value = boost_asio_execution_set_value_fn::static_instance<>::instance;

} // namespace

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename... Vs>
struct can_set_value :
  integral_constant<bool,
    boost_asio_execution_set_value_fn::call_traits<R, void(Vs...)>::overload !=
      boost_asio_execution_set_value_fn::ill_formed>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename... Vs>
constexpr bool can_set_value_v = can_set_value<R, Vs...>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename... Vs>
struct is_nothrow_set_value :
  integral_constant<bool,
    boost_asio_execution_set_value_fn::call_traits<R, void(Vs...)>::is_noexcept>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename... Vs>
constexpr bool is_nothrow_set_value_v
  = is_nothrow_set_value<R, Vs...>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct can_set_value;

template <typename R, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct is_nothrow_set_value;

template <typename R>
struct can_set_value<R> :
  integral_constant<bool,
    boost_asio_execution_set_value_fn::call_traits<R, void()>::overload !=
      boost_asio_execution_set_value_fn::ill_formed>
{
};

template <typename R>
struct is_nothrow_set_value<R> :
  integral_constant<bool,
    boost_asio_execution_set_value_fn::call_traits<R, void()>::is_noexcept>
{
};

#define BOOST_ASIO_PRIVATE_SET_VALUE_TRAITS_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct can_set_value<R, BOOST_ASIO_VARIADIC_TARGS(n)> : \
    integral_constant<bool, \
      boost_asio_execution_set_value_fn::call_traits<R, \
        void(BOOST_ASIO_VARIADIC_TARGS(n))>::overload != \
          boost_asio_execution_set_value_fn::ill_formed> \
  { \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct is_nothrow_set_value<R, BOOST_ASIO_VARIADIC_TARGS(n)> : \
    integral_constant<bool, \
      boost_asio_execution_set_value_fn::call_traits<R, \
        void(BOOST_ASIO_VARIADIC_TARGS(n))>::is_noexcept> \
  { \
  }; \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SET_VALUE_TRAITS_DEF)
#undef BOOST_ASIO_PRIVATE_SET_VALUE_TRAITS_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

} // namespace execution
} // namespace asio
} // namespace boost

#endif // defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_SET_VALUE_HPP
