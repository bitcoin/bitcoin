//
// io_context.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IO_CONTEXT_HPP
#define BOOST_ASIO_IO_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <stdexcept>
#include <typeinfo>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/wrapped_handler.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/execution.hpp>
#include <boost/asio/execution_context.hpp>

#if defined(BOOST_ASIO_HAS_CHRONO)
# include <boost/asio/detail/chrono.hpp>
#endif // defined(BOOST_ASIO_HAS_CHRONO)

#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
# include <boost/asio/detail/winsock_init.hpp>
#elif defined(__sun) || defined(__QNX__) || defined(__hpux) || defined(_AIX) \
  || defined(__osf__)
# include <boost/asio/detail/signal_init.hpp>
#endif

#if defined(BOOST_ASIO_HAS_IOCP)
# include <boost/asio/detail/win_iocp_io_context.hpp>
#else
# include <boost/asio/detail/scheduler.hpp>
#endif

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

namespace detail {
#if defined(BOOST_ASIO_HAS_IOCP)
  typedef win_iocp_io_context io_context_impl;
  class win_iocp_overlapped_ptr;
#else
  typedef scheduler io_context_impl;
#endif

  struct io_context_bits
  {
    BOOST_ASIO_STATIC_CONSTEXPR(unsigned int, blocking_never = 1);
    BOOST_ASIO_STATIC_CONSTEXPR(unsigned int, relationship_continuation = 2);
    BOOST_ASIO_STATIC_CONSTEXPR(unsigned int, outstanding_work_tracked = 4);
  };
} // namespace detail

/// Provides core I/O functionality.
/**
 * The io_context class provides the core I/O functionality for users of the
 * asynchronous I/O objects, including:
 *
 * @li boost::asio::ip::tcp::socket
 * @li boost::asio::ip::tcp::acceptor
 * @li boost::asio::ip::udp::socket
 * @li boost::asio::deadline_timer.
 *
 * The io_context class also includes facilities intended for developers of
 * custom asynchronous services.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Safe, with the specific exceptions of the restart()
 * and notify_fork() functions. Calling restart() while there are unfinished
 * run(), run_one(), run_for(), run_until(), poll() or poll_one() calls results
 * in undefined behaviour. The notify_fork() function should not be called
 * while any io_context function, or any function on an I/O object that is
 * associated with the io_context, is being called in another thread.
 *
 * @par Concepts:
 * Dispatcher.
 *
 * @par Synchronous and asynchronous operations
 *
 * Synchronous operations on I/O objects implicitly run the io_context object
 * for an individual operation. The io_context functions run(), run_one(),
 * run_for(), run_until(), poll() or poll_one() must be called for the
 * io_context to perform asynchronous operations on behalf of a C++ program.
 * Notification that an asynchronous operation has completed is delivered by
 * invocation of the associated handler. Handlers are invoked only by a thread
 * that is currently calling any overload of run(), run_one(), run_for(),
 * run_until(), poll() or poll_one() for the io_context.
 *
 * @par Effect of exceptions thrown from handlers
 *
 * If an exception is thrown from a handler, the exception is allowed to
 * propagate through the throwing thread's invocation of run(), run_one(),
 * run_for(), run_until(), poll() or poll_one(). No other threads that are
 * calling any of these functions are affected. It is then the responsibility
 * of the application to catch the exception.
 *
 * After the exception has been caught, the run(), run_one(), run_for(),
 * run_until(), poll() or poll_one() call may be restarted @em without the need
 * for an intervening call to restart(). This allows the thread to rejoin the
 * io_context object's thread pool without impacting any other threads in the
 * pool.
 *
 * For example:
 *
 * @code
 * boost::asio::io_context io_context;
 * ...
 * for (;;)
 * {
 *   try
 *   {
 *     io_context.run();
 *     break; // run() exited normally
 *   }
 *   catch (my_exception& e)
 *   {
 *     // Deal with exception as appropriate.
 *   }
 * }
 * @endcode
 *
 * @par Submitting arbitrary tasks to the io_context
 *
 * To submit functions to the io_context, use the @ref boost::asio::dispatch,
 * @ref boost::asio::post or @ref boost::asio::defer free functions.
 *
 * For example:
 *
 * @code void my_task()
 * {
 *   ...
 * }
 *
 * ...
 *
 * boost::asio::io_context io_context;
 *
 * // Submit a function to the io_context.
 * boost::asio::post(io_context, my_task);
 *
 * // Submit a lambda object to the io_context.
 * boost::asio::post(io_context,
 *     []()
 *     {
 *       ...
 *     });
 *
 * // Run the io_context until it runs out of work.
 * io_context.run(); @endcode
 *
 * @par Stopping the io_context from running out of work
 *
 * Some applications may need to prevent an io_context object's run() call from
 * returning when there is no more work to do. For example, the io_context may
 * be being run in a background thread that is launched prior to the
 * application's asynchronous operations. The run() call may be kept running by
 * creating an executor that tracks work against the io_context:
 *
 * @code boost::asio::io_context io_context;
 * auto work = boost::asio::require(io_context.get_executor(),
 *     boost::asio::execution::outstanding_work.tracked);
 * ... @endcode
 *
 * If using C++03, which lacks automatic variable type deduction, you may
 * compute the return type of the require call:
 *
 * @code boost::asio::io_context io_context;
 * typename boost::asio::require_result<
 *     boost::asio::io_context::executor_type,
 *     boost::asio::exeution::outstanding_work_t::tracked_t>
 *   work = boost::asio::require(io_context.get_executor(),
 *     boost::asio::execution::outstanding_work.tracked);
 * ... @endcode
 *
 * or store the result in the type-erasing executor wrapper, any_io_executor:
 *
 * @code boost::asio::io_context io_context;
 * boost::asio::any_io_executor work
 *   = boost::asio::require(io_context.get_executor(),
 *       boost::asio::execution::outstanding_work.tracked);
 * ... @endcode
 *
 * To effect a shutdown, the application will then need to call the io_context
 * object's stop() member function. This will cause the io_context run() call
 * to return as soon as possible, abandoning unfinished operations and without
 * permitting ready handlers to be dispatched.
 *
 * Alternatively, if the application requires that all operations and handlers
 * be allowed to finish normally, store the work-tracking executor in an
 * any_io_executor object, so that it may be explicitly reset.
 *
 * @code boost::asio::io_context io_context;
 * boost::asio::any_io_executor work
 *   = boost::asio::require(io_context.get_executor(),
 *       boost::asio::execution::outstanding_work.tracked);
 * ...
 * work = boost::asio::any_io_executor(); // Allow run() to exit. @endcode
 */
