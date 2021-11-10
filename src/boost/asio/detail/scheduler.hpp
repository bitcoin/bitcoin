//
// detail/scheduler.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_SCHEDULER_HPP
#define BOOST_ASIO_DETAIL_SCHEDULER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/system/error_code.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/detail/atomic_count.hpp>
#include <boost/asio/detail/conditionally_enabled_event.hpp>
#include <boost/asio/detail/conditionally_enabled_mutex.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/detail/reactor_fwd.hpp>
#include <boost/asio/detail/scheduler_operation.hpp>
#include <boost/asio/detail/thread.hpp>
#include <boost/asio/detail/thread_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

struct scheduler_thread_info;

class scheduler
  : public execution_context_service_base<scheduler>,
    public thread_context
{
public:
  typedef scheduler_operation operation;

  // Constructor. Specifies the number of concurrent threads that are likely to
  // run the scheduler. If set to 1 certain optimisation are performed.
  BOOST_ASIO_DECL scheduler(boost::asio::execution_context& ctx,
      int concurrency_hint = 0, bool own_thread = true);

  // Destructor.
  BOOST_ASIO_DECL ~scheduler();

  // Destroy all user-defined handler objects owned by the service.
  BOOST_ASIO_DECL void shutdown();

  // Initialise the task, if required.
  BOOST_ASIO_DECL void init_task();

  // Run the event loop until interrupted or no more work.
  BOOST_ASIO_DECL std::size_t run(boost::system::error_code& ec);

  // Run until interrupted or one operation is performed.
  BOOST_ASIO_DECL std::size_t run_one(boost::system::error_code& ec);

  // Run until timeout, interrupted, or one operation is performed.
  BOOST_ASIO_DECL std::size_t wait_one(
      long usec, boost::system::error_code& ec);

  // Poll for operations without blocking.
  BOOST_ASIO_DECL std::size_t poll(boost::system::error_code& ec);

  // Poll for one operation without blocking.
  BOOST_ASIO_DECL std::size_t poll_one(boost::system::error_code& ec);

  // Interrupt the event processing loop.
  BOOST_ASIO_DECL void stop();

  // Determine whether the scheduler is stopped.
  BOOST_ASIO_DECL bool stopped() const;

  // Restart in preparation for a subsequent run invocation.
  BOOST_ASIO_DECL void restart();

  // Notify that some work has started.
  void work_started()
  {
    ++outstanding_work_;
  }

  // Used to compensate for a forthcoming work_finished call. Must be called
  // from within a scheduler-owned thread.
  BOOST_ASIO_DECL void compensating_work_started();

  // Notify that some work has finished.
  void work_finished()
  {
    if (--outstanding_work_ == 0)
      stop();
  }

  // Return whether a handler can be dispatched immediately.
  BOOST_ASIO_DECL bool can_dispatch();

  /// Capture the current exception so it can be rethrown from a run function.
  BOOST_ASIO_DECL void capture_current_exception();

  // Request invocation of the given operation and return immediately. Assumes
  // that work_started() has not yet been called for the operation.
  BOOST_ASIO_DECL void post_immediate_completion(
      operation* op, bool is_continuation);

  // Request invocation of the given operations and return immediately. Assumes
  // that work_started() has not yet been called for the operations.
  BOOST_ASIO_DECL void post_immediate_completions(std::size_t n,
      op_queue<operation>& ops, bool is_continuation);

  // Request invocation of the given operation and return immediately. Assumes
  // that work_started() was previously called for the operation.
  BOOST_ASIO_DECL void post_deferred_completion(operation* op);

  // Request invocation of the given operations and return immediately. Assumes
  // that work_started() was previously called for each operation.
  BOOST_ASIO_DECL void post_deferred_completions(op_queue<operation>& ops);

  // Enqueue the given operation following a failed attempt to dispatch the
  // operation for immediate invocation.
  BOOST_ASIO_DECL void do_dispatch(operation* op);

  // Process unfinished operations as part of a shutdownoperation. Assumes that
  // work_started() was previously called for the operations.
  BOOST_ASIO_DECL void abandon_operations(op_queue<operation>& ops);

  // Get the concurrency hint that was used to initialise the scheduler.
  int concurrency_hint() const
  {
    return concurrency_hint_;
  }

private:
  // The mutex type used by this scheduler.
  typedef conditionally_enabled_mutex mutex;

  // The event type used by this scheduler.
  typedef conditionally_enabled_event event;

  // Structure containing thread-specific data.
  typedef scheduler_thread_info thread_info;

  // Run at most one operation. May block.
  BOOST_ASIO_DECL std::size_t do_run_one(mutex::scoped_lock& lock,
      thread_info& this_thread, const boost::system::error_code& ec);

  // Run at most one operation with a timeout. May block.
  BOOST_ASIO_DECL std::size_t do_wait_one(mutex::scoped_lock& lock,
      thread_info& this_thread, long usec, const boost::system::error_code& ec);

  // Poll for at most one operation.
  BOOST_ASIO_DECL std::size_t do_poll_one(mutex::scoped_lock& lock,
      thread_info& this_thread, const boost::system::error_code& ec);

  // Stop the task and all idle threads.
  BOOST_ASIO_DECL void stop_all_threads(mutex::scoped_lock& lock);

  // Wake a single idle thread, or the task, and always unlock the mutex.
  BOOST_ASIO_DECL void wake_one_thread_and_unlock(
      mutex::scoped_lock& lock);

  // Helper class to run the scheduler in its own thread.
  class thread_function;
  friend class thread_function;

  // Helper class to perform task-related operations on block exit.
  struct task_cleanup;
  friend struct task_cleanup;

  // Helper class to call work-related operations on block exit.
  struct work_cleanup;
  friend struct work_cleanup;

  // Whether to optimise for single-threaded use cases.
  const bool one_thread_;

  // Mutex to protect access to internal data.
  mutable mutex mutex_;

  // Event to wake up blocked threads.
  event wakeup_event_;

  // The task to be run by this service.
  reactor* task_;

  // Operation object to represent the position of the task in the queue.
  struct task_operation : operation
  {
    task_operation() : operation(0) {}
  } task_operation_;

  // Whether the task has been interrupted.
  bool task_interrupted_;

  // The count of unfinished work.
  atomic_count outstanding_work_;

  // The queue of handlers that are ready to be delivered.
  op_queue<operation> op_queue_;

  // Flag to indicate that the dispatcher has been stopped.
  bool stopped_;

  // Flag to indicate that the dispatcher has been shut down.
  bool shutdown_;

  // The concurrency hint used to initialise the scheduler.
  const int concurrency_hint_;

  // The thread that is running the scheduler.
  boost::asio::detail::thread* thread_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/scheduler.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_DETAIL_SCHEDULER_HPP
