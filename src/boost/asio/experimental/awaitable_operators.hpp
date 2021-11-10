//
// experimental/awaitable_operators.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_AWAITABLE_OPERATORS_HPP
#define BOOST_ASIO_EXPERIMENTAL_AWAITABLE_OPERATORS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <variant>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/experimental/deferred.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/multiple_exceptions.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {
namespace awaitable_operators {
namespace detail {

template <typename T, typename Executor>
awaitable<T, Executor> awaitable_wrap(awaitable<T, Executor> a,
    typename constraint<is_constructible<T>::value>::type* = 0)
{
  return a;
}

template <typename T, typename Executor>
awaitable<std::optional<T>, Executor> awaitable_wrap(awaitable<T, Executor> a,
    typename constraint<!is_constructible<T>::value>::type* = 0)
{
  co_return std::optional<T>(co_await std::move(a));
}

template <typename T>
T& awaitable_unwrap(typename conditional<true, T, void>::type& r,
    typename constraint<is_constructible<T>::value>::type* = 0)
{
  return r;
}

template <typename T>
T& awaitable_unwrap(std::optional<typename conditional<true, T, void>::type>& r,
    typename constraint<!is_constructible<T>::value>::type* = 0)
{
  return *r;
}

} // namespace detail

/// Wait for both operations to succeed.
/**
 * If one operations fails, the other is cancelled as the AND-condition can no
 * longer be satisfied.
 */
template <typename Executor>
awaitable<void, Executor> operator&&(
    awaitable<void, Executor> t, awaitable<void, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, ex1] =
    co_await make_parallel_group(
      co_spawn(ex, std::move(t), deferred),
      co_spawn(ex, std::move(u), deferred)
    ).async_wait(
      wait_for_one_error(),
      use_awaitable_t<Executor>{}
    );

  if (ex0 && ex1)
    throw multiple_exceptions(ex0);
  if (ex0)
    std::rethrow_exception(ex0);
  if (ex1)
    std::rethrow_exception(ex1);
  co_return;
}

/// Wait for both operations to succeed.
/**
 * If one operations fails, the other is cancelled as the AND-condition can no
 * longer be satisfied.
 */
template <typename U, typename Executor>
awaitable<U, Executor> operator&&(
    awaitable<void, Executor> t, awaitable<U, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, std::move(t), deferred),
      co_spawn(ex, detail::awaitable_wrap(std::move(u)), deferred)
    ).async_wait(
      wait_for_one_error(),
      use_awaitable_t<Executor>{}
    );

  if (ex0 && ex1)
    throw multiple_exceptions(ex0);
  if (ex0)
    std::rethrow_exception(ex0);
  if (ex1)
    std::rethrow_exception(ex1);
  co_return std::move(detail::awaitable_unwrap<U>(r1));
}

/// Wait for both operations to succeed.
/**
 * If one operations fails, the other is cancelled as the AND-condition can no
 * longer be satisfied.
 */
template <typename T, typename Executor>
awaitable<T, Executor> operator&&(
    awaitable<T, Executor> t, awaitable<void, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, std::move(u), deferred)
    ).async_wait(
      wait_for_one_error(),
      use_awaitable_t<Executor>{}
    );

  if (ex0 && ex1)
    throw multiple_exceptions(ex0);
  if (ex0)
    std::rethrow_exception(ex0);
  if (ex1)
    std::rethrow_exception(ex1);
  co_return std::move(detail::awaitable_unwrap<T>(r0));
}

/// Wait for both operations to succeed.
/**
 * If one operations fails, the other is cancelled as the AND-condition can no
 * longer be satisfied.
 */
template <typename T, typename U, typename Executor>
awaitable<std::tuple<T, U>, Executor> operator&&(
    awaitable<T, Executor> t, awaitable<U, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, detail::awaitable_wrap(std::move(u)), deferred)
    ).async_wait(
      wait_for_one_error(),
      use_awaitable_t<Executor>{}
    );

  if (ex0 && ex1)
    throw multiple_exceptions(ex0);
  if (ex0)
    std::rethrow_exception(ex0);
  if (ex1)
    std::rethrow_exception(ex1);
  co_return std::make_tuple(
      std::move(detail::awaitable_unwrap<T>(r0)),
      std::move(detail::awaitable_unwrap<U>(r1)));
}

/// Wait for both operations to succeed.
/**
 * If one operations fails, the other is cancelled as the AND-condition can no
 * longer be satisfied.
 */
