//
// io_context_strand.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IO_CONTEXT_STRAND_HPP
#define BOOST_ASIO_IO_CONTEXT_STRAND_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_NO_EXTENSIONS) \
  && !defined(BOOST_ASIO_NO_TS_EXECUTORS)

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>
#include <boost/asio/detail/strand_service.hpp>
#include <boost/asio/detail/wrapped_handler.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Provides serialised handler execution.
/**
 * The io_context::strand class provides the ability to post and dispatch
 * handlers with the guarantee that none of those handlers will execute
 * concurrently.
 *
 * @par Order of handler invocation
 * Given:
 *
 * @li a strand object @c s
 *
 * @li an object @c a meeting completion handler requirements
 *
 * @li an object @c a1 which is an arbitrary copy of @c a made by the
 * implementation
 *
 * @li an object @c b meeting completion handler requirements
 *
 * @li an object @c b1 which is an arbitrary copy of @c b made by the
 * implementation
 *
 * if any of the following conditions are true:
 *
 * @li @c s.post(a) happens-before @c s.post(b)
 * 
 * @li @c s.post(a) happens-before @c s.dispatch(b), where the latter is
 * performed outside the strand
 * 
 * @li @c s.dispatch(a) happens-before @c s.post(b), where the former is
 * performed outside the strand
 * 
 * @li @c s.dispatch(a) happens-before @c s.dispatch(b), where both are
 * performed outside the strand
 *   
 * then @c asio_handler_invoke(a1, &a1) happens-before
 * @c asio_handler_invoke(b1, &b1).
 * 
 * Note that in the following case:
 * @code async_op_1(..., s.wrap(a));
 * async_op_2(..., s.wrap(b)); @endcode
 * the completion of the first async operation will perform @c s.dispatch(a),
 * and the second will perform @c s.dispatch(b), but the order in which those
 * are performed is unspecified. That is, you cannot state whether one
 * happens-before the other. Therefore none of the above conditions are met and
 * no ordering guarantee is made.
 *
 * @note The implementation makes no guarantee that handlers posted or
 * dispatched through different @c strand objects will be invoked concurrently.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Safe.
 *
 * @par Concepts:
 * Dispatcher.
 */
class io_context::strand
{
public:
  /// Constructor.
  /**
   * Constructs the strand.
   *
   * @param io_context The io_context object that the strand will use to
   * dispatch handlers that are ready to be run.
   */
  explicit strand(boost::asio::io_context& io_context)
    : service_(boost::asio::use_service<
        boost::asio::detail::strand_service>(io_context))
  {
    service_.construct(impl_);
  }

  /// Destructor.
  /**
   * Destroys a strand.
   *
   * Handlers posted through the strand that have not yet been invoked will
   * still be dispatched in a way that meets the guarantee of non-concurrency.
   */
  ~strand()
  {
  }

  /// Obtain the underlying execution context.
  boost::asio::io_context& context() const BOOST_ASIO_NOEXCEPT
  {
    return service_.get_io_context();
  }

  /// Inform the strand that it has some outstanding work to do.
  /**
   * The strand delegates this call to its underlying io_context.
   */
  void on_work_started() const BOOST_ASIO_NOEXCEPT
  {
    context().get_executor().on_work_started();
  }

  /// Inform the strand that some work is no longer outstanding.
  /**
   * The strand delegates this call to its underlying io_context.
   */
  void on_work_finished() const BOOST_ASIO_NOEXCEPT
  {
    context().get_executor().on_work_finished();
  }

