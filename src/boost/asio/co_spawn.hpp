//
// co_spawn.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_CO_SPAWN_HPP
#define BOOST_ASIO_CO_SPAWN_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_CO_AWAIT) || defined(GENERATING_DOCUMENTATION)

#include <boost/asio/awaitable.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/is_executor.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename T>
struct awaitable_signature;

template <typename T, typename Executor>
struct awaitable_signature<awaitable<T, Executor>>
{
  typedef void type(std::exception_ptr, T);
};

template <typename Executor>
struct awaitable_signature<awaitable<void, Executor>>
{
  typedef void type(std::exception_ptr);
};

} // namespace detail

/// Spawn a new coroutined-based thread of execution.
/**
 * @param ex The executor that will be used to schedule the new thread of
 * execution.
 *
 * @param a The boost::asio::awaitable object that is the result of calling the
 * coroutine's entry point function.
 *
 * @param token The completion token that will handle the notification that
 * the thread of execution has completed. The function signature of the
 * completion handler must be:
 * @code void handler(std::exception_ptr, T); @endcode
 *
 * @par Example
 * @code
 * boost::asio::awaitable<std::size_t> echo(tcp::socket socket)
 * {
 *   std::size_t bytes_transferred = 0;
 *
 *   try
 *   {
 *     char data[1024];
 *     for (;;)
 *     {
 *       std::size_t n = co_await socket.async_read_some(
 *           boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *       co_await boost::asio::async_write(socket,
 *           boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *
 *       bytes_transferred += n;
 *     }
 *   }
 *   catch (const std::exception&)
 *   {
 *   }
 *
 *   co_return bytes_transferred;
 * }
 *
 * // ...
 *
 * boost::asio::co_spawn(my_executor,
 *   echo(std::move(my_tcp_socket)),
 *   [](std::exception_ptr e, std::size_t n)
 *   {
 *     std::cout << "transferred " << n << "\n";
 *   });
 * @endcode
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call boost::asio::this_coro::reset_cancellation_state.
 */
template <typename Executor, typename T, typename AwaitableExecutor,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(
      void(std::exception_ptr, T)) CompletionToken
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken, void(std::exception_ptr, T))
co_spawn(const Executor& ex, awaitable<T, AwaitableExecutor> a,
    CompletionToken&& token
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor),
    typename constraint<
      (is_executor<Executor>::value || execution::is_executor<Executor>::value)
        && is_convertible<Executor, AwaitableExecutor>::value
    >::type = 0);

/// Spawn a new coroutined-based thread of execution.
/**
 * @param ex The executor that will be used to schedule the new thread of
 * execution.
 *
 * @param a The boost::asio::awaitable object that is the result of calling the
 * coroutine's entry point function.
 *
 * @param token The completion token that will handle the notification that
 * the thread of execution has completed. The function signature of the
 * completion handler must be:
 * @code void handler(std::exception_ptr); @endcode
 *
 * @par Example
 * @code
 * boost::asio::awaitable<void> echo(tcp::socket socket)
 * {
 *   try
 *   {
 *     char data[1024];
 *     for (;;)
 *     {
 *       std::size_t n = co_await socket.async_read_some(
 *           boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *       co_await boost::asio::async_write(socket,
 *           boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *     }
 *   }
 *   catch (const std::exception& e)
 *   {
 *     std::cerr << "Exception: " << e.what() << "\n";
 *   }
 * }
 *
 * // ...
 *
 * boost::asio::co_spawn(my_executor,
 *   echo(std::move(my_tcp_socket)),
 *   boost::asio::detached);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call boost::asio::this_coro::reset_cancellation_state.
 */
template <typename Executor, typename AwaitableExecutor,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(
      void(std::exception_ptr)) CompletionToken
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken, void(std::exception_ptr))
co_spawn(const Executor& ex, awaitable<void, AwaitableExecutor> a,
    CompletionToken&& token
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor),
    typename constraint<
      (is_executor<Executor>::value || execution::is_executor<Executor>::value)
        && is_convertible<Executor, AwaitableExecutor>::value
    >::type = 0);

