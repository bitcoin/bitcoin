//
// execution/outstanding_work.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_OUTSTANDING_WORK_HPP
#define BOOST_ASIO_EXECUTION_OUTSTANDING_WORK_HPP

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

/// A property to describe whether task submission is likely in the future.
struct outstanding_work_t
{
  /// The outstanding_work_t property applies to executors, senders, and
  /// schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The top-level outstanding_work_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level outstanding_work_t property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef outstanding_work_t polymorphic_query_result_type;

  /// A sub-property that indicates that the executor does not represent likely
  /// future submission of a function object.
  struct untracked_t
  {
    /// The outstanding_work_t::untracked_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The outstanding_work_t::untracked_t property can be required.
    static constexpr bool is_requirable = true;

    /// The outstanding_work_t::untracked_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef outstanding_work_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr untracked_t();

    /// Get the value associated with a property object.
    /**
     * @returns untracked_t();
     */
    static constexpr outstanding_work_t value();
  };

  /// A sub-property that indicates that the executor represents likely
  /// future submission of a function object.
  struct tracked_t
  {
    /// The outstanding_work_t::untracked_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The outstanding_work_t::tracked_t property can be required.
    static constexpr bool is_requirable = true;

    /// The outstanding_work_t::tracked_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef outstanding_work_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr tracked_t();

    /// Get the value associated with a property object.
    /**
     * @returns tracked_t();
     */
    static constexpr outstanding_work_t value();
  };

  /// A special value used for accessing the outstanding_work_t::untracked_t
  /// property.
  static constexpr untracked_t untracked;

  /// A special value used for accessing the outstanding_work_t::tracked_t
  /// property.
  static constexpr tracked_t tracked;

  /// Default constructor.
  constexpr outstanding_work_t();

  /// Construct from a sub-property value.
  constexpr outstanding_work_t(untracked_t);

  /// Construct from a sub-property value.
  constexpr outstanding_work_t(tracked_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(
      const outstanding_work_t& a, const outstanding_work_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(
      const outstanding_work_t& a, const outstanding_work_t& b) noexcept;
};

/// A special value used for accessing the outstanding_work_t property.
constexpr outstanding_work_t outstanding_work;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace outstanding_work {

template <int I> struct untracked_t;
template <int I> struct tracked_t;

} // namespace outstanding_work

template <int I = 0>
struct outstanding_work_t
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
  typedef outstanding_work_t polymorphic_query_result_type;

  typedef detail::outstanding_work::untracked_t<I> untracked_t;
  typedef detail::outstanding_work::tracked_t<I> tracked_t;

  BOOST_ASIO_CONSTEXPR outstanding_work_t()
    : value_(-1)
  {
  }

  BOOST_ASIO_CONSTEXPR outstanding_work_t(untracked_t)
    : value_(0)
  {
  }

  BOOST_ASIO_CONSTEXPR outstanding_work_t(tracked_t)
    : value_(1)
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
    traits::query_member<typename proxy<T>::type, outstanding_work_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, outstanding_work_t> {};

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
  typename traits::static_query<T, untracked_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, untracked_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, untracked_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, tracked_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, untracked_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, tracked_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, tracked_t>::value();
  }