class io_context
  : public execution_context
{
private:
  typedef detail::io_context_impl impl_type;
#if defined(BOOST_ASIO_HAS_IOCP)
  friend class detail::win_iocp_overlapped_ptr;
#endif

public:
  template <typename Allocator, unsigned int Bits>
  class basic_executor_type;

  template <typename Allocator, unsigned int Bits>
  friend class basic_executor_type;

  /// Executor used to submit functions to an io_context.
  typedef basic_executor_type<std::allocator<void>, 0> executor_type;

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  class work;
  friend class work;
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  class service;

#if !defined(BOOST_ASIO_NO_EXTENSIONS) \
  && !defined(BOOST_ASIO_NO_TS_EXECUTORS)
  class strand;
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)
       //   && !defined(BOOST_ASIO_NO_TS_EXECUTORS)

  /// The type used to count the number of handlers executed by the context.
  typedef std::size_t count_type;

  /// Constructor.
  BOOST_ASIO_DECL io_context();

  /// Constructor.
  /**
   * Construct with a hint about the required level of concurrency.
   *
   * @param concurrency_hint A suggestion to the implementation on how many
   * threads it should allow to run simultaneously.
   */
  BOOST_ASIO_DECL explicit io_context(int concurrency_hint);

  /// Destructor.
  /**
   * On destruction, the io_context performs the following sequence of
   * operations:
   *
   * @li For each service object @c svc in the io_context set, in reverse order
   * of the beginning of service object lifetime, performs
   * @c svc->shutdown().
   *
   * @li Uninvoked handler objects that were scheduled for deferred invocation
   * on the io_context, or any associated strand, are destroyed.
   *
   * @li For each service object @c svc in the io_context set, in reverse order
   * of the beginning of service object lifetime, performs
   * <tt>delete static_cast<io_context::service*>(svc)</tt>.
   *
   * @note The destruction sequence described above permits programs to
   * simplify their resource management by using @c shared_ptr<>. Where an
   * object's lifetime is tied to the lifetime of a connection (or some other
   * sequence of asynchronous operations), a @c shared_ptr to the object would
   * be bound into the handlers for all asynchronous operations associated with
   * it. This works as follows:
   *
   * @li When a single connection ends, all associated asynchronous operations
   * complete. The corresponding handler objects are destroyed, and all
   * @c shared_ptr references to the objects are destroyed.
   *
   * @li To shut down the whole program, the io_context function stop() is
   * called to terminate any run() calls as soon as possible. The io_context
   * destructor defined above destroys all handlers, causing all @c shared_ptr
   * references to all connection objects to be destroyed.
   */
  BOOST_ASIO_DECL ~io_context();

  /// Obtains the executor associated with the io_context.
  executor_type get_executor() BOOST_ASIO_NOEXCEPT;

  /// Run the io_context object's event processing loop.
  /**
   * The run() function blocks until all work has finished and there are no
   * more handlers to be dispatched, or until the io_context has been stopped.
   *
   * Multiple threads may call the run() function to set up a pool of threads
   * from which the io_context may execute handlers. All threads that are
   * waiting in the pool are equivalent and the io_context may choose any one
   * of them to invoke a handler.
   *
   * A normal exit from the run() function implies that the io_context object
   * is stopped (the stopped() function returns @c true). Subsequent calls to
   * run(), run_one(), poll() or poll_one() will return immediately unless there
   * is a prior call to restart().
   *
   * @return The number of handlers that were executed.
   *
   * @note Calling the run() function from a thread that is currently calling
   * one of run(), run_one(), run_for(), run_until(), poll() or poll_one() on
   * the same io_context object may introduce the potential for deadlock. It is
   * the caller's reponsibility to avoid this.
   *
   * The poll() function may also be used to dispatch ready handlers, but
   * without blocking.
   */
  BOOST_ASIO_DECL count_type run();

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use non-error_code overload.) Run the io_context object's
  /// event processing loop.
  /**
   * The run() function blocks until all work has finished and there are no
   * more handlers to be dispatched, or until the io_context has been stopped.
   *
   * Multiple threads may call the run() function to set up a pool of threads
   * from which the io_context may execute handlers. All threads that are
   * waiting in the pool are equivalent and the io_context may choose any one
   * of them to invoke a handler.
   *
   * A normal exit from the run() function implies that the io_context object
   * is stopped (the stopped() function returns @c true). Subsequent calls to
   * run(), run_one(), poll() or poll_one() will return immediately unless there
   * is a prior call to restart().
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @return The number of handlers that were executed.
   *
   * @note Calling the run() function from a thread that is currently calling
   * one of run(), run_one(), run_for(), run_until(), poll() or poll_one() on
   * the same io_context object may introduce the potential for deadlock. It is
   * the caller's reponsibility to avoid this.
   *
   * The poll() function may also be used to dispatch ready handlers, but
   * without blocking.
   */
  BOOST_ASIO_DECL count_type run(boost::system::error_code& ec);
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