template <typename... T, typename Executor>
awaitable<std::tuple<T..., std::monostate>, Executor> operator&&(
    awaitable<std::tuple<T...>, Executor> t, awaitable<void, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, std::move(u), deferred)
    ).async_wait(
      wait_for_one_error(),
      use_awaitable_t<Executor>{}
    );

  if (ex0 && ex1)
    throw multiple_exceptions(ex0);
  if (ex0)
    std::rethrow_exception(ex0);
  if (ex1)
    std::rethrow_exception(ex1);
  co_return std::move(detail::awaitable_unwrap<std::tuple<T...>>(r0));
}

/// Wait for both operations to succeed.
/**
 * If one operations fails, the other is cancelled as the AND-condition can no
 * longer be satisfied.
 */
template <typename... T, typename U, typename Executor>
awaitable<std::tuple<T..., U>, Executor> operator&&(
    awaitable<std::tuple<T...>, Executor> t, awaitable<U, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, detail::awaitable_wrap(std::move(u)), deferred)
    ).async_wait(
      wait_for_one_error(),
      use_awaitable_t<Executor>{}
    );

  if (ex0 && ex1)
    throw multiple_exceptions(ex0);
  if (ex0)
    std::rethrow_exception(ex0);
  if (ex1)
    std::rethrow_exception(ex1);
  co_return std::tuple_cat(
      std::move(detail::awaitable_unwrap<std::tuple<T...>>(r0)),
      std::make_tuple(std::move(detail::awaitable_unwrap<U>(r1))));
}

/// Wait for one operation to succeed.
/**
 * If one operations succeeds, the other is cancelled as the OR-condition is
 * already satisfied.
 */
template <typename Executor>
awaitable<std::variant<std::monostate, std::monostate>, Executor> operator||(
    awaitable<void, Executor> t, awaitable<void, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, ex1] =
    co_await make_parallel_group(
      co_spawn(ex, std::move(t), deferred),
      co_spawn(ex, std::move(u), deferred)
    ).async_wait(
      wait_for_one_success(),
      use_awaitable_t<Executor>{}
    );

  if (order[0] == 0)
  {
    if (!ex0)
      co_return std::variant<std::monostate, std::monostate>{
          std::in_place_index<0>};
    if (!ex1)
      co_return std::variant<std::monostate, std::monostate>{
          std::in_place_index<1>};
    throw multiple_exceptions(ex0);
  }
  else
  {
    if (!ex1)
      co_return std::variant<std::monostate, std::monostate>{
          std::in_place_index<1>};
    if (!ex0)
      co_return std::variant<std::monostate, std::monostate>{
          std::in_place_index<0>};
    throw multiple_exceptions(ex1);
  }
}

/// Wait for one operation to succeed.
/**
 * If one operations succeeds, the other is cancelled as the OR-condition is
 * already satisfied.
 */
template <typename U, typename Executor>
awaitable<std::variant<std::monostate, U>, Executor> operator||(
    awaitable<void, Executor> t, awaitable<U, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, std::move(t), deferred),
      co_spawn(ex, detail::awaitable_wrap(std::move(u)), deferred)
    ).async_wait(
      wait_for_one_success(),
      use_awaitable_t<Executor>{}
    );

  if (order[0] == 0)
  {
    if (!ex0)
      co_return std::variant<std::monostate, U>{
          std::in_place_index<0>};
    if (!ex1)
      co_return std::variant<std::monostate, U>{
          std::in_place_index<1>,
          std::move(detail::awaitable_unwrap<U>(r1))};
    throw multiple_exceptions(ex0);
  }
  else
  {
    if (!ex1)
      co_return std::variant<std::monostate, U>{
          std::in_place_index<1>,
          std::move(detail::awaitable_unwrap<U>(r1))};
    if (!ex0)
      co_return std::variant<std::monostate, U>{
          std::in_place_index<0>};
    throw multiple_exceptions(ex1);
  }
}

/// Wait for one operation to succeed.
/**
 * If one operations succeeds, the other is cancelled as the OR-condition is
 * already satisfied.
 */
template <typename T, typename Executor>
awaitable<std::variant<T, std::monostate>, Executor> operator||(
    awaitable<T, Executor> t, awaitable<void, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, std::move(u), deferred)
    ).async_wait(
      wait_for_one_success(),
      use_awaitable_t<Executor>{}
    );

  if (order[0] == 0)
  {
    if (!ex0)
      co_return std::variant<T, std::monostate>{
          std::in_place_index<0>,
          std::move(detail::awaitable_unwrap<T>(r0))};
    if (!ex1)
      co_return std::variant<T, std::monostate>{
          std::in_place_index<1>};
    throw multiple_exceptions(ex0);
  }
  else
  {
    if (!ex1)
      co_return std::variant<T, std::monostate>{
          std::in_place_index<1>};
    if (!ex0)
      co_return std::variant<T, std::monostate>{
          std::in_place_index<0>,
          std::move(detail::awaitable_unwrap<T>(r0))};
    throw multiple_exceptions(ex1);
  }
}

