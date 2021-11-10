//
// this_coro.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_THIS_CORO_HPP
#define BOOST_ASIO_THIS_CORO_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace this_coro {

/// Awaitable type that returns the executor of the current coroutine.
struct executor_t
{
  BOOST_ASIO_CONSTEXPR executor_t()
  {
  }
};

/// Awaitable object that returns the executor of the current coroutine.
#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr executor_t executor;
#elif defined(BOOST_ASIO_MSVC)
__declspec(selectany) executor_t executor;
#endif

/// Awaitable type that returns the cancellation state of the current coroutine.
struct cancellation_state_t
{
  BOOST_ASIO_CONSTEXPR cancellation_state_t()
  {
  }
};

/// Awaitable object that returns the cancellation state of the current
/// coroutine.
/**
 * @par Example
 * @code boost::asio::awaitable<void> my_coroutine()
 * {
 *   boost::asio::cancellation_state cs
 *     = co_await boost::asio::this_coro::cancellation_state;
 *
 *   // ...
 *
 *   if (cs.cancelled() != boost::asio::cancellation_type::none)
 *     // ...
 * } @endcode
 */
#if defined(BOOST_ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr cancellation_state_t cancellation_state;
#elif defined(BOOST_ASIO_MSVC)
__declspec(selectany) cancellation_state_t cancellation_state;
#endif

#if defined(GENERATING_DOCUMENTATION)

/// Returns an awaitable object that may be used to reset the cancellation state
/// of the current coroutine.
/**
 * Let <tt>P</tt> be the cancellation slot associated with the current
 * coroutine's `co_spawn` completion handler. Assigns a new
 * boost::asio::cancellation_state object <tt>S</tt>, constructed as
 * <tt>S(P)</tt>, into the current coroutine's cancellation state object.
 *
 * @par Example
 * @code boost::asio::awaitable<void> my_coroutine()
 * {
 *   co_await boost::asio::this_coro::reset_cancellation_state();
 *
 *   // ...
 * } @endcode
 *
 * @note The cancellation state is shared by all coroutines in the same "thread
 * of execution" that was created using boost::asio::co_spawn.
 */
BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR unspecified
reset_cancellation_state();

/// Returns an awaitable object that may be used to reset the cancellation state
/// of the current coroutine.
/**
 * Let <tt>P</tt> be the cancellation slot associated with the current
 * coroutine's `co_spawn` completion handler. Assigns a new
 * boost::asio::cancellation_state object <tt>S</tt>, constructed as <tt>S(P,
 * std::forward<Filter>(filter))</tt>, into the current coroutine's
 * cancellation state object.
 *
 * @par Example
 * @code boost::asio::awaitable<void> my_coroutine()
 * {
 *   co_await boost::asio::this_coro::reset_cancellation_state(
 *       boost::asio::enable_partial_cancellation());
 *
 *   // ...
 * } @endcode
 *
 * @note The cancellation state is shared by all coroutines in the same "thread
 * of execution" that was created using boost::asio::co_spawn.
 */
template <typename Filter>
BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR unspecified
reset_cancellation_state(BOOST_ASIO_MOVE_ARG(Filter) filter);

/// Returns an awaitable object that may be used to reset the cancellation state
/// of the current coroutine.
/**
 * Let <tt>P</tt> be the cancellation slot associated with the current
 * coroutine's `co_spawn` completion handler. Assigns a new
 * boost::asio::cancellation_state object <tt>S</tt>, constructed as <tt>S(P,
 * std::forward<InFilter>(in_filter),
 * std::forward<OutFilter>(out_filter))</tt>, into the current coroutine's
 * cancellation state object.
 *
 * @par Example
 * @code boost::asio::awaitable<void> my_coroutine()
 * {
 *   co_await boost::asio::this_coro::reset_cancellation_state(
 *       boost::asio::enable_partial_cancellation(),
 *       boost::asio::disable_cancellation());
 *
 *   // ...
 * } @endcode
 *
 * @note The cancellation state is shared by all coroutines in the same "thread
 * of execution" that was created using boost::asio::co_spawn.
 */
template <typename InFilter, typename OutFilter>
BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR unspecified
reset_cancellation_state(
    BOOST_ASIO_MOVE_ARG(InFilter) in_filter,
    BOOST_ASIO_MOVE_ARG(OutFilter) out_filter);