#if defined(BOOST_ASIO_HAS_CHRONO) || defined(GENERATING_DOCUMENTATION)
  /// Run the io_context object's event processing loop for a specified
  /// duration.
  /**
   * The run_for() function blocks until all work has finished and there are no
   * more handlers to be dispatched, until the io_context has been stopped, or
   * until the specified duration has elapsed.
   *
   * @param rel_time The duration for which the call may block.
   *
   * @return The number of handlers that were executed.
   */
  template <typename Rep, typename Period>
  std::size_t run_for(const chrono::duration<Rep, Period>& rel_time);

  /// Run the io_context object's event processing loop until a specified time.
  /**
   * The run_until() function blocks until all work has finished and there are
   * no more handlers to be dispatched, until the io_context has been stopped,
   * or until the specified time has been reached.
   *
   * @param abs_time The time point until which the call may block.
   *
   * @return The number of handlers that were executed.
   */
  template <typename Clock, typename Duration>
  std::size_t run_until(const chrono::time_point<Clock, Duration>& abs_time);
#endif // defined(BOOST_ASIO_HAS_CHRONO) || defined(GENERATING_DOCUMENTATION)

  /// Run the io_context object's event processing loop to execute at most one
  /// handler.
  /**
   * The run_one() function blocks until one handler has been dispatched, or
   * until the io_context has been stopped.
   *
   * @return The number of handlers that were executed. A zero return value
   * implies that the io_context object is stopped (the stopped() function
   * returns @c true). Subsequent calls to run(), run_one(), poll() or
   * poll_one() will return immediately unless there is a prior call to
   * restart().
   *
   * @note Calling the run_one() function from a thread that is currently
   * calling one of run(), run_one(), run_for(), run_until(), poll() or
   * poll_one() on the same io_context object may introduce the potential for
   * deadlock. It is the caller's reponsibility to avoid this.
   */
  BOOST_ASIO_DECL count_type run_one();

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use non-error_code overlaod.) Run the io_context object's
  /// event processing loop to execute at most one handler.
  /**
   * The run_one() function blocks until one handler has been dispatched, or
   * until the io_context has been stopped.
   *
   * @return The number of handlers that were executed. A zero return value
   * implies that the io_context object is stopped (the stopped() function
   * returns @c true). Subsequent calls to run(), run_one(), poll() or
   * poll_one() will return immediately unless there is a prior call to
   * restart().
   *
   * @return The number of handlers that were executed.
   *
   * @note Calling the run_one() function from a thread that is currently
   * calling one of run(), run_one(), run_for(), run_until(), poll() or
   * poll_one() on the same io_context object may introduce the potential for
   * deadlock. It is the caller's reponsibility to avoid this.
   */
  BOOST_ASIO_DECL count_type run_one(boost::system::error_code& ec);
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

#if defined(BOOST_ASIO_HAS_CHRONO) || defined(GENERATING_DOCUMENTATION)
  /// Run the io_context object's event processing loop for a specified duration
  /// to execute at most one handler.
  /**
   * The run_one_for() function blocks until one handler has been dispatched,
   * until the io_context has been stopped, or until the specified duration has
   * elapsed.
   *
   * @param rel_time The duration for which the call may block.
   *
   * @return The number of handlers that were executed.
   */
  template <typename Rep, typename Period>
  std::size_t run_one_for(const chrono::duration<Rep, Period>& rel_time);

  /// Run the io_context object's event processing loop until a specified time
  /// to execute at most one handler.
  /**
   * The run_one_until() function blocks until one handler has been dispatched,
   * until the io_context has been stopped, or until the specified time has
   * been reached.
   *
   * @param abs_time The time point until which the call may block.
   *
   * @return The number of handlers that were executed.
   */
  template <typename Clock, typename Duration>
  std::size_t run_one_until(
      const chrono::time_point<Clock, Duration>& abs_time);
