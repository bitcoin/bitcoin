#ifndef BOOST_THREAD_WIN32_ONCE_HPP
#define BOOST_THREAD_WIN32_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2005-7 Anthony Williams
//  (C) Copyright 2005 John Maddock
//  (C) Copyright 2011-2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <cstddef>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/invoke.hpp>

#include <boost/bind/bind.hpp>

#include <boost/config/abi_prefix.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
    using ::memcpy;
    using ::ptrdiff_t;
}
#endif

namespace boost
{
  struct once_flag;
  namespace detail
  {
  struct once_context;

  inline bool enter_once_region(once_flag& flag, once_context& ctx) BOOST_NOEXCEPT;
  inline void commit_once_region(once_flag& flag, once_context& ctx) BOOST_NOEXCEPT;
  inline void rollback_once_region(once_flag& flag, once_context& ctx) BOOST_NOEXCEPT;
  }

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11

  struct once_flag
  {
      BOOST_THREAD_NO_COPYABLE(once_flag)
      BOOST_CONSTEXPR once_flag() BOOST_NOEXCEPT
        : status(0), count(0)
      {}
      long status;
      long count;
  private:
      friend inline bool enter_once_region(once_flag& flag, detail::once_context& ctx) BOOST_NOEXCEPT;
      friend inline void commit_once_region(once_flag& flag, detail::once_context& ctx) BOOST_NOEXCEPT;
      friend inline void rollback_once_region(once_flag& flag, detail::once_context& ctx) BOOST_NOEXCEPT;
  };

#define BOOST_ONCE_INIT once_flag()
#else // BOOST_THREAD_PROVIDES_ONCE_CXX11

    struct once_flag
    {
        long status;
        long count;
    };

#define BOOST_ONCE_INIT {0,0}
#endif  // BOOST_THREAD_PROVIDES_ONCE_CXX11

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

    namespace detail
    {
#ifdef BOOST_NO_ANSI_APIS
        typedef wchar_t once_char_type;
#else
        typedef char once_char_type;
#endif
        unsigned const once_mutex_name_fixed_length=54;
        unsigned const once_mutex_name_length=once_mutex_name_fixed_length+
            sizeof(void*)*2+sizeof(unsigned long)*2+1;

        template <class I>
        void int_to_string(I p, once_char_type* buf)
        {
            for(unsigned i=0; i < sizeof(I)*2; ++i,++buf)
            {
#ifdef BOOST_NO_ANSI_APIS
                once_char_type const a=L'A';
#else
                once_char_type const a='A';
#endif
                *buf = a + static_cast<once_char_type>((p >> (i*4)) & 0x0f);
            }
            *buf = 0;
        }

        inline void name_once_mutex(once_char_type* mutex_name,void* flag_address)
        {
#ifdef BOOST_NO_ANSI_APIS
            static const once_char_type fixed_mutex_name[]=L"Local\\{C15730E2-145C-4c5e-B005-3BC753F42475}-once-flag";
#else
            static const once_char_type fixed_mutex_name[]="Local\\{C15730E2-145C-4c5e-B005-3BC753F42475}-once-flag";
#endif
            BOOST_STATIC_ASSERT(sizeof(fixed_mutex_name) ==
                                (sizeof(once_char_type)*(once_mutex_name_fixed_length+1)));

            std::memcpy(mutex_name,fixed_mutex_name,sizeof(fixed_mutex_name));
            detail::int_to_string(reinterpret_cast<std::ptrdiff_t>(flag_address),
                                  mutex_name + once_mutex_name_fixed_length);
            detail::int_to_string(winapi::GetCurrentProcessId(),
                                  mutex_name + once_mutex_name_fixed_length + sizeof(void*)*2);
        }

