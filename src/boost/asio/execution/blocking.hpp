//
// execution/blocking.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_BLOCKING_HPP
#define BOOST_ASIO_EXECUTION_BLOCKING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/scheduler.hpp>
#include <boost/asio/execution/sender.hpp>
#include <boost/asio/is_applicable_property.hpp>
#include <boost/asio/prefer.hpp>
#include <boost/asio/query.hpp>
#include <boost/asio/require.hpp>
#include <boost/asio/traits/query_free.hpp>
#include <boost/asio/traits/query_member.hpp>
#include <boost/asio/traits/query_static_constexpr_member.hpp>
#include <boost/asio/traits/static_query.hpp>
#include <boost/asio/traits/static_require.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// A property to describe what guarantees an executor makes about the blocking
/// behaviour of their execution functions.
struct blocking_t
{
  /// The blocking_t property applies to executors, senders, and schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The top-level blocking_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level blocking_t property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef blocking_t polymorphic_query_result_type;

  /// A sub-property that indicates that invocation of an executor's execution
  /// function may block pending completion of one or more invocations of the
  /// submitted function object.
  struct possibly_t
  {
    /// The blocking_t::possibly_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_t::possibly_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_t::possibly_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr possibly_t();

    /// Get the value associated with a property object.
    /**
     * @returns possibly_t();
     */
    static constexpr blocking_t value();
  };

  /// A sub-property that indicates that invocation of an executor's execution
  /// function shall block until completion of all invocations of the submitted
  /// function object.
  struct always_t
  {
    /// The blocking_t::always_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_t::always_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_t::always_t property can be preferred.
    static constexpr bool is_preferable = false;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr always_t();

    /// Get the value associated with a property object.
    /**
     * @returns always_t();
     */
    static constexpr blocking_t value();
  };

  /// A sub-property that indicates that invocation of an executor's execution
  /// function shall not block pending completion of the invocations of the
  /// submitted function object.
  struct never_t
  {
    /// The blocking_t::never_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_t::never_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_t::never_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr never_t();

    /// Get the value associated with a property object.
    /**
     * @returns never_t();
     */
    static constexpr blocking_t value();
  };

  /// A special value used for accessing the blocking_t::possibly_t property.
  static constexpr possibly_t possibly;

  /// A special value used for accessing the blocking_t::always_t property.
  static constexpr always_t always;

  /// A special value used for accessing the blocking_t::never_t property.
  static constexpr never_t never;

  /// Default constructor.
  constexpr blocking_t();

  /// Construct from a sub-property value.
  constexpr blocking_t(possibly_t);

  /// Construct from a sub-property value.
  constexpr blocking_t(always_t);

