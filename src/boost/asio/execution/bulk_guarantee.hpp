//
// execution/bulk_guarantee.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_BULK_GUARANTEE_HPP
#define BOOST_ASIO_EXECUTION_BULK_GUARANTEE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/scheduler.hpp>
#include <boost/asio/execution/sender.hpp>
#include <boost/asio/is_applicable_property.hpp>
#include <boost/asio/query.hpp>
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

/// A property to communicate the forward progress and ordering guarantees of
/// execution agents associated with the bulk execution.
struct bulk_guarantee_t
{
  /// The bulk_guarantee_t property applies to executors, senders, and
  /// schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The top-level bulk_guarantee_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level bulk_guarantee_t property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef bulk_guarantee_t polymorphic_query_result_type;

  /// A sub-property that indicates that execution agents within the same bulk
  /// execution may be parallelised and vectorised.
  struct unsequenced_t
  {
    /// The bulk_guarantee_t::unsequenced_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The bulk_guarantee_t::unsequenced_t property can be required.
    static constexpr bool is_requirable = true;

    /// The bulk_guarantee_t::unsequenced_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef bulk_guarantee_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr unsequenced_t();

    /// Get the value associated with a property object.
    /**
     * @returns unsequenced_t();
     */
    static constexpr bulk_guarantee_t value();
  };

  /// A sub-property that indicates that execution agents within the same bulk
  /// execution may not be parallelised and vectorised.
  struct sequenced_t
  {
    /// The bulk_guarantee_t::sequenced_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The bulk_guarantee_t::sequenced_t property can be required.
    static constexpr bool is_requirable = true;

    /// The bulk_guarantee_t::sequenced_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef bulk_guarantee_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr sequenced_t();

    /// Get the value associated with a property object.
    /**
     * @returns sequenced_t();
     */
    static constexpr bulk_guarantee_t value();
  };

  /// A sub-property that indicates that execution agents within the same bulk
  /// execution may be parallelised.
  struct parallel_t
  {
    /// The bulk_guarantee_t::parallel_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The bulk_guarantee_t::parallel_t property can be required.
    static constexpr bool is_requirable = true;

    /// The bulk_guarantee_t::parallel_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef bulk_guarantee_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr parallel_t();

    /// Get the value associated with a property object.
    /**
     * @returns parallel_t();
     */
    static constexpr bulk_guarantee_t value();
  };

  /// A special value used for accessing the bulk_guarantee_t::unsequenced_t
  /// property.
  static constexpr unsequenced_t unsequenced;

  /// A special value used for accessing the bulk_guarantee_t::sequenced_t
  /// property.
  static constexpr sequenced_t sequenced;

  /// A special value used for accessing the bulk_guarantee_t::parallel_t
  /// property.
  static constexpr parallel_t parallel;

  /// Default constructor.
  constexpr bulk_guarantee_t();

  /// Construct from a sub-property value.
  constexpr bulk_guarantee_t(unsequenced_t);

  /// Construct from a sub-property value.
  constexpr bulk_guarantee_t(sequenced_t);

  /// Construct from a sub-property value.
  constexpr bulk_guarantee_t(parallel_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(
      const bulk_guarantee_t& a, const bulk_guarantee_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(
      const bulk_guarantee_t& a, const bulk_guarantee_t& b) noexcept;
};

/// A special value used for accessing the bulk_guarantee_t property.
constexpr bulk_guarantee_t bulk_guarantee;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace bulk_guarantee {

template <int I> struct unsequenced_t;
template <int I> struct sequenced_t;
template <int I> struct parallel_t;

} // namespace bulk_guarantee

template <int I = 0>
struct bulk_guarantee_t
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
  typedef bulk_guarantee_t polymorphic_query_result_type;

  typedef detail::bulk_guarantee::unsequenced_t<I> unsequenced_t;
  typedef detail::bulk_guarantee::sequenced_t<I> sequenced_t;
  typedef detail::bulk_guarantee::parallel_t<I> parallel_t;

  BOOST_ASIO_CONSTEXPR bulk_guarantee_t()
    : value_(-1)
  {
  }

  BOOST_ASIO_CONSTEXPR bulk_guarantee_t(unsequenced_t)
    : value_(0)
  {
  }

  BOOST_ASIO_CONSTEXPR bulk_guarantee_t(sequenced_t)
    : value_(1)
  {
  }

  BOOST_ASIO_CONSTEXPR bulk_guarantee_t(parallel_t)
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
    traits::query_member<typename proxy<T>::type, bulk_guarantee_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, bulk_guarantee_t> {};

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
  typename traits::static_query<T, unsequenced_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, unsequenced_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, unsequenced_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, sequenced_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, unsequenced_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, sequenced_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, sequenced_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, parallel_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, unsequenced_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, sequenced_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, parallel_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, parallel_t>::value();
  }

