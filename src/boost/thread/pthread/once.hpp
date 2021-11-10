#ifndef BOOST_THREAD_PTHREAD_ONCE_HPP
#define BOOST_THREAD_PTHREAD_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2007-8 Anthony Williams
//  (C) Copyright 2011-2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/invoke.hpp>

#include <boost/thread/pthread/pthread_helpers.hpp>
#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/core/no_exceptions_support.hpp>

#include <boost/bind/bind.hpp>
#include <boost/assert.hpp>
#include <boost/config/abi_prefix.hpp>

#include <boost/cstdint.hpp>
#include <pthread.h>
#include <csignal>

namespace boost
{

  struct once_flag;

  #define BOOST_ONCE_INITIAL_FLAG_VALUE 0

  namespace thread_detail
  {
    typedef boost::uint32_t  uintmax_atomic_t;
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_C2(value) value##u
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_MAX_C BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_C2(~0)

  }

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template<typename Function, class ...ArgTypes>
    inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(ArgTypes)... args);
#else
    template<typename Function>
    inline void call_once(once_flag& flag, Function f);
    template<typename Function, typename T1>
    inline void call_once(once_flag& flag, Function f, T1 p1);
    template<typename Function, typename T1, typename T2>
    inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2);
    template<typename Function, typename T1, typename T2, typename T3>
    inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3);
#endif

  struct once_flag
  {
      BOOST_THREAD_NO_COPYABLE(once_flag)
      BOOST_CONSTEXPR once_flag() BOOST_NOEXCEPT
        : epoch(BOOST_ONCE_INITIAL_FLAG_VALUE)
      {}
  private:
      volatile thread_detail::uintmax_atomic_t epoch;

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template<typename Function, class ...ArgTypes>
      friend void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(ArgTypes)... args);
#else
      template<typename Function>
      friend void call_once(once_flag& flag, Function f);
      template<typename Function, typename T1>
      friend void call_once(once_flag& flag, Function f, T1 p1);
      template<typename Function, typename T1, typename T2>
      friend void call_once(once_flag& flag, Function f, T1 p1, T2 p2);
      template<typename Function, typename T1, typename T2, typename T3>
      friend void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3);

#endif

  };

#define BOOST_ONCE_INIT once_flag()

#else // BOOST_THREAD_PROVIDES_ONCE_CXX11

    struct once_flag
    {
      volatile thread_detail::uintmax_atomic_t epoch;
    };

#define BOOST_ONCE_INIT {BOOST_ONCE_INITIAL_FLAG_VALUE}
#endif // BOOST_THREAD_PROVIDES_ONCE_CXX11


#if defined BOOST_THREAD_PROVIDES_INVOKE
#define BOOST_THREAD_INVOKE_RET_VOID detail::invoke
#define BOOST_THREAD_INVOKE_RET_VOID_CALL
#elif defined BOOST_THREAD_PROVIDES_INVOKE_RET
#define BOOST_THREAD_INVOKE_RET_VOID detail::invoke<void>
#define BOOST_THREAD_INVOKE_RET_VOID_CALL
#else
#define BOOST_THREAD_INVOKE_RET_VOID boost::bind
#define BOOST_THREAD_INVOKE_RET_VOID_CALL ()
#endif

    namespace thread_detail
    {
        BOOST_THREAD_DECL uintmax_atomic_t& get_once_per_thread_epoch();
        BOOST_THREAD_DECL extern uintmax_atomic_t once_global_epoch;
        BOOST_THREAD_DECL extern pthread_mutex_t once_epoch_mutex;
        BOOST_THREAD_DECL extern pthread_cond_t once_epoch_cv;
    }

    // Based on Mike Burrows fast_pthread_once algorithm as described in
    // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2444.html


#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)


  template<typename Function, class ...ArgTypes>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(ArgTypes)... args)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(
                        thread_detail::decay_copy(boost::forward<Function>(f)),
                        thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
                    ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;

    }
  }
#else
  template<typename Function>
  inline void call_once(once_flag& flag, Function f)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    f();
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }

  template<typename Function, typename T1>
  inline void call_once(once_flag& flag, Function f, T1 p1)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(f,p1) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }
  template<typename Function, typename T1, typename T2>
  inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(f,p1, p2) BOOST_THREAD_INVOKE_RET_VOID_CALL;
        }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }

  template<typename Function, typename T1, typename T2, typename T3>
  inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(f,p1, p2, p3) BOOST_THREAD_INVOKE_RET_VOID_CALL;
        }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }

  template<typename Function>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    f();
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }

  template<typename Function, typename T1>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(
                        thread_detail::decay_copy(boost::forward<Function>(f)),
                        thread_detail::decay_copy(boost::forward<T1>(p1))
                    ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }
  template<typename Function, typename T1, typename T2>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(
                        thread_detail::decay_copy(boost::forward<Function>(f)),
                        thread_detail::decay_copy(boost::forward<T1>(p1)),
                        thread_detail::decay_copy(boost::forward<T1>(p2))
                    ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }

  template<typename Function, typename T1, typename T2, typename T3>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2, BOOST_THREAD_RV_REF(T3) p3)
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    thread_detail::uintmax_atomic_t const epoch=flag.epoch;
    thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();

    if(epoch<this_thread_epoch)
    {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

        while(flag.epoch<=being_initialized)
        {
            if(flag.epoch==uninitialized_flag)
            {
                flag.epoch=being_initialized;
                BOOST_TRY
                {
                    pthread::pthread_mutex_scoped_unlock relocker(&thread_detail::once_epoch_mutex);
                    BOOST_THREAD_INVOKE_RET_VOID(
                        thread_detail::decay_copy(boost::forward<Function>(f)),
                        thread_detail::decay_copy(boost::forward<T1>(p1)),
                        thread_detail::decay_copy(boost::forward<T1>(p2)),
                        thread_detail::decay_copy(boost::forward<T1>(p3))
                    ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH (...)
                {
                    flag.epoch=uninitialized_flag;
                    BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                flag.epoch=--thread_detail::once_global_epoch;
                BOOST_VERIFY(!posix::pthread_cond_broadcast(&thread_detail::once_epoch_cv));
            }
            else
            {
                while(flag.epoch==being_initialized)
                {
                    BOOST_VERIFY(!posix::pthread_cond_wait(&thread_detail::once_epoch_cv,&thread_detail::once_epoch_mutex));
                }
            }
        }
        this_thread_epoch=thread_detail::once_global_epoch;
    }
  }

#endif

}

#include <boost/config/abi_suffix.hpp>

#endif