        inline void* open_once_event(once_char_type* mutex_name,void* flag_address)
        {
            if(!*mutex_name)
            {
                name_once_mutex(mutex_name,flag_address);
            }

#ifdef BOOST_NO_ANSI_APIS
            return ::boost::winapi::OpenEventW(
#else
            return ::boost::winapi::OpenEventA(
#endif
                ::boost::detail::win32::synchronize |
                ::boost::detail::win32::event_modify_state,
                false,
                mutex_name);
        }

        inline void* create_once_event(once_char_type* mutex_name,void* flag_address)
        {
            if(!*mutex_name)
            {
                name_once_mutex(mutex_name,flag_address);
            }
            
            return ::boost::detail::win32::create_event(
                mutex_name, 
                ::boost::detail::win32::manual_reset_event,
                ::boost::detail::win32::event_initially_reset);
        }

        struct once_context {
          long const function_complete_flag_value;
          long const running_value;
          bool counted;
          detail::win32::handle_manager event_handle;
          detail::once_char_type mutex_name[once_mutex_name_length];
          once_context() :
            function_complete_flag_value(0xc15730e2),
            running_value(0x7f0725e3),
            counted(false)
          {
            mutex_name[0]=0;
          }
        };
        enum once_action {try_, break_, continue_};

        inline bool enter_once_region(once_flag& flag, once_context& ctx) BOOST_NOEXCEPT
        {
          long status=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&flag.status,ctx.running_value,0);
          if(!status)
          {
            if(!ctx.event_handle)
            {
                ctx.event_handle=detail::open_once_event(ctx.mutex_name,&flag);
            }
            if(ctx.event_handle)
            {
                ::boost::winapi::ResetEvent(ctx.event_handle);
            }
            return true;
          }
          return false;
        }
        inline void commit_once_region(once_flag& flag, once_context& ctx) BOOST_NOEXCEPT
        {
          if(!ctx.counted)
          {
              BOOST_INTERLOCKED_INCREMENT(&flag.count);
              ctx.counted=true;
          }
          BOOST_INTERLOCKED_EXCHANGE(&flag.status,ctx.function_complete_flag_value);
          if(!ctx.event_handle &&
             (::boost::detail::interlocked_read_acquire(&flag.count)>1))
          {
              ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
          }
          if(ctx.event_handle)
          {
              ::boost::winapi::SetEvent(ctx.event_handle);
          }
        }
        inline void rollback_once_region(once_flag& flag, once_context& ctx) BOOST_NOEXCEPT
        {
          BOOST_INTERLOCKED_EXCHANGE(&flag.status,0);
          if(!ctx.event_handle)
          {
              ctx.event_handle=detail::open_once_event(ctx.mutex_name,&flag);
          }
          if(ctx.event_handle)
          {
              ::boost::winapi::SetEvent(ctx.event_handle);
          }
        }
    }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
//#if defined(BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR)
    inline void call_once(once_flag& flag, void (*f)())
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  f();
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite, 0));
        }
    }
