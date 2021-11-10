#ifndef BOOST_THREAD_PTHREAD_MUTEX_HPP
#define BOOST_THREAD_PTHREAD_MUTEX_HPP
// (C) Copyright 2007-8 Anthony Williams
// (C) Copyright 2011,2012,2015 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/assert.hpp>
#include <pthread.h>
#include <boost/throw_exception.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/thread/exceptions.hpp>
#if defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
#include <boost/thread/lock_types.hpp>
#endif
#include <boost/thread/thread_time.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif
#include <boost/assert.hpp>
#include <errno.h>
#include <boost/thread/detail/platform_time.hpp>
#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#include <boost/thread/pthread/pthread_helpers.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>


#include <boost/config/abi_prefix.hpp>

namespace boost
{

    class BOOST_THREAD_CAPABILITY("mutex") mutex
    {
    private:
        pthread_mutex_t m;
    public:
        BOOST_THREAD_NO_COPYABLE(mutex)

        mutex()
        {
            int const res=posix::pthread_mutex_init(&m);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost:: mutex constructor failed in pthread_mutex_init"));
            }
        }
        ~mutex()
        {
          BOOST_VERIFY(!posix::pthread_mutex_destroy(&m));
        }

        void lock() BOOST_THREAD_ACQUIRE()
        {
            int res = posix::pthread_mutex_lock(&m);
            if (res)
            {
                boost::throw_exception(lock_error(res,"boost: mutex lock failed in pthread_mutex_lock"));
            }
        }

        void unlock() BOOST_THREAD_RELEASE()
        {
            BOOST_VERIFY(!posix::pthread_mutex_unlock(&m));
        }

        bool try_lock() BOOST_THREAD_TRY_ACQUIRE(true)
        {
            int res = posix::pthread_mutex_trylock(&m);
            if (res==EBUSY)
            {
                return false;
            }

            return !res;
        }

#define BOOST_THREAD_DEFINES_MUTEX_NATIVE_HANDLE
        typedef pthread_mutex_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &m;
        }

#if defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
        typedef unique_lock<mutex> scoped_lock;
        typedef detail::try_lock_wrapper<mutex> scoped_try_lock;
#endif
    };

    typedef mutex try_mutex;

    class timed_mutex
    {
    private:
        pthread_mutex_t m;
#ifndef BOOST_THREAD_USES_PTHREAD_TIMEDLOCK
        pthread_cond_t cond;
        bool is_locked;
#endif
    public:
        BOOST_THREAD_NO_COPYABLE(timed_mutex)
        timed_mutex()
        {
            int const res=posix::pthread_mutex_init(&m);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost:: timed_mutex constructor failed in pthread_mutex_init"));
            }
#ifndef BOOST_THREAD_USES_PTHREAD_TIMEDLOCK
            int const res2=posix::pthread_cond_init(&cond);
            if(res2)
            {
                BOOST_VERIFY(!posix::pthread_mutex_destroy(&m));
                boost::throw_exception(thread_resource_error(res2, "boost:: timed_mutex constructor failed in pthread_cond_init"));
            }
            is_locked=false;
#endif
        }
        ~timed_mutex()
        {
            BOOST_VERIFY(!posix::pthread_mutex_destroy(&m));
#ifndef BOOST_THREAD_USES_PTHREAD_TIMEDLOCK
            BOOST_VERIFY(!posix::pthread_cond_destroy(&cond));
#endif
        }

#if defined BOOST_THREAD_USES_DATETIME
        template<typename TimeDuration>
        bool timed_lock(TimeDuration const & relative_time)
        {
            if (relative_time.is_pos_infinity())
            {
                lock();
                return true;
            }
            if (relative_time.is_special())
            {
                return true;
            }
            detail::platform_duration d(relative_time);
#if defined(BOOST_THREAD_HAS_MONO_CLOCK) && !defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
            const detail::mono_platform_timepoint ts(detail::mono_platform_clock::now() + d);
            d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
            while ( ! do_try_lock_until(detail::internal_platform_clock::now() + d) )
            {
              d = ts - detail::mono_platform_clock::now();
              if ( d <= detail::platform_duration::zero() ) return false; // timeout occurred
              d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
            }
            return true;
#else
            return do_try_lock_until(detail::internal_platform_clock::now() + d);
#endif
        }
        bool timed_lock(boost::xtime const & absolute_time)
        {
            return timed_lock(system_time(absolute_time));
        }
