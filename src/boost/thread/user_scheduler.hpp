// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_USER_SCHEDULER_HPP
#define BOOST_THREAD_USER_SCHEDULER_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  class user_scheduler
  {
    /// type-erasure to store the works to do
    typedef  executors::work work;

    /// the thread safe work queue
    sync_queue<work > work_queue;

  public:
    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one()
    {
      work task;
      try
      {
        if (work_queue.try_pull(task) == queue_op_status::success)
        {
          task();
          return true;
        }
        return false;
      }
      catch (std::exception& )
      {
        return false;
      }
      catch (...)
      {
        return false;
      }
    }
  private:
    /**
     * Effects: schedule one task or yields
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    void schedule_one_or_yield()
    {
        if ( ! try_executing_one())
        {
          this_thread::yield();
        }
    }


    /**
     * The main loop of the worker thread
     */
    void worker_thread()
    {
      while (!closed())
      {
        schedule_one_or_yield();
      }
      while (try_executing_one())
      {
      }
    }

  public:
    /// user_scheduler is not copyable.
    BOOST_THREAD_NO_COPYABLE(user_scheduler)

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    user_scheduler()
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c user_scheduler destructor.
     */
    ~user_scheduler()
    {
      // signal to all the worker thread that there will be no more submissions.
      close();
    }

    /**
     * loop
     */
    void loop() { worker_thread(); }
    /**
     * \b Effects: close the \c user_scheduler for submissions.
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
     * If invoked closure throws an exception the \c user_scheduler will call \c std::terminate, as is the case with threads.
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
      work w ((closure));
      work_queue.push(boost::move(w));
      //work_queue.push(work(closure)); // todo check why this doesn't work
    }
#endif
    void submit(void (*closure)())
    {
      work w ((closure));
      work_queue.push(boost::move(w));
      //work_queue.push(work(closure)); // todo check why this doesn't work
    }

    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      work w =boost::move(closure);
      work_queue.push(boost::move(w));
      //work_queue.push(work(boost::move(closure))); // todo check why this doesn't work
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
      while (q.empty())
      {
        work task = q.front();
        q.pop_front();
        task();
      }
    }

  };

}

#include <boost/config/abi_suffix.hpp>

#endif
#endif