#endif // defined(BOOST_ASIO_HAS_CHRONO) || defined(GENERATING_DOCUMENTATION)

  /// Run the io_context object's event processing loop to execute ready
  /// handlers.
  /**
   * The poll() function runs handlers that are ready to run, without blocking,
   * until the io_context has been stopped or there are no more ready handlers.
   *
   * @return The number of handlers that were executed.
   */
  BOOST_ASIO_DECL count_type poll();

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use non-error_code overload.) Run the io_context object's
  /// event processing loop to execute ready handlers.
  /**
   * The poll() function runs handlers that are ready to run, without blocking,
   * until the io_context has been stopped or there are no more ready handlers.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @return The number of handlers that were executed.
   */
  BOOST_ASIO_DECL count_type poll(boost::system::error_code& ec);
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Run the io_context object's event processing loop to execute one ready
  /// handler.
  /**
   * The poll_one() function runs at most one handler that is ready to run,
   * without blocking.
   *
   * @return The number of handlers that were executed.
   */
  BOOST_ASIO_DECL count_type poll_one();

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use non-error_code overload.) Run the io_context object's
  /// event processing loop to execute one ready handler.
  /**
   * The poll_one() function runs at most one handler that is ready to run,
   * without blocking.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @return The number of handlers that were executed.
   */
  BOOST_ASIO_DECL count_type poll_one(boost::system::error_code& ec);
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Stop the io_context object's event processing loop.
  /**
   * This function does not block, but instead simply signals the io_context to
   * stop. All invocations of its run() or run_one() member functions should
   * return as soon as possible. Subsequent calls to run(), run_one(), poll()
   * or poll_one() will return immediately until restart() is called.
   */
  BOOST_ASIO_DECL void stop();

  /// Determine whether the io_context object has been stopped.
  /**
   * This function is used to determine whether an io_context object has been
   * stopped, either through an explicit call to stop(), or due to running out
   * of work. When an io_context object is stopped, calls to run(), run_one(),
   * poll() or poll_one() will return immediately without invoking any
   * handlers.
   *
   * @return @c true if the io_context object is stopped, otherwise @c false.
   */
  BOOST_ASIO_DECL bool stopped() const;

  /// Restart the io_context in preparation for a subsequent run() invocation.
  /**
   * This function must be called prior to any second or later set of
   * invocations of the run(), run_one(), poll() or poll_one() functions when a
   * previous invocation of these functions returned due to the io_context
   * being stopped or running out of work. After a call to restart(), the
   * io_context object's stopped() function will return @c false.
   *
   * This function must not be called while there are any unfinished calls to
   * the run(), run_one(), poll() or poll_one() functions.
   */
  BOOST_ASIO_DECL void restart();

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use restart().) Reset the io_context in preparation for a
  /// subsequent run() invocation.
  /**
   * This function must be called prior to any second or later set of
   * invocations of the run(), run_one(), poll() or poll_one() functions when a
   * previous invocation of these functions returned due to the io_context
   * being stopped or running out of work. After a call to restart(), the
   * io_context object's stopped() function will return @c false.
   *
   * This function must not be called while there are any unfinished calls to
   * the run(), run_one(), poll() or poll_one() functions.
   */
  void reset();

  /// (Deprecated: Use boost::asio::dispatch().) Request the io_context to
  /// invoke the given handler.
  /**
   * This function is used to ask the io_context to execute the given handler.
   *
   * The io_context guarantees that the handler will only be called in a thread
   * in which the run(), run_one(), poll() or poll_one() member functions is
   * currently being invoked. The handler may be executed inside this function
   * if the guarantee can be met.
   *
   * @param handler The handler to be called. The io_context will make
   * a copy of the handler object as required. The function signature of the
   * handler must be: @code void handler(); @endcode
   *
   * @note This function throws an exception only if:
   *
   * @li the handler's @c asio_handler_allocate function; or
   *
   * @li the handler's copy constructor
   *
   * throws an exception.
   */
  template <typename LegacyCompletionHandler>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(LegacyCompletionHandler, void ())
  dispatch(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler);

  /// (Deprecated: Use boost::asio::post().) Request the io_context to invoke
  /// the given handler and return immediately.
  /**
   * This function is used to ask the io_context to execute the given handler,
   * but without allowing the io_context to call the handler from inside this
   * function.
   *
   * The io_context guarantees that the handler will only be called in a thread
   * in which the run(), run_one(), poll() or poll_one() member functions is
   * currently being invoked.
   *
   * @param handler The handler to be called. The io_context will make
   * a copy of the handler object as required. The function signature of the
   * handler must be: @code void handler(); @endcode
   *
   * @note This function throws an exception only if:
   *
   * @li the handler's @c asio_handler_allocate function; or
   *
   * @li the handler's copy constructor
   *
   * throws an exception.
   */
  template <typename LegacyCompletionHandler>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(LegacyCompletionHandler, void ())
  post(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler);

  /// (Deprecated: Use boost::asio::bind_executor().) Create a new handler that
  /// automatically dispatches the wrapped handler on the io_context.
  /**
   * This function is used to create a new handler function object that, when
   * invoked, will automatically pass the wrapped handler to the io_context
   * object's dispatch function.
   *
   * @param handler The handler to be wrapped. The io_context will make a copy
   * of the handler object as required. The function signature of the handler
   * must be: @code void handler(A1 a1, ... An an); @endcode
   *
   * @return A function object that, when invoked, passes the wrapped handler to
   * the io_context object's dispatch function. Given a function object with the
   * signature:
   * @code R f(A1 a1, ... An an); @endcode
   * If this function object is passed to the wrap function like so:
   * @code io_context.wrap(f); @endcode
   * then the return value is a function object with the signature
   * @code void g(A1 a1, ... An an); @endcode
   * that, when invoked, executes code equivalent to:
   * @code io_context.dispatch(boost::bind(f, a1, ... an)); @endcode
   */
  template <typename Handler>