  /// Construct from a sub-property value.
  constexpr blocking_t(never_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(
      const blocking_t& a, const blocking_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(
      const blocking_t& a, const blocking_t& b) noexcept;
};

/// A special value used for accessing the blocking_t property.
constexpr blocking_t blocking;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace blocking {

template <int I> struct possibly_t;
template <int I> struct always_t;
template <int I> struct never_t;

} // namespace blocking
namespace blocking_adaptation {

template <int I> struct allowed_t;

template <typename Executor, typename Function>
void blocking_execute(
    BOOST_ASIO_MOVE_ARG(Executor) ex,
    BOOST_ASIO_MOVE_ARG(Function) func);

} // namespace blocking_adaptation

template <int I = 0>
struct blocking_t
{
#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  BOOST_ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_requirable = false);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_preferable = false);
  typedef blocking_t polymorphic_query_result_type;

  typedef detail::blocking::possibly_t<I> possibly_t;
  typedef detail::blocking::always_t<I> always_t;
  typedef detail::blocking::never_t<I> never_t;

  BOOST_ASIO_CONSTEXPR blocking_t()
    : value_(-1)
  {
  }

  BOOST_ASIO_CONSTEXPR blocking_t(possibly_t)
    : value_(0)
  {
  }

  BOOST_ASIO_CONSTEXPR blocking_t(always_t)
    : value_(1)
  {
  }

  BOOST_ASIO_CONSTEXPR blocking_t(never_t)
    : value_(2)
  {
  }

  template <typename T>
  struct proxy
  {
#if defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
    struct type
    {
      template <typename P>
      auto query(BOOST_ASIO_MOVE_ARG(P) p) const
        noexcept(
          noexcept(
            declval<typename conditional<true, T, P>::type>().query(
              BOOST_ASIO_MOVE_CAST(P)(p))
          )
        )
        -> decltype(
          declval<typename conditional<true, T, P>::type>().query(
            BOOST_ASIO_MOVE_CAST(P)(p))
        );
    };
#else // defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
    typedef T type;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
  };

  template <typename T>
  struct static_proxy
  {
#if defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
    struct type
    {
      template <typename P>
      static constexpr auto query(BOOST_ASIO_MOVE_ARG(P) p)
        noexcept(
          noexcept(
            conditional<true, T, P>::type::query(BOOST_ASIO_MOVE_CAST(P)(p))
          )
        )
        -> decltype(
          conditional<true, T, P>::type::query(BOOST_ASIO_MOVE_CAST(P)(p))
        )
      {
        return T::query(BOOST_ASIO_MOVE_CAST(P)(p));
      }
    };
#else // defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
    typedef T type;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
  };

  template <typename T>
  struct query_member :
    traits::query_member<typename proxy<T>::type, blocking_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, blocking_t> {};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    BOOST_ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, possibly_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, possibly_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, possibly_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, always_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, possibly_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, always_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, always_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, never_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, possibly_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, always_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, never_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, never_t>::value();
  }

  template <typename E, typename T = decltype(blocking_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = blocking_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const blocking_t& a, const blocking_t& b)
  {
    return a.value_ == b.value_;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const blocking_t& a, const blocking_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_blocking_t
  {
    BOOST_ASIO_CONSTEXPR convertible_from_blocking_t(blocking_t) {}
  };

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR blocking_t query(
      const Executor& ex, convertible_from_blocking_t,
      typename enable_if<
        can_query<const Executor&, possibly_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, blocking_t<>::possibly_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, possibly_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, possibly_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR blocking_t query(
      const Executor& ex, convertible_from_blocking_t,
      typename enable_if<
        !can_query<const Executor&, possibly_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, always_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, blocking_t<>::always_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, always_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, always_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR blocking_t query(
      const Executor& ex, convertible_from_blocking_t,
      typename enable_if<
        !can_query<const Executor&, possibly_t>::value
      >::type* = 0,
      typename enable_if<
        !can_query<const Executor&, always_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, never_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, blocking_t<>::never_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, never_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, never_t());
  }

  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(possibly_t, possibly);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(always_t, always);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(never_t, never);

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
  static const blocking_t instance;
#endif // !defined(BOOST_ASIO_HAS_CONSTEXPR)

private:
  int value_;
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T blocking_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
template <int I>
const blocking_t<I> blocking_t<I>::instance;
#endif

template <int I>
const typename blocking_t<I>::possibly_t blocking_t<I>::possibly;

template <int I>
const typename blocking_t<I>::always_t blocking_t<I>::always;

template <int I>
const typename blocking_t<I>::never_t blocking_t<I>::never;

namespace blocking {

template <int I = 0>
struct possibly_t
{
#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  BOOST_ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_preferable = true);
  typedef blocking_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR possibly_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_t<I>::template proxy<T>::type, possibly_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_t<I>::template static_proxy<T>::type, possibly_t> {};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    BOOST_ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR possibly_t static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::query_free<T, possibly_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !can_query<T, always_t<I> >::value
      >::type* = 0,
      typename enable_if<
        !can_query<T, never_t<I> >::value
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return possibly_t();
  }

  template <typename E, typename T = decltype(possibly_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = possibly_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR blocking_t<I> value()
  {
    return possibly_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const possibly_t&, const possibly_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const possibly_t&, const possibly_t&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const possibly_t&, const always_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const possibly_t&, const always_t<I>&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const possibly_t&, const never_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const possibly_t&, const never_t<I>&)
  {
    return true;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T possibly_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename Executor>
class adapter
{
public:
  adapter(int, const Executor& e) BOOST_ASIO_NOEXCEPT
    : executor_(e)
  {
  }

  adapter(const adapter& other) BOOST_ASIO_NOEXCEPT
    : executor_(other.executor_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  adapter(adapter&& other) BOOST_ASIO_NOEXCEPT
    : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  template <int I>
  static BOOST_ASIO_CONSTEXPR always_t<I> query(
      blocking_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <int I>
  static BOOST_ASIO_CONSTEXPR always_t<I> query(
      possibly_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <int I>
  static BOOST_ASIO_CONSTEXPR always_t<I> query(
      always_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <int I>
  static BOOST_ASIO_CONSTEXPR always_t<I> query(
      never_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return always_t<I>();
  }

  template <typename Property>
  typename enable_if<
    can_query<const Executor&, Property>::value,
    typename query_result<const Executor&, Property>::type
  >::type query(const Property& p) const
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, Property>::value))
  {
    return boost::asio::query(executor_, p);
  }

  template <int I>
  typename enable_if<
    can_require<const Executor&, possibly_t<I> >::value,
    typename require_result<const Executor&, possibly_t<I> >::type
  >::type require(possibly_t<I>) const BOOST_ASIO_NOEXCEPT
  {
    return boost::asio::require(executor_, possibly_t<I>());
  }

  template <int I>
  typename enable_if<
    can_require<const Executor&, never_t<I> >::value,
    typename require_result<const Executor&, never_t<I> >::type
  >::type require(never_t<I>) const BOOST_ASIO_NOEXCEPT
  {
    return boost::asio::require(executor_, never_t<I>());
  }

  template <typename Property>
  typename enable_if<
    can_require<const Executor&, Property>::value,
    adapter<typename decay<
      typename require_result<const Executor&, Property>::type
    >::type>
  >::type require(const Property& p) const
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_require<const Executor&, Property>::value))
  {
    return adapter<typename decay<
      typename require_result<const Executor&, Property>::type
        >::type>(0, boost::asio::require(executor_, p));
  }

  template <typename Property>
  typename enable_if<
    can_prefer<const Executor&, Property>::value,
    adapter<typename decay<
      typename prefer_result<const Executor&, Property>::type
    >::type>
  >::type prefer(const Property& p) const
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_prefer<const Executor&, Property>::value))
  {
    return adapter<typename decay<
      typename prefer_result<const Executor&, Property>::type
        >::type>(0, boost::asio::prefer(executor_, p));
  }

  template <typename Function>
  typename enable_if<
    execution::can_execute<const Executor&, Function>::value
  >::type execute(BOOST_ASIO_MOVE_ARG(Function) f) const
  {
    blocking_adaptation::blocking_execute(
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f));
  }

  friend bool operator==(const adapter& a, const adapter& b) BOOST_ASIO_NOEXCEPT
  {
    return a.executor_ == b.executor_;
  }

  friend bool operator!=(const adapter& a, const adapter& b) BOOST_ASIO_NOEXCEPT
  {
    return a.executor_ != b.executor_;
  }

private:
  Executor executor_;
};

template <int I = 0>
struct always_t
{
#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  BOOST_ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_preferable = false);
  typedef blocking_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR always_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_t<I>::template proxy<T>::type, always_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_t<I>::template static_proxy<T>::type, always_t> {};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    BOOST_ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename E, typename T = decltype(always_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = always_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR blocking_t<I> value()
  {
    return always_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const always_t&, const always_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const always_t&, const always_t&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const always_t&, const possibly_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const always_t&, const possibly_t<I>&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const always_t&, const never_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const always_t&, const never_t<I>&)
  {
    return true;
  }

  template <typename Executor>
  friend adapter<Executor> require(
      const Executor& e, const always_t&,
      typename enable_if<
        is_executor<Executor>::value
      >::type* = 0,
      typename enable_if<
        traits::static_require<
          const Executor&,
          blocking_adaptation::allowed_t<0>
        >::is_valid
      >::type* = 0)
  {
    return adapter<Executor>(0, e);
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T always_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I>
struct never_t
{
#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)
  template <typename T>
  BOOST_ASIO_STATIC_CONSTEXPR(bool,
    is_applicable_property_v = (
      is_executor<T>::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_sender<T>
          >::type::value
        || conditional<
            is_executor<T>::value,
            false_type,
            is_scheduler<T>
          >::type::value));
#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_requirable = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_preferable = true);
  typedef blocking_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR never_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_t<I>::template proxy<T>::type, never_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_t<I>::template static_proxy<T>::type, never_t> {};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename query_static_constexpr_member<T>::result_type
  static_query()
    BOOST_ASIO_NOEXCEPT_IF((
      query_static_constexpr_member<T>::is_noexcept))
  {
    return query_static_constexpr_member<T>::value();
  }

  template <typename E, typename T = decltype(never_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = never_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR blocking_t<I> value()
  {
    return never_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const never_t&, const never_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const never_t&, const never_t&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const never_t&, const possibly_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const never_t&, const possibly_t<I>&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const never_t&, const always_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const never_t&, const always_t<I>&)
  {
    return true;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T never_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace blocking
} // namespace detail

typedef detail::blocking_t<> blocking_t;

#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr blocking_t blocking;
#else // defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
namespace { static const blocking_t& blocking = blocking_t::instance; }
#endif

} // namespace execution

#if !defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::blocking_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

template <typename T>
struct is_applicable_property<T, execution::blocking_t::possibly_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

template <typename T>
struct is_applicable_property<T, execution::blocking_t::always_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

template <typename T>
struct is_applicable_property<T, execution::blocking_t::never_t>
  : integral_constant<bool,
      execution::is_executor<T>::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_sender<T>
          >::type::value
        || conditional<
            execution::is_executor<T>::value,
            false_type,
            execution::is_scheduler<T>
          >::type::value>
{
};

#endif // !defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

template <typename T>
struct query_free_default<T, execution::blocking_t,
  typename enable_if<
    can_query<T, execution::blocking_t::possibly_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_t::possibly_t>::value));

  typedef execution::blocking_t result_type;
};

template <typename T>
struct query_free_default<T, execution::blocking_t,
  typename enable_if<
    !can_query<T, execution::blocking_t::possibly_t>::value
      && can_query<T, execution::blocking_t::always_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_t::always_t>::value));

  typedef execution::blocking_t result_type;
};

template <typename T>
struct query_free_default<T, execution::blocking_t,
  typename enable_if<
    !can_query<T, execution::blocking_t::possibly_t>::value
      && !can_query<T, execution::blocking_t::always_t>::value
      && can_query<T, execution::blocking_t::never_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_t::never_t>::value));

  typedef execution::blocking_t result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    execution::detail::blocking_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::blocking_t::query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    !execution::detail::blocking_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T, execution::blocking_t::possibly_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_t::possibly_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T, execution::blocking_t::possibly_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    !execution::detail::blocking_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T, execution::blocking_t::possibly_t>::is_valid
      && traits::static_query<T, execution::blocking_t::always_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_t::always_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T, execution::blocking_t::always_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t,
  typename enable_if<
    !execution::detail::blocking_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T, execution::blocking_t::possibly_t>::is_valid
      && !traits::static_query<T, execution::blocking_t::always_t>::is_valid
      && traits::static_query<T, execution::blocking_t::never_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_t::never_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T, execution::blocking_t::never_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::possibly_t,
  typename enable_if<
    execution::detail::blocking::possibly_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking::possibly_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking::possibly_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::possibly_t,
  typename enable_if<
    !execution::detail::blocking::possibly_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking::possibly_t<0>::
        query_member<T>::is_valid
      && !traits::query_free<T, execution::blocking_t::possibly_t>::is_valid
      && !can_query<T, execution::blocking_t::always_t>::value
      && !can_query<T, execution::blocking_t::never_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef execution::blocking_t::possibly_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::always_t,
  typename enable_if<
    execution::detail::blocking::always_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking::always_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking::always_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_t::never_t,
  typename enable_if<
    execution::detail::blocking::never_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking::never_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking::never_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

