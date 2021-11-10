#ifndef BOOST_THREAD_THREAD_COMMON_HPP
#define BOOST_THREAD_THREAD_COMMON_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-2010 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/thread/detail/config.hpp>
#include <boost/predef/platform.h>

#include <boost/thread/exceptions.hpp>
#ifndef BOOST_NO_IOSTREAM
#include <ostream>
#endif
#include <boost/thread/detail/move.hpp>
#include <boost/thread/mutex.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
#include <boost/thread/interruption.hpp>
#endif
#include <boost/thread/detail/thread_heap_alloc.hpp>
#include <boost/thread/detail/make_tuple_indices.hpp>
#include <boost/thread/detail/invoke.hpp>
#include <boost/thread/detail/is_convertible.hpp>
#include <boost/assert.hpp>
#include <list>
#include <algorithm>
#include <boost/core/ref.hpp>
#include <boost/cstdint.hpp>
#include <boost/bind/bind.hpp>
#include <stdlib.h>
#include <memory>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/functional/hash.hpp>
#include <boost/thread/detail/platform_time.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif

#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
#include <tuple>
#endif
#include <boost/config/abi_prefix.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace boost
{

    namespace detail
    {

#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

      template<typename F, class ...ArgTypes>
      class thread_data:
          public detail::thread_data_base
      {
      public:
          BOOST_THREAD_NO_COPYABLE(thread_data)
            thread_data(BOOST_THREAD_RV_REF(F) f_, BOOST_THREAD_RV_REF(ArgTypes)... args_):
              fp(boost::forward<F>(f_), boost::forward<ArgTypes>(args_)...)
            {}
          template <std::size_t ...Indices>
          void run2(tuple_indices<Indices...>)
          {

              detail::invoke(std::move(std::get<0>(fp)), std::move(std::get<Indices>(fp))...);
          }
          void run()
          {
              typedef typename make_tuple_indices<std::tuple_size<std::tuple<F, ArgTypes...> >::value, 1>::type index_type;

              run2(index_type());
          }

      private:
          std::tuple<typename decay<F>::type, typename decay<ArgTypes>::type...> fp;
      };
#else // defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

        template<typename F>
        class thread_data:
            public detail::thread_data_base
        {
        public:
            BOOST_THREAD_NO_COPYABLE(thread_data)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
              thread_data(BOOST_THREAD_RV_REF(F) f_):
                f(boost::forward<F>(f_))
              {}
// This overloading must be removed if we want the packaged_task's tests to pass.
//            thread_data(F& f_):
//                f(f_)
//            {}
#else

            thread_data(BOOST_THREAD_RV_REF(F) f_):
              f(f_)
            {}
            thread_data(F f_):
                f(f_)
            {}
#endif
            //thread_data() {}

            void run()
            {
                f();
            }

        private:
            F f;
        };

        template<typename F>
        class thread_data<boost::reference_wrapper<F> >:
            public detail::thread_data_base
        {
        private:
            F& f;
        public:
            BOOST_THREAD_NO_COPYABLE(thread_data)
            thread_data(boost::reference_wrapper<F> f_):
                f(f_)
            {}
            void run()
            {
                f();
            }
        };

        template<typename F>
        class thread_data<const boost::reference_wrapper<F> >:
            public detail::thread_data_base
        {
        private:
            F& f;
        public:
            BOOST_THREAD_NO_COPYABLE(thread_data)
            thread_data(const boost::reference_wrapper<F> f_):
                f(f_)
            {}
            void run()
            {
                f();
            }
        };
#endif
    }

    class BOOST_THREAD_DECL thread
    {
    public:
      typedef thread_attributes attributes;

      BOOST_THREAD_MOVABLE_ONLY(thread)
    private:

        struct dummy;

        void release_handle();

        detail::thread_data_ptr thread_info;

    private:
        bool start_thread_noexcept();
        bool start_thread_noexcept(const attributes& attr);
        void start_thread()
        {
          if (!start_thread_noexcept())
          {
            boost::throw_exception(thread_resource_error());
          }
        }
        void start_thread(const attributes& attr)
        {
          if (!start_thread_noexcept(attr))
          {
            boost::throw_exception(thread_resource_error());
          }
        }

        explicit thread(detail::thread_data_ptr data);

        detail::thread_data_ptr get_thread_info BOOST_PREVENT_MACRO_SUBSTITUTION () const;

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename F, class ...ArgTypes>
        static inline detail::thread_data_ptr make_thread_info(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_RV_REF(ArgTypes)... args)
        {
            return detail::thread_data_ptr(detail::heap_new<
                  detail::thread_data<typename boost::remove_reference<F>::type, ArgTypes...>
                  >(
                    boost::forward<F>(f), boost::forward<ArgTypes>(args)...
                  )
                );
        }
#else
        template<typename F>
        static inline detail::thread_data_ptr make_thread_info(BOOST_THREAD_RV_REF(F) f)
        {
            return detail::thread_data_ptr(detail::heap_new<detail::thread_data<typename boost::remove_reference<F>::type> >(
                boost::forward<F>(f)));
        }
#endif
        static inline detail::thread_data_ptr make_thread_info(void (*f)())
        {
            return detail::thread_data_ptr(detail::heap_new<detail::thread_data<void(*)()> >(
                boost::forward<void(*)()>(f)));
        }
#else
        template<typename F>
        static inline detail::thread_data_ptr make_thread_info(F f
            , typename disable_if_c<
                //boost::thread_detail::is_convertible<F&,BOOST_THREAD_RV_REF(F)>::value ||
                is_same<typename decay<F>::type, thread>::value,
                dummy* >::type=0
                )
        {
            return detail::thread_data_ptr(detail::heap_new<detail::thread_data<F> >(f));
        }
        template<typename F>
        static inline detail::thread_data_ptr make_thread_info(BOOST_THREAD_RV_REF(F) f)
        {
            return detail::thread_data_ptr(detail::heap_new<detail::thread_data<F> >(f));
        }

#endif
    public:
#if 0 // This should not be needed anymore. Use instead BOOST_THREAD_MAKE_RV_REF.
#if BOOST_WORKAROUND(__SUNPRO_CC, < 0x5100)
        thread(const volatile thread&);
#endif
#endif
        thread() BOOST_NOEXCEPT;
        ~thread()
        {

    #if defined BOOST_THREAD_PROVIDES_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE
          if (joinable()) {
            std::terminate();
          }
    #else
            detach();
    #endif
        }
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        template <
          class F
        >
        explicit thread(BOOST_THREAD_RV_REF(F) f
        //, typename disable_if<is_same<typename decay<F>::type, thread>, dummy* >::type=0
        ):
          thread_info(make_thread_info(thread_detail::decay_copy(boost::forward<F>(f))))
        {
            start_thread();
        }
        template <
          class F
        >
        thread(attributes const& attrs, BOOST_THREAD_RV_REF(F) f):
          thread_info(make_thread_info(thread_detail::decay_copy(boost::forward<F>(f))))
        {
            start_thread(attrs);
        }

#else
#ifdef BOOST_NO_SFINAE
        template <class F>
        explicit thread(F f):
            thread_info(make_thread_info(f))
        {
            start_thread();
        }
        template <class F>
        thread(attributes const& attrs, F f):
            thread_info(make_thread_info(f))
        {
            start_thread(attrs);
        }
#else
        template <class F>
        explicit thread(F f
        , typename disable_if_c<
        boost::thread_detail::is_rv<F>::value // todo as a thread_detail::is_rv
        //boost::thread_detail::is_convertible<F&,BOOST_THREAD_RV_REF(F)>::value
            //|| is_same<typename decay<F>::type, thread>::value
           , dummy* >::type=0
        ):
            thread_info(make_thread_info(f))
        {
            start_thread();
        }
        template <class F>
        thread(attributes const& attrs, F f
            , typename disable_if<boost::thread_detail::is_rv<F>, dummy* >::type=0
            //, typename disable_if<boost::thread_detail::is_convertible<F&,BOOST_THREAD_RV_REF(F) >, dummy* >::type=0
        ):
            thread_info(make_thread_info(f))
        {
            start_thread(attrs);
        }
#endif
        template <class F>
        explicit thread(BOOST_THREAD_RV_REF(F) f
        , typename disable_if<is_same<typename decay<F>::type, thread>, dummy* >::type=0
        ):
#ifdef BOOST_THREAD_USES_MOVE
        thread_info(make_thread_info(boost::move<F>(f))) // todo : Add forward
#else
        thread_info(make_thread_info(f)) // todo : Add forward
#endif
        {
            start_thread();
        }

        template <class F>
        thread(attributes const& attrs, BOOST_THREAD_RV_REF(F) f):
#ifdef BOOST_THREAD_USES_MOVE
            thread_info(make_thread_info(boost::move<F>(f))) // todo : Add forward
#else
            thread_info(make_thread_info(f)) // todo : Add forward
#endif
        {
            start_thread(attrs);
        }
#endif
        thread(BOOST_THREAD_RV_REF(thread) x) BOOST_NOEXCEPT
        {
            thread_info=BOOST_THREAD_RV(x).thread_info;
            BOOST_THREAD_RV(x).thread_info.reset();
        }
#if 0 // This should not be needed anymore. Use instead BOOST_THREAD_MAKE_RV_REF.
#if BOOST_WORKAROUND(__SUNPRO_CC, < 0x5100)
        thread& operator=(thread x)
        {
            swap(x);
            return *this;
        }
#endif
#endif

        thread& operator=(BOOST_THREAD_RV_REF(thread) other) BOOST_NOEXCEPT
        {

#if defined BOOST_THREAD_PROVIDES_THREAD_MOVE_ASSIGN_CALLS_TERMINATE_IF_JOINABLE
            if (joinable()) std::terminate();
#else
            detach();
#endif
            thread_info=BOOST_THREAD_RV(other).thread_info;
            BOOST_THREAD_RV(other).thread_info.reset();
            return *this;
        }

#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template <class F, class Arg, class ...Args>
        thread(F&& f, Arg&& arg, Args&&... args) :
          thread_info(make_thread_info(
              thread_detail::decay_copy(boost::forward<F>(f)),
              thread_detail::decay_copy(boost::forward<Arg>(arg)),
              thread_detail::decay_copy(boost::forward<Args>(args))...)
          )

        {
          start_thread();
        }
        template <class F, class Arg, class ...Args>
        thread(attributes const& attrs, F&& f, Arg&& arg, Args&&... args) :
          thread_info(make_thread_info(
              thread_detail::decay_copy(boost::forward<F>(f)),
              thread_detail::decay_copy(boost::forward<Arg>(arg)),
              thread_detail::decay_copy(boost::forward<Args>(args))...)
          )

        {
          start_thread(attrs);
        }
#else
        template <class F,class A1>
        thread(F f,A1 a1,typename disable_if<boost::thread_detail::is_convertible<F&,thread_attributes >, dummy* >::type=0):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1)))
        {
            start_thread();
        }
        template <class F,class A1,class A2>
        thread(F f,A1 a1,A2 a2):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3>
        thread(F f,A1 a1,A2 a2,A3 a3):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3,class A4>
        thread(F f,A1 a1,A2 a2,A3 a3,A4 a4):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3,a4)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3,class A4,class A5>
        thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3,a4,a5)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3,class A4,class A5,class A6>
        thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3,a4,a5,a6)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7>
        thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3,a4,a5,a6,a7)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8>
        thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3,a4,a5,a6,a7,a8)))
        {
            start_thread();
        }

        template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8,class A9>
        thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9):
            thread_info(make_thread_info(boost::bind(boost::type<void>(),f,a1,a2,a3,a4,a5,a6,a7,a8,a9)))
        {
            start_thread();
        }