#if defined(GENERATING_DOCUMENTATION)
  unspecified
#else
  detail::wrapped_handler<io_context&, Handler>
#endif
  wrap(Handler handler);
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

private:
  io_context(const io_context&) BOOST_ASIO_DELETED;
  io_context& operator=(const io_context&) BOOST_ASIO_DELETED;

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  struct initiate_dispatch;
  struct initiate_post;
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  // Helper function to add the implementation.
  BOOST_ASIO_DECL impl_type& add_impl(impl_type* impl);

  // Backwards compatible overload for use with services derived from
  // io_context::service.
  template <typename Service>
  friend Service& use_service(io_context& ioc);

#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  detail::winsock_init<> init_;
#elif defined(__sun) || defined(__QNX__) || defined(__hpux) || defined(_AIX) \
  || defined(__osf__)
  detail::signal_init<> init_;
#endif

  // The implementation.
  impl_type& impl_;
};

namespace detail {

} // namespace detail

/// Executor implementation type used to submit functions to an io_context.
template <typename Allocator, unsigned int Bits>
class io_context::basic_executor_type : detail::io_context_bits
{
public:
  /// Copy constructor.
  basic_executor_type(
      const basic_executor_type& other) BOOST_ASIO_NOEXCEPT
    : io_context_(other.io_context_),
      allocator_(other.allocator_),
      bits_(other.bits_)
  {
    if (Bits & outstanding_work_tracked)
      if (io_context_)
        io_context_->impl_.work_started();
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move constructor.
  basic_executor_type(basic_executor_type&& other) BOOST_ASIO_NOEXCEPT
    : io_context_(other.io_context_),
      allocator_(BOOST_ASIO_MOVE_CAST(Allocator)(other.allocator_)),
      bits_(other.bits_)
  {
    if (Bits & outstanding_work_tracked)
      other.io_context_ = 0;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Destructor.
  ~basic_executor_type() BOOST_ASIO_NOEXCEPT
  {
    if (Bits & outstanding_work_tracked)
      if (io_context_)
        io_context_->impl_.work_finished();
  }

  /// Assignment operator.
  basic_executor_type& operator=(
      const basic_executor_type& other) BOOST_ASIO_NOEXCEPT;

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move assignment operator.
  basic_executor_type& operator=(
      basic_executor_type&& other) BOOST_ASIO_NOEXCEPT;
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct boost_asio_require_fn::impl;
  friend struct boost_asio_prefer_fn::impl;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Obtain an executor with the @c blocking.possibly property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::blocking.possibly); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type require(
      execution::blocking_t::possibly_t) const
  {
    return basic_executor_type(io_context_,
        allocator_, bits_ & ~blocking_never);
  }

  /// Obtain an executor with the @c blocking.never property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::blocking.never); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type require(
      execution::blocking_t::never_t) const
  {
    return basic_executor_type(io_context_,
        allocator_, bits_ | blocking_never);
  }

