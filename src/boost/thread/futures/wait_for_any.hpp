//  (C) Copyright 2008-10 Anthony Williams
//  (C) Copyright 2011-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURES_WAIT_FOR_ANY_HPP
#define BOOST_THREAD_FUTURES_WAIT_FOR_ANY_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/detail/move.hpp>
#include <boost/thread/futures/is_future_type.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <boost/core/enable_if.hpp>
#include <boost/next_prior.hpp>
#include <boost/scoped_array.hpp>

#include <iterator>
#include <vector>

namespace boost
{
  namespace detail
  {
    template <class Future>
    class waiter_for_any_in_seq
    {
      struct registered_waiter;
      typedef std::vector<int>::size_type count_type;

      struct registered_waiter
      {
        typedef Future future_type;
        future_type* future_;
        typedef typename Future::notify_when_ready_handle notify_when_ready_handle;
        notify_when_ready_handle handle;
        count_type index;

        registered_waiter(future_type & a_future,
            notify_when_ready_handle handle_, count_type index_) :
          future_(&a_future), handle(handle_), index(index_)
        {
        }
      };

      struct all_futures_lock
      {
#ifdef _MANAGED
        typedef std::ptrdiff_t count_type_portable;
#else
        typedef count_type count_type_portable;
#endif
        count_type_portable count;
        boost::scoped_array<boost::unique_lock<boost::mutex> > locks;

        all_futures_lock(std::vector<registered_waiter>& waiters) :
          count(waiters.size()), locks(new boost::unique_lock<boost::mutex>[count])
        {
          for (count_type_portable i = 0; i < count; ++i)
          {
            locks[i] = BOOST_THREAD_MAKE_RV_REF(boost::unique_lock<boost::mutex>(waiters[i].future_->mutex()));
          }
        }

        void lock()
        {
          boost::lock(locks.get(), locks.get() + count);
        }

        void unlock()
        {
          for (count_type_portable i = 0; i < count; ++i)
          {
            locks[i].unlock();
          }
        }
      };

      boost::condition_variable_any cv;
      std::vector<registered_waiter> waiters_;
      count_type future_count;

    public:
      waiter_for_any_in_seq() :
        future_count(0)
      {
      }

      template <typename F>
      void add(F& f)
      {
        if (f.valid())
        {
          registered_waiter waiter(f, f.notify_when_ready(cv), future_count);
          try
          {
            waiters_.push_back(waiter);
          }
          catch (...)
          {
            f.future_->unnotify_when_ready(waiter.handle);
            throw;
          }
          ++future_count;
        }
      }

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
      template <typename F1, typename ... Fs>
      void add(F1& f1, Fs&... fs)
      {
        add(f1);
        add(fs...);
      }
#endif

      count_type wait()
      {
        all_futures_lock lk(waiters_);
        for (;;)
        {
          for (count_type i = 0; i < waiters_.size(); ++i)
          {
            if (waiters_[i].future_->is_ready(lk.locks[i]))
            {
              return waiters_[i].index;
            }
          }
          cv.wait(lk);
        }
      }

      ~waiter_for_any_in_seq()
      {
        for (count_type i = 0; i < waiters_.size(); ++i)
        {
          waiters_[i].future_->unnotify_when_ready(waiters_[i].handle);
        }
      }
    };
  }

  template <typename Iterator>
  typename boost::disable_if<is_future_type<Iterator> , Iterator>::type wait_for_any(Iterator begin, Iterator end)
  {
    if (begin == end) return end;

    detail::waiter_for_any_in_seq<typename std::iterator_traits<Iterator>::value_type> waiter;
    for (Iterator current = begin; current != end; ++current)
    {
      waiter.add(*current);
    }
    return boost::next(begin, waiter.wait());
  }
}

#endif // header
