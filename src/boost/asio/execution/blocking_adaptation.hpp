//
// execution/blocking_adaptation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_BLOCKING_ADAPTATION_HPP
#define BOOST_ASIO_EXECUTION_BLOCKING_ADAPTATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/event.hpp>
#include <boost/asio/detail/mutex.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/scheduler.hpp>
#include <boost/asio/execution/sender.hpp>
#include <boost/asio/is_applicable_property.hpp>
#include <boost/asio/prefer.hpp>
#include <boost/asio/query.hpp>
#include <boost/asio/require.hpp>
#include <boost/asio/traits/prefer_member.hpp>
#include <boost/asio/traits/query_free.hpp>
#include <boost/asio/traits/query_member.hpp>
#include <boost/asio/traits/query_static_constexpr_member.hpp>
#include <boost/asio/traits/require_member.hpp>
#include <boost/asio/traits/static_query.hpp>
#include <boost/asio/traits/static_require.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// A property to describe whether automatic adaptation of an executor is
/// allowed in order to apply the blocking_adaptation_t::allowed_t property.
struct blocking_adaptation_t
{
  /// The blocking_adaptation_t property applies to executors, senders, and
  /// schedulers.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

  /// The top-level blocking_adaptation_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The top-level blocking_adaptation_t property cannot be preferred.
  static constexpr bool is_preferable = false;

  /// The type returned by queries against an @c any_executor.
  typedef blocking_adaptation_t polymorphic_query_result_type;

  /// A sub-property that indicates that automatic adaptation is not allowed.
  struct disallowed_t
  {
    /// The blocking_adaptation_t::disallowed_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_adaptation_t::disallowed_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_adaptation_t::disallowed_t property can be preferred.
    static constexpr bool is_preferable = true;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_adaptation_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr disallowed_t();

    /// Get the value associated with a property object.
    /**
     * @returns disallowed_t();
     */
    static constexpr blocking_adaptation_t value();
  };

  /// A sub-property that indicates that automatic adaptation is allowed.
  struct allowed_t
  {
    /// The blocking_adaptation_t::allowed_t property applies to executors,
    /// senders, and schedulers.
    template <typename T>
    static constexpr bool is_applicable_property_v =
      is_executor_v<T> || is_sender_v<T> || is_scheduler_v<T>;

    /// The blocking_adaptation_t::allowed_t property can be required.
    static constexpr bool is_requirable = true;

    /// The blocking_adaptation_t::allowed_t property can be preferred.
    static constexpr bool is_preferable = false;

    /// The type returned by queries against an @c any_executor.
    typedef blocking_adaptation_t polymorphic_query_result_type;

    /// Default constructor.
    constexpr allowed_t();

    /// Get the value associated with a property object.
    /**
     * @returns allowed_t();
     */
    static constexpr blocking_adaptation_t value();
  };

  /// A special value used for accessing the blocking_adaptation_t::disallowed_t
  /// property.
  static constexpr disallowed_t disallowed;

  /// A special value used for accessing the blocking_adaptation_t::allowed_t
  /// property.
  static constexpr allowed_t allowed;

  /// Default constructor.
  constexpr blocking_adaptation_t();

  /// Construct from a sub-property value.
  constexpr blocking_adaptation_t(disallowed_t);

  /// Construct from a sub-property value.
  constexpr blocking_adaptation_t(allowed_t);

  /// Compare property values for equality.
  friend constexpr bool operator==(
      const blocking_adaptation_t& a, const blocking_adaptation_t& b) noexcept;

  /// Compare property values for inequality.
  friend constexpr bool operator!=(
      const blocking_adaptation_t& a, const blocking_adaptation_t& b) noexcept;
};