  /// Obtain an executor with the @c relationship.fork property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::relationship.fork); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type require(
      execution::relationship_t::fork_t) const
  {
    return basic_executor_type(io_context_,
        allocator_, bits_ & ~relationship_continuation);
  }

  /// Obtain an executor with the @c relationship.continuation property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::relationship.continuation); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type require(
      execution::relationship_t::continuation_t) const
  {
    return basic_executor_type(io_context_,
        allocator_, bits_ | relationship_continuation);
  }

  /// Obtain an executor with the @c outstanding_work.tracked property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::outstanding_work.tracked); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type<Allocator,
      BOOST_ASIO_UNSPECIFIED(Bits | outstanding_work_tracked)>
  require(execution::outstanding_work_t::tracked_t) const
  {
    return basic_executor_type<Allocator, Bits | outstanding_work_tracked>(
        io_context_, allocator_, bits_);
  }

  /// Obtain an executor with the @c outstanding_work.untracked property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::outstanding_work.untracked); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type<Allocator,
      BOOST_ASIO_UNSPECIFIED(Bits & ~outstanding_work_tracked)>
  require(execution::outstanding_work_t::untracked_t) const
  {
    return basic_executor_type<Allocator, Bits & ~outstanding_work_tracked>(
        io_context_, allocator_, bits_);
  }

  /// Obtain an executor with the specified @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::allocator(my_allocator)); @endcode
   */
  template <typename OtherAllocator>
  BOOST_ASIO_CONSTEXPR basic_executor_type<OtherAllocator, Bits>
  require(execution::allocator_t<OtherAllocator> a) const
  {
    return basic_executor_type<OtherAllocator, Bits>(
        io_context_, a.value(), bits_);
  }

  /// Obtain an executor with the default @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = my_io_context.get_executor();
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::allocator); @endcode
   */
  BOOST_ASIO_CONSTEXPR basic_executor_type<std::allocator<void>, Bits>
  require(execution::allocator_t<void>) const
  {
    return basic_executor_type<std::allocator<void>, Bits>(
        io_context_, std::allocator<void>(), bits_);
  }

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct boost_asio_query_fn::impl;
  friend struct boost::asio::execution::detail::mapping_t<0>;
  friend struct boost::asio::execution::detail::outstanding_work_t<0>;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Query the current value of the @c mapping property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * if (boost::asio::query(ex, boost::asio::execution::mapping)
   *       == boost::asio::execution::mapping.thread)
   *   ... @endcode
   */
  static BOOST_ASIO_CONSTEXPR execution::mapping_t query(
      execution::mapping_t) BOOST_ASIO_NOEXCEPT
  {
    return execution::mapping.thread;
  }

  /// Query the current value of the @c context property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * boost::asio::io_context& ctx = boost::asio::query(
   *     ex, boost::asio::execution::context); @endcode
   */
  io_context& query(execution::context_t) const BOOST_ASIO_NOEXCEPT
  {
    return *io_context_;
  }

  /// Query the current value of the @c blocking property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * if (boost::asio::query(ex, boost::asio::execution::blocking)
   *       == boost::asio::execution::blocking.always)
   *   ... @endcode
   */
  BOOST_ASIO_CONSTEXPR execution::blocking_t query(
      execution::blocking_t) const BOOST_ASIO_NOEXCEPT
  {
    return (bits_ & blocking_never)
      ? execution::blocking_t(execution::blocking.never)
      : execution::blocking_t(execution::blocking.possibly);
  }

  /// Query the current value of the @c relationship property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * if (boost::asio::query(ex, boost::asio::execution::relationship)
   *       == boost::asio::execution::relationship.continuation)
   *   ... @endcode
   */
  BOOST_ASIO_CONSTEXPR execution::relationship_t query(
      execution::relationship_t) const BOOST_ASIO_NOEXCEPT
  {
    return (bits_ & relationship_continuation)
      ? execution::relationship_t(execution::relationship.continuation)
      : execution::relationship_t(execution::relationship.fork);
  }

  /// Query the current value of the @c outstanding_work property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * if (boost::asio::query(ex, boost::asio::execution::outstanding_work)
   *       == boost::asio::execution::outstanding_work.tracked)
   *   ... @endcode
   */
  static BOOST_ASIO_CONSTEXPR execution::outstanding_work_t query(
      execution::outstanding_work_t) BOOST_ASIO_NOEXCEPT
  {
    return (Bits & outstanding_work_tracked)
      ? execution::outstanding_work_t(execution::outstanding_work.tracked)
      : execution::outstanding_work_t(execution::outstanding_work.untracked);
  }

  /// Query the current value of the @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * auto alloc = boost::asio::query(ex,
   *     boost::asio::execution::allocator); @endcode
   */
  template <typename OtherAllocator>
  BOOST_ASIO_CONSTEXPR Allocator query(
      execution::allocator_t<OtherAllocator>) const BOOST_ASIO_NOEXCEPT
  {
    return allocator_;
  }

  /// Query the current value of the @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * auto alloc = boost::asio::query(ex,
   *     boost::asio::execution::allocator); @endcode
   */
  BOOST_ASIO_CONSTEXPR Allocator query(
      execution::allocator_t<void>) const BOOST_ASIO_NOEXCEPT
  {
    return allocator_;
  }

