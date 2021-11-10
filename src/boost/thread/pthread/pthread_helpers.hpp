#ifndef BOOST_THREAD_PTHREAD_PTHREAD_HELPERS_HPP
#define BOOST_THREAD_PTHREAD_PTHREAD_HELPERS_HPP
// Copyright (C) 2017
// Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/throw_exception.hpp>
#include <pthread.h>
#include <errno.h>

#include <boost/config/abi_prefix.hpp>

#ifndef BOOST_THREAD_HAS_NO_EINTR_BUG
#define BOOST_THREAD_HAS_EINTR_BUG
#endif

namespace boost
{
  namespace posix
  {
    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* attr = NULL)
    {
      return ::pthread_mutex_init(m, attr);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_init(pthread_cond_t* c)
    {
#ifdef BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
      pthread_condattr_t attr;
      int res = pthread_condattr_init(&attr);
      if (res)
      {
        return res;
      }
      BOOST_VERIFY(!pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
      res = ::pthread_cond_init(c, &attr);
      BOOST_VERIFY(!pthread_condattr_destroy(&attr));
      return res;
#else
      return ::pthread_cond_init(c, NULL);
#endif
    }

#ifdef BOOST_THREAD_HAS_EINTR_BUG
    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_destroy(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_destroy(m);
      } while (ret == EINTR);
      return ret;
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_destroy(pthread_cond_t* c)
    {
      int ret;
      do
      {
          ret = ::pthread_cond_destroy(c);
      } while (ret == EINTR);
      return ret;
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_lock(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_lock(m);
      } while (ret == EINTR);
      return ret;
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_trylock(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_trylock(m);
      } while (ret == EINTR);
      return ret;
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_unlock(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_unlock(m);
      } while (ret == EINTR);
      return ret;
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_cond_wait(c, m);
      } while (ret == EINTR);
      return ret;
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_timedwait(pthread_cond_t* c, pthread_mutex_t* m, const struct timespec* t)
    {
      int ret;
      do
      {
          ret = ::pthread_cond_timedwait(c, m, t);
      } while (ret == EINTR);
      return ret;
    }
#else
    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_destroy(pthread_mutex_t* m)
    {
      return ::pthread_mutex_destroy(m);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_destroy(pthread_cond_t* c)
    {
      return ::pthread_cond_destroy(c);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_lock(pthread_mutex_t* m)
    {
      return ::pthread_mutex_lock(m);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_trylock(pthread_mutex_t* m)
    {
      return ::pthread_mutex_trylock(m);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_mutex_unlock(pthread_mutex_t* m)
    {
      return ::pthread_mutex_unlock(m);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m)
    {
      return ::pthread_cond_wait(c, m);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_timedwait(pthread_cond_t* c, pthread_mutex_t* m, const struct timespec* t)
    {
      return ::pthread_cond_timedwait(c, m, t);
    }
#endif

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_signal(pthread_cond_t* c)
    {
      return ::pthread_cond_signal(c);
    }

    BOOST_FORCEINLINE BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
    int pthread_cond_broadcast(pthread_cond_t* c)
    {
      return ::pthread_cond_broadcast(c);
    }
  }
}

#include <boost/config/abi_suffix.hpp>

#endif
