#ifndef BOOST_THREAD_EXPERIMENTAL_PARALLEL_V2_TASK_REGION_HPP
#define BOOST_THREAD_EXPERIMENTAL_PARALLEL_V2_TASK_REGION_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <boost/throw_exception.hpp>
#include <boost/thread/detail/config.hpp>

#include <boost/thread/future.hpp>
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
#include <boost/thread/executors/basic_thread_pool.hpp>
#endif
#include <boost/thread/experimental/exception_list.hpp>
#include <boost/thread/experimental/parallel/v2/inline_namespace.hpp>
#include <boost/thread/csbl/vector.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/config/abi_prefix.hpp>

#define BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED

namespace boost
{
namespace experimental
{
namespace parallel
{
BOOST_THREAD_INLINE_NAMESPACE(v2)
{
  class BOOST_SYMBOL_VISIBLE task_canceled_exception: public std::exception
  {
  public:
    //task_canceled_exception() BOOST_NOEXCEPT {}
    //task_canceled_exception(const task_canceled_exception&) BOOST_NOEXCEPT {}
    //task_canceled_exception& operator=(const task_canceled_exception&) BOOST_NOEXCEPT {}
    virtual const char* what() const BOOST_NOEXCEPT_OR_NOTHROW
    { return "task_canceled_exception";}
  };

  template <class Executor>
  class task_region_handle_gen;

  namespace detail
  {
    void handle_task_region_exceptions(exception_list& errors)
    {
      try {
        throw;
      }
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
      catch (task_canceled_exception&)
      {
      }
#endif
      catch (exception_list const& el)
      {
        for (exception_list::const_iterator it = el.begin(); it != el.end(); ++it)
        {
          boost::exception_ptr const& e = *it;
          try {
            rethrow_exception(e);
          }
          catch (...)
          {
            handle_task_region_exceptions(errors);
          }
        }
      }
      catch (...)
      {
        errors.add(boost::current_exception());
      }
    }

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    template <class TRH, class F>
    struct wrapped
    {
      TRH& tr;
      F f;
      wrapped(TRH& tr, BOOST_THREAD_RV_REF(F) f) : tr(tr), f(move(f))
      {}
      void operator()()
      {
        try
        {
          f();
        }
        catch (...)
        {
          lock_guard<mutex> lk(tr.mtx);
          tr.canceled = true;
          throw;
        }
      }
    };
#endif
  }

  template <class Executor>
  class task_region_handle_gen
  {
  private:
    // Private members and friends
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    template <class TRH, class F>
    friend struct detail::wrapped;
#endif
    template <typename F>
    friend void task_region(BOOST_THREAD_FWD_REF(F) f);
    template<typename F>
    friend void task_region_final(BOOST_THREAD_FWD_REF(F) f);
    template <class Ex, typename F>
    friend void task_region(Ex&, BOOST_THREAD_FWD_REF(F) f);
    template<class Ex, typename F>
    friend void task_region_final(Ex&, BOOST_THREAD_FWD_REF(F) f);

    void wait_all()
    {
      wait_for_all(group.begin(), group.end());

      for (group_type::iterator it = group.begin(); it != group.end(); ++it)
      {
        BOOST_THREAD_FUTURE<void>& f = *it;
        if (f.has_exception())
        {
          try
          {
            boost::rethrow_exception(f.get_exception_ptr());
          }
          catch (...)
          {
            detail::handle_task_region_exceptions(exs);
          }
        }
      }
      if (exs.size() != 0)
      {
        boost::throw_exception(exs);
      }
    }
protected:
#if ! defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && ! defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    {}
#endif

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    : canceled(false)
    , ex(0)
    {}
    task_region_handle_gen(Executor& ex)
    : canceled(false)
    , ex(&ex)
    {}

#endif

#if ! defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    : ex(0)
    {}
    task_region_handle_gen(Executor& ex)
    : ex(&ex)
    {}
#endif

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && ! defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    : canceled(false)
    {
    }
#endif

    ~task_region_handle_gen()
    {
      //wait_all();
    }

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    mutable mutex mtx;
    bool canceled;
#endif
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
    Executor* ex;
#endif
    exception_list exs;
    typedef csbl::vector<BOOST_THREAD_FUTURE<void> > group_type;
    group_type group;

  public:
    BOOST_DELETED_FUNCTION(task_region_handle_gen(const task_region_handle_gen&))
    BOOST_DELETED_FUNCTION(task_region_handle_gen& operator=(const task_region_handle_gen&))
    BOOST_DELETED_FUNCTION(task_region_handle_gen* operator&() const)

  public:
    template<typename F>
    void run(BOOST_THREAD_FWD_REF(F) f)
    {
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
      {
        lock_guard<mutex> lk(mtx);
        if (canceled) {
          boost::throw_exception(task_canceled_exception());
        }
      }
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
      group.push_back(async(*ex, detail::wrapped<task_region_handle_gen<Executor>, F>(*this, forward<F>(f))));
#else
      group.push_back(async(detail::wrapped<task_region_handle_gen<Executor>, F>(*this, forward<F>(f))));
#endif
#else
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
      group.push_back(async(*ex, forward<F>(f)));
#else
      group.push_back(async(forward<F>(f)));
#endif
#endif
    }

    void wait()
    {
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
      {
        lock_guard<mutex> lk(mtx);
        if (canceled) {
          boost::throw_exception(task_canceled_exception());
        }
      }
#endif
      wait_all();
    }
  };
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
  typedef basic_thread_pool default_executor;
#else
  typedef int default_executor;
#endif
  class task_region_handle :
    public task_region_handle_gen<default_executor>
  {
    default_executor tp;
    template <typename F>
    friend void task_region(BOOST_THREAD_FWD_REF(F) f);
    template<typename F>
    friend void task_region_final(BOOST_THREAD_FWD_REF(F) f);

  protected:
    task_region_handle() : task_region_handle_gen<default_executor>()
    {
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
      ex = &tp;
#endif
    }
    BOOST_DELETED_FUNCTION(task_region_handle(const task_region_handle&))
    BOOST_DELETED_FUNCTION(task_region_handle& operator=(const task_region_handle&))
    BOOST_DELETED_FUNCTION(task_region_handle* operator&() const)

  };

  template <typename Executor, typename F>
  void task_region_final(Executor& ex, BOOST_THREAD_FWD_REF(F) f)
  {
    task_region_handle_gen<Executor> tr(ex);
    try
    {
      f(tr);
    }
    catch (...)
    {
      detail::handle_task_region_exceptions(tr.exs);
    }
    tr.wait_all();
  }

  template <typename Executor, typename F>
  void task_region(Executor& ex, BOOST_THREAD_FWD_REF(F) f)
  {
    task_region_final(ex, forward<F>(f));
  }

  template <typename F>
  void task_region_final(BOOST_THREAD_FWD_REF(F) f)
  {
    task_region_handle tr;
    try
    {
      f(tr);
    }
    catch (...)
    {
      detail::handle_task_region_exceptions(tr.exs);
    }
    tr.wait_all();
  }

  template <typename F>
  void task_region(BOOST_THREAD_FWD_REF(F) f)
  {
    task_region_final(forward<F>(f));
  }

} // v2
} // parallel
} // experimental
} // boost

#include <boost/config/abi_suffix.hpp>

#endif // header