/// A special value used for accessing the blocking_adaptation_t property.
constexpr blocking_adaptation_t blocking_adaptation;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {
namespace blocking_adaptation {

template <int I> struct disallowed_t;
template <int I> struct allowed_t;

} // namespace blocking_adaptation

template <int I = 0>
struct blocking_adaptation_t
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
  typedef blocking_adaptation_t polymorphic_query_result_type;

  typedef detail::blocking_adaptation::disallowed_t<I> disallowed_t;
  typedef detail::blocking_adaptation::allowed_t<I> allowed_t;

  BOOST_ASIO_CONSTEXPR blocking_adaptation_t()
    : value_(-1)
  {
  }

  BOOST_ASIO_CONSTEXPR blocking_adaptation_t(disallowed_t)
    : value_(0)
  {
  }

  BOOST_ASIO_CONSTEXPR blocking_adaptation_t(allowed_t)
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
    traits::query_member<typename proxy<T>::type, blocking_adaptation_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename static_proxy<T>::type, blocking_adaptation_t> {};

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
  typename traits::static_query<T, disallowed_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, disallowed_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, disallowed_t>::value();
  }

  template <typename T>
  static BOOST_ASIO_CONSTEXPR
  typename traits::static_query<T, allowed_t>::result_type
  static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::static_query<T, disallowed_t>::is_valid
      >::type* = 0,
      typename enable_if<
        traits::static_query<T, allowed_t>::is_valid
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return traits::static_query<T, allowed_t>::value();
  }

  template <typename E,
      typename T = decltype(blocking_adaptation_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = blocking_adaptation_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const blocking_adaptation_t& a, const blocking_adaptation_t& b)
  {
    return a.value_ == b.value_;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const blocking_adaptation_t& a, const blocking_adaptation_t& b)
  {
    return a.value_ != b.value_;
  }

  struct convertible_from_blocking_adaptation_t
  {
    BOOST_ASIO_CONSTEXPR convertible_from_blocking_adaptation_t(
        blocking_adaptation_t)
    {
    }
  };

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR blocking_adaptation_t query(
      const Executor& ex, convertible_from_blocking_adaptation_t,
      typename enable_if<
        can_query<const Executor&, disallowed_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        blocking_adaptation_t<>::disallowed_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, disallowed_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, disallowed_t());
  }

  template <typename Executor>
  friend BOOST_ASIO_CONSTEXPR blocking_adaptation_t query(
      const Executor& ex, convertible_from_blocking_adaptation_t,
      typename enable_if<
        !can_query<const Executor&, disallowed_t>::value
      >::type* = 0,
      typename enable_if<
        can_query<const Executor&, allowed_t>::value
      >::type* = 0)
#if !defined(__clang__) // Clang crashes if noexcept is used here.
#if defined(BOOST_ASIO_MSVC) // Visual C++ wants the type to be qualified.
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&,
        blocking_adaptation_t<>::allowed_t>::value))
#else // defined(BOOST_ASIO_MSVC)
    BOOST_ASIO_NOEXCEPT_IF((
      is_nothrow_query<const Executor&, allowed_t>::value))
#endif // defined(BOOST_ASIO_MSVC)
#endif // !defined(__clang__)
  {
    return boost::asio::query(ex, allowed_t());
  }

  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(disallowed_t, disallowed);
  BOOST_ASIO_STATIC_CONSTEXPR_DEFAULT_INIT(allowed_t, allowed);

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
  static const blocking_adaptation_t instance;
#endif // !defined(BOOST_ASIO_HAS_CONSTEXPR)

private:
  int value_;
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T blocking_adaptation_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_CONSTEXPR)
template <int I>
const blocking_adaptation_t<I> blocking_adaptation_t<I>::instance;
#endif

template <int I>
const typename blocking_adaptation_t<I>::disallowed_t
blocking_adaptation_t<I>::disallowed;

template <int I>
const typename blocking_adaptation_t<I>::allowed_t
blocking_adaptation_t<I>::allowed;

namespace blocking_adaptation {

template <int I = 0>
struct disallowed_t
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
  typedef blocking_adaptation_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR disallowed_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_adaptation_t<I>::template proxy<T>::type,
        disallowed_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_adaptation_t<I>::template static_proxy<T>::type,
        disallowed_t> {};

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
  static BOOST_ASIO_CONSTEXPR disallowed_t static_query(
      typename enable_if<
        !query_static_constexpr_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !query_member<T>::is_valid
      >::type* = 0,
      typename enable_if<
        !traits::query_free<T, disallowed_t>::is_valid
      >::type* = 0,
      typename enable_if<
        !can_query<T, allowed_t<I> >::value
      >::type* = 0) BOOST_ASIO_NOEXCEPT
  {
    return disallowed_t();
  }