/// Spawn a new coroutined-based thread of execution.
/**
 * @param ctx An execution context that will provide the executor to be used to
 * schedule the new thread of execution.
 *
 * @param a The boost::asio::awaitable object that is the result of calling the
 * coroutine's entry point function.
 *
 * @param token The completion token that will handle the notification that
 * the thread of execution has completed. The function signature of the
 * completion handler must be:
 * @code void handler(std::exception_ptr); @endcode
 *
 * @par Example
 * @code
 * boost::asio::awaitable<std::size_t> echo(tcp::socket socket)
 * {
 *   std::size_t bytes_transferred = 0;
 *
 *   try
 *   {
 *     char data[1024];
 *     for (;;)
 *     {
 *       std::size_t n = co_await socket.async_read_some(
 *           boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *       co_await boost::asio::async_write(socket,
 *           boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *
 *       bytes_transferred += n;
 *     }
 *   }
 *   catch (const std::exception&)
 *   {
 *   }
 *
 *   co_return bytes_transferred;
 * }
 *
 * // ...
 *
 * boost::asio::co_spawn(my_io_context,
 *   echo(std::move(my_tcp_socket)),
 *   [](std::exception_ptr e, std::size_t n)
 *   {
 *     std::cout << "transferred " << n << "\n";
 *   });
 * @endcode
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call boost::asio::this_coro::reset_cancellation_state.
 */
template <typename ExecutionContext, typename T, typename AwaitableExecutor,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(
      void(std::exception_ptr, T)) CompletionToken
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename ExecutionContext::executor_type)>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken, void(std::exception_ptr, T))
co_spawn(ExecutionContext& ctx, awaitable<T, AwaitableExecutor> a,
    CompletionToken&& token
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename ExecutionContext::executor_type),
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
        && is_convertible<typename ExecutionContext::executor_type,
          AwaitableExecutor>::value
    >::type = 0);

/// Spawn a new coroutined-based thread of execution.
/**
 * @param ctx An execution context that will provide the executor to be used to
 * schedule the new thread of execution.
 *
 * @param a The boost::asio::awaitable object that is the result of calling the
 * coroutine's entry point function.
 *
 * @param token The completion token that will handle the notification that
 * the thread of execution has completed. The function signature of the
 * completion handler must be:
 * @code void handler(std::exception_ptr); @endcode
 *
 * @par Example
 * @code
 * boost::asio::awaitable<void> echo(tcp::socket socket)
 * {
 *   try
 *   {
 *     char data[1024];
 *     for (;;)
 *     {
 *       std::size_t n = co_await socket.async_read_some(
 *           boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *       co_await boost::asio::async_write(socket,
 *           boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *     }
 *   }
 *   catch (const std::exception& e)
 *   {
 *     std::cerr << "Exception: " << e.what() << "\n";
 *   }
 * }
 *
 * // ...
 *
 * boost::asio::co_spawn(my_io_context,
 *   echo(std::move(my_tcp_socket)),
 *   boost::asio::detached);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call boost::asio::this_coro::reset_cancellation_state.
 */
template <typename ExecutionContext, typename AwaitableExecutor,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(
      void(std::exception_ptr)) CompletionToken
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename ExecutionContext::executor_type)>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken, void(std::exception_ptr))
co_spawn(ExecutionContext& ctx, awaitable<void, AwaitableExecutor> a,
    CompletionToken&& token
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename ExecutionContext::executor_type),
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
        && is_convertible<typename ExecutionContext::executor_type,
          AwaitableExecutor>::value
    >::type = 0);

