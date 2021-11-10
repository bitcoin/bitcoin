#ifndef BOOST_THREAD_PTHREAD_ONCE_ATOMIC_HPP
#define BOOST_THREAD_PTHREAD_ONCE_ATOMIC_HPP

//  once.hpp
//
//  (C) Copyright 2013 Andrey Semashev
//  (C) Copyright 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/cstdint.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/invoke.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/bind/bind.hpp>
#include <boost/atomic.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  struct once_flag;

  namespace thread_detail
  {

#if BOOST_ATOMIC_INT_LOCK_FREE == 2
    typedef unsigned int atomic_int_type;
#elif BOOST_ATOMIC_SHORT_LOCK_FREE == 2
    typedef unsigned short atomic_int_type;
#elif BOOST_ATOMIC_CHAR_LOCK_FREE == 2
    typedef unsigned char atomic_int_type;
#elif BOOST_ATOMIC_LONG_LOCK_FREE == 2
    typedef unsigned long atomic_int_type;
#elif defined(BOOST_HAS_LONG_LONG) && BOOST_ATOMIC_LLONG_LOCK_FREE == 2
    typedef ulong_long_type atomic_int_type;
#else
    // All tested integer types are not atomic, the spinlock pool will be used
    typedef unsigned int atomic_int_type;
#endif

    typedef boost::atomic<atomic_int_type> atomic_type;

    BOOST_THREAD_DECL bool enter_once_region(once_flag& flag) BOOST_NOEXCEPT;
    BOOST_THREAD_DECL void commit_once_region(once_flag& flag) BOOST_NOEXCEPT;
    BOOST_THREAD_DECL void rollback_once_region(once_flag& flag) BOOST_NOEXCEPT;
    inline atomic_type& get_atomic_storage(once_flag& flag)  BOOST_NOEXCEPT;
  }

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11

  struct once_flag
  {
    BOOST_THREAD_NO_COPYABLE(once_flag)
    BOOST_CONSTEXPR once_flag() BOOST_NOEXCEPT : storage(0)
    {
    }

  private:
    thread_detail::atomic_type storage;

    friend BOOST_THREAD_DECL bool thread_detail::enter_once_region(once_flag& flag) BOOST_NOEXCEPT;
    friend BOOST_THREAD_DECL void thread_detail::commit_once_region(once_flag& flag) BOOST_NOEXCEPT;
    friend BOOST_THREAD_DECL void thread_detail::rollback_once_region(once_flag& flag) BOOST_NOEXCEPT;
    friend thread_detail::atomic_type& thread_detail::get_atomic_storage(once_flag& flag) BOOST_NOEXCEPT;
  };

#define BOOST_ONCE_INIT boost::once_flag()

  namespace thread_detail
  {
    inline atomic_type& get_atomic_storage(once_flag& flag) BOOST_NOEXCEPT
    {
      //return reinterpret_cast< atomic_type& >(flag.storage);
      return flag.storage;
    }
  }

#else // BOOST_THREAD_PROVIDES_ONCE_CXX11
  struct once_flag
  {
    // The thread_detail::atomic_int_type storage is marked
    // with this attribute in order to let the compiler know that it will alias this member
    // and silence compilation warnings.
    BOOST_THREAD_ATTRIBUTE_MAY_ALIAS thread_detail::atomic_int_type storage;
  };

  #define BOOST_ONCE_INIT {0}

  namespace thread_detail
  {
    inline atomic_type& get_atomic_storage(once_flag& flag) BOOST_NOEXCEPT
    {
      return reinterpret_cast< atomic_type& >(flag.storage);
    }

  }

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


#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

  template<typename Function, class ...ArgTypes>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(ArgTypes)... args)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(
                        thread_detail::decay_copy(boost::forward<Function>(f)),
                        thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
        ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }
#else
  template<typename Function>
  inline void call_once(once_flag& flag, Function f)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f();
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1>
  inline void call_once(once_flag& flag, Function f, T1 p1)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(f, p1) BOOST_THREAD_INVOKE_RET_VOID_CALL;
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1, typename T2>
  inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(f, p1, p2) BOOST_THREAD_INVOKE_RET_VOID_CALL;
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1, typename T2, typename T3>
  inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(f, p1, p2, p3) BOOST_THREAD_INVOKE_RET_VOID_CALL;
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }
#if !(defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x5130))
  template<typename Function>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f();
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(
            thread_detail::decay_copy(boost::forward<Function>(f)),
            thread_detail::decay_copy(boost::forward<T1>(p1))
        ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }
  template<typename Function, typename T1, typename T2>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(
            thread_detail::decay_copy(boost::forward<Function>(f)),
            thread_detail::decay_copy(boost::forward<T1>(p1)),
            thread_detail::decay_copy(boost::forward<T1>(p2))
        ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }
  template<typename Function, typename T1, typename T2, typename T3>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2, BOOST_THREAD_RV_REF(T3) p3)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        BOOST_THREAD_INVOKE_RET_VOID(
            thread_detail::decay_copy(boost::forward<Function>(f)),
            thread_detail::decay_copy(boost::forward<T1>(p1)),
            thread_detail::decay_copy(boost::forward<T1>(p2)),
            thread_detail::decay_copy(boost::forward<T1>(p3))
        ) BOOST_THREAD_INVOKE_RET_VOID_CALL;

      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

#endif // __SUNPRO_CC

#endif
}

#include <boost/config/abi_suffix.hpp>

#endif