template <typename T>
struct static_require<T, execution::blocking_t::possibly_t,
  typename enable_if<
    static_query<T, execution::blocking_t::possibly_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_t::possibly_t>::result_type,
        execution::blocking_t::possibly_t>::value));
};

template <typename T>
struct static_require<T, execution::blocking_t::always_t,
  typename enable_if<
    static_query<T, execution::blocking_t::always_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_t::always_t>::result_type,
        execution::blocking_t::always_t>::value));
};

template <typename T>
struct static_require<T, execution::blocking_t::never_t,
  typename enable_if<
    static_query<T, execution::blocking_t::never_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_t::never_t>::result_type,
        execution::blocking_t::never_t>::value));
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_FREE_TRAIT)

template <typename T>
struct require_free_default<T, execution::blocking_t::always_t,
  typename enable_if<
    is_same<T, typename decay<T>::type>::value
      && execution::is_executor<T>::value
      && traits::static_require<
          const T&,
          execution::detail::blocking_adaptation::allowed_t<0>
        >::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef execution::detail::blocking::adapter<T> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Executor>
struct equality_comparable<
  execution::detail::blocking::adapter<Executor> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Executor, typename Function>
struct execute_member<
  execution::detail::blocking::adapter<Executor>, Function>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::always_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::possibly_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::never_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_t::always_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Executor, typename Property>