  template <typename E,
      typename T = decltype(bulk_guarantee_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = bulk_guarantee_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const bulk_guarantee_t& a, const bulk_guarantee_t& b)
  {
    return a.value_ == b.value_;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const bulk_guarantee_t& a, const bulk_guarantee_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_bulk_guarantee_t
  {
    BOOST_ASIO_CONSTEXPR convertible_from_bulk_guarantee_t(bulk_guarantee_t) {}
  };

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR bulk_guarantee_t query(
      const Executor& ex, convertible_from_bulk_guarantee_t,
      typename enable_if<
        can_query<const Executor&, unsequenced_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        bulk_guarantee_t<>::unsequenced_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, unsequenced_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, unsequenced_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR bulk_guarantee_t query(
      const Executor& ex, convertible_from_bulk_guarantee_t,
      typename enable_if<
        !can_query<const Executor&, unsequenced_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, sequenced_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        bulk_guarantee_t<>::sequenced_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, sequenced_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, sequenced_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR bulk_guarantee_t query(
      const Executor& ex, convertible_from_bulk_guarantee_t,
      typename enable_if<
        !can_query<const Executor&, unsequenced_t>::value
      >::type* = 0,
      typename enable_if<
        !can_query<const Executor&, sequenced_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, parallel_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, bulk_guarantee_t<>::parallel_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, parallel_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, parallel_t());
  }

  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(unsequenced_t, unsequenced);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(sequenced_t, sequenced);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(parallel_t, parallel);

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
  static const bulk_guarantee_t instance;
#endif // !defined(BOOST_ASIO_HAS_CONSTEXPR)

private:
  int value_;
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T bulk_guarantee_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
template <int I>
const bulk_guarantee_t<I> bulk_guarantee_t<I>::instance;
#endif

template <int I>
const typename bulk_guarantee_t<I>::unsequenced_t
bulk_guarantee_t<I>::unsequenced;

template <int I>
const typename bulk_guarantee_t<I>::sequenced_t
bulk_guarantee_t<I>::sequenced;

template <int I>
const typename bulk_guarantee_t<I>::parallel_t
bulk_guarantee_t<I>::parallel;

namespace bulk_guarantee {

template <int I = 0>
struct unsequenced_t
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
  typedef bulk_guarantee_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR unsequenced_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename bulk_guarantee_t<I>::template proxy<T>::type,
        unsequenced_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename bulk_guarantee_t<I>::template static_proxy<T>::type,
        unsequenced_t> {};

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
  static BOOST_ASIO_CONSTEXPR unsequenced_t static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::query_free<T, unsequenced_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !can_query<T, sequenced_t<I> >::value
      >::type* = 0,
      typename enable_if<
        !can_query<T, parallel_t<I> >::value
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return unsequenced_t();
  }

  template <typename E, typename T = decltype(unsequenced_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = unsequenced_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR bulk_guarantee_t<I> value()
  {
    return unsequenced_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const unsequenced_t&, const unsequenced_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const unsequenced_t&, const unsequenced_t&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const unsequenced_t&, const sequenced_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const unsequenced_t&, const sequenced_t<I>&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const unsequenced_t&, const parallel_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const unsequenced_t&, const parallel_t<I>&)
  {
    return true;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T unsequenced_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I = 0>
struct sequenced_t
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
  typedef bulk_guarantee_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR sequenced_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename bulk_guarantee_t<I>::template proxy<T>::type,
        sequenced_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename bulk_guarantee_t<I>::template static_proxy<T>::type,
        sequenced_t> {};

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

  template <typename E, typename T = decltype(sequenced_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = sequenced_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR bulk_guarantee_t<I> value()
  {
    return sequenced_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const sequenced_t&, const sequenced_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const sequenced_t&, const sequenced_t&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const sequenced_t&, const unsequenced_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const sequenced_t&, const unsequenced_t<I>&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const sequenced_t&, const parallel_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const sequenced_t&, const parallel_t<I>&)
  {
    return true;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T sequenced_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I>
struct parallel_t
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
  typedef bulk_guarantee_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR parallel_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename bulk_guarantee_t<I>::template proxy<T>::type,
        parallel_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename bulk_guarantee_t<I>::template static_proxy<T>::type,
        parallel_t> {};

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

  template <typename E, typename T = decltype(parallel_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = parallel_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR bulk_guarantee_t<I> value()
  {
    return parallel_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const parallel_t&, const parallel_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const parallel_t&, const parallel_t&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const parallel_t&, const unsequenced_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const parallel_t&, const unsequenced_t<I>&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const parallel_t&, const sequenced_t<I>&)
  {
    return false;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const parallel_t&, const sequenced_t<I>&)
  {
    return true;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T parallel_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace bulk_guarantee
} // namespace detail

typedef detail::bulk_guarantee_t<> bulk_guarantee_t;

#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr bulk_guarantee_t bulk_guarantee;
#else // defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
namespace { static const bulk_guarantee_t&
  bulk_guarantee = bulk_guarantee_t::instance; }
#endif

} // namespace execution

#if !defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::bulk_guarantee_t>
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
struct is_applicable_property<T, execution::bulk_guarantee_t::unsequenced_t>
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
struct is_applicable_property<T, execution::bulk_guarantee_t::sequenced_t>
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
struct is_applicable_property<T, execution::bulk_guarantee_t::parallel_t>
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
struct query_free_default<T, execution::bulk_guarantee_t,
  typename enable_if<
    can_query<T, execution::bulk_guarantee_t::unsequenced_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::bulk_guarantee_t::unsequenced_t>::value));

  typedef execution::bulk_guarantee_t result_type;
};

template <typename T>
struct query_free_default<T, execution::bulk_guarantee_t,
  typename enable_if<
    !can_query<T, execution::bulk_guarantee_t::unsequenced_t>::value
      && can_query<T, execution::bulk_guarantee_t::sequenced_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::bulk_guarantee_t::sequenced_t>::value));

  typedef execution::bulk_guarantee_t result_type;
};

template <typename T>
struct query_free_default<T, execution::bulk_guarantee_t,
  typename enable_if<
    !can_query<T, execution::bulk_guarantee_t::unsequenced_t>::value
      && !can_query<T, execution::bulk_guarantee_t::sequenced_t>::value
      && can_query<T, execution::bulk_guarantee_t::parallel_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::bulk_guarantee_t::parallel_t>::value));

  typedef execution::bulk_guarantee_t result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::bulk_guarantee_t,
  typename enable_if<
    execution::detail::bulk_guarantee_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::bulk_guarantee_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::bulk_guarantee_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t,
  typename enable_if<
    !execution::detail::bulk_guarantee_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::bulk_guarantee_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T,
        execution::bulk_guarantee_t::unsequenced_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::bulk_guarantee_t::unsequenced_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::bulk_guarantee_t::unsequenced_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t,
  typename enable_if<
    !execution::detail::bulk_guarantee_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::bulk_guarantee_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::bulk_guarantee_t::unsequenced_t>::is_valid
      && traits::static_query<T,
        execution::bulk_guarantee_t::sequenced_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::bulk_guarantee_t::sequenced_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::bulk_guarantee_t::sequenced_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t,
  typename enable_if<
    !execution::detail::bulk_guarantee_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::bulk_guarantee_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::bulk_guarantee_t::unsequenced_t>::is_valid
      && !traits::static_query<T,
        execution::bulk_guarantee_t::sequenced_t>::is_valid
      && traits::static_query<T,
        execution::bulk_guarantee_t::parallel_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::bulk_guarantee_t::parallel_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::bulk_guarantee_t::parallel_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t::unsequenced_t,
  typename enable_if<
    execution::detail::bulk_guarantee::unsequenced_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::bulk_guarantee::unsequenced_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::bulk_guarantee::unsequenced_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t::unsequenced_t,
  typename enable_if<
    !execution::detail::bulk_guarantee::unsequenced_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::bulk_guarantee::unsequenced_t<0>::
        query_member<T>::is_valid
      && !traits::query_free<T,
        execution::bulk_guarantee_t::unsequenced_t>::is_valid
      && !can_query<T, execution::bulk_guarantee_t::sequenced_t>::value
      && !can_query<T, execution::bulk_guarantee_t::parallel_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef execution::bulk_guarantee_t::unsequenced_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t::sequenced_t,
  typename enable_if<
    execution::detail::bulk_guarantee::sequenced_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::bulk_guarantee::sequenced_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::bulk_guarantee::sequenced_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::bulk_guarantee_t::parallel_t,
  typename enable_if<
    execution::detail::bulk_guarantee::parallel_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::bulk_guarantee::parallel_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::bulk_guarantee::parallel_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

template <typename T>
struct static_require<T, execution::bulk_guarantee_t::unsequenced_t,
  typename enable_if<
    static_query<T, execution::bulk_guarantee_t::unsequenced_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::bulk_guarantee_t::unsequenced_t>::result_type,
        execution::bulk_guarantee_t::unsequenced_t>::value));
};

template <typename T>
struct static_require<T, execution::bulk_guarantee_t::sequenced_t,
  typename enable_if<
    static_query<T, execution::bulk_guarantee_t::sequenced_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::bulk_guarantee_t::sequenced_t>::result_type,
        execution::bulk_guarantee_t::sequenced_t>::value));
};

template <typename T>
struct static_require<T, execution::bulk_guarantee_t::parallel_t,
  typename enable_if<
    static_query<T, execution::bulk_guarantee_t::parallel_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::bulk_guarantee_t::parallel_t>::result_type,
        execution::bulk_guarantee_t::parallel_t>::value));
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_BULK_GUARANTEE_HPP