/// Wait for one operation to succeed.
/**
 * If one operations succeeds, the other is cancelled as the OR-condition is
 * already satisfied.
 */
template <typename T, typename U, typename Executor>
awaitable<std::variant<T, U>, Executor> operator||(
    awaitable<T, Executor> t, awaitable<U, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, detail::awaitable_wrap(std::move(u)), deferred)
    ).async_wait(
      wait_for_one_success(),
      use_awaitable_t<Executor>{}
    );

  if (order[0] == 0)
  {
    if (!ex0)
      co_return std::variant<T, U>{
          std::in_place_index<0>,
          std::move(detail::awaitable_unwrap<T>(r0))};
    if (!ex1)
      co_return std::variant<T, U>{
          std::in_place_index<1>,
          std::move(detail::awaitable_unwrap<U>(r1))};
    throw multiple_exceptions(ex0);
  }
  else
  {
    if (!ex1)
      co_return std::variant<T, U>{
          std::in_place_index<1>,
          std::move(detail::awaitable_unwrap<U>(r1))};
    if (!ex0)
      co_return std::variant<T, U>{
          std::in_place_index<0>,
          std::move(detail::awaitable_unwrap<T>(r0))};
    throw multiple_exceptions(ex1);
  }
}

namespace detail {

template <typename... T>
struct widen_variant
{
  template <std::size_t I, typename SourceVariant>
  static std::variant<T...> call(SourceVariant& source)
  {
    if (source.index() == I)
      return std::variant<T...>{
          std::in_place_index<I>, std::move(std::get<I>(source))};
    else if constexpr (I + 1 < std::variant_size_v<SourceVariant>)
      return call<I + 1>(source);
    else
      throw std::logic_error("empty variant");
  }
};

} // namespace detail

/// Wait for one operation to succeed.
/**
 * If one operations succeeds, the other is cancelled as the OR-condition is
 * already satisfied.
 */
template <typename... T, typename Executor>
awaitable<std::variant<T..., std::monostate>, Executor> operator||(
    awaitable<std::variant<T...>, Executor> t, awaitable<void, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, std::move(u), deferred)
    ).async_wait(
      wait_for_one_success(),
      use_awaitable_t<Executor>{}
    );

  using widen = detail::widen_variant<T..., std::monostate>;
  if (order[0] == 0)
  {
    if (!ex0)
      co_return widen::template call<0>(
          detail::awaitable_unwrap<std::variant<T...>>(r0));
    if (!ex1)
      co_return std::variant<T..., std::monostate>{
          std::in_place_index<sizeof...(T)>};
    throw multiple_exceptions(ex0);
  }
  else
  {
    if (!ex1)
      co_return std::variant<T..., std::monostate>{
          std::in_place_index<sizeof...(T)>};
    if (!ex0)
      co_return widen::template call<0>(
          detail::awaitable_unwrap<std::variant<T...>>(r0));
    throw multiple_exceptions(ex1);
  }
}

/// Wait for one operation to succeed.
/**
 * If one operations succeeds, the other is cancelled as the OR-condition is
 * already satisfied.
 */
template <typename... T, typename U, typename Executor>
awaitable<std::variant<T..., U>, Executor> operator||(
    awaitable<std::variant<T...>, Executor> t, awaitable<U, Executor> u)
{
  auto ex = co_await this_coro::executor;

  auto [order, ex0, r0, ex1, r1] =
    co_await make_parallel_group(
      co_spawn(ex, detail::awaitable_wrap(std::move(t)), deferred),
      co_spawn(ex, detail::awaitable_wrap(std::move(u)), deferred)
    ).async_wait(
      wait_for_one_success(),
      use_awaitable_t<Executor>{}
    );

  using widen = detail::widen_variant<T..., U>;
  if (order[0] == 0)
  {
    if (!ex0)
      co_return widen::template call<0>(
          detail::awaitable_unwrap<std::variant<T...>>(r0));
    if (!ex1)
      co_return std::variant<T..., U>{
          std::in_place_index<sizeof...(T)>,
          std::move(detail::awaitable_unwrap<U>(r1))};
    throw multiple_exceptions(ex0);
  }
  else
  {
    if (!ex1)
      co_return std::variant<T..., U>{
          std::in_place_index<sizeof...(T)>,
          std::move(detail::awaitable_unwrap<U>(r1))};
    if (!ex0)
      co_return widen::template call<0>(
          detail::awaitable_unwrap<std::variant<T...>>(r0));
    throw multiple_exceptions(ex1);
  }
}

} // namespace awaitable_operators
} // namespace experimental
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_AWAITABLE_OPERATORS_HPP