  template <typename E,
      typename T = decltype(outstanding_work_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = outstanding_work_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const outstanding_work_t& a, const outstanding_work_t& b)
  {
    return a.value_ == b.value_;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const outstanding_work_t& a, const outstanding_work_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_outstanding_work_t
  {
    BOOST_ASIO_CONSTEXPR convertible_from_outstanding_work_t(outstanding_work_t)
    {
    }
  };

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR outstanding_work_t query(
      const Executor& ex, convertible_from_outstanding_work_t,
      typename enable_if<
        can_query<const Executor&, untracked_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        outstanding_work_t<>::untracked_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, untracked_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, untracked_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR outstanding_work_t query(
      const Executor& ex, convertible_from_outstanding_work_t,
      typename enable_if<
        !can_query<const Executor&, untracked_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, tracked_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        outstanding_work_t<>::tracked_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, tracked_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, tracked_t());
  }

  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(untracked_t, untracked);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(tracked_t, tracked);

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
  static const outstanding_work_t instance;
#endif // !defined(BOOST_ASIO_HAS_CONSTEXPR)

private:
  int value_;
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T outstanding_work_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
template <int I>
const outstanding_work_t<I> outstanding_work_t<I>::instance;
#endif

template <int I>
const typename outstanding_work_t<I>::untracked_t
outstanding_work_t<I>::untracked;

template <int I>
const typename outstanding_work_t<I>::tracked_t
outstanding_work_t<I>::tracked;

namespace outstanding_work {

template <int I = 0>
struct untracked_t
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
  typedef outstanding_work_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR untracked_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename outstanding_work_t<I>::template proxy<T>::type, untracked_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename outstanding_work_t<I>::template static_proxy<T>::type,
        untracked_t> {};

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
  static BOOST_ASIO_CONSTEXPR untracked_t static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::query_free<T, untracked_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !can_query<T, tracked_t<I> >::value
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return untracked_t();
  }

  template <typename E, typename T = decltype(untracked_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = untracked_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR outstanding_work_t<I> value()
  {
    return untracked_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const untracked_t&, const untracked_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const untracked_t&, const untracked_t&)
  {
    return false;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T untracked_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I = 0>
struct tracked_t
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
  typedef outstanding_work_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR tracked_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename outstanding_work_t<I>::template proxy<T>::type, tracked_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename outstanding_work_t<I>::template static_proxy<T>::type,
        tracked_t> {};

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

  template <typename E, typename T = decltype(tracked_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = tracked_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR outstanding_work_t<I> value()
  {
    return tracked_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const tracked_t&, const tracked_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const tracked_t&, const tracked_t&)
  {
    return false;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T tracked_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace outstanding_work
} // namespace detail

typedef detail::outstanding_work_t<> outstanding_work_t;

#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr outstanding_work_t outstanding_work;
#else // defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
namespace { static const outstanding_work_t&
  outstanding_work = outstanding_work_t::instance; }
#endif

} // namespace execution

#if !defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::outstanding_work_t>
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
struct is_applicable_property<T, execution::outstanding_work_t::untracked_t>
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
struct is_applicable_property<T, execution::outstanding_work_t::tracked_t>
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
struct query_free_default<T, execution::outstanding_work_t,
  typename enable_if<
    can_query<T, execution::outstanding_work_t::untracked_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::outstanding_work_t::untracked_t>::value));

  typedef execution::outstanding_work_t result_type;
};

template <typename T>
struct query_free_default<T, execution::outstanding_work_t,
  typename enable_if<
    !can_query<T, execution::outstanding_work_t::untracked_t>::value
      && can_query<T, execution::outstanding_work_t::tracked_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::outstanding_work_t::tracked_t>::value));

  typedef execution::outstanding_work_t result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::outstanding_work_t,
  typename enable_if<
    execution::detail::outstanding_work_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::outstanding_work_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::outstanding_work_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::outstanding_work_t,
  typename enable_if<
    !execution::detail::outstanding_work_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::outstanding_work_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T,
        execution::outstanding_work_t::untracked_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::outstanding_work_t::untracked_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::outstanding_work_t::untracked_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::outstanding_work_t,
  typename enable_if<
    !execution::detail::outstanding_work_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::outstanding_work_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::outstanding_work_t::untracked_t>::is_valid
      && traits::static_query<T,
        execution::outstanding_work_t::tracked_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::outstanding_work_t::tracked_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::outstanding_work_t::tracked_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::outstanding_work_t::untracked_t,
  typename enable_if<
    execution::detail::outstanding_work::untracked_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::outstanding_work::untracked_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::outstanding_work::untracked_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::outstanding_work_t::untracked_t,
  typename enable_if<
    !execution::detail::outstanding_work::untracked_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::outstanding_work::untracked_t<0>::
          query_member<T>::is_valid
      && !traits::query_free<T,
        execution::outstanding_work_t::untracked_t>::is_valid
      && !can_query<T, execution::outstanding_work_t::tracked_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef execution::outstanding_work_t::untracked_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::outstanding_work_t::tracked_t,
  typename enable_if<
    execution::detail::outstanding_work::tracked_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::outstanding_work::tracked_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::outstanding_work::tracked_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

template <typename T>
struct static_require<T, execution::outstanding_work_t::untracked_t,
  typename enable_if<
    static_query<T, execution::outstanding_work_t::untracked_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::outstanding_work_t::untracked_t>::result_type,
        execution::outstanding_work_t::untracked_t>::value));
};

template <typename T>
struct static_require<T, execution::outstanding_work_t::tracked_t,
  typename enable_if<
    static_query<T, execution::outstanding_work_t::tracked_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::outstanding_work_t::tracked_t>::result_type,
        execution::outstanding_work_t::tracked_t>::value));
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_OUTSTANDING_WORK_HPP