//#endif
    template<typename Function>
    inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                    f();
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, class A, class ...ArgTypes>
    inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(A) a, BOOST_THREAD_RV_REF(ArgTypes)... args)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(
                        thread_detail::decay_copy(boost::forward<Function>(f)),
                        thread_detail::decay_copy(boost::forward<A>(a)),
                        thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
                  ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
#else
#if ! defined(BOOST_MSVC) && ! defined(BOOST_INTEL)
    template<typename Function>
    void call_once(once_flag& flag,Function f)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                    f();
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1>
    void call_once(once_flag& flag,Function f, T1 p1)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(f,p1) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1, typename T2>
    void call_once(once_flag& flag,Function f, T1 p1, T2 p2)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(f,p1,p2) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1, typename T2, typename T3>
    void call_once(once_flag& flag,Function f, T1 p1, T2 p2, T3 p3)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(f,p1,p2,p3) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
#elif defined BOOST_NO_CXX11_RVALUE_REFERENCES

    template<typename Function>
    void call_once(once_flag& flag,Function const&f)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                    f();
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1>
    void call_once(once_flag& flag,Function const&f, T1 const&p1)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(f,p1) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1, typename T2>
    void call_once(once_flag& flag,Function const&f, T1 const&p1, T2 const&p2)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(f,p1,p2) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1, typename T2, typename T3>
    void call_once(once_flag& flag,Function const&f, T1 const&p1, T2 const&p2, T3 const&p3)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(f,p1,p2,p3) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
#endif
#if 1
#if defined(BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR)
        inline void call_once(once_flag& flag, void (*f)())
        {
            // Try for a quick win: if the procedure has already been called
            // just skip through:
            detail::once_context ctx;
            while(::boost::detail::interlocked_read_acquire(&flag.status)
                  !=ctx.function_complete_flag_value)
            {
                if(detail::enter_once_region(flag, ctx))
                {
                    BOOST_TRY
                    {
                      f();
                    }
                    BOOST_CATCH(...)
                    {
                        detail::rollback_once_region(flag, ctx);
                        BOOST_RETHROW
                    }
                    BOOST_CATCH_END
                    detail::commit_once_region(flag, ctx);
                    break;
                }
                if(!ctx.counted)
                {
                    BOOST_INTERLOCKED_INCREMENT(&flag.count);
                    ctx.counted=true;
                    long status=::boost::detail::interlocked_read_acquire(&flag.status);
                    if(status==ctx.function_complete_flag_value)
                    {
                        break;
                    }
                    if(!ctx.event_handle)
                    {
                        ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                        continue;
                    }
                }
                BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                                 ctx.event_handle,::boost::detail::win32::infinite,0));
            }
        }
        template<typename T1>
        void call_once(once_flag& flag,void (*f)(BOOST_THREAD_RV_REF(T1)), BOOST_THREAD_RV_REF(T1) p1)
        {
            // Try for a quick win: if the procedure has already been called
            // just skip through:
            detail::once_context ctx;
            while(::boost::detail::interlocked_read_acquire(&flag.status)
                  !=ctx.function_complete_flag_value)
            {
                if(detail::enter_once_region(flag, ctx))
                {
                    BOOST_TRY
                    {
                       f(
                           thread_detail::decay_copy(boost::forward<T1>(p1))
                       );
                    }
                    BOOST_CATCH(...)
                    {
                        detail::rollback_once_region(flag, ctx);
                        BOOST_RETHROW
                    }
                    BOOST_CATCH_END
                    detail::commit_once_region(flag, ctx);
                    break;
                }
                if(!ctx.counted)
                {
                    BOOST_INTERLOCKED_INCREMENT(&flag.count);
                    ctx.counted=true;
                    long status=::boost::detail::interlocked_read_acquire(&flag.status);
                    if(status==ctx.function_complete_flag_value)
                    {
                        break;
                    }
                    if(!ctx.event_handle)
                    {
                        ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                        continue;
                    }
                }
                BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                                 ctx.event_handle,::boost::detail::win32::infinite,0));
            }
        }
        template<typename Function, typename T1, typename T2>
        void call_once(once_flag& flag,void (*f)(BOOST_THREAD_RV_REF(T1),BOOST_THREAD_RV_REF(T2)), BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2)
        {
            // Try for a quick win: if the procedure has already been called
            // just skip through:
            detail::once_context ctx;
            while(::boost::detail::interlocked_read_acquire(&flag.status)
                  !=ctx.function_complete_flag_value)
            {
                if(detail::enter_once_region(flag, ctx))
                {
                    BOOST_TRY
                    {
                      f(
                          thread_detail::decay_copy(boost::forward<T1>(p1)),
                          thread_detail::decay_copy(boost::forward<T2>(p2))
                      );
                    }
                    BOOST_CATCH(...)
                    {
                        detail::rollback_once_region(flag, ctx);
                        BOOST_RETHROW
                    }
                    BOOST_CATCH_END
                    detail::commit_once_region(flag, ctx);
                    break;
                }
                if(!ctx.counted)
                {
                    BOOST_INTERLOCKED_INCREMENT(&flag.count);
                    ctx.counted=true;
                    long status=::boost::detail::interlocked_read_acquire(&flag.status);
                    if(status==ctx.function_complete_flag_value)
                    {
                        break;
                    }
                    if(!ctx.event_handle)
                    {
                        ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                        continue;
                    }
                }
                BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                                 ctx.event_handle,::boost::detail::win32::infinite,0));
            }
        }
        template<typename Function, typename T1, typename T2, typename T3>
        void call_once(once_flag& flag,void (*f)(BOOST_THREAD_RV_REF(T1),BOOST_THREAD_RV_REF(T2)), BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2, BOOST_THREAD_RV_REF(T3) p3)
        {
            // Try for a quick win: if the procedure has already been called
            // just skip through:
            detail::once_context ctx;
            while(::boost::detail::interlocked_read_acquire(&flag.status)
                  !=ctx.function_complete_flag_value)
            {
                if(detail::enter_once_region(flag, ctx))
                {
                    BOOST_TRY
                    {
                      f(
                          thread_detail::decay_copy(boost::forward<T1>(p1)),
                          thread_detail::decay_copy(boost::forward<T2>(p2)),
                          thread_detail::decay_copy(boost::forward<T3>(p3))
                      );
                    }
                    BOOST_CATCH(...)
                    {
                        detail::rollback_once_region(flag, ctx);
                        BOOST_RETHROW
                    }
                    BOOST_CATCH_END
                    detail::commit_once_region(flag, ctx);
                    break;
                }
                if(!ctx.counted)
                {
                    BOOST_INTERLOCKED_INCREMENT(&flag.count);
                    ctx.counted=true;
                    long status=::boost::detail::interlocked_read_acquire(&flag.status);
                    if(status==ctx.function_complete_flag_value)
                    {
                        break;
                    }
                    if(!ctx.event_handle)
                    {
                        ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                        continue;
                    }
                }
                BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                                 ctx.event_handle,::boost::detail::win32::infinite,0));
            }
        }