/// Returns an awaitable object that may be used to determine whether the
/// coroutine throws if trying to suspend when it has been cancelled.
/**
 * @par Example
 * @code boost::asio::awaitable<void> my_coroutine()
 * {
 *   if (co_await boost::asio::this_coro::throw_if_cancelled)
 *     // ...
 *
 *   // ...
 * } @endcode
 */
BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR unspecified
throw_if_cancelled();

/// Returns an awaitable object that may be used to specify whether the
/// coroutine throws if trying to suspend when it has been cancelled.
/**
 * @par Example
 * @code boost::asio::awaitable<void> my_coroutine()
 * {
 *   co_await boost::asio::this_coro::throw_if_cancelled(false);
 *
 *   // ...
 * } @endcode
 */
BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR unspecified
throw_if_cancelled(bool value);

#else // defined(GENERATING_DOCUMENTATION)

struct reset_cancellation_state_0_t
{
  BOOST_ASIO_CONSTEXPR reset_cancellation_state_0_t()
  {
  }
};

BOOST_ASIO_NODISCARD inline BOOST_ASIO_CONSTEXPR reset_cancellation_state_0_t
reset_cancellation_state()
{
  return reset_cancellation_state_0_t();
}

template <typename Filter>
struct reset_cancellation_state_1_t
{
  template <typename F>
  BOOST_ASIO_CONSTEXPR reset_cancellation_state_1_t(
      BOOST_ASIO_MOVE_ARG(F) filter)
    : filter(BOOST_ASIO_MOVE_CAST(F)(filter))
  {
  }

  Filter filter;
};

template <typename Filter>
BOOST_ASIO_NODISCARD inline BOOST_ASIO_CONSTEXPR reset_cancellation_state_1_t<
    typename decay<Filter>::type>
reset_cancellation_state(BOOST_ASIO_MOVE_ARG(Filter) filter)
{
  return reset_cancellation_state_1_t<typename decay<Filter>::type>(
      BOOST_ASIO_MOVE_CAST(Filter)(filter));
}

template <typename InFilter, typename OutFilter>
struct reset_cancellation_state_2_t
{
  template <typename F1, typename F2>
  BOOST_ASIO_CONSTEXPR reset_cancellation_state_2_t(
      BOOST_ASIO_MOVE_ARG(F1) in_filter, BOOST_ASIO_MOVE_ARG(F2) out_filter)
    : in_filter(BOOST_ASIO_MOVE_CAST(F1)(in_filter)),
      out_filter(BOOST_ASIO_MOVE_CAST(F2)(out_filter))
  {
  }

  InFilter in_filter;
  OutFilter out_filter;
};

template <typename InFilter, typename OutFilter>
BOOST_ASIO_NODISCARD inline BOOST_ASIO_CONSTEXPR reset_cancellation_state_2_t<
    typename decay<InFilter>::type,
    typename decay<OutFilter>::type>
reset_cancellation_state(
    BOOST_ASIO_MOVE_ARG(InFilter) in_filter,
    BOOST_ASIO_MOVE_ARG(OutFilter) out_filter)
{
  return reset_cancellation_state_2_t<
      typename decay<InFilter>::type,
      typename decay<OutFilter>::type>(
        BOOST_ASIO_MOVE_CAST(InFilter)(in_filter),
        BOOST_ASIO_MOVE_CAST(OutFilter)(out_filter));
}

struct throw_if_cancelled_0_t
{
  BOOST_ASIO_CONSTEXPR throw_if_cancelled_0_t()
  {
  }
};

BOOST_ASIO_NODISCARD inline BOOST_ASIO_CONSTEXPR throw_if_cancelled_0_t
throw_if_cancelled()
{
  return throw_if_cancelled_0_t();
}

struct throw_if_cancelled_1_t
{
  BOOST_ASIO_CONSTEXPR throw_if_cancelled_1_t(bool value)
    : value(value)
  {
  }

  bool value;
};

BOOST_ASIO_NODISCARD inline BOOST_ASIO_CONSTEXPR throw_if_cancelled_1_t
throw_if_cancelled(bool value)
{
  return throw_if_cancelled_1_t(value);
}

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace this_coro
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_THIS_CORO_HPP