/// Spawn a new coroutined-based thread of execution.
/**
 * @param ex The executor that will be used to schedule the new thread of
 * execution.
 *
 * @param f A nullary function object with a return type of the form
 * @c boost::asio::awaitable<R,E> that will be used as the coroutine's entry
 * point.
 *
 * @param token The completion token that will handle the notification that the
 * thread of execution has completed. If @c R is @c void, the function
 * signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Example
 * @code
 * boost::asio::awaitable<std::size_t> echo(tcp::socket socket)
 * {
 *   std::size_t bytes_transferred = 0;
 *
 *   try
 *   {
 *     char data[1024];
 *     for (;;)
 *     {
 *       std::size_t n = co_await socket.async_read_some(
 *           boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *       co_await boost::asio::async_write(socket,
 *           boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *
 *       bytes_transferred += n;
 *     }
 *   }
 *   catch (const std::exception&)
 *   {
 *   }
 *
 *   co_return bytes_transferred;
 * }
 *
 * // ...
 *
 * boost::asio::co_spawn(my_executor,
 *   [socket = std::move(my_tcp_socket)]() mutable
 *     -> boost::asio::awaitable<void>
 *   {
 *     try
 *     {
 *       char data[1024];
 *       for (;;)
 *       {
 *         std::size_t n = co_await socket.async_read_some(
 *             boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *         co_await boost::asio::async_write(socket,
 *             boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *       }
 *     }
 *     catch (const std::exception& e)
 *     {
 *       std::cerr << "Exception: " << e.what() << "\n";
 *     }
 *   }, boost::asio::detached);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call boost::asio::this_coro::reset_cancellation_state.
 */
template <typename Executor, typename F,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(typename detail::awaitable_signature<
      typename result_of<F()>::type>::type) CompletionToken
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
    typename detail::awaitable_signature<typename result_of<F()>::type>::type)
co_spawn(const Executor& ex, F&& f,
    CompletionToken&& token
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor),
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type = 0);

/// Spawn a new coroutined-based thread of execution.
/**
 * @param ctx An execution context that will provide the executor to be used to
 * schedule the new thread of execution.
 *
 * @param f A nullary function object with a return type of the form
 * @c boost::asio::awaitable<R,E> that will be used as the coroutine's entry
 * point.
 *
 * @param token The completion token that will handle the notification that the
 * thread of execution has completed. If @c R is @c void, the function
 * signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Example
 * @code
 * boost::asio::awaitable<std::size_t> echo(tcp::socket socket)
 * {
 *   std::size_t bytes_transferred = 0;
 *
 *   try
 *   {
 *     char data[1024];
 *     for (;;)
 *     {
 *       std::size_t n = co_await socket.async_read_some(
 *           boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *       co_await boost::asio::async_write(socket,
 *           boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *
 *       bytes_transferred += n;
 *     }
 *   }
 *   catch (const std::exception&)
 *   {
 *   }
 *
 *   co_return bytes_transferred;
 * }
 *
 * // ...
 *
 * boost::asio::co_spawn(my_io_context,
 *   [socket = std::move(my_tcp_socket)]() mutable
 *     -> boost::asio::awaitable<void>
 *   {
 *     try
 *     {
 *       char data[1024];
 *       for (;;)
 *       {
 *         std::size_t n = co_await socket.async_read_some(
 *             boost::asio::buffer(data), boost::asio::use_awaitable);
 *
 *         co_await boost::asio::async_write(socket,
 *             boost::asio::buffer(data, n), boost::asio::use_awaitable);
 *       }
 *     }
 *     catch (const std::exception& e)
 *     {
 *       std::cerr << "Exception: " << e.what() << "\n";
 *     }
 *   }, boost::asio::detached);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call boost::asio::this_coro::reset_cancellation_state.
 */
template <typename ExecutionContext, typename F,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(typename detail::awaitable_signature<
      typename result_of<F()>::type>::type) CompletionToken
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename ExecutionContext::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
    typename detail::awaitable_signature<typename result_of<F()>::type>::type)
co_spawn(ExecutionContext& ctx, F&& f,
    CompletionToken&& token
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename ExecutionContext::executor_type),
    typename constraint<
      is_convertible<ExecutionContext&, execution_context&>::value
    >::type = 0);

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/co_spawn.hpp>

#endif // defined(BOOST_ASIO_HAS_CO_AWAIT) || defined(GENERATING_DOCUMENTATION)

#endif // BOOST_ASIO_CO_SPAWN_HPP