#endif
    template<typename Function>
    void call_once(once_flag& flag,BOOST_THREAD_RV_REF(Function) f)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                    f();
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }

    template<typename Function, typename T1>
    void call_once(once_flag& flag,BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(
                      thread_detail::decay_copy(boost::forward<Function>(f)),
                      thread_detail::decay_copy(boost::forward<T1>(p1))
                  ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1, typename T2>
    void call_once(once_flag& flag,BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(
                      thread_detail::decay_copy(boost::forward<Function>(f)),
                      thread_detail::decay_copy(boost::forward<T1>(p1)),
                      thread_detail::decay_copy(boost::forward<T2>(p2))
                  ) BOOST_THREAD_INVOKE_RET_VOID_CALL;
                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }
    template<typename Function, typename T1, typename T2, typename T3>
    void call_once(once_flag& flag,BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(T1) p1, BOOST_THREAD_RV_REF(T2) p2, BOOST_THREAD_RV_REF(T3) p3)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        detail::once_context ctx;
        while(::boost::detail::interlocked_read_acquire(&flag.status)
              !=ctx.function_complete_flag_value)
        {
            if(detail::enter_once_region(flag, ctx))
            {
                BOOST_TRY
                {
                  BOOST_THREAD_INVOKE_RET_VOID(
                      thread_detail::decay_copy(boost::forward<Function>(f)),
                      thread_detail::decay_copy(boost::forward<T1>(p1)),
                      thread_detail::decay_copy(boost::forward<T2>(p2)),
                      thread_detail::decay_copy(boost::forward<T3>(p3))
                  ) BOOST_THREAD_INVOKE_RET_VOID_CALL;

                }
                BOOST_CATCH(...)
                {
                    detail::rollback_once_region(flag, ctx);
                    BOOST_RETHROW
                }
                BOOST_CATCH_END
                detail::commit_once_region(flag, ctx);
                break;
            }
            if(!ctx.counted)
            {
                BOOST_INTERLOCKED_INCREMENT(&flag.count);
                ctx.counted=true;
                long status=::boost::detail::interlocked_read_acquire(&flag.status);
                if(status==ctx.function_complete_flag_value)
                {
                    break;
                }
                if(!ctx.event_handle)
                {
                    ctx.event_handle=detail::create_once_event(ctx.mutex_name,&flag);
                    continue;
                }
            }
            BOOST_VERIFY(!::boost::winapi::WaitForSingleObjectEx(
                             ctx.event_handle,::boost::detail::win32::infinite,0));
        }
    }

#endif
#endif
}

#include <boost/config/abi_suffix.hpp>

#endif