  template <typename E, typename T = decltype(disallowed_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = disallowed_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR blocking_adaptation_t<I> value()
  {
    return disallowed_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const disallowed_t&, const disallowed_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const disallowed_t&, const disallowed_t&)
  {
    return false;
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T disallowed_t<I>::static_query_v;
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
  static BOOST_ASIO_CONSTEXPR allowed_t<I> query(
      blocking_adaptation_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return allowed_t<I>();
  }

  template <int I>
  static BOOST_ASIO_CONSTEXPR allowed_t<I> query(
      allowed_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return allowed_t<I>();
  }

  template <int I>
  static BOOST_ASIO_CONSTEXPR allowed_t<I> query(
      disallowed_t<I>) BOOST_ASIO_NOEXCEPT
  {
    return allowed_t<I>();
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
  Executor require(disallowed_t<I>) const BOOST_ASIO_NOEXCEPT
  {
    return executor_;
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
    execution::execute(executor_, BOOST_ASIO_MOVE_CAST(Function)(f));
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
struct allowed_t
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
  typedef blocking_adaptation_t<I> polymorphic_query_result_type;

  BOOST_ASIO_CONSTEXPR allowed_t()
  {
  }

  template <typename T>
  struct query_member :
    traits::query_member<
      typename blocking_adaptation_t<I>::template proxy<T>::type,
        allowed_t> {};

  template <typename T>
  struct query_static_constexpr_member :
    traits::query_static_constexpr_member<
      typename blocking_adaptation_t<I>::template static_proxy<T>::type,
        allowed_t> {};

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

  template <typename E, typename T = decltype(allowed_t::static_query<E>())>
  static BOOST_ASIO_CONSTEXPR const T static_query_v
    = allowed_t::static_query<E>();
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  static BOOST_ASIO_CONSTEXPR blocking_adaptation_t<I> value()
  {
    return allowed_t();
  }

  friend BOOST_ASIO_CONSTEXPR bool operator==(
      const allowed_t&, const allowed_t&)
  {
    return true;
  }

  friend BOOST_ASIO_CONSTEXPR bool operator!=(
      const allowed_t&, const allowed_t&)
  {
    return false;
  }

  template <typename Executor>
  friend adapter<Executor> require(
      const Executor& e, const allowed_t&,
      typename enable_if<
        is_executor<Executor>::value
      >::type* = 0)
  {
    return adapter<Executor>(0, e);
  }
};

#if defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <int I> template <typename E, typename T>
const T allowed_t<I>::static_query_v;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename Function>
class blocking_execute_state
{
public:
  template <typename F>
  blocking_execute_state(BOOST_ASIO_MOVE_ARG(F) f)
    : func_(BOOST_ASIO_MOVE_CAST(F)(f)),
      is_complete_(false)
  {
  }

  template <typename Executor>
  void execute_and_wait(BOOST_ASIO_MOVE_ARG(Executor) ex)
  {
    handler h = { this };
    execution::execute(BOOST_ASIO_MOVE_CAST(Executor)(ex), h);
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    while (!is_complete_)
      event_.wait(lock);
  }

  struct cleanup
  {
    ~cleanup()
    {
      boost::asio::detail::mutex::scoped_lock lock(state_->mutex_);
      state_->is_complete_ = true;
      state_->event_.unlock_and_signal_one_for_destruction(lock);
    }

    blocking_execute_state* state_;
  };

  struct handler
  {
    void operator()()
    {
      cleanup c = { state_ };
      state_->func_();
    }

    blocking_execute_state* state_;
  };

  Function func_;
  boost::asio::detail::mutex mutex_;
  boost::asio::detail::event event_;
  bool is_complete_;
};

template <typename Executor, typename Function>
void blocking_execute(
    BOOST_ASIO_MOVE_ARG(Executor) ex,
    BOOST_ASIO_MOVE_ARG(Function) func)
{
  typedef typename decay<Function>::type func_t;
  blocking_execute_state<func_t> state(BOOST_ASIO_MOVE_CAST(Function)(func));
  state.execute_and_wait(ex);
}

} // namespace blocking_adaptation
} // namespace detail

typedef detail::blocking_adaptation_t<> blocking_adaptation_t;

#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr blocking_adaptation_t blocking_adaptation;
#else // defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
namespace { static const blocking_adaptation_t&
  blocking_adaptation = blocking_adaptation_t::instance; }
#endif

} // namespace execution

#if !defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
struct is_applicable_property<T, execution::blocking_adaptation_t>
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
struct is_applicable_property<T, execution::blocking_adaptation_t::disallowed_t>
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
struct is_applicable_property<T, execution::blocking_adaptation_t::allowed_t>
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
struct query_free_default<T, execution::blocking_adaptation_t,
  typename enable_if<
    can_query<T, execution::blocking_adaptation_t::disallowed_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = (is_nothrow_query<T,
      execution::blocking_adaptation_t::disallowed_t>::value));

  typedef execution::blocking_adaptation_t result_type;
};

template <typename T>
struct query_free_default<T, execution::blocking_adaptation_t,
  typename enable_if<
    !can_query<T, execution::blocking_adaptation_t::disallowed_t>::value
      && can_query<T, execution::blocking_adaptation_t::allowed_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
    (is_nothrow_query<T, execution::blocking_adaptation_t::allowed_t>::value));

  typedef execution::blocking_adaptation_t result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T>
