// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_SCHEDULER_HPP
#define BOOST_THREAD_EXECUTORS_SCHEDULER_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE
#include <boost/thread/executors/detail/scheduled_executor_base.hpp>

#include <boost/chrono/time_point.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>

#include <boost/config/abi_prefix.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif

namespace boost
{
  namespace executors
  {
    /// Wraps the reference to an executor and a function to make a work that submit the function using the executor.
    template <class Executor, class Function>
    class resubmitter
    {
    public:
      resubmitter(Executor& ex, Function funct) :
        ex(ex),
        funct(boost::move(funct))
      {}

      void operator()()
      {
        ex.submit(funct);
      }

    private:
      Executor&   ex;
      Function funct;
    };

    /// resubmitter factory
    template <class Executor, class Function>
    resubmitter<Executor, typename decay<Function>::type>
    resubmit(Executor& ex, BOOST_THREAD_FWD_REF(Function) funct) {
      return resubmitter<Executor, typename decay<Function>::type >(ex, boost::move(funct));
    }

    /// Wraps references to a @c Scheduler and an @c Executor providing an @c Executor that
    /// resubmit the function using the referenced Executor at a given @c time_point known at construction.
    template <class Scheduler, class Executor>
    class resubmit_at_executor
    {
    public:
      typedef typename Scheduler::clock clock;
      typedef typename Scheduler::work work;

      template <class Duration>
      resubmit_at_executor(Scheduler& sch, Executor& ex, chrono::time_point<clock, Duration> const& tp) :
        sch(sch),
        ex(ex),
        tp(tp),
        is_closed(false)
      {
      }

      ~resubmit_at_executor()
      {
        close();
      }

      template <class Work>
      void submit(BOOST_THREAD_FWD_REF(Work) w)
      {
        if (closed())
        {
          BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
        }
        sch.submit_at(resubmit(ex,boost::forward<Work>(w)), tp);
      }

      Executor& underlying_executor()
      {
          return ex;
      }
      Scheduler& underlying_scheduler()
      {
          return sch;
      }

      void close()
      {
        is_closed = true;
      }

      bool closed()
      {
        return is_closed || sch.closed() || ex.closed();
      }

    private:
      Scheduler&  sch;
      Executor&   ex;
      typename clock::time_point  tp;
      bool  is_closed;
    };


    /// Expression template helper storing a pair of references to an @c Scheduler and an @c Executor
    /// It provides factory helper functions such as at/after that convert these a pair of @c Scheduler @c Executor
    /// into an new @c Executor that submit the work using the referenced @c Executor at/after a specific time/duration
    /// respectively, using the referenced @Scheduler.
    template <class Scheduler, class Executor>
    class scheduler_executor_wrapper
    {
    public:
      typedef typename Scheduler::clock clock;
      typedef typename Scheduler::work work;
      typedef resubmit_at_executor<Scheduler, Executor> the_executor;

      scheduler_executor_wrapper(Scheduler& sch, Executor& ex) :
          sch(sch),
          ex(ex)
      {}

      ~scheduler_executor_wrapper()
      {
      }

      Executor& underlying_executor()
      {
          return ex;
      }
      Scheduler& underlying_scheduler()
      {
          return sch;
      }

      template <class Rep, class Period>
      the_executor after(chrono::duration<Rep,Period> const& rel_time)
      {
        return at(clock::now() + rel_time );
      }

      template <class Duration>
      the_executor at(chrono::time_point<clock,Duration> const& abs_time)
      {
        return the_executor(sch, ex, abs_time);
      }

    private:
      Scheduler& sch;
      Executor& ex;
    }; //end class

    /// Wraps a reference to a @c Scheduler providing an @c Executor that
    /// run the function at a given @c time_point known at construction.
    template <class Scheduler>
    class at_executor
    {
    public:
      typedef typename Scheduler::clock clock;
      typedef typename Scheduler::work work;
      typedef typename clock::time_point time_point;

      template <class Duration>
      at_executor(Scheduler& sch, chrono::time_point<clock,Duration> const& tp) :
          sch(sch),
          tp(tp),
          is_closed(false)
      {}

      ~at_executor()
      {
        close();
      }

      Scheduler& underlying_scheduler()
      {
          return sch;
      }

      void close()
      {
        is_closed = true;
      }

      bool closed()
      {
        return is_closed || sch.closed();
      }

      template <class Work>
      void submit(BOOST_THREAD_FWD_REF(Work) w)
      {
        if (closed())
        {
          BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
        }
        sch.submit_at(boost::forward<Work>(w), tp);
      }

      template <class Executor>
      resubmit_at_executor<Scheduler, Executor> on(Executor& ex)
      {
        return resubmit_at_executor<Scheduler, Executor>(sch, ex, tp);
      }

    private:
      Scheduler& sch;
      time_point  tp;
      bool  is_closed;
    }; //end class

    /// A @c Scheduler using a specific thread. Note that a Scheduler is not an Executor.
    /// It provides factory helper functions such as at/after that convert a @c Scheduler into an @c Executor
    /// that submit the work at/after a specific time/duration respectively.
    template <class Clock = chrono::steady_clock>
    class scheduler : public detail::scheduled_executor_base<Clock>
    {
    public:
      typedef typename detail::scheduled_executor_base<Clock>::work work;

      typedef Clock clock;

      scheduler()
        : super(),
          thr(&super::loop, this) {}

      ~scheduler()
      {
        this->close();
        thr.interrupt();
        thr.join();
      }
      template <class Ex>
      scheduler_executor_wrapper<scheduler, Ex> on(Ex& ex)
      {
        return scheduler_executor_wrapper<scheduler, Ex>(*this, ex);
      }

      template <class Rep, class Period>
      at_executor<scheduler> after(chrono::duration<Rep,Period> const& rel_time)
      {
        return at(rel_time + clock::now());
      }

      template <class Duration>
      at_executor<scheduler> at(chrono::time_point<clock,Duration> const& tp)
      {
        return at_executor<scheduler>(*this, tp);
      }

    private:
      typedef detail::scheduled_executor_base<Clock> super;
      thread thr;
    };


  }
  using executors::resubmitter;
  using executors::resubmit;
  using executors::resubmit_at_executor;
  using executors::scheduler_executor_wrapper;
  using executors::at_executor;
  using executors::scheduler;
}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#include <boost/config/abi_suffix.hpp>

#endif
#endif
