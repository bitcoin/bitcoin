// Copyright (C) 2013,2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/09 Vicente J. Botet Escriba
//    Adapt to boost from CCIA C++11 implementation

#ifndef BOOST_THREAD_EXECUTORS_EXECUTOR_HPP
#define BOOST_THREAD_EXECUTORS_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/executors/work.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace executors
  {
  class executor
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;

    /// executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(executor)
    executor() {}

    /**
     * \par Effects
     * Destroys the executor.
     *
     * \par Synchronization
     * The completion of all the closures happen before the completion of the executor destructor.
     */
    virtual ~executor() {}

    /**
     * \par Effects
     * Close the \c executor for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    virtual void close() = 0;

    /**
     * \par Returns
     * Whether the pool is closed for submissions.
     */
    virtual bool closed() = 0;

    /**
     * \par Effects
     * The specified closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the executor will call std::terminate, as is the case with threads.
     *
     * \par Synchronization
     * Ccompletion of closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \par Throws
     * \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */
    virtual void submit(BOOST_THREAD_RV_REF(work) closure) = 0;
//    virtual void submit(work& closure) = 0;

    /**
     * \par Requires
     * \c Closure is a model of Callable(void()) and a model of CopyConstructible/MoveConstructible.
     *
     * \par Effects
     * The specified closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the thread pool will call std::terminate, as is the case with threads.
     *
     * \par Synchronization
     * Completion of closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \par Throws
     * \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      work w ((closure));
      submit(boost::move(w));
    }
#endif
    void submit(void (*closure)())
    {
      work w ((closure));
      submit(boost::move(w));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_FWD_REF(Closure) closure)
    {
      //submit(work(boost::forward<Closure>(closure)));
      work w((boost::forward<Closure>(closure)));
      submit(boost::move(w));
    }

    /**
     * \par Effects
     * Try to execute one task.
     *
     * \par Returns
     * Whether a task has been executed.
     *
     * \par Throws
     * Whatever the current task constructor throws or the task() throws.
     */
    virtual bool try_executing_one() = 0;

    /**
     * \par Requires
     * This must be called from an scheduled task.
     *
     * \par Effects
     * Reschedule functions until pred()
     */
    template <typename Pred>
    bool reschedule_until(Pred const& pred)
    {
      do {
        //schedule_one_or_yield();
        if ( ! try_executing_one())
        {
          return false;
        }
      } while (! pred());
      return true;
    }
  };

  }
  using executors::executor;
}

#include <boost/config/abi_suffix.hpp>

#endif
#endif