struct query_member<
  execution::detail::blocking::adapter<Executor>, Property,
  typename enable_if<
    can_query<const Executor&, Property>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_query<Executor, Property>::value));
  typedef typename query_result<Executor, Property>::type result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Executor, int I>
struct require_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::possibly_t<I>,
  typename enable_if<
    can_require<
      const Executor&,
      execution::detail::blocking::possibly_t<I>
    >::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<const Executor&,
        execution::detail::blocking::possibly_t<I> >::value));
  typedef typename require_result<const Executor&,
    execution::detail::blocking::possibly_t<I> >::type result_type;
};

template <typename Executor, int I>
struct require_member<
  execution::detail::blocking::adapter<Executor>,
  execution::detail::blocking::never_t<I>,
  typename enable_if<
    can_require<
      const Executor&,
      execution::detail::blocking::never_t<I>
    >::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<const Executor&,
        execution::detail::blocking::never_t<I> >::value));
  typedef typename require_result<const Executor&,
    execution::detail::blocking::never_t<I> >::type result_type;
};

template <typename Executor, typename Property>
struct require_member<
  execution::detail::blocking::adapter<Executor>, Property,
  typename enable_if<
    can_require<const Executor&, Property>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<Executor, Property>::value));
  typedef execution::detail::blocking::adapter<typename decay<
    typename require_result<Executor, Property>::type
      >::type> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

template <typename Executor, typename Property>
struct prefer_member<
  execution::detail::blocking::adapter<Executor>, Property,
  typename enable_if<
    can_prefer<const Executor&, Property>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_prefer<Executor, Property>::value));
  typedef execution::detail::blocking::adapter<typename decay<
    typename prefer_result<Executor, Property>::type
      >::type> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_BLOCKING_HPP
