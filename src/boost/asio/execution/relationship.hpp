//
// execution/relationship.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_RELATIONSHIP_HPP
#define BOOST_ASIO_EXECUTION_RELATIONSHIP_HPP

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

/// A property to describe whether submitted tasks represent continuations of
/// the calling context.
struct relationship_t
{
  /// The relationship_t property applies to executors, senders, and schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The top-level relationship_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level relationship_t property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef relationship_t polymorphic_query_result_type;

  /// A sub-property that indicates that the executor does not represent a
  /// continuation of the calling context.
  struct fork_t
  {
    /// The relationship_t::fork_t property applies to executors, senders, and
    /// schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The relationship_t::fork_t property can be required.
    static constexpr bool is_requirable = true;

    /// The relationship_t::fork_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef relationship_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr fork_t();

    /// Get the value associated with a property object.
    /**
     * @returns fork_t();
     */
    static constexpr relationship_t value();
  };

  /// A sub-property that indicates that the executor represents a continuation
  /// of the calling context.
  struct continuation_t
  {
    /// The relationship_t::continuation_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The relationship_t::continuation_t property can be required.
    static constexpr bool is_requirable = true;

    /// The relationship_t::continuation_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef relationship_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr continuation_t();

    /// Get the value associated with a property object.
    /**
     * @returns continuation_t();
     */
    static constexpr relationship_t value();
  };

  /// A special value used for accessing the relationship_t::fork_t property.
  static constexpr fork_t fork;

  /// A special value used for accessing the relationship_t::continuation_t
  /// property.
  static constexpr continuation_t continuation;

  /// Default constructor.
  constexpr relationship_t();

  /// Construct from a sub-property value.
  constexpr relationship_t(fork_t);

  /// Construct from a sub-property value.
  constexpr relationship_t(continuation_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(
      const relationship_t& a, const relationship_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(
      const relationship_t& a, const relationship_t& b) noexcept;
};

/// A special value used for accessing the relationship_t property.
constexpr relationship_t relationship;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace relationship {

template <int I> struct fork_t;
template <int I> struct continuation_t;

} // namespace relationship

template <int I = 0>
struct relationship_t
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
  typedef relationship_t polymorphic_query_result_type;

  typedef detail::relationship::fork_t<I> fork_t;
  typedef detail::relationship::continuation_t<I> continuation_t;

  BOOST_ASIO_CONSTEXPR relationship_t()
    : value_(-1)
  {
  }

  BOOST_ASIO_CONSTEXPR relationship_t(fork_t)
    : value_(0)
  {
  }

  BOOST_ASIO_CONSTEXPR relationship_t(continuation_t)
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
    traits::query_member<typename proxy<T>::type, relationship_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, relationship_t> {};

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
  typename traits::static_query<T, fork_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, fork_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, fork_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, continuation_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, fork_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, continuation_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, continuation_t>::value();
  }

