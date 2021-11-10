#ifndef BOOST_THREAD_PTHREAD_THREAD_DATA_HPP
#define BOOST_THREAD_PTHREAD_THREAD_DATA_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/thread/detail/config.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/pthread/condition_variable_fwd.hpp>
#include <boost/thread/pthread/pthread_helpers.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/assert.hpp>
#include <boost/thread/detail/platform_time.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif

#include <map>
#include <vector>
#include <utility>

#if defined(__ANDROID__)
# ifndef PAGE_SIZE
#  define PAGE_SIZE 4096
# endif
#endif

#include <pthread.h>
#include <unistd.h>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    class thread_attributes {
    public:
        thread_attributes() BOOST_NOEXCEPT {
            int res = pthread_attr_init(&val_);
            BOOST_VERIFY(!res && "pthread_attr_init failed");
        }
        ~thread_attributes() {
          int res = pthread_attr_destroy(&val_);
          BOOST_VERIFY(!res && "pthread_attr_destroy failed");
        }
        // stack
        void set_stack_size(std::size_t size) BOOST_NOEXCEPT {
          if (size==0) return;
#ifdef BOOST_THREAD_USES_GETPAGESIZE
          std::size_t page_size = getpagesize();
#else
          std::size_t page_size = ::sysconf( _SC_PAGESIZE);
#endif
#ifdef PTHREAD_STACK_MIN
          if (size<PTHREAD_STACK_MIN) size=PTHREAD_STACK_MIN;
#endif
          size = ((size+page_size-1)/page_size)*page_size;
          int res = pthread_attr_setstacksize(&val_, size);
          BOOST_VERIFY(!res && "pthread_attr_setstacksize failed");
        }

        std::size_t get_stack_size() const BOOST_NOEXCEPT {
            std::size_t size;
            int res = pthread_attr_getstacksize(&val_, &size);
            BOOST_VERIFY(!res && "pthread_attr_getstacksize failed");
            return size;
        }
