// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2014/01 Vicente J. Botet Escriba
//    first implementation of a thread_executor.

#ifndef BOOST_THREAD_THREAD_EXECUTOR_HPP
#define BOOST_THREAD_THREAD_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/throw_exception.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/executor.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/csbl/vector.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  class thread_executor
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
    bool closed_;
    typedef scoped_thread<> thread_t;
    typedef csbl::vector<thread_t> threads_type;
    threads_type threads_;
    mutable mutex mtx_;

    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one()
    {
      return false;
    }

  public:
    /// thread_executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(thread_executor)

    /**
     * \b Effects: creates a inline executor that runs closures immediately.
     *
     * \b Throws: Nothing.
     */
    thread_executor()
    : closed_(false)
    {
    }
    /**
     * \b Effects: Waits for closures (if any) to complete, then joins and destroys the threads.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c thread_executor destructor.
     */
    ~thread_executor()
    {
      // signal to all the worker thread that there will be no more submissions.
      close();
      // all the scoped threads will join before destroying
    }

    /**
     * \b Effects: close the \c thread_executor for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
      lock_guard<mutex> lk(mtx_);
      closed_ = true;
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed(lock_guard<mutex>& )
    {
      return closed_;
    }
    bool closed()
    {
      lock_guard<mutex> lk(mtx_);
      return closed(lk);
    }

    /**
     * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
     *
     * \b Effects: The specified \c closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the \c thread_executor will call \c std::terminate, as is the case with threads.
     *
     * \b Synchronization: completion of \c closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \b Throws: \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      lock_guard<mutex> lk(mtx_);
      if (closed(lk))  BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
      threads_.reserve(threads_.size() + 1);
      thread th(closure);
      threads_.push_back(thread_t(boost::move(th)));
    }
#endif
    void submit(void (*closure)())
    {
      lock_guard<mutex> lk(mtx_);
      if (closed(lk))  BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
      threads_.reserve(threads_.size() + 1);
      thread th(closure);
      threads_.push_back(thread_t(boost::move(th)));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_FWD_REF(Closure) closure)
    {
      lock_guard<mutex> lk(mtx_);
      if (closed(lk))  BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
      threads_.reserve(threads_.size() + 1);
      thread th(boost::forward<Closure>(closure));
      threads_.push_back(thread_t(boost::move(th)));
    }

    /**
     * \b Requires: This must be called from an scheduled task.
     *
     * \b Effects: reschedule functions until pred()
     */
    template <typename Pred>
    bool reschedule_until(Pred const&)
    {
      return false;
    }

  };
}
using executors::thread_executor;
}

#include <boost/config/abi_suffix.hpp>

#endif
#endif
