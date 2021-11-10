//
// experimental/use_coro.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2021 Klemens D. Morgenstern
//                    (klemens dot morgenstern at gmx dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_USE_CORO_HPP
#define BOOST_ASIO_EXPERIMENTAL_USE_CORO_HPP

#include <boost/asio/detail/config.hpp>
#include <optional>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/experimental/detail/partial_promise.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

class any_io_executor;

namespace experimental {

template <typename Executor = any_io_executor>
struct use_coro_t
{
};

constexpr use_coro_t<> use_coro;

template <typename Yield, typename Return, typename Executor>
struct coro;

namespace detail {

template <typename Yield, typename Return, typename Executor>
struct coro_promise;

template <typename Executor, typename... Ts>
struct coro_init_handler
{
  struct handler_t
  {
  };

  constexpr static handler_t handler{};

  struct init_helper;

  struct promise_type
  {
    auto initial_suspend() noexcept { return suspend_always{}; }

    auto final_suspend() noexcept { return suspend_always(); }

    void return_void() {}

    void unhandled_exception() { assert(false); }

    auto await_transform(handler_t)
    {
      assert(executor);
      assert(h);
      return init_helper{this};
    }

    std::optional<Executor> executor;
    std::optional<std::tuple<Ts...>> result;
    coroutine_handle<> h;

    coro_init_handler get_return_object() { return coro_init_handler{this}; }

    cancellation_slot cancel_slot;
  };

  struct init_helper
  {
    promise_type *self_;

    constexpr static bool await_ready() noexcept { return true; }

    constexpr static void await_suspend(coroutine_handle<>) noexcept {}

    auto await_resume() const noexcept
    {
      assert(self_);
      return bind_cancellation_slot(self_->cancel_slot,
          bind_executor(*self_->executor, [self = self_](Ts... ts)
          {
            self->result.emplace(std::move(ts)...);
            self->h.resume();
          }));
    }
  };

  promise_type* promise;

  void unhandled_exception() noexcept
  {
    throw;
  }

  struct noexcept_version
  {
    promise_type *promise;

    constexpr static bool await_ready() noexcept { return false; }

    template <typename Yield, typename Return,
        convertible_to<Executor> Executor1>
    auto await_suspend(
        coroutine_handle<coro_promise<Yield, Return, Executor1> > h) noexcept
    {
      promise->executor = h.promise().get_executor();
      promise->h = h;
      return coroutine_handle<promise_type>::from_promise(*promise);
    }

    template <typename... Args>
    static auto resume_impl(std::tuple<Args...>&& tup)
    {
      return std::move(tup);
    }

    template <typename Arg>
    static auto resume_impl(std::tuple<Arg>&& tup)
    {
      return get<0>(std::move(tup));
    }

    static void resume_impl(std::tuple<>&&) {}

    auto await_resume() const noexcept
    {
      auto res = std::move(promise->result.value());
      coroutine_handle<promise_type>::from_promise(*promise).destroy();
      return resume_impl(std::move(res));
    }
  };

  struct throwing_version
  {
    promise_type *promise;

    constexpr static bool await_ready() noexcept { return false; }

    template <typename Yield, typename Return,
        convertible_to<Executor> Executor1>
    auto await_suspend(
        coroutine_handle<coro_promise<Yield, Return, Executor1> > h) noexcept
    {
      promise->executor = h.promise().get_executor();
      promise->h = h;
      return coroutine_handle<promise_type>::from_promise(*promise);
    }

    template <typename... Args>
    static auto resume_impl(std::tuple<Args...>&& tup)
    {
      return std::move(tup);
    }

    static void resume_impl(std::tuple<>&&) {}

    template <typename Arg>
    static auto resume_impl(std::tuple<Arg>&& tup)
    {
      return get<0>(std::move(tup));
    }

    template <typename... Args>
    static auto resume_impl(std::tuple<std::exception_ptr, Args...>&& tup)
    {
      auto ex = get<0>(std::move(tup));
      if (ex)
        std::rethrow_exception(ex);

      if constexpr (sizeof...(Args) == 0u)
        return;
      else if constexpr (sizeof...(Args) == 1u)
        return get<1>(std::move(tup));
      else
      {
        return
          [&]<std::size_t... Idx>(std::index_sequence<Idx...>)
          {
            return std::make_tuple(std::get<Idx + 1>(std::move(tup))...);
          }(std::make_index_sequence<sizeof...(Args) - 1>{});
      }
    }

    template <typename... Args>
    static auto resume_impl(
        std::tuple<boost::system::error_code, Args...>&& tup)
    {
      auto ec = get<0>(std::move(tup));
      if (ec)
        boost::asio::detail::throw_exception(
            boost::system::system_error(ec, "error_code in use_coro"));

      if constexpr (sizeof...(Args) == 0u)
        return;
      else if constexpr (sizeof...(Args) == 1u)
        return get<1>(std::move(tup));
      else
        return
          [&]<std::size_t... Idx>(std::index_sequence<Idx...>)
          {
            return std::make_tuple(std::get<Idx + 1>(std::move(tup))...);
          }(std::make_index_sequence<sizeof...(Args) - 1>{});
    }

    static auto resume_impl(std::tuple<std::exception_ptr>&& tup)
    {
      auto ex = get<0>(std::move(tup));
      if (ex)
        std::rethrow_exception(ex);
    }

    static auto resume_impl(
        std::tuple<boost::system::error_code>&& tup)
    {
      auto ec = get<0>(std::move(tup));
      if (ec)
        boost::asio::detail::throw_error(ec, "error_code in use_coro");
    }

    auto await_resume() const
    {
      auto res = std::move(promise->result.value());
      coroutine_handle<promise_type>::from_promise(*promise).destroy();
      return resume_impl(std::move(res));
    }
  };

  auto as_noexcept(cancellation_slot&& sl) && noexcept
  {
    promise->cancel_slot = std::move(sl);
    return noexcept_version{promise};
  }

  auto as_throwing(cancellation_slot&& sl) && noexcept
  {
    promise->cancel_slot = std::move(sl);
    return throwing_version{promise};
  }
};

} // namespace detail
} // namespace experimental

#if !defined(GENERATING_DOCUMENTATION)

template <typename Executor, typename R, typename... Args>
struct async_result<experimental::use_coro_t<Executor>, R(Args...)>
{
  using return_type = experimental::detail::coro_init_handler<
    Executor, typename decay<Args>::type...>;

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(Initiation initiation,
      experimental::use_coro_t<Executor>, InitArgs... args)
  {
    std::move(initiation)(co_await return_type::handler, std::move(args)...);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/experimental/coro.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_USE_CORO_HPP