#endif
#ifdef BOOST_THREAD_USES_PTHREAD_TIMEDLOCK
        void lock()
        {
            int res = posix::pthread_mutex_lock(&m);
            if (res)
            {
                boost::throw_exception(lock_error(res,"boost: mutex lock failed in pthread_mutex_lock"));
            }
        }

        void unlock()
        {
            BOOST_VERIFY(!posix::pthread_mutex_unlock(&m));
        }

        bool try_lock()
        {
          int res = posix::pthread_mutex_trylock(&m);
          if (res==EBUSY)
          {
              return false;
          }

          return !res;
        }


    private:
        bool do_try_lock_until(detail::internal_platform_timepoint const &timeout)
        {
          int const res=pthread_mutex_timedlock(&m,&timeout.getTs());
          BOOST_ASSERT(!res || res==ETIMEDOUT);
          return !res;
        }
    public:

#else
        void lock()
        {
            boost::pthread::pthread_mutex_scoped_lock const local_lock(&m);
            while(is_locked)
            {
                BOOST_VERIFY(!posix::pthread_cond_wait(&cond,&m));
            }
            is_locked=true;
        }

        void unlock()
        {
            boost::pthread::pthread_mutex_scoped_lock const local_lock(&m);
            is_locked=false;
            BOOST_VERIFY(!posix::pthread_cond_signal(&cond));
        }

        bool try_lock()
        {
            boost::pthread::pthread_mutex_scoped_lock const local_lock(&m);
            if(is_locked)
            {
                return false;
            }
            is_locked=true;
            return true;
        }

    private:
        bool do_try_lock_until(detail::internal_platform_timepoint const &timeout)
        {
            boost::pthread::pthread_mutex_scoped_lock const local_lock(&m);
            while(is_locked)
            {
                int const cond_res=posix::pthread_cond_timedwait(&cond,&m,&timeout.getTs());
                if(cond_res==ETIMEDOUT)
                {
                    break;
                }
                BOOST_ASSERT(!cond_res);
            }
            if(is_locked)
            {
                return false;
            }
            is_locked=true;
            return true;
        }
    public:
#endif

#if defined BOOST_THREAD_USES_DATETIME
        bool timed_lock(system_time const & abs_time)
        {
            const detail::real_platform_timepoint ts(abs_time);
#if defined BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
            detail::platform_duration d(ts - detail::real_platform_clock::now());
            d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
            while ( ! do_try_lock_until(detail::internal_platform_clock::now() + d) )
            {
              d = ts - detail::real_platform_clock::now();
              if ( d <= detail::platform_duration::zero() ) return false; // timeout occurred
              d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
            }
            return true;
#else
            return do_try_lock_until(ts);
#endif
        }
#endif
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
        {
          return try_lock_until(chrono::steady_clock::now() + rel_time);
        }
        template <class Clock, class Duration>
        bool try_lock_until(const chrono::time_point<Clock, Duration>& t)
        {
          typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
          common_duration d(t - Clock::now());
          d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
          while ( ! try_lock_until(detail::internal_chrono_clock::now() + d))
          {
              d = t - Clock::now();
              if ( d <= common_duration::zero() ) return false; // timeout occurred
              d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
          }
          return true;
        }
        template <class Duration>
        bool try_lock_until(const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
        {
          detail::internal_platform_timepoint ts(t);
          return do_try_lock_until(ts);
        }
#endif

#define BOOST_THREAD_DEFINES_TIMED_MUTEX_NATIVE_HANDLE
        typedef pthread_mutex_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &m;
        }

#if defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
        typedef unique_lock<timed_mutex> scoped_timed_lock;
        typedef detail::try_lock_wrapper<timed_mutex> scoped_try_lock;
        typedef scoped_timed_lock scoped_lock;
#endif
    };
}

#include <boost/config/abi_suffix.hpp>


#endif