#define BOOST_THREAD_DEFINES_THREAD_ATTRIBUTES_NATIVE_HANDLE

        typedef pthread_attr_t native_handle_type;
        native_handle_type* native_handle() BOOST_NOEXCEPT {
          return &val_;
        }
        const native_handle_type* native_handle() const BOOST_NOEXCEPT {
          return &val_;
        }

    private:
        pthread_attr_t val_;
    };

    class thread;

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
        typedef boost::shared_ptr<thread_data_base> thread_data_ptr;

        struct BOOST_THREAD_DECL thread_data_base:
            enable_shared_from_this<thread_data_base>
        {
            thread_data_ptr self;
            pthread_t thread_handle;
            boost::mutex data_mutex;
            boost::condition_variable done_condition;
            bool done;
            bool join_started;
            bool joined;
            boost::detail::thread_exit_callback_node* thread_exit_callbacks;
            std::map<void const*,boost::detail::tss_data_node> tss_data;

//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            // These data must be at the end so that the access to the other fields doesn't change
            // when BOOST_THREAD_PROVIDES_INTERRUPTIONS is defined.
            // Another option is to have them always
            pthread_mutex_t* cond_mutex;
            pthread_cond_t* current_cond;
//#endif
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
            // when BOOST_THREAD_PROVIDES_INTERRUPTIONS is defined.
            // Another option is to have them always
            bool interrupt_enabled;
            bool interrupt_requested;
//#endif
            thread_data_base():
                thread_handle(0),
                done(false),join_started(false),joined(false),
                thread_exit_callbacks(0),
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                cond_mutex(0),
                current_cond(0),
//#endif
                notify()
//#ifndef BOOST_NO_EXCEPTIONS
                , async_states_()
//#endif
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                , interrupt_enabled(true)
                , interrupt_requested(false)
//#endif
            {}
            virtual ~thread_data_base();

            typedef pthread_t native_handle_type;

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

        BOOST_THREAD_DECL thread_data_base* get_current_thread_data();

#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        class interruption_checker
        {
            thread_data_base* const thread_info;
            pthread_mutex_t* m;
            bool set;
            bool done;

            void check_for_interruption()
            {
#ifndef BOOST_NO_EXCEPTIONS
                if(thread_info->interrupt_requested)
                {
                    thread_info->interrupt_requested=false;
                    throw thread_interrupted(); // BOOST_NO_EXCEPTIONS protected
                }
#endif
            }

            void operator=(interruption_checker&);
        public:
            explicit interruption_checker(pthread_mutex_t* cond_mutex,pthread_cond_t* cond):
                thread_info(detail::get_current_thread_data()),m(cond_mutex),
                set(thread_info && thread_info->interrupt_enabled), done(false)
            {
                if(set)
                {
                    lock_guard<mutex> guard(thread_info->data_mutex);
                    check_for_interruption();
                    thread_info->cond_mutex=cond_mutex;
                    thread_info->current_cond=cond;
                    BOOST_VERIFY(!posix::pthread_mutex_lock(m));
                }
                else
                {
                    BOOST_VERIFY(!posix::pthread_mutex_lock(m));
                }
            }
            void unlock_if_locked()
            {
              if ( ! done) {
                if (set)
                {
                    BOOST_VERIFY(!posix::pthread_mutex_unlock(m));
                    lock_guard<mutex> guard(thread_info->data_mutex);
                    thread_info->cond_mutex=NULL;
                    thread_info->current_cond=NULL;
                }
                else
                {
                    BOOST_VERIFY(!posix::pthread_mutex_unlock(m));
                }
                done = true;
              }
            }

            ~interruption_checker() BOOST_NOEXCEPT_IF(false)
            {
                unlock_if_locked();
            }
        };
#endif
    }

    namespace this_thread
    {
        void BOOST_THREAD_DECL yield() BOOST_NOEXCEPT;

        namespace hidden
        {
          inline bool always_false()
          {
            return false;
          }
        }

#if defined BOOST_THREAD_USES_DATETIME
#ifdef __DECXXX
        /// Workaround of DECCXX issue of incorrect template substitution
        template<>
#endif
        inline void sleep(system_time const& abs_time)
        {
          mutex mx;
          unique_lock<mutex> lock(mx);
          condition_variable cond;
          cond.timed_wait(lock, abs_time, hidden::always_false);
        }

        template<typename TimeDuration>
        void sleep(TimeDuration const& rel_time)
        {
          mutex mx;
          unique_lock<mutex> lock(mx);
          condition_variable cond;
          cond.timed_wait(lock, rel_time, hidden::always_false);
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        void sleep_until(const chrono::time_point<Clock, Duration>& t)
        {
          mutex mut;
          unique_lock<mutex> lk(mut);
          condition_variable cv;
          cv.wait_until(lk, t, hidden::always_false);
        }

        template <class Rep, class Period>
        void sleep_for(const chrono::duration<Rep, Period>& d)
        {
          mutex mut;
          unique_lock<mutex> lk(mut);
          condition_variable cv;
          cv.wait_for(lk, d, hidden::always_false);
        }
#endif

        namespace no_interruption_point
        {
#if defined BOOST_THREAD_SLEEP_FOR_IS_STEADY
// Use pthread_delay_np or nanosleep when available
// because they do not provide an interruption point.

          namespace hidden
          {
            void BOOST_THREAD_DECL sleep_for_internal(const detail::platform_duration& ts);
          }

#if defined BOOST_THREAD_USES_DATETIME
#ifdef __DECXXX
          /// Workaround of DECCXX issue of incorrect template substitution
          template<>
#endif
          inline void sleep(system_time const& abs_time)
          {
            const detail::real_platform_timepoint ts(abs_time);
            detail::platform_duration d(ts - detail::real_platform_clock::now());
            while (d > detail::platform_duration::zero())
            {
              d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
              hidden::sleep_for_internal(d);
              d = ts - detail::real_platform_clock::now();
            }
          }

          template<typename TimeDuration>
          void sleep(TimeDuration const& rel_time)
          {
            hidden::sleep_for_internal(detail::platform_duration(rel_time));
          }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
          template <class Rep, class Period>
          void sleep_for(const chrono::duration<Rep, Period>& d)
          {
            hidden::sleep_for_internal(detail::platform_duration(d));
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
              hidden::sleep_for_internal(detail::platform_duration(d));
              d = t - Clock::now();
            }
          }
#endif

#else // BOOST_THREAD_SLEEP_FOR_IS_STEADY
// When pthread_delay_np and nanosleep are not available,
// fall back to using the interruptible sleep functions.

#if defined BOOST_THREAD_USES_DATETIME
#ifdef __DECXXX
          /// Workaround of DECCXX issue of incorrect template substitution
          template<>
#endif
          inline void sleep(system_time const& abs_time)
          {
            this_thread::sleep(abs_time);
          }

          template<typename TimeDuration>
          void sleep(TimeDuration const& rel_time)
          {
            this_thread::sleep(rel_time);
          }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
          template <class Clock, class Duration>
          void sleep_until(const chrono::time_point<Clock, Duration>& t)
          {
            this_thread::sleep_until(t);
          }

          template <class Rep, class Period>
          void sleep_for(const chrono::duration<Rep, Period>& d)
          {
            this_thread::sleep_for(d);
          }
#endif

#endif // BOOST_THREAD_SLEEP_FOR_IS_STEADY
        } // no_interruption_point
    } // this_thread
}

#include <boost/config/abi_suffix.hpp>

#endif
