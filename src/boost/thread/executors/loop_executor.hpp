// Copyright (C) 2013,2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple user scheduler.
// 2013/11 Vicente J. Botet Escriba
//    rename loop_executor.

#ifndef BOOST_THREAD_EXECUTORS_LOOP_EXECUTOR_HPP
#define BOOST_THREAD_EXECUTORS_LOOP_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/assert.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{

  class loop_executor
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:
    /// the thread safe work queue
    concurrent::sync_queue<work > work_queue;

  public:
    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one()
    {
      return execute_one(/*wait:*/false);
    }

  private:
    /**
     * Effects: Execute one task.
     * Remark: If wait is true, waits until a task is available or the executor
     *         is closed. If wait is false, returns false immediately if no
     *         task is available.
     * Returns: whether a task has been executed (if wait is true, only returns false if closed).
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool execute_one(bool wait)
    {
      work task;
      try
      {
        queue_op_status status = wait ?
          work_queue.wait_pull(task) :
          work_queue.try_pull(task);
        if (status == queue_op_status::success)
        {
          task();
          return true;
        }
        BOOST_ASSERT(!wait || status == queue_op_status::closed);
        return false;
      }
      catch (...)
      {
        std::terminate();
        //return false;
      }
    }

  public:
    /// loop_executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(loop_executor)

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    loop_executor()
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c loop_executor destructor.
     */
    ~loop_executor()
    {
      // signal to all the worker thread that there will be no more submissions.
      close();
    }

    /**
     * The main loop of the worker thread
     */
    void loop()
    {
      while (execute_one(/*wait:*/true))
      {
      }
      BOOST_ASSERT(closed());
      while (try_executing_one())
      {
      }
    }

    /**
     * \b Effects: close the \c loop_executor for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
      work_queue.close();
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed()
    {
      return work_queue.closed();
    }

    /**
     * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
     *
     * \b Effects: The specified \c closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the \c loop_executor will call \c std::terminate, as is the case with threads.
     *
     * \b Synchronization: completion of \c closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \b Throws: \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */
    void submit(BOOST_THREAD_RV_REF(work) closure)  {
      work_queue.push(boost::move(closure));
    }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      submit(work(closure));
   }
#endif

    void submit(void (*closure)())
    {
      submit(work(closure));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_FWD_REF(Closure) closure)
    {
      //work_queue.push(work(boost::forward<Closure>(closure)));
      work w((boost::forward<Closure>(closure)));
      submit(boost::move(w));
    }

    /**
     * \b Requires: This must be called from an scheduled task.
     *
     * \b Effects: reschedule functions until pred()
     */
    template <typename Pred>
    bool reschedule_until(Pred const& pred)
    {
      do {
        if ( ! try_executing_one())
        {
          return false;
        }
      } while (! pred());
      return true;
    }

    /**
     * run queued closures
     */
    void run_queued_closures()
    {
      sync_queue<work>::underlying_queue_type q = work_queue.underlying_queue();
      while (! q.empty())
      {
        work& task = q.front();
        task();
        q.pop_front();
      }
    }

  };
}
using executors::loop_executor;

}

#include <boost/config/abi_suffix.hpp>
#endif

#endif