public:
  /// Determine whether the io_context is running in the current thread.
  /**
   * @return @c true if the current thread is running the io_context. Otherwise
   * returns @c false.
   */
  bool running_in_this_thread() const BOOST_ASIO_NOEXCEPT;

  /// Compare two executors for equality.
  /**
   * Two executors are equal if they refer to the same underlying io_context.
   */
  friend bool operator==(const basic_executor_type& a,
      const basic_executor_type& b) BOOST_ASIO_NOEXCEPT
  {
    return a.io_context_ == b.io_context_
      && a.allocator_ == b.allocator_
      && a.bits_ == b.bits_;
  }

  /// Compare two executors for inequality.
  /**
   * Two executors are equal if they refer to the same underlying io_context.
   */
  friend bool operator!=(const basic_executor_type& a,
      const basic_executor_type& b) BOOST_ASIO_NOEXCEPT
  {
    return a.io_context_ != b.io_context_
      || a.allocator_ != b.allocator_
      || a.bits_ != b.bits_;
  }

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct boost_asio_execution_execute_fn::impl;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Execution function.
  /**
   * Do not call this function directly. It is intended for use with the
   * execution::execute customisation point.
   *
   * For example:
   * @code auto ex = my_io_context.get_executor();
   * execution::execute(ex, my_function_object); @endcode
   */
  template <typename Function>
  void execute(BOOST_ASIO_MOVE_ARG(Function) f) const;

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)
public:
  /// Obtain the underlying execution context.
  io_context& context() const BOOST_ASIO_NOEXCEPT;

  /// Inform the io_context that it has some outstanding work to do.
  /**
   * This function is used to inform the io_context that some work has begun.
   * This ensures that the io_context's run() and run_one() functions do not
   * exit while the work is underway.
   */
  void on_work_started() const BOOST_ASIO_NOEXCEPT;

  /// Inform the io_context that some work is no longer outstanding.
  /**
   * This function is used to inform the io_context that some work has
   * finished. Once the count of unfinished work reaches zero, the io_context
   * is stopped and the run() and run_one() functions may exit.
   */
  void on_work_finished() const BOOST_ASIO_NOEXCEPT;

  /// Request the io_context to invoke the given function object.
  /**
   * This function is used to ask the io_context to execute the given function
   * object. If the current thread is running the io_context, @c dispatch()
   * executes the function before returning. Otherwise, the function will be
   * scheduled to run on the io_context.
   *
   * @param f The function object to be called. The executor will make a copy
   * of the handler object as required. The function signature of the function
   * object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename OtherAllocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f,
      const OtherAllocator& a) const;

  /// Request the io_context to invoke the given function object.
  /**
   * This function is used to ask the io_context to execute the given function
   * object. The function object will never be executed inside @c post().
   * Instead, it will be scheduled to run on the io_context.
   *
   * @param f The function object to be called. The executor will make a copy
   * of the handler object as required. The function signature of the function
   * object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename OtherAllocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f,
      const OtherAllocator& a) const;

  /// Request the io_context to invoke the given function object.
  /**
   * This function is used to ask the io_context to execute the given function
   * object. The function object will never be executed inside @c defer().
   * Instead, it will be scheduled to run on the io_context.
   *
   * If the current thread belongs to the io_context, @c defer() will delay
   * scheduling the function object until the current thread returns control to
   * the pool.
   *
   * @param f The function object to be called. The executor will make a copy
   * of the handler object as required. The function signature of the function
   * object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename OtherAllocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f,
      const OtherAllocator& a) const;
#endif // !defined(BOOST_ASIO_NO_TS_EXECUTORS)

private:
  friend class io_context;
  template <typename, unsigned int> friend class basic_executor_type;

  // Constructor used by io_context::get_executor().
  explicit basic_executor_type(io_context& i) BOOST_ASIO_NOEXCEPT
    : io_context_(&i),
      allocator_(),
      bits_(0)
  {
    if (Bits & outstanding_work_tracked)
      io_context_->impl_.work_started();
  }

  // Constructor used by require().
  basic_executor_type(io_context* i,
      const Allocator& a, unsigned int bits) BOOST_ASIO_NOEXCEPT
    : io_context_(i),
      allocator_(a),
      bits_(bits)
  {
    if (Bits & outstanding_work_tracked)
      if (io_context_)
        io_context_->impl_.work_started();
  }

  // The underlying io_context.
  io_context* io_context_;

  // The allocator used for execution functions.
  Allocator allocator_;

  // The runtime-switched properties of the io_context executor.
  unsigned int bits_;
};

#if !defined(BOOST_ASIO_NO_DEPRECATED)
/// (Deprecated: Use executor_work_guard.) Class to inform the io_context when
/// it has work to do.
/**
 * The work class is used to inform the io_context when work starts and
 * finishes. This ensures that the io_context object's run() function will not
 * exit while work is underway, and that it does exit when there is no
 * unfinished work remaining.
 *
 * The work class is copy-constructible so that it may be used as a data member
 * in a handler class. It is not assignable.
 */
class io_context::work
{
public:
  /// Constructor notifies the io_context that work is starting.
  /**
   * The constructor is used to inform the io_context that some work has begun.
   * This ensures that the io_context object's run() function will not exit
   * while the work is underway.
   */
  explicit work(boost::asio::io_context& io_context);

  /// Copy constructor notifies the io_context that work is starting.
  /**
   * The constructor is used to inform the io_context that some work has begun.
   * This ensures that the io_context object's run() function will not exit
   * while the work is underway.
   */
  work(const work& other);

