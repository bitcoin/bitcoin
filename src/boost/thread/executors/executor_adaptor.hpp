// Copyright (C) 2013,2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/09 Vicente J. Botet Escriba
//    Adapt to boost from CCIA C++11 implementation

#ifndef BOOST_THREAD_EXECUTORS_EXECUTOR_ADAPTOR_HPP
#define BOOST_THREAD_EXECUTORS_EXECUTOR_ADAPTOR_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE

#include <boost/thread/executors/executor.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  /**
   * Polymorphic adaptor of a model of Executor to an executor.
   */
  template <typename Executor>
  class executor_adaptor : public executor
  {
    Executor ex;
  public:
    /// type-erasure to store the works to do
    typedef  executor::work work;

    /// executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(executor_adaptor)

    /**
     * executor_adaptor constructor
     */
#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <typename ...Args>
    executor_adaptor(BOOST_THREAD_RV_REF(Args) ... args) : ex(boost::forward<Args>(args)...) {}
#else
    /**
     * executor_adaptor constructor
     */
    executor_adaptor() : ex() {}

    template <typename A1>
    executor_adaptor(
        BOOST_THREAD_FWD_REF(A1) a1
        ) :
      ex(
          boost::forward<A1>(a1)
          ) {}
    template <typename A1, typename A2>
    executor_adaptor(
        BOOST_THREAD_FWD_REF(A1) a1,
        BOOST_THREAD_FWD_REF(A2) a2
        ) :
      ex(
          boost::forward<A1>(a1),
          boost::forward<A2>(a2)
          ) {}
    template <typename A1, typename A2, typename A3>
    executor_adaptor(
        BOOST_THREAD_FWD_REF(A1) a1,
        BOOST_THREAD_FWD_REF(A2) a2,
        BOOST_THREAD_FWD_REF(A3) a3
        ) :
      ex(
          boost::forward<A1>(a1),
          boost::forward<A2>(a2),
          boost::forward<A3>(a3)
          ) {}
#endif
    Executor& underlying_executor() { return ex; }

    /**
     * \b Effects: close the \c executor for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    void close() { ex.close(); }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed() { return ex.closed(); }

    /**
     * \b Effects: The specified closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the executor will call std::terminate, as is the case with threads.
     *
     * \b Synchronization: completion of closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \b Throws: \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */
    void submit(BOOST_THREAD_RV_REF(work) closure)  {
      return ex.submit(boost::move(closure));
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
      //submit(work(boost::forward<Closure>(closure)));
      work w((boost::forward<Closure>(closure)));
      submit(boost::move(w));
    }

    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one() { return ex.try_executing_one(); }

  };
}
using executors::executor_adaptor;
}

#include <boost/config/abi_suffix.hpp>

#endif
#endif
