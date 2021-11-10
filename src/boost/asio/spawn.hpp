//
// spawn.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SPAWN_HPP
#define BOOST_ASIO_SPAWN_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/wrapped_handler.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/is_executor.hpp>
#include <boost/asio/strand.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Context object the represents the currently executing coroutine.
/**
 * The basic_yield_context class is used to represent the currently executing
 * stackful coroutine. A basic_yield_context may be passed as a handler to an
 * asynchronous operation. For example:
 *
 * @code template <typename Handler>
 * void my_coroutine(basic_yield_context<Handler> yield)
 * {
 *   ...
 *   std::size_t n = my_socket.async_read_some(buffer, yield);
 *   ...
 * } @endcode
 *
 * The initiating function (async_read_some in the above example) suspends the
 * current coroutine. The coroutine is resumed when the asynchronous operation
 * completes, and the result of the operation is returned.
 */
template <typename Handler>
class basic_yield_context
{
public:
  /// The coroutine callee type, used by the implementation.
  /**
   * When using Boost.Coroutine v1, this type is:
   * @code typename coroutine<void()> @endcode
   * When using Boost.Coroutine v2 (unidirectional coroutines), this type is:
   * @code push_coroutine<void> @endcode
   */
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined callee_type;
#elif defined(BOOST_COROUTINES_UNIDIRECT) || defined(BOOST_COROUTINES_V2)
  typedef boost::coroutines::push_coroutine<void> callee_type;
#else
  typedef boost::coroutines::coroutine<void()> callee_type;
#endif
  
  /// The coroutine caller type, used by the implementation.
  /**
   * When using Boost.Coroutine v1, this type is:
   * @code typename coroutine<void()>::caller_type @endcode
   * When using Boost.Coroutine v2 (unidirectional coroutines), this type is:
   * @code pull_coroutine<void> @endcode
   */
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined caller_type;
#elif defined(BOOST_COROUTINES_UNIDIRECT) || defined(BOOST_COROUTINES_V2)
  typedef boost::coroutines::pull_coroutine<void> caller_type;
#else
  typedef boost::coroutines::coroutine<void()>::caller_type caller_type;
#endif

  /// Construct a yield context to represent the specified coroutine.
  /**
   * Most applications do not need to use this constructor. Instead, the
   * spawn() function passes a yield context as an argument to the coroutine
   * function.
   */
  basic_yield_context(
      const detail::weak_ptr<callee_type>& coro,
      caller_type& ca, Handler& handler)
    : coro_(coro),
      ca_(ca),
      handler_(handler),
      ec_(0)
  {
  }

  /// Construct a yield context from another yield context type.
  /**
   * Requires that OtherHandler be convertible to Handler.
   */
  template <typename OtherHandler>
  basic_yield_context(const basic_yield_context<OtherHandler>& other)
    : coro_(other.coro_),
      ca_(other.ca_),
      handler_(other.handler_),
      ec_(other.ec_)
  {
  }

  /// Return a yield context that sets the specified error_code.
  /**
   * By default, when a yield context is used with an asynchronous operation, a
   * non-success error_code is converted to system_error and thrown. This
   * operator may be used to specify an error_code object that should instead be
   * set with the asynchronous operation's result. For example:
   *
   * @code template <typename Handler>
   * void my_coroutine(basic_yield_context<Handler> yield)
   * {
   *   ...
   *   std::size_t n = my_socket.async_read_some(buffer, yield[ec]);
   *   if (ec)
   *   {
   *     // An error occurred.
   *   }
   *   ...
   * } @endcode
   */
  basic_yield_context operator[](boost::system::error_code& ec) const
  {
    basic_yield_context tmp(*this);
    tmp.ec_ = &ec;
    return tmp;
  }

#if defined(GENERATING_DOCUMENTATION)
private:
#endif // defined(GENERATING_DOCUMENTATION)
  detail::weak_ptr<callee_type> coro_;
  caller_type& ca_;
  Handler handler_;
  boost::system::error_code* ec_;
};

#if defined(GENERATING_DOCUMENTATION)
/// Context object that represents the currently executing coroutine.
typedef basic_yield_context<unspecified> yield_context;
#else // defined(GENERATING_DOCUMENTATION)
typedef basic_yield_context<
  executor_binder<void(*)(), any_io_executor> > yield_context;