  /// Destructor notifies the io_context that the work is complete.
  /**
   * The destructor is used to inform the io_context that some work has
   * finished. Once the count of unfinished work reaches zero, the io_context
   * object's run() function is permitted to exit.
   */
  ~work();

  /// Get the io_context associated with the work.
  boost::asio::io_context& get_io_context();

private:
  // Prevent assignment.
  void operator=(const work& other);

  // The io_context implementation.
  detail::io_context_impl& io_context_impl_;
};
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

/// Base class for all io_context services.
class io_context::service
  : public execution_context::service
{
public:
  /// Get the io_context object that owns the service.
  boost::asio::io_context& get_io_context();

private:
  /// Destroy all user-defined handler objects owned by the service.
  BOOST_ASIO_DECL virtual void shutdown();

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use shutdown().) Destroy all user-defined handler objects
  /// owned by the service.
  BOOST_ASIO_DECL virtual void shutdown_service();
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Handle notification of a fork-related event to perform any necessary
  /// housekeeping.
  /**
   * This function is not a pure virtual so that services only have to
   * implement it if necessary. The default implementation does nothing.
   */
  BOOST_ASIO_DECL virtual void notify_fork(
      execution_context::fork_event event);

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use notify_fork().) Handle notification of a fork-related
  /// event to perform any necessary housekeeping.
  /**
   * This function is not a pure virtual so that services only have to
   * implement it if necessary. The default implementation does nothing.
   */
  BOOST_ASIO_DECL virtual void fork_service(
      execution_context::fork_event event);
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

protected:
  /// Constructor.
  /**
   * @param owner The io_context object that owns the service.
   */
  BOOST_ASIO_DECL service(boost::asio::io_context& owner);

  /// Destructor.
  BOOST_ASIO_DECL virtual ~service();
};

namespace detail {

// Special service base class to keep classes header-file only.
template <typename Type>
class service_base
  : public boost::asio::io_context::service
{
public:
  static boost::asio::detail::service_id<Type> id;

  // Constructor.
  service_base(boost::asio::io_context& io_context)
    : boost::asio::io_context::service(io_context)
  {
  }
};

template <typename Type>
boost::asio::detail::service_id<Type> service_base<Type>::id;

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Allocator, unsigned int Bits>
struct equality_comparable<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Allocator, unsigned int Bits, typename Function>
struct execute_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    Function
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::blocking_t::possibly_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      Allocator, Bits> result_type;
};

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::blocking_t::never_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      Allocator, Bits> result_type;
};

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::relationship_t::fork_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      Allocator, Bits> result_type;
};

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::relationship_t::continuation_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      Allocator, Bits> result_type;
};

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::outstanding_work_t::tracked_t
  > : boost::asio::detail::io_context_bits
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      Allocator, Bits | outstanding_work_tracked> result_type;
};

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::outstanding_work_t::untracked_t
  > : boost::asio::detail::io_context_bits
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      Allocator, Bits & ~outstanding_work_tracked> result_type;
};

template <typename Allocator, unsigned int Bits>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::allocator_t<void>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      std::allocator<void>, Bits> result_type;
};

template <unsigned int Bits,
    typename Allocator, typename OtherAllocator>
struct require_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::allocator_t<OtherAllocator>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::io_context::basic_executor_type<
      OtherAllocator, Bits> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Allocator, unsigned int Bits, typename Property>
struct query_static_constexpr_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::outstanding_work_t
      >::value
    >::type
  > : boost::asio::detail::io_context_bits
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::outstanding_work_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return (Bits & outstanding_work_tracked)
      ? execution::outstanding_work_t(execution::outstanding_work.tracked)
      : execution::outstanding_work_t(execution::outstanding_work.untracked);
  }
};

template <typename Allocator, unsigned int Bits, typename Property>
struct query_static_constexpr_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::mapping_t
      >::value
    >::type
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::mapping_t::thread_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Allocator, unsigned int Bits, typename Property>
struct query_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::blocking_t
      >::value
    >::type
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::blocking_t result_type;
};

template <typename Allocator, unsigned int Bits, typename Property>
struct query_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::relationship_t
      >::value
    >::type
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::relationship_t result_type;
};

template <typename Allocator, unsigned int Bits>
struct query_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::context_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::io_context& result_type;
};

template <typename Allocator, unsigned int Bits>
struct query_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::allocator_t<void>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef Allocator result_type;
};

template <typename Allocator, unsigned int Bits, typename OtherAllocator>
struct query_member<
    boost::asio::io_context::basic_executor_type<Allocator, Bits>,
    boost::asio::execution::allocator_t<OtherAllocator>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef Allocator result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

} // namespace traits

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/io_context.hpp>
#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/impl/io_context.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

// If both io_context.hpp and strand.hpp have been included, automatically
// include the header file needed for the io_context::strand class.
#if !defined(BOOST_ASIO_NO_EXTENSIONS)
# if defined(BOOST_ASIO_STRAND_HPP)
#  include <boost/asio/io_context_strand.hpp>
# endif // defined(BOOST_ASIO_STRAND_HPP)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

#endif // BOOST_ASIO_IO_CONTEXT_HPP
