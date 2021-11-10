//
// basic_signal_set.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_SIGNAL_SET_HPP
#define BOOST_ASIO_BASIC_SIGNAL_SET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>
#include <boost/asio/detail/io_object_impl.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>
#include <boost/asio/detail/signal_set_service.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/execution_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Provides signal functionality.
/**
 * The basic_signal_set class provides the ability to perform an asynchronous
 * wait for one or more signals to occur.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *
 * @par Example
 * Performing an asynchronous wait:
 * @code
 * void handler(
 *     const boost::system::error_code& error,
 *     int signal_number)
 * {
 *   if (!error)
 *   {
 *     // A signal occurred.
 *   }
 * }
 *
 * ...
 *
 * // Construct a signal set registered for process termination.
 * boost::asio::signal_set signals(my_context, SIGINT, SIGTERM);
 *
 * // Start an asynchronous wait for one of the signals to occur.
 * signals.async_wait(handler);
 * @endcode
 *
 * @par Queueing of signal notifications
 *
 * If a signal is registered with a signal_set, and the signal occurs when
 * there are no waiting handlers, then the signal notification is queued. The
 * next async_wait operation on that signal_set will dequeue the notification.
 * If multiple notifications are queued, subsequent async_wait operations
 * dequeue them one at a time. Signal notifications are dequeued in order of
 * ascending signal number.
 *
 * If a signal number is removed from a signal_set (using the @c remove or @c
 * erase member functions) then any queued notifications for that signal are
 * discarded.
 *
 * @par Multiple registration of signals
 *
 * The same signal number may be registered with different signal_set objects.
 * When the signal occurs, one handler is called for each signal_set object.
 *
 * Note that multiple registration only works for signals that are registered
 * using Asio. The application must not also register a signal handler using
 * functions such as @c signal() or @c sigaction().
 *
 * @par Signal masking on POSIX platforms
 *
 * POSIX allows signals to be blocked using functions such as @c sigprocmask()
 * and @c pthread_sigmask(). For signals to be delivered, programs must ensure
 * that any signals registered using signal_set objects are unblocked in at
 * least one thread.
 */
template <typename Executor = any_io_executor>
class basic_signal_set
{
public:
  /// The type of the executor associated with the object.
  typedef Executor executor_type;

  /// Rebinds the signal set type to another executor.
  template <typename Executor1>
  struct rebind_executor
  {
    /// The signal set type when rebound to the specified executor.
    typedef basic_signal_set<Executor1> other;
  };

  /// Construct a signal set without adding any signals.
  /**
   * This constructor creates a signal set without registering for any signals.
   *
   * @param ex The I/O executor that the signal set will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the
   * signal set.
   */
  explicit basic_signal_set(const executor_type& ex)
    : impl_(0, ex)
  {
  }

  /// Construct a signal set without adding any signals.
  /**
   * This constructor creates a signal set without registering for any signals.
   *
   * @param context An execution context which provides the I/O executor that
   * the signal set will use, by default, to dispatch handlers for any
   * asynchronous operations performed on the signal set.
   */
  template <typename ExecutionContext>
  explicit basic_signal_set(ExecutionContext& context,
      typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      >::type = defaulted_constraint())
    : impl_(0, 0, context)
  {
  }