  template <typename E,
      typename T = decltype(relationship_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = relationship_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const relationship_t& a, const relationship_t& b)
  {
    return a.value_ == b.value_;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const relationship_t& a, const relationship_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_relationship_t
  {
    BOOST_ASIO_CONSTEXPR convertible_from_relationship_t(relationship_t)
    {
    }
  };

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR relationship_t query(
      const Executor& ex, convertible_from_relationship_t,
      typename enable_if<
        can_query<const Executor&, fork_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, relationship_t<>::fork_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, fork_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, fork_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR relationship_t query(
      const Executor& ex, convertible_from_relationship_t,
      typename enable_if<
        !can_query<const Executor&, fork_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, continuation_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        relationship_t<>::continuation_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, continuation_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, continuation_t());
  }

  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(fork_t, fork);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(continuation_t, continuation);

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
  static const relationship_t instance;
#endif // !defined(BOOST_ASIO_HAS_CONSTEXPR)

private:
  int value_;
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T relationship_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
template <int I>
const relationship_t<I> relationship_t<I>::instance;
#endif

template <int I>
const typename relationship_t<I>::fork_t
relationship_t<I>::fork;

template <int I>
const typename relationship_t<I>::continuation_t
relationship_t<I>::continuation;

namespace relationship {

template <int I = 0>
struct fork_t
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
  typedef relationship_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR fork_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename relationship_t<I>::template proxy<T>::type, fork_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename relationship_t<I>::template static_proxy<T>::type, fork_t> {};

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
  static BOOST_ASIO_CONSTEXPR fork_t static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::query_free<T, fork_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !can_query<T, continuation_t<I> >::value
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return fork_t();
  }

  template <typename E, typename T = decltype(fork_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = fork_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR relationship_t<I> value()
  {
    return fork_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const fork_t&, const fork_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const fork_t&, const fork_t&)
  {
    return false;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T fork_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <int I = 0>
struct continuation_t
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
  typedef relationship_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR continuation_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename relationship_t<I>::template proxy<T>::type, continuation_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename relationship_t<I>::template static_proxy<T>::type,
        continuation_t> {};

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

  template <typename E,
      typename T = decltype(continuation_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = continuation_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR relationship_t<I> value()
  {
    return continuation_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const continuation_t&, const continuation_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const continuation_t&, const continuation_t&)
  {
    return false;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T continuation_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace relationship
} // namespace detail

typedef detail::relationship_t<> relationship_t;

#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr relationship_t relationship;
#else // defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
namespace { static const relationship_t&
  relationship = relationship_t::instance; }
#endif

} // namespace execution

#if !defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::relationship_t>
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
struct is_applicable_property<T, execution::relationship_t::fork_t>
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
struct is_applicable_property<T, execution::relationship_t::continuation_t>
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
struct query_free_default<T, execution::relationship_t,
  typename enable_if<
    can_query<T, execution::relationship_t::fork_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::relationship_t::fork_t>::value));

  typedef execution::relationship_t result_type;
};

template <typename T>
struct query_free_default<T, execution::relationship_t,
  typename enable_if<
    !can_query<T, execution::relationship_t::fork_t>::value
      && can_query<T, execution::relationship_t::continuation_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::relationship_t::continuation_t>::value));

  typedef execution::relationship_t result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::relationship_t,
  typename enable_if<
    execution::detail::relationship_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::relationship_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::relationship_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::relationship_t,
  typename enable_if<
    !execution::detail::relationship_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::relationship_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T,
        execution::relationship_t::fork_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::relationship_t::fork_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::relationship_t::fork_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::relationship_t,
  typename enable_if<
    !execution::detail::relationship_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::relationship_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::relationship_t::fork_t>::is_valid
      && traits::static_query<T,
        execution::relationship_t::continuation_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::relationship_t::continuation_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::relationship_t::continuation_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::relationship_t::fork_t,
  typename enable_if<
    execution::detail::relationship::fork_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::relationship::fork_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::relationship::fork_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::relationship_t::fork_t,
  typename enable_if<
    !execution::detail::relationship::fork_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::relationship::fork_t<0>::
        query_member<T>::is_valid
      && !traits::query_free<T,
        execution::relationship_t::fork_t>::is_valid
      && !can_query<T, execution::relationship_t::continuation_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef execution::relationship_t::fork_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::relationship_t::continuation_t,
  typename enable_if<
    execution::detail::relationship::continuation_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::relationship::continuation_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::relationship::continuation_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

template <typename T>
struct static_require<T, execution::relationship_t::fork_t,
  typename enable_if<
    static_query<T, execution::relationship_t::fork_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::relationship_t::fork_t>::result_type,
        execution::relationship_t::fork_t>::value));
};

template <typename T>
struct static_require<T, execution::relationship_t::continuation_t,
  typename enable_if<
    static_query<T, execution::relationship_t::continuation_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::relationship_t::continuation_t>::result_type,
        execution::relationship_t::continuation_t>::value));
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_RELATIONSHIP_HPP