struct static_query<T, execution::blocking_adaptation_t,
  typename enable_if<
    execution::detail::blocking_adaptation_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking_adaptation_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking_adaptation_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_adaptation_t,
  typename enable_if<
    !execution::detail::blocking_adaptation_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_adaptation_t<0>::
        query_member<T>::is_valid
      && traits::static_query<T,
        execution::blocking_adaptation_t::disallowed_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_adaptation_t::disallowed_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::blocking_adaptation_t::disallowed_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_adaptation_t,
  typename enable_if<
    !execution::detail::blocking_adaptation_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_adaptation_t<0>::
        query_member<T>::is_valid
      && !traits::static_query<T,
        execution::blocking_adaptation_t::disallowed_t>::is_valid
      && traits::static_query<T,
        execution::blocking_adaptation_t::allowed_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename traits::static_query<T,
    execution::blocking_adaptation_t::allowed_t>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return traits::static_query<T,
        execution::blocking_adaptation_t::allowed_t>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_adaptation_t::disallowed_t,
  typename enable_if<
    execution::detail::blocking_adaptation::disallowed_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking_adaptation::disallowed_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking_adaptation::disallowed_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

template <typename T>
struct static_query<T, execution::blocking_adaptation_t::disallowed_t,
  typename enable_if<
    !execution::detail::blocking_adaptation::disallowed_t<0>::
        query_static_constexpr_member<T>::is_valid
      && !execution::detail::blocking_adaptation::disallowed_t<0>::
        query_member<T>::is_valid
      && !traits::query_free<T,
        execution::blocking_adaptation_t::disallowed_t>::is_valid
      && !can_query<T, execution::blocking_adaptation_t::allowed_t>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef execution::blocking_adaptation_t::disallowed_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return result_type();
  }
};

template <typename T>
struct static_query<T, execution::blocking_adaptation_t::allowed_t,
  typename enable_if<
    execution::detail::blocking_adaptation::allowed_t<0>::
      query_static_constexpr_member<T>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

  typedef typename execution::detail::blocking_adaptation::allowed_t<0>::
    query_static_constexpr_member<T>::result_type result_type;

  static BOOST_ASIO_CONSTEXPR result_type value()
  {
    return execution::detail::blocking_adaptation::allowed_t<0>::
      query_static_constexpr_member<T>::value();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(BOOST_ASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

template <typename T>
struct static_require<T, execution::blocking_adaptation_t::disallowed_t,
  typename enable_if<
    static_query<T, execution::blocking_adaptation_t::disallowed_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_adaptation_t::disallowed_t>::result_type,
        execution::blocking_adaptation_t::disallowed_t>::value));
};

template <typename T>
struct static_require<T, execution::blocking_adaptation_t::allowed_t,
  typename enable_if<
    static_query<T, execution::blocking_adaptation_t::allowed_t>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid =
    (is_same<typename static_query<T,
      execution::blocking_adaptation_t::allowed_t>::result_type,
        execution::blocking_adaptation_t::allowed_t>::value));
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_STATIC_REQUIRE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_FREE_TRAIT)

template <typename T>
struct require_free_default<T, execution::blocking_adaptation_t::allowed_t,
  typename enable_if<
    is_same<T, typename decay<T>::type>::value
      && execution::is_executor<T>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef execution::detail::blocking_adaptation::adapter<T> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_FREE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Executor>
struct equality_comparable<
  execution::detail::blocking_adaptation::adapter<Executor> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Executor, typename Function>
struct execute_member<
  execution::detail::blocking_adaptation::adapter<Executor>, Function>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking_adaptation::adapter<Executor>,
  execution::detail::blocking_adaptation_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_adaptation_t::allowed_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking_adaptation::adapter<Executor>,
  execution::detail::blocking_adaptation::allowed_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_adaptation_t::allowed_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

template <typename Executor, int I>
struct query_static_constexpr_member<
  execution::detail::blocking_adaptation::adapter<Executor>,
  execution::detail::blocking_adaptation::disallowed_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef execution::blocking_adaptation_t::allowed_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Executor, typename Property>
struct query_member<
  execution::detail::blocking_adaptation::adapter<Executor>, Property,
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
  execution::detail::blocking_adaptation::adapter<Executor>,
  execution::detail::blocking_adaptation::disallowed_t<I> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef Executor result_type;
};

template <typename Executor, typename Property>
struct require_member<
  execution::detail::blocking_adaptation::adapter<Executor>, Property,
  typename enable_if<
    can_require<const Executor&, Property>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_require<Executor, Property>::value));
  typedef execution::detail::blocking_adaptation::adapter<typename decay<
    typename require_result<Executor, Property>::type
      >::type> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

template <typename Executor, typename Property>
struct prefer_member<
  execution::detail::blocking_adaptation::adapter<Executor>, Property,
  typename enable_if<
    can_prefer<const Executor&, Property>::value
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept =
      (is_nothrow_prefer<Executor, Property>::value));
  typedef execution::detail::blocking_adaptation::adapter<typename decay<
    typename prefer_result<Executor, Property>::type
      >::type> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_BLOCKING_ADAPTATION_HPP