  /// Construct a signal set and add one signal.
  /**
   * This constructor creates a signal set and registers for one signal.
   *
   * @param ex The I/O executor that the signal set will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the
   * signal set.
   *
   * @param signal_number_1 The signal number to be added.
   *
   * @note This constructor is equivalent to performing:
   * @code boost::asio::signal_set signals(ex);
   * signals.add(signal_number_1); @endcode
   */
  basic_signal_set(const executor_type& ex, int signal_number_1)
    : impl_(0, ex)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number_1, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Construct a signal set and add one signal.
  /**
   * This constructor creates a signal set and registers for one signal.
   *
   * @param context An execution context which provides the I/O executor that
   * the signal set will use, by default, to dispatch handlers for any
   * asynchronous operations performed on the signal set.
   *
   * @param signal_number_1 The signal number to be added.
   *
   * @note This constructor is equivalent to performing:
   * @code boost::asio::signal_set signals(context);
   * signals.add(signal_number_1); @endcode
   */
  template <typename ExecutionContext>
  basic_signal_set(ExecutionContext& context, int signal_number_1,
      typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      >::type = defaulted_constraint())
    : impl_(0, 0, context)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number_1, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Construct a signal set and add two signals.
  /**
   * This constructor creates a signal set and registers for two signals.
   *
   * @param ex The I/O executor that the signal set will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the
   * signal set.
   *
   * @param signal_number_1 The first signal number to be added.
   *
   * @param signal_number_2 The second signal number to be added.
   *
   * @note This constructor is equivalent to performing:
   * @code boost::asio::signal_set signals(ex);
   * signals.add(signal_number_1);
   * signals.add(signal_number_2); @endcode
   */
  basic_signal_set(const executor_type& ex, int signal_number_1,
      int signal_number_2)
    : impl_(0, ex)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number_1, ec);
    boost::asio::detail::throw_error(ec, "add");
    impl_.get_service().add(impl_.get_implementation(), signal_number_2, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Construct a signal set and add two signals.
  /**
   * This constructor creates a signal set and registers for two signals.
   *
   * @param context An execution context which provides the I/O executor that
   * the signal set will use, by default, to dispatch handlers for any
   * asynchronous operations performed on the signal set.
   *
   * @param signal_number_1 The first signal number to be added.
   *
   * @param signal_number_2 The second signal number to be added.
   *
   * @note This constructor is equivalent to performing:
   * @code boost::asio::signal_set signals(context);
   * signals.add(signal_number_1);
   * signals.add(signal_number_2); @endcode
   */
  template <typename ExecutionContext>
  basic_signal_set(ExecutionContext& context, int signal_number_1,
      int signal_number_2,
      typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      >::type = defaulted_constraint())
    : impl_(0, 0, context)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number_1, ec);
    boost::asio::detail::throw_error(ec, "add");
    impl_.get_service().add(impl_.get_implementation(), signal_number_2, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Construct a signal set and add three signals.
  /**
   * This constructor creates a signal set and registers for three signals.
   *
   * @param ex The I/O executor that the signal set will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the
   * signal set.
   *
   * @param signal_number_1 The first signal number to be added.
   *
   * @param signal_number_2 The second signal number to be added.
   *
   * @param signal_number_3 The third signal number to be added.
   *
   * @note This constructor is equivalent to performing:
   * @code boost::asio::signal_set signals(ex);
   * signals.add(signal_number_1);
   * signals.add(signal_number_2);
   * signals.add(signal_number_3); @endcode
   */
  basic_signal_set(const executor_type& ex, int signal_number_1,
      int signal_number_2, int signal_number_3)
    : impl_(0, ex)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number_1, ec);
    boost::asio::detail::throw_error(ec, "add");
    impl_.get_service().add(impl_.get_implementation(), signal_number_2, ec);
    boost::asio::detail::throw_error(ec, "add");
    impl_.get_service().add(impl_.get_implementation(), signal_number_3, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Construct a signal set and add three signals.
  /**
   * This constructor creates a signal set and registers for three signals.
   *
   * @param context An execution context which provides the I/O executor that
   * the signal set will use, by default, to dispatch handlers for any
   * asynchronous operations performed on the signal set.
   *
   * @param signal_number_1 The first signal number to be added.
   *
   * @param signal_number_2 The second signal number to be added.
   *
   * @param signal_number_3 The third signal number to be added.
   *
   * @note This constructor is equivalent to performing:
   * @code boost::asio::signal_set signals(context);
   * signals.add(signal_number_1);
   * signals.add(signal_number_2);
   * signals.add(signal_number_3); @endcode
   */
  template <typename ExecutionContext>
  basic_signal_set(ExecutionContext& context, int signal_number_1,
      int signal_number_2, int signal_number_3,
      typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      >::type = defaulted_constraint())
    : impl_(0, 0, context)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number_1, ec);
    boost::asio::detail::throw_error(ec, "add");
    impl_.get_service().add(impl_.get_implementation(), signal_number_2, ec);
    boost::asio::detail::throw_error(ec, "add");
    impl_.get_service().add(impl_.get_implementation(), signal_number_3, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Destroys the signal set.
  /**
   * This function destroys the signal set, cancelling any outstanding
   * asynchronous wait operations associated with the signal set as if by
   * calling @c cancel.
   */
  ~basic_signal_set()
  {
  }

  /// Get the executor associated with the object.
  executor_type get_executor() BOOST_ASIO_NOEXCEPT
  {
    return impl_.get_executor();
  }

  /// Add a signal to a signal_set.
  /**
   * This function adds the specified signal to the set. It has no effect if the
   * signal is already in the set.
   *
   * @param signal_number The signal to be added to the set.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void add(int signal_number)
  {
    boost::system::error_code ec;
    impl_.get_service().add(impl_.get_implementation(), signal_number, ec);
    boost::asio::detail::throw_error(ec, "add");
  }

  /// Add a signal to a signal_set.
  /**
   * This function adds the specified signal to the set. It has no effect if the
   * signal is already in the set.
   *
   * @param signal_number The signal to be added to the set.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  BOOST_ASIO_SYNC_OP_VOID add(int signal_number,
      boost::system::error_code& ec)
  {
    impl_.get_service().add(impl_.get_implementation(), signal_number, ec);
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }

  /// Remove a signal from a signal_set.
  /**
   * This function removes the specified signal from the set. It has no effect
   * if the signal is not in the set.
   *
   * @param signal_number The signal to be removed from the set.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note Removes any notifications that have been queued for the specified
   * signal number.
   */
  void remove(int signal_number)
  {
    boost::system::error_code ec;
    impl_.get_service().remove(impl_.get_implementation(), signal_number, ec);
    boost::asio::detail::throw_error(ec, "remove");
  }

  /// Remove a signal from a signal_set.
  /**
   * This function removes the specified signal from the set. It has no effect
   * if the signal is not in the set.
   *
   * @param signal_number The signal to be removed from the set.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @note Removes any notifications that have been queued for the specified
   * signal number.
   */
  BOOST_ASIO_SYNC_OP_VOID remove(int signal_number,
      boost::system::error_code& ec)
  {
    impl_.get_service().remove(impl_.get_implementation(), signal_number, ec);
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }

  /// Remove all signals from a signal_set.
  /**
   * This function removes all signals from the set. It has no effect if the set
   * is already empty.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note Removes all queued notifications.
   */
  void clear()
  {
    boost::system::error_code ec;
    impl_.get_service().clear(impl_.get_implementation(), ec);
    boost::asio::detail::throw_error(ec, "clear");
  }

  /// Remove all signals from a signal_set.
  /**
   * This function removes all signals from the set. It has no effect if the set
   * is already empty.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @note Removes all queued notifications.
   */
  BOOST_ASIO_SYNC_OP_VOID clear(boost::system::error_code& ec)
  {
    impl_.get_service().clear(impl_.get_implementation(), ec);
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }

  /// Cancel all operations associated with the signal set.
  /**
   * This function forces the completion of any pending asynchronous wait
   * operations against the signal set. The handler for each cancelled
   * operation will be invoked with the boost::asio::error::operation_aborted
   * error code.
   *
   * Cancellation does not alter the set of registered signals.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note If a registered signal occurred before cancel() is called, then the
   * handlers for asynchronous wait operations will:
   *
   * @li have already been invoked; or
   *
   * @li have been queued for invocation in the near future.
   *
   * These handlers can no longer be cancelled, and therefore are passed an
   * error code that indicates the successful completion of the wait operation.
   */
  void cancel()
  {
    boost::system::error_code ec;
    impl_.get_service().cancel(impl_.get_implementation(), ec);
    boost::asio::detail::throw_error(ec, "cancel");
  }

  /// Cancel all operations associated with the signal set.
  /**
   * This function forces the completion of any pending asynchronous wait
   * operations against the signal set. The handler for each cancelled
   * operation will be invoked with the boost::asio::error::operation_aborted
   * error code.
   *
   * Cancellation does not alter the set of registered signals.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @note If a registered signal occurred before cancel() is called, then the
   * handlers for asynchronous wait operations will:
   *
   * @li have already been invoked; or
   *
   * @li have been queued for invocation in the near future.
   *
   * These handlers can no longer be cancelled, and therefore are passed an
   * error code that indicates the successful completion of the wait operation.
   */
  BOOST_ASIO_SYNC_OP_VOID cancel(boost::system::error_code& ec)
  {
    impl_.get_service().cancel(impl_.get_implementation(), ec);
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }

  /// Start an asynchronous operation to wait for a signal to be delivered.
  /**
   * This function may be used to initiate an asynchronous wait against the
   * signal set. It always returns immediately.
   *
   * For each call to async_wait(), the supplied handler will be called exactly
   * once. The handler will be called when:
   *
   * @li One of the registered signals in the signal set occurs; or
   *
   * @li The signal set was cancelled, in which case the handler is passed the
   * error code boost::asio::error::operation_aborted.
   *
   * @param handler The handler to be called when the signal occurs. Copies
   * will be made of the handler as required. The function signature of the
   * handler must be:
   * @code void handler(
   *   const boost::system::error_code& error, // Result of operation.
   *   int signal_number // Indicates which signal occurred.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. On
   * immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using boost::asio::post().
   */
  template <
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code, int))
      SignalHandler BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(SignalHandler,
      void (boost::system::error_code, int))
  async_wait(
      BOOST_ASIO_MOVE_ARG(SignalHandler) handler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return async_initiate<SignalHandler, void (boost::system::error_code, int)>(
        initiate_async_wait(this), handler);
  }

private:
  // Disallow copying and assignment.
  basic_signal_set(const basic_signal_set&) BOOST_ASIO_DELETED;
  basic_signal_set& operator=(const basic_signal_set&) BOOST_ASIO_DELETED;

  class initiate_async_wait
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_wait(basic_signal_set* self)
      : self_(self)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return self_->get_executor();
    }

    template <typename SignalHandler>
    void operator()(BOOST_ASIO_MOVE_ARG(SignalHandler) handler) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a SignalHandler.
      BOOST_ASIO_SIGNAL_HANDLER_CHECK(SignalHandler, handler) type_check;

      detail::non_const_lvalue<SignalHandler> handler2(handler);
      self_->impl_.get_service().async_wait(
          self_->impl_.get_implementation(),
          handler2.value, self_->impl_.get_executor());
    }

  private:
    basic_signal_set* self_;
  };

  detail::io_object_impl<detail::signal_set_service, Executor> impl_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BASIC_SIGNAL_SET_HPP
