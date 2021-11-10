#ifndef BOOST_THREAD_PTHREAD_THREAD_DATA_HPP
#define BOOST_THREAD_PTHREAD_THREAD_DATA_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2008 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/thread/detail/config.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/thread_heap_alloc.hpp>
#include <boost/thread/detail/platform_time.hpp>

#include <boost/predef/platform.h>

#include <boost/intrusive_ptr.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif

#include <map>
#include <vector>
#include <utility>

#include <boost/config/abi_prefix.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace boost
{
  class condition_variable;
  class mutex;

  class thread_attributes {
  public:
      thread_attributes() BOOST_NOEXCEPT {
        val_.stack_size = 0;
        //val_.lpThreadAttributes=0;
      }
      ~thread_attributes() {
      }
      // stack size
      void set_stack_size(std::size_t size) BOOST_NOEXCEPT {
        val_.stack_size = size;
      }

      std::size_t get_stack_size() const BOOST_NOEXCEPT {
          return val_.stack_size;
      }

      //void set_security(LPSECURITY_ATTRIBUTES lpThreadAttributes)
      //{
      //  val_.lpThreadAttributes=lpThreadAttributes;
      //}
      //LPSECURITY_ATTRIBUTES get_security()
      //{
      //  return val_.lpThreadAttributes;
      //}

      struct win_attrs {
        std::size_t stack_size;
        //LPSECURITY_ATTRIBUTES lpThreadAttributes;
      };
      typedef win_attrs native_handle_type;
      native_handle_type* native_handle() {return &val_;}
      const native_handle_type* native_handle() const {return &val_;}

  private:
      win_attrs val_;
  };

    namespace detail
    {
        struct shared_state_base;
        struct tss_cleanup_function;
        struct thread_exit_callback_node;
        struct tss_data_node
        {
            typedef void(*cleanup_func_t)(void*);
            typedef void(*cleanup_caller_t)(cleanup_func_t, void*);

            cleanup_caller_t caller;
            cleanup_func_t func;
            void* value;

            tss_data_node(cleanup_caller_t caller_,cleanup_func_t func_,void* value_):
                caller(caller_),func(func_),value(value_)
            {}
        };

        struct thread_data_base;
        void intrusive_ptr_add_ref(thread_data_base * p);
        void intrusive_ptr_release(thread_data_base * p);

        struct BOOST_THREAD_DECL thread_data_base
        {
            long count;

            // Win32 threading APIs are not available in store apps so
            // use abstraction on top of Windows::System::Threading.
#if BOOST_PLAT_WINDOWS_RUNTIME
            detail::win32::scoped_winrt_thread thread_handle;
#else
            detail::win32::handle_manager thread_handle;
#endif

            boost::detail::thread_exit_callback_node* thread_exit_callbacks;
            unsigned id;
            std::map<void const*,boost::detail::tss_data_node> tss_data;
            typedef std::vector<std::pair<condition_variable*, mutex*>
            //, hidden_allocator<std::pair<condition_variable*, mutex*> >
            > notify_list_t;
            notify_list_t notify;

//#ifndef BOOST_NO_EXCEPTIONS
            typedef std::vector<shared_ptr<shared_state_base> > async_states_t;
            async_states_t async_states_;
//#endif
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            // These data must be at the end so that the access to the other fields doesn't change
            // when BOOST_THREAD_PROVIDES_INTERRUPTIONS is defined
            // Another option is to have them always
            detail::win32::handle_manager interruption_handle;
            bool interruption_enabled;
//#endif

            thread_data_base():
                count(0),
                thread_handle(),
                thread_exit_callbacks(0),
                id(0),
                tss_data(),
                notify()
//#ifndef BOOST_NO_EXCEPTIONS
                , async_states_()
//#endif
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                , interruption_handle(create_anonymous_event(detail::win32::manual_reset_event,detail::win32::event_initially_reset))
                , interruption_enabled(true)
//#endif
            {}
            virtual ~thread_data_base();

            #if !defined(BOOST_EMBTC)
              
            friend void intrusive_ptr_add_ref(thread_data_base * p)
            {
                BOOST_INTERLOCKED_INCREMENT(&p->count);
            }

            friend void intrusive_ptr_release(thread_data_base * p)
            {
                if(!BOOST_INTERLOCKED_DECREMENT(&p->count))
                {
                    detail::heap_delete(p);
                }
            }

            #else
              
            friend void intrusive_ptr_add_ref(thread_data_base * p);
            friend void intrusive_ptr_release(thread_data_base * p);

            #endif
      
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            void interrupt()
            {
                BOOST_VERIFY(winapi::SetEvent(interruption_handle)!=0);
            }
#endif
            typedef detail::win32::handle native_handle_type;

            virtual void run()=0;

            virtual void notify_all_at_thread_exit(condition_variable* cv, mutex* m)
            {
              notify.push_back(std::pair<condition_variable*, mutex*>(cv, m));
            }

//#ifndef BOOST_NO_EXCEPTIONS
            void make_ready_at_thread_exit(shared_ptr<shared_state_base> as)
            {
              async_states_.push_back(as);
            }
//#endif
        };
        
#if defined(BOOST_EMBTC)

        inline void intrusive_ptr_add_ref(thread_data_base * p)
        {
            BOOST_INTERLOCKED_INCREMENT(&p->count);
        }

        inline void intrusive_ptr_release(thread_data_base * p)
        {
            if(!BOOST_INTERLOCKED_DECREMENT(&p->count))
            {
                detail::heap_delete(p);
            }
        }

#endif
        
        BOOST_THREAD_DECL thread_data_base* get_current_thread_data();

        typedef boost::intrusive_ptr<detail::thread_data_base> thread_data_ptr;
    }

    namespace this_thread
    {
        void BOOST_THREAD_DECL yield() BOOST_NOEXCEPT;

        bool BOOST_THREAD_DECL interruptible_wait(detail::win32::handle handle_to_wait_for, detail::internal_platform_timepoint const &timeout);

#if defined BOOST_THREAD_USES_DATETIME
        template<typename TimeDuration>
        BOOST_SYMBOL_VISIBLE void sleep(TimeDuration const& rel_time)
        {
          interruptible_wait(detail::win32::invalid_handle_value, detail::internal_platform_clock::now() + detail::platform_duration(rel_time));
        }

        inline BOOST_SYMBOL_VISIBLE void sleep(system_time const& abs_time)
        {
          const detail::real_platform_timepoint ts(abs_time);
          detail::platform_duration d(ts - detail::real_platform_clock::now());
          while (d > detail::platform_duration::zero())
          {
            d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
            interruptible_wait(detail::win32::invalid_handle_value, detail::internal_platform_clock::now() + d);
            d = ts - detail::real_platform_clock::now();
          }
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        void sleep_for(const chrono::duration<Rep, Period>& d)
        {
          interruptible_wait(detail::win32::invalid_handle_value, detail::internal_platform_clock::now() + detail::platform_duration(d));
        }

        template <class Duration>
        void sleep_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
        {
          sleep_for(t - chrono::steady_clock::now());
        }

        template <class Clock, class Duration>
        void sleep_until(const chrono::time_point<Clock, Duration>& t)
        {
          typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
          common_duration d(t - Clock::now());
          while (d > common_duration::zero())
          {
            d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
            sleep_for(d);
            d = t - Clock::now();
          }
        }
#endif

        namespace no_interruption_point
        {
          bool BOOST_THREAD_DECL non_interruptible_wait(detail::win32::handle handle_to_wait_for, detail::internal_platform_timepoint const &timeout);

#if defined BOOST_THREAD_USES_DATETIME
          template<typename TimeDuration>
          BOOST_SYMBOL_VISIBLE void sleep(TimeDuration const& rel_time)
          {
            non_interruptible_wait(detail::win32::invalid_handle_value, detail::internal_platform_clock::now() + detail::platform_duration(rel_time));
          }

          inline BOOST_SYMBOL_VISIBLE void sleep(system_time const& abs_time)
          {
            const detail::real_platform_timepoint ts(abs_time);
            detail::platform_duration d(ts - detail::real_platform_clock::now());
            while (d > detail::platform_duration::zero())
            {
              d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
              non_interruptible_wait(detail::win32::invalid_handle_value, detail::internal_platform_clock::now() + d);
              d = ts - detail::real_platform_clock::now();
            }
          }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
          template <class Rep, class Period>
          void sleep_for(const chrono::duration<Rep, Period>& d)
          {
            non_interruptible_wait(detail::win32::invalid_handle_value, detail::internal_platform_clock::now() + detail::platform_duration(d));
          }

          template <class Duration>
          void sleep_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
          {
            sleep_for(t - chrono::steady_clock::now());
          }

          template <class Clock, class Duration>
          void sleep_until(const chrono::time_point<Clock, Duration>& t)
          {
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            common_duration d(t - Clock::now());
            while (d > common_duration::zero())
            {
              d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
              sleep_for(d);
              d = t - Clock::now();
            }
          }
#endif
        }
    }

}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/config/abi_suffix.hpp>

#endif