#endif
        void swap(thread& x) BOOST_NOEXCEPT
        {
            thread_info.swap(x.thread_info);
        }

        class id;
        id get_id() const BOOST_NOEXCEPT;

        bool joinable() const BOOST_NOEXCEPT;
    private:
        bool join_noexcept();
        bool do_try_join_until_noexcept(detail::internal_platform_timepoint const &timeout, bool& res);
        bool do_try_join_until(detail::internal_platform_timepoint const &timeout);
    public:
        void join();

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Duration>
        bool try_join_until(const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
        {
          return do_try_join_until(boost::detail::internal_platform_timepoint(t));
        }

        template <class Clock, class Duration>
        bool try_join_until(const chrono::time_point<Clock, Duration>& t)
        {
          typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
          common_duration d(t - Clock::now());
          d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
          while ( ! try_join_until(detail::internal_chrono_clock::now() + d) )
          {
            d = t - Clock::now();
            if ( d <= common_duration::zero() ) return false; // timeout occurred
            d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
          }
          return true;
        }

        template <class Rep, class Period>
        bool try_join_for(const chrono::duration<Rep, Period>& rel_time)
        {
          return try_join_until(chrono::steady_clock::now() + rel_time);
        }
#endif
#if defined BOOST_THREAD_USES_DATETIME
        bool timed_join(const system_time& abs_time)
        {
          const detail::real_platform_timepoint ts(abs_time);
#if defined BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
          detail::platform_duration d(ts - detail::real_platform_clock::now());
          d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
          while ( ! do_try_join_until(detail::internal_platform_clock::now() + d) )
          {
            d = ts - detail::real_platform_clock::now();
            if ( d <= detail::platform_duration::zero() ) return false; // timeout occurred
            d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
          }
          return true;
#else
          return do_try_join_until(ts);
#endif
        }

        template<typename TimeDuration>
        bool timed_join(TimeDuration const& rel_time)
        {
          detail::platform_duration d(rel_time);
#if defined(BOOST_THREAD_HAS_MONO_CLOCK) && !defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
          const detail::mono_platform_timepoint ts(detail::mono_platform_clock::now() + d);
          d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
          while ( ! do_try_join_until(detail::internal_platform_clock::now() + d) )
          {
            d = ts - detail::mono_platform_clock::now();
            if ( d <= detail::platform_duration::zero() ) return false; // timeout occurred
            d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
          }
          return true;
#else
          return do_try_join_until(detail::internal_platform_clock::now() + d);
#endif
        }
#endif
        void detach();

        static unsigned hardware_concurrency() BOOST_NOEXCEPT;
        static unsigned physical_concurrency() BOOST_NOEXCEPT;

#define BOOST_THREAD_DEFINES_THREAD_NATIVE_HANDLE
        typedef detail::thread_data_base::native_handle_type native_handle_type;
        native_handle_type native_handle();

#if defined BOOST_THREAD_PROVIDES_THREAD_EQ
        // Use thread::id when comparisions are needed
        // backwards compatibility
        bool operator==(const thread& other) const;
        bool operator!=(const thread& other) const;
#endif
#if defined BOOST_THREAD_USES_DATETIME
        static inline void yield() BOOST_NOEXCEPT
        {
            this_thread::yield();
        }

        static inline void sleep(const system_time& xt)
        {
            this_thread::sleep(xt);
        }
#endif

#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        // extensions
        void interrupt();
        bool interruption_requested() const BOOST_NOEXCEPT;
#endif
    };

    inline void swap(thread& lhs,thread& rhs) BOOST_NOEXCEPT
    {
        return lhs.swap(rhs);
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    inline thread&& move(thread& t) BOOST_NOEXCEPT
    {
        return static_cast<thread&&>(t);
    }
#endif

    BOOST_THREAD_DCL_MOVABLE(thread)

    namespace this_thread
    {
#ifdef BOOST_THREAD_PLATFORM_PTHREAD
        thread::id get_id() BOOST_NOEXCEPT;
#else
        thread::id BOOST_THREAD_DECL get_id() BOOST_NOEXCEPT;
#endif

#if defined BOOST_THREAD_USES_DATETIME
        inline BOOST_SYMBOL_VISIBLE void sleep(::boost::xtime const& abs_time)
        {
            sleep(system_time(abs_time));
        }
#endif
    }

    class BOOST_SYMBOL_VISIBLE thread::id
    {
    private:
    
    #if !defined(BOOST_EMBTC)
      
        friend inline
        std::size_t
        hash_value(const thread::id &v)
        {
#if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
          return hash_value(v.thread_data);
#else
          return hash_value(v.thread_data.get());
#endif
        }

    #else
      
        friend
        std::size_t
        hash_value(const thread::id &v);

    #endif
      
#if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
#if defined(BOOST_THREAD_PLATFORM_WIN32)
        typedef unsigned int data;
#else
        typedef thread::native_handle_type data;
#endif
#else
        typedef detail::thread_data_ptr data;
#endif
        data thread_data;

        id(data thread_data_):
            thread_data(thread_data_)
        {}
        friend class thread;
        friend id BOOST_THREAD_DECL this_thread::get_id() BOOST_NOEXCEPT;
    public:
        id() BOOST_NOEXCEPT:
#if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
        thread_data(0)
#else
        thread_data()
#endif
        {}

        bool operator==(const id& y) const BOOST_NOEXCEPT
        {
            return thread_data==y.thread_data;
        }

        bool operator!=(const id& y) const BOOST_NOEXCEPT
        {
            return thread_data!=y.thread_data;
        }

        bool operator<(const id& y) const BOOST_NOEXCEPT
        {
            return thread_data<y.thread_data;
        }

        bool operator>(const id& y) const BOOST_NOEXCEPT
        {
            return y.thread_data<thread_data;
        }

        bool operator<=(const id& y) const BOOST_NOEXCEPT
        {
            return !(y.thread_data<thread_data);
        }

        bool operator>=(const id& y) const BOOST_NOEXCEPT
        {
            return !(thread_data<y.thread_data);
        }

#ifndef BOOST_NO_IOSTREAM
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
        template<class charT, class traits>
        friend BOOST_SYMBOL_VISIBLE
  std::basic_ostream<charT, traits>&
        operator<<(std::basic_ostream<charT, traits>& os, const id& x)
        {
            if(x.thread_data)
            {
                io::ios_flags_saver  ifs( os );
                return os<< std::hex << x.thread_data;
            }
            else
            {
                return os<<"{Not-any-thread}";
            }
        }
#else
        template<class charT, class traits>
        BOOST_SYMBOL_VISIBLE
  std::basic_ostream<charT, traits>&
        print(std::basic_ostream<charT, traits>& os) const
        {
            if(thread_data)
            {
              io::ios_flags_saver  ifs( os );
              return os<< std::hex << thread_data;
            }
            else
            {
                return os<<"{Not-any-thread}";
            }
        }

#endif
#endif
    };
    
#if defined(BOOST_EMBTC)

        inline
        std::size_t
        hash_value(const thread::id &v)
        {
#if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
          return hash_value(v.thread_data);
#else
          return hash_value(v.thread_data.get());
#endif
        }

#endif

#ifdef BOOST_THREAD_PLATFORM_PTHREAD
    inline thread::id thread::get_id() const BOOST_NOEXCEPT
    {
    #if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
        return const_cast<thread*>(this)->native_handle();
    #else
        detail::thread_data_ptr const local_thread_info=(get_thread_info)();
        return (local_thread_info? id(local_thread_info) : id());
    #endif
    }

    namespace this_thread
    {
        inline thread::id get_id() BOOST_NOEXCEPT
        {
        #if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
             return pthread_self();
        #else
            boost::detail::thread_data_base* const thread_info=get_or_make_current_thread_data();
            return (thread_info?thread::id(thread_info->shared_from_this()):thread::id());
        #endif
        }
    }
#endif
    inline void thread::join() {
        if (this_thread::get_id() == get_id())
          boost::throw_exception(thread_resource_error(static_cast<int>(system::errc::resource_deadlock_would_occur), "boost thread: trying joining itself"));

        BOOST_THREAD_VERIFY_PRECONDITION( join_noexcept(),
            thread_resource_error(static_cast<int>(system::errc::invalid_argument), "boost thread: thread not joinable")
        );
    }

    inline bool thread::do_try_join_until(detail::internal_platform_timepoint const &timeout)
    {
        if (this_thread::get_id() == get_id())
          boost::throw_exception(thread_resource_error(static_cast<int>(system::errc::resource_deadlock_would_occur), "boost thread: trying joining itself"));
        bool res;
        if (do_try_join_until_noexcept(timeout, res))
        {
          return res;
        }
        else
        {
          BOOST_THREAD_THROW_ELSE_RETURN(
            (thread_resource_error(static_cast<int>(system::errc::invalid_argument), "boost thread: thread not joinable")),
            false
          );
        }
    }

#if !defined(BOOST_NO_IOSTREAM) && defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    template<class charT, class traits>
    BOOST_SYMBOL_VISIBLE
    std::basic_ostream<charT, traits>&
    operator<<(std::basic_ostream<charT, traits>& os, const thread::id& x)
    {
        return x.print(os);
    }
#endif

#if defined BOOST_THREAD_PROVIDES_THREAD_EQ
    inline bool thread::operator==(const thread& other) const
    {
        return get_id()==other.get_id();
    }

    inline bool thread::operator!=(const thread& other) const
    {
        return get_id()!=other.get_id();
    }
#endif

    namespace detail
    {
        struct thread_exit_function_base
        {
            virtual ~thread_exit_function_base()
            {}
            virtual void operator()()=0;
        };

        template<typename F>
        struct thread_exit_function:
            thread_exit_function_base
        {
            F f;

            thread_exit_function(F f_):
                f(f_)
            {}

            void operator()()
            {
                f();
            }
        };

        void BOOST_THREAD_DECL add_thread_exit_function(thread_exit_function_base*);
//#ifndef BOOST_NO_EXCEPTIONS
        struct shared_state_base;
#if defined(BOOST_THREAD_PLATFORM_WIN32)
        inline void make_ready_at_thread_exit(shared_ptr<shared_state_base> as)
        {
          detail::thread_data_base* const current_thread_data(detail::get_current_thread_data());
          if(current_thread_data)
          {
            current_thread_data->make_ready_at_thread_exit(as);
          }
        }
#else
        void BOOST_THREAD_DECL make_ready_at_thread_exit(shared_ptr<shared_state_base> as);
#endif
//#endif
    }

    namespace this_thread
    {
        template<typename F>
        void at_thread_exit(F f)
        {
            detail::thread_exit_function_base* const thread_exit_func=detail::heap_new<detail::thread_exit_function<F> >(f);
            detail::add_thread_exit_function(thread_exit_func);
        }
    }
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/config/abi_suffix.hpp>

#endif
