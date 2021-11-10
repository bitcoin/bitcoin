// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_GENERIC_EXECUTOR_REF_HPP
#define BOOST_THREAD_EXECUTORS_GENERIC_EXECUTOR_REF_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/executors/executor.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace executors
  {

  template <class Executor>
  class executor_ref : public executor
  {
    Executor& ex;
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;

    /// executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(executor_ref)
    executor_ref(Executor& ex_) : ex(ex_) {}

    /**
     * \par Effects
     * Destroys the executor.
     *
     * \par Synchronization
     * The completion of all the closures happen before the completion of the executor destructor.
     */
    ~executor_ref() {}

    /**
     * \par Effects
     * Close the \c executor for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    void close() { ex.close(); }

    /**
     * \par Returns
     * Whether the pool is closed for submissions.
     */
    bool closed() { return ex.closed(); }

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
    void submit(BOOST_THREAD_RV_REF(work) closure) {
      ex.submit(boost::move(closure));
    }
//    void submit(work& closure) {
//      ex.submit(closure);
//    }


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
    bool try_executing_one() { return ex.try_executing_one(); }

  };

  class generic_executor_ref
  {
    shared_ptr<executor> ex;
  public:
    /// type-erasure to store the works to do
    typedef executors::work work;

    template<typename Executor>
    generic_executor_ref(Executor& ex_)
    //: ex(make_shared<executor_ref<Executor> >(ex_)) // todo check why this doesn't works with C++03
    : ex( new executor_ref<Executor>(ex_) )
    {
    }

    //generic_executor_ref(generic_executor_ref const& other) noexcept    {}
    //generic_executor_ref& operator=(generic_executor_ref const& other) noexcept    {}


    /**
     * \par Effects
     * Close the \c executor for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    void close() { ex->close(); }

    /**
     * \par Returns
     * Whether the pool is closed for submissions.
     */
    bool closed() { return ex->closed(); }

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

    void submit(BOOST_THREAD_RV_REF(work) closure)
    {
      ex->submit(boost::move(closure));
    }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      //work w ((closure));
      //submit(boost::move(w));
      submit(work(closure));
    }
#endif
    void submit(void (*closure)())
    {
      work w ((closure));
      submit(boost::move(w));
      //submit(work(closure));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_FWD_REF(Closure) closure)
    {
      work w((boost::forward<Closure>(closure)));
      submit(boost::move(w));
    }

//    size_t num_pending_closures() const
//    {
//      return ex->num_pending_closures();
//    }

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
    bool try_executing_one() { return ex->try_executing_one(); }

    /**
     * \par Requires
     * This must be called from an scheduled task.
     *
     * \par Effects
     * reschedule functions until pred()
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
  using executors::executor_ref;
  using executors::generic_executor_ref;
}

#include <boost/config/abi_suffix.hpp>

#endif
#endif