#endif // defined(GENERATING_DOCUMENTATION)

/**
 * @defgroup spawn boost::asio::spawn
 *
 * @brief Start a new stackful coroutine.
 *
 * The spawn() function is a high-level wrapper over the Boost.Coroutine
 * library. This function enables programs to implement asynchronous logic in a
 * synchronous manner, as illustrated by the following example:
 *
 * @code boost::asio::spawn(my_strand, do_echo);
 *
 * // ...
 *
 * void do_echo(boost::asio::yield_context yield)
 * {
 *   try
 *   {
 *     char data[128];
 *     for (;;)
 *     {
 *       std::size_t length =
 *         my_socket.async_read_some(
 *           boost::asio::buffer(data), yield);
 *
 *       boost::asio::async_write(my_socket,
 *           boost::asio::buffer(data, length), yield);
 *     }
 *   }
 *   catch (std::exception& e)
 *   {
 *     // ...
 *   }
 * } @endcode
 */
/*@{*/

/// Start a new stackful coroutine, calling the specified handler when it
/// completes.
/**
 * This function is used to launch a new coroutine.
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(basic_yield_context<Handler> yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Function>
void spawn(BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes());

/// Start a new stackful coroutine, calling the specified handler when it
/// completes.
/**
 * This function is used to launch a new coroutine.
 *
 * @param handler A handler to be called when the coroutine exits. More
 * importantly, the handler provides an execution context (via the the handler
 * invocation hook) for the coroutine. The handler must have the signature:
 * @code void handler(); @endcode
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(basic_yield_context<Handler> yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Handler, typename Function>
void spawn(BOOST_ASIO_MOVE_ARG(Handler) handler,
    BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes(),
    typename constraint<
      !is_executor<typename decay<Handler>::type>::value &&
      !execution::is_executor<typename decay<Handler>::type>::value &&
      !is_convertible<Handler&, execution_context&>::value>::type = 0);

/// Start a new stackful coroutine, inheriting the execution context of another.
/**
 * This function is used to launch a new coroutine.
 *
 * @param ctx Identifies the current coroutine as a parent of the new
 * coroutine. This specifies that the new coroutine should inherit the
 * execution context of the parent. For example, if the parent coroutine is
 * executing in a particular strand, then the new coroutine will execute in the
 * same strand.
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(basic_yield_context<Handler> yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Handler, typename Function>
void spawn(basic_yield_context<Handler> ctx,
    BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes());

/// Start a new stackful coroutine that executes on a given executor.
/**
 * This function is used to launch a new coroutine.
 *
 * @param ex Identifies the executor that will run the coroutine. The new
 * coroutine is implicitly given its own strand within this executor.
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Function, typename Executor>
void spawn(const Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes(),
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type = 0);

/// Start a new stackful coroutine that executes on a given strand.
/**
 * This function is used to launch a new coroutine.
 *
 * @param ex Identifies the strand that will run the coroutine.
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Function, typename Executor>
void spawn(const strand<Executor>& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes());

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)

/// Start a new stackful coroutine that executes in the context of a strand.
/**
 * This function is used to launch a new coroutine.
 *
 * @param s Identifies a strand. By starting multiple coroutines on the same
 * strand, the implementation ensures that none of those coroutines can execute
 * simultaneously.
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Function>
void spawn(const boost::asio::io_context::strand& s,
    BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes());

#endif // !defined(BOOST_ASIO_NO_TS_EXECUTORS)

/// Start a new stackful coroutine that executes on a given execution context.
/**
 * This function is used to launch a new coroutine.
 *
 * @param ctx Identifies the execution context that will run the coroutine. The
 * new coroutine is implicitly given its own strand within this execution
 * context.
 *
 * @param function The coroutine function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param attributes Boost.Coroutine attributes used to customise the coroutine.
 */
template <typename Function, typename ExecutionContext>
void spawn(ExecutionContext& ctx,
    BOOST_ASIO_MOVE_ARG(Function) function,
    const boost::coroutines::attributes& attributes
      = boost::coroutines::attributes(),
    typename constraint<is_convertible<
      ExecutionContext&, execution_context&>::value>::type = 0);

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/spawn.hpp>

#endif // BOOST_ASIO_SPAWN_HPP