  /// Request the strand to invoke the given function object.
  /**
   * This function is used to ask the strand to execute the given function
   * object on its underlying io_context. The function object will be executed
   * inside this function if the strand is not otherwise busy and if the
   * underlying io_context's executor's @c dispatch() function is also able to
   * execute the function before returning.
   *
   * @param f The function object to be called. The executor will make
   * a copy of the handler object as required. The function signature of the
   * function object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename Allocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const
  {
    typename decay<Function>::type tmp(BOOST_ASIO_MOVE_CAST(Function)(f));
    service_.dispatch(impl_, tmp);
    (void)a;
  }

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use boost::asio::dispatch().) Request the strand to invoke
  /// the given handler.
  /**
   * This function is used to ask the strand to execute the given handler.
   *
   * The strand object guarantees that handlers posted or dispatched through
   * the strand will not be executed concurrently. The handler may be executed
   * inside this function if the guarantee can be met. If this function is
   * called from within a handler that was posted or dispatched through the same
   * strand, then the new handler will be executed immediately.
   *
   * The strand's guarantee is in addition to the guarantee provided by the
   * underlying io_context. The io_context guarantees that the handler will only
   * be called in a thread in which the io_context's run member function is
   * currently being invoked.
   *
   * @param handler The handler to be called. The strand will make a copy of the
   * handler object as required. The function signature of the handler must be:
   * @code void handler(); @endcode
   */
  template <typename LegacyCompletionHandler>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(LegacyCompletionHandler, void ())
  dispatch(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler)
  {
    return async_initiate<LegacyCompletionHandler, void ()>(
        initiate_dispatch(), handler, this);
  }
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Request the strand to invoke the given function object.
  /**
   * This function is used to ask the executor to execute the given function
   * object. The function object will never be executed inside this function.
   * Instead, it will be scheduled to run by the underlying io_context.
   *
   * @param f The function object to be called. The executor will make
   * a copy of the handler object as required. The function signature of the
   * function object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename Allocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const
  {
    typename decay<Function>::type tmp(BOOST_ASIO_MOVE_CAST(Function)(f));
    service_.post(impl_, tmp);
    (void)a;
  }

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use boost::asio::post().) Request the strand to invoke the
  /// given handler and return immediately.
  /**
   * This function is used to ask the strand to execute the given handler, but
   * without allowing the strand to call the handler from inside this function.
   *
   * The strand object guarantees that handlers posted or dispatched through
   * the strand will not be executed concurrently. The strand's guarantee is in
   * addition to the guarantee provided by the underlying io_context. The
   * io_context guarantees that the handler will only be called in a thread in
   * which the io_context's run member function is currently being invoked.
   *
   * @param handler The handler to be called. The strand will make a copy of the
   * handler object as required. The function signature of the handler must be:
   * @code void handler(); @endcode
   */
  template <typename LegacyCompletionHandler>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(LegacyCompletionHandler, void ())
  post(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler)
  {
    return async_initiate<LegacyCompletionHandler, void ()>(
        initiate_post(), handler, this);
  }
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Request the strand to invoke the given function object.
  /**
   * This function is used to ask the executor to execute the given function
   * object. The function object will never be executed inside this function.
   * Instead, it will be scheduled to run by the underlying io_context.
   *
   * @param f The function object to be called. The executor will make
   * a copy of the handler object as required. The function signature of the
   * function object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename Allocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const
  {
    typename decay<Function>::type tmp(BOOST_ASIO_MOVE_CAST(Function)(f));
    service_.post(impl_, tmp);
    (void)a;
  }

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use boost::asio::bind_executor().) Create a new handler that
  /// automatically dispatches the wrapped handler on the strand.
  /**
   * This function is used to create a new handler function object that, when
   * invoked, will automatically pass the wrapped handler to the strand's
   * dispatch function.
   *
   * @param handler The handler to be wrapped. The strand will make a copy of
   * the handler object as required. The function signature of the handler must
   * be: @code void handler(A1 a1, ... An an); @endcode
   *
   * @return A function object that, when invoked, passes the wrapped handler to
   * the strand's dispatch function. Given a function object with the signature:
   * @code R f(A1 a1, ... An an); @endcode
   * If this function object is passed to the wrap function like so:
   * @code strand.wrap(f); @endcode
   * then the return value is a function object with the signature
   * @code void g(A1 a1, ... An an); @endcode
   * that, when invoked, executes code equivalent to:
   * @code strand.dispatch(boost::bind(f, a1, ... an)); @endcode
   */
  template <typename Handler>
#if defined(GENERATING_DOCUMENTATION)
  unspecified
#else
  detail::wrapped_handler<strand, Handler, detail::is_continuation_if_running>
#endif
  wrap(Handler handler)
  {
    return detail::wrapped_handler<io_context::strand, Handler,
        detail::is_continuation_if_running>(*this, handler);
  }
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Determine whether the strand is running in the current thread.
  /**
   * @return @c true if the current thread is executing a handler that was
   * submitted to the strand using post(), dispatch() or wrap(). Otherwise
   * returns @c false.
   */
  bool running_in_this_thread() const BOOST_ASIO_NOEXCEPT
  {
    return service_.running_in_this_thread(impl_);
  }

  /// Compare two strands for equality.
  /**
   * Two strands are equal if they refer to the same ordered, non-concurrent
   * state.
   */
  friend bool operator==(const strand& a, const strand& b) BOOST_ASIO_NOEXCEPT
  {
    return a.impl_ == b.impl_;
  }

  /// Compare two strands for inequality.
  /**
   * Two strands are equal if they refer to the same ordered, non-concurrent
   * state.
   */
  friend bool operator!=(const strand& a, const strand& b) BOOST_ASIO_NOEXCEPT
  {
    return a.impl_ != b.impl_;
  }

private:
#if !defined(BOOST_ASIO_NO_DEPRECATED)
  struct initiate_dispatch
  {
    template <typename LegacyCompletionHandler>
    void operator()(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler,
        strand* self) const
    {
      // If you get an error on the following line it means that your
      // handler does not meet the documented type requirements for a
      // LegacyCompletionHandler.
      BOOST_ASIO_LEGACY_COMPLETION_HANDLER_CHECK(
          LegacyCompletionHandler, handler) type_check;

      detail::non_const_lvalue<LegacyCompletionHandler> handler2(handler);
      self->service_.dispatch(self->impl_, handler2.value);
    }
  };

  struct initiate_post
  {
    template <typename LegacyCompletionHandler>
    void operator()(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler,
        strand* self) const
    {
      // If you get an error on the following line it means that your
      // handler does not meet the documented type requirements for a
      // LegacyCompletionHandler.
      BOOST_ASIO_LEGACY_COMPLETION_HANDLER_CHECK(
          LegacyCompletionHandler, handler) type_check;

      detail::non_const_lvalue<LegacyCompletionHandler> handler2(handler);
      self->service_.post(self->impl_, handler2.value);
    }
  };
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  boost::asio::detail::strand_service& service_;
  mutable boost::asio::detail::strand_service::implementation_type impl_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)
       //   && !defined(BOOST_ASIO_NO_TS_EXECUTORS)

#endif // BOOST_ASIO_IO_CONTEXT_STRAND_HPP
