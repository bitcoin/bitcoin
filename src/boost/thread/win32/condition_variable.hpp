#ifndef BOOST_THREAD_CONDITION_VARIABLE_WIN32_HPP
#define BOOST_THREAD_CONDITION_VARIABLE_WIN32_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/thread_data.hpp>
#include <boost/thread/win32/thread_data.hpp>
#include <boost/thread/win32/interlocked_read.hpp>
#include <boost/thread/cv_status.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/detail/platform_time.hpp>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>

#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif

#include <limits.h>
#include <algorithm>
#include <vector>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace detail
    {
        class basic_cv_list_entry;
        void intrusive_ptr_add_ref(basic_cv_list_entry * p);
        void intrusive_ptr_release(basic_cv_list_entry * p);

        class basic_cv_list_entry
        {
        private:
            detail::win32::handle_manager semaphore;
            detail::win32::handle_manager wake_sem;
            long waiters;
            bool notified;
            long references;

        public:
            BOOST_THREAD_NO_COPYABLE(basic_cv_list_entry)
            explicit basic_cv_list_entry(detail::win32::handle_manager const& wake_sem_):
                semaphore(detail::win32::create_anonymous_semaphore(0,LONG_MAX)),
                wake_sem(wake_sem_.duplicate()),
                waiters(1),notified(false),references(0)
            {}

            static bool no_waiters(boost::intrusive_ptr<basic_cv_list_entry> const& entry)
            {
                return !detail::interlocked_read_acquire(&entry->waiters);
            }

            void add_waiter()
            {
                BOOST_INTERLOCKED_INCREMENT(&waiters);
            }

            void remove_waiter()
            {
                BOOST_INTERLOCKED_DECREMENT(&waiters);
            }

            void release(unsigned count_to_release)
            {
                notified=true;
                winapi::ReleaseSemaphore(semaphore,count_to_release,0);
            }

            void release_waiters()
            {
                release(detail::interlocked_read_acquire(&waiters));
            }

            bool is_notified() const
            {
                return notified;
            }

            bool interruptible_wait(detail::internal_platform_timepoint const &timeout)
            {
                return this_thread::interruptible_wait(semaphore, timeout);
            }

            bool woken()
            {
                unsigned long const woken_result=winapi::WaitForSingleObjectEx(wake_sem,0,0);
                BOOST_ASSERT((woken_result==detail::win32::timeout) || (woken_result==0));
                return woken_result==0;
            }

            friend void intrusive_ptr_add_ref(basic_cv_list_entry * p);
            friend void intrusive_ptr_release(basic_cv_list_entry * p);
        };

        inline void intrusive_ptr_add_ref(basic_cv_list_entry * p)
        {
            BOOST_INTERLOCKED_INCREMENT(&p->references);
        }

        inline void intrusive_ptr_release(basic_cv_list_entry * p)
        {
            if(!BOOST_INTERLOCKED_DECREMENT(&p->references))
            {
                delete p;
            }
        }

        class basic_condition_variable
        {
            boost::mutex internal_mutex;
            long total_count;
            unsigned active_generation_count;

            typedef basic_cv_list_entry list_entry;

            typedef boost::intrusive_ptr<list_entry> entry_ptr;
            typedef std::vector<entry_ptr> generation_list;

            generation_list generations;
            detail::win32::handle_manager wake_sem;

            void wake_waiters(long count_to_wake)
            {
                detail::interlocked_write_release(&total_count,total_count-count_to_wake);
                winapi::ReleaseSemaphore(wake_sem,count_to_wake,0);
            }

            template<typename lock_type>
            struct relocker
            {
                BOOST_THREAD_NO_COPYABLE(relocker)
                lock_type& _lock;
                bool _unlocked;

                relocker(lock_type& lock_):
                    _lock(lock_), _unlocked(false)
                {}
                void unlock()
                {
                  if ( ! _unlocked )
                  {
                    _lock.unlock();
                    _unlocked=true;
                  }
                }
                void lock()
                {
                  if ( _unlocked )
                  {
                    _lock.lock();
                    _unlocked=false;
                  }
                }
                ~relocker() BOOST_NOEXCEPT_IF(false)
                {
                  lock();
                }
            };


            entry_ptr get_wait_entry()
            {
                boost::lock_guard<boost::mutex> lk(internal_mutex);
                if(!wake_sem)
                {
                    wake_sem=detail::win32::create_anonymous_semaphore(0,LONG_MAX);
                    BOOST_ASSERT(wake_sem);
                }

                detail::interlocked_write_release(&total_count,total_count+1);
                if(generations.empty() || generations.back()->is_notified())
                {
                    entry_ptr new_entry(new list_entry(wake_sem));
                    generations.push_back(new_entry);
                    return new_entry;
                }
                else
                {
                    generations.back()->add_waiter();
                    return generations.back();
                }
            }

            struct entry_manager
            {
                entry_ptr entry;
                boost::mutex& internal_mutex;


                BOOST_THREAD_NO_COPYABLE(entry_manager)
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
                entry_manager(entry_ptr&& entry_, boost::mutex& mutex_):
                    entry(static_cast< entry_ptr&& >(entry_)), internal_mutex(mutex_)
                {}
#else
                entry_manager(entry_ptr const& entry_, boost::mutex& mutex_):
                    entry(entry_), internal_mutex(mutex_)
                {}
#endif

                void remove_waiter_and_reset()
                {
                  if (entry) {
                    boost::lock_guard<boost::mutex> internal_lock(internal_mutex);
                    entry->remove_waiter();
                    entry.reset();
                  }
                }
                ~entry_manager() BOOST_NOEXCEPT_IF(false)
                {
                  remove_waiter_and_reset();
                }

                list_entry* operator->()
                {
                    return entry.get();
                }
            };

        protected:
            basic_condition_variable(const basic_condition_variable& other);
            basic_condition_variable& operator=(const basic_condition_variable& other);

        public:
            basic_condition_variable():
                total_count(0),active_generation_count(0),wake_sem(0)
            {}

            ~basic_condition_variable()
            {}

            // When this function returns true:
            // * A notification (or sometimes a spurious OS signal) has been received
            // * Do not assume that the timeout has not been reached
            // * Do not assume that the predicate has been changed
            //
            // When this function returns false:
            // * The timeout has been reached
            // * Do not assume that a notification has not been received
            // * Do not assume that the predicate has not been changed
            template<typename lock_type>
            bool do_wait_until(lock_type& lock, detail::internal_platform_timepoint const &timeout)
            {
              relocker<lock_type> locker(lock);
              entry_manager entry(get_wait_entry(), internal_mutex);
              locker.unlock();

              bool woken=false;
              while(!woken)
              {
                  if(!entry->interruptible_wait(timeout))
                  {
                      return false;
                  }

                  woken=entry->woken();
              }
              // do it here to avoid throwing on the destructor
              entry.remove_waiter_and_reset();
              locker.lock();
              return true;
            }

            void notify_one() BOOST_NOEXCEPT
            {
                if(detail::interlocked_read_acquire(&total_count))
                {
                    boost::lock_guard<boost::mutex> internal_lock(internal_mutex);
                    if(!total_count)
                    {
                        return;
                    }
                    wake_waiters(1);

                    for(generation_list::iterator it=generations.begin(),
                            end=generations.end();
                        it!=end;++it)
                    {
                        (*it)->release(1);
                    }
                    generations.erase(std::remove_if(generations.begin(),generations.end(),&basic_cv_list_entry::no_waiters),generations.end());
                }
            }

            void notify_all() BOOST_NOEXCEPT
            {
                if(detail::interlocked_read_acquire(&total_count))
                {
                    boost::lock_guard<boost::mutex> internal_lock(internal_mutex);
                    if(!total_count)
                    {
                        return;
                    }
                    wake_waiters(total_count);
                    for(generation_list::iterator it=generations.begin(),
                            end=generations.end();
                        it!=end;++it)
                    {
                        (*it)->release_waiters();
                    }
                    generations.clear();
                    wake_sem=detail::win32::handle(0);
                }
            }

        };
    }

    class condition_variable:
        private detail::basic_condition_variable
    {
    public:
        BOOST_THREAD_NO_COPYABLE(condition_variable)
        condition_variable()
        {}

        using detail::basic_condition_variable::do_wait_until;
        using detail::basic_condition_variable::notify_one;
        using detail::basic_condition_variable::notify_all;

        void wait(unique_lock<mutex>& m)
        {
            do_wait_until(m, detail::internal_platform_timepoint::getMax());
        }

        template<typename predicate_type>
        void wait(unique_lock<mutex>& m,predicate_type pred)
        {
            while (!pred())
            {
                wait(m);
            }
        }

#if defined BOOST_THREAD_USES_DATETIME
        bool timed_wait(unique_lock<mutex>& m,boost::system_time const& abs_time)
        {
            // The system time may jump while this function is waiting. To compensate for this and time
            // out near the correct time, we could call do_wait_until() in a loop with a short timeout
            // and recheck the time remaining each time through the loop. However, because we can't
            // check the predicate each time do_wait_until() completes, this introduces the possibility
            // of not exiting the function when a notification occurs, since do_wait_until() may report
            // that it timed out even though a notification was received. The best this function can do
            // is report correctly whether or not it reached the timeout time.
            const detail::real_platform_timepoint ts(abs_time);
            const detail::platform_duration d(ts - detail::real_platform_clock::now());
            do_wait_until(m, detail::internal_platform_clock::now() + d);
            return ts > detail::real_platform_clock::now();
        }
        bool timed_wait(unique_lock<mutex>& m,boost::xtime const& abs_time)
        {
            return timed_wait(m, system_time(abs_time));
        }
        template<typename duration_type>
        bool timed_wait(unique_lock<mutex>& m,duration_type const& wait_duration)
        {
            if (wait_duration.is_pos_infinity())
            {
                wait(m);
                return true;
            }
            if (wait_duration.is_special())
            {
                return true;
            }
            const detail::platform_duration d(wait_duration);
            return do_wait_until(m, detail::internal_platform_clock::now() + d);
        }

        template<typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,boost::system_time const& abs_time,predicate_type pred)
        {
            // The system time may jump while this function is waiting. To compensate for this
            // and time out near the correct time, we call do_wait_until() in a loop with a
            // short timeout and recheck the time remaining each time through the loop.
            const detail::real_platform_timepoint ts(abs_time);
            while (!pred())
            {
                detail::platform_duration d(ts - detail::real_platform_clock::now());
                if (d <= detail::platform_duration::zero()) break; // timeout occurred
                d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
                do_wait_until(m, detail::internal_platform_clock::now() + d);
            }
            return pred();
        }
        template<typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,boost::xtime const& abs_time,predicate_type pred)
        {
            return timed_wait(m, system_time(abs_time), pred);
        }
        template<typename duration_type,typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,duration_type const& wait_duration,predicate_type pred)
        {
            if (wait_duration.is_pos_infinity())
            {
                while (!pred())
                {
                    wait(m);
                }
                return true;
            }
            if (wait_duration.is_special())
            {
                return pred();
            }
            const detail::platform_duration d(wait_duration);
            const detail::internal_platform_timepoint ts(detail::internal_platform_clock::now() + d);
            while (!pred())
            {
                if (!do_wait_until(m, ts)) break; // timeout occurred
            }
            return pred();
        }
#endif
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
        {
            const detail::internal_platform_timepoint ts(t);
            if (do_wait_until(lock, ts)) return cv_status::no_timeout;
            else return cv_status::timeout;
        }

        template <class Clock, class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
            // The system time may jump while this function is waiting. To compensate for this and time
            // out near the correct time, we could call do_wait_until() in a loop with a short timeout
            // and recheck the time remaining each time through the loop. However, because we can't
            // check the predicate each time do_wait_until() completes, this introduces the possibility
            // of not exiting the function when a notification occurs, since do_wait_until() may report
            // that it timed out even though a notification was received. The best this function can do
            // is report correctly whether or not it reached the timeout time.
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            common_duration d(t - Clock::now());
            do_wait_until(lock, detail::internal_chrono_clock::now() + d);
            if (t > Clock::now()) return cv_status::no_timeout;
            else return cv_status::timeout;
        }

        template <class Rep, class Period>
        cv_status
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d)
        {
            return wait_until(lock, chrono::steady_clock::now() + d);
        }

        template <class Duration, class Predicate>
        bool
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<detail::internal_chrono_clock, Duration>& t,
                Predicate pred)
        {
            const detail::internal_platform_timepoint ts(t);
            while (!pred())
            {
                if (!do_wait_until(lock, ts)) break; // timeout occurred
            }
            return pred();
        }

        template <class Clock, class Duration, class Predicate>
        bool
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t,
                Predicate pred)
        {
            // The system time may jump while this function is waiting. To compensate for this
            // and time out near the correct time, we call do_wait_until() in a loop with a
            // short timeout and recheck the time remaining each time through the loop.
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            while (!pred())
            {
                common_duration d(t - Clock::now());
                if (d <= common_duration::zero()) break; // timeout occurred
                d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
                do_wait_until(lock, detail::internal_platform_clock::now() + detail::platform_duration(d));
            }
            return pred();
        }

        template <class Rep, class Period, class Predicate>
        bool
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
            return wait_until(lock, chrono::steady_clock::now() + d, boost::move(pred));
        }
#endif
    };

    class condition_variable_any:
        private detail::basic_condition_variable
    {
    public:
        BOOST_THREAD_NO_COPYABLE(condition_variable_any)
        condition_variable_any()
        {}

        using detail::basic_condition_variable::do_wait_until;
        using detail::basic_condition_variable::notify_one;
        using detail::basic_condition_variable::notify_all;

        template<typename lock_type>
        void wait(lock_type& m)
        {
            do_wait_until(m, detail::internal_platform_timepoint::getMax());
        }

        template<typename lock_type,typename predicate_type>
        void wait(lock_type& m,predicate_type pred)
        {
            while (!pred())
            {
                wait(m);
            }
        }

#if defined BOOST_THREAD_USES_DATETIME
        template<typename lock_type>
        bool timed_wait(lock_type& m,boost::system_time const& abs_time)
        {
            // The system time may jump while this function is waiting. To compensate for this and time
            // out near the correct time, we could call do_wait_until() in a loop with a short timeout
            // and recheck the time remaining each time through the loop. However, because we can't
            // check the predicate each time do_wait_until() completes, this introduces the possibility
            // of not exiting the function when a notification occurs, since do_wait_until() may report
            // that it timed out even though a notification was received. The best this function can do
            // is report correctly whether or not it reached the timeout time.
            const detail::real_platform_timepoint ts(abs_time);
            const detail::platform_duration d(ts - detail::real_platform_clock::now());
            do_wait_until(m, detail::internal_platform_clock::now() + d);
            return ts > detail::real_platform_clock::now();
        }

        template<typename lock_type>
        bool timed_wait(lock_type& m,boost::xtime const& abs_time)
        {
            return timed_wait(m, system_time(abs_time));
        }

        template<typename lock_type,typename duration_type>
        bool timed_wait(lock_type& m,duration_type const& wait_duration)
        {
            if (wait_duration.is_pos_infinity())
            {
                wait(m);
                return true;
            }
            if (wait_duration.is_special())
            {
                return true;
            }
            const detail::platform_duration d(wait_duration);
            return do_wait_until(m, detail::internal_platform_clock::now() + d);
        }

        template<typename lock_type,typename predicate_type>
        bool timed_wait(lock_type& m,boost::system_time const& abs_time,predicate_type pred)
        {
            // The system time may jump while this function is waiting. To compensate for this
            // and time out near the correct time, we call do_wait_until() in a loop with a
            // short timeout and recheck the time remaining each time through the loop.
            const detail::real_platform_timepoint ts(abs_time);
            while (!pred())
            {
                detail::platform_duration d(ts - detail::real_platform_clock::now());
                if (d <= detail::platform_duration::zero()) break; // timeout occurred
                d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
                do_wait_until(m, detail::internal_platform_clock::now() + d);
            }
            return pred();
        }

        template<typename lock_type,typename predicate_type>
        bool timed_wait(lock_type& m,boost::xtime const& abs_time,predicate_type pred)
        {
            return timed_wait(m, system_time(abs_time), pred);
        }

        template<typename lock_type,typename duration_type,typename predicate_type>
        bool timed_wait(lock_type& m,duration_type const& wait_duration,predicate_type pred)
        {
            if (wait_duration.is_pos_infinity())
            {
                while (!pred())
                {
                    wait(m);
                }
                return true;
            }
            if (wait_duration.is_special())
            {
                return pred();
            }
            const detail::platform_duration d(wait_duration);
            const detail::internal_platform_timepoint ts(detail::internal_platform_clock::now() + d);
            while (!pred())
            {
                if (!do_wait_until(m, ts)) break; // timeout occurred
            }
            return pred();
        }
#endif
#ifdef BOOST_THREAD_USES_CHRONO
        template <class lock_type,class Duration>
        cv_status
        wait_until(
                lock_type& lock,
                const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
        {
            const detail::internal_platform_timepoint ts(t);
            if (do_wait_until(lock, ts)) return cv_status::no_timeout;
            else return cv_status::timeout;
        }

        template <class lock_type, class Clock, class Duration>
        cv_status
        wait_until(
                lock_type& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
            // The system time may jump while this function is waiting. To compensate for this and time
            // out near the correct time, we could call do_wait_until() in a loop with a short timeout
            // and recheck the time remaining each time through the loop. However, because we can't
            // check the predicate each time do_wait_until() completes, this introduces the possibility
            // of not exiting the function when a notification occurs, since do_wait_until() may report
            // that it timed out even though a notification was received. The best this function can do
            // is report correctly whether or not it reached the timeout time.
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            common_duration d(t - Clock::now());
            do_wait_until(lock, detail::internal_chrono_clock::now() + d);
            if (t > Clock::now()) return cv_status::no_timeout;
            else return cv_status::timeout;
        }

        template <class lock_type,  class Rep, class Period>
        cv_status
        wait_for(
                lock_type& lock,
                const chrono::duration<Rep, Period>& d)
        {
            return wait_until(lock, chrono::steady_clock::now() + d);
        }

        template <class lock_type, class Clock, class Duration, class Predicate>
        bool
        wait_until(
                lock_type& lock,
                const chrono::time_point<detail::internal_chrono_clock, Duration>& t,
                Predicate pred)
        {
            const detail::internal_platform_timepoint ts(t);
            while (!pred())
            {
                if (!do_wait_until(lock, ts)) break; // timeout occurred
            }
            return pred();
        }

        template <class lock_type, class Clock, class Duration, class Predicate>
        bool
        wait_until(
                lock_type& lock,
                const chrono::time_point<Clock, Duration>& t,
                Predicate pred)
        {
            // The system time may jump while this function is waiting. To compensate for this
            // and time out near the correct time, we call do_wait_until() in a loop with a
            // short timeout and recheck the time remaining each time through the loop.
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            while (!pred())
            {
                common_duration d(t - Clock::now());
                if (d <= common_duration::zero()) break; // timeout occurred
                d = (std::min)(d, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
                do_wait_until(lock, detail::internal_platform_clock::now() + detail::platform_duration(d));
            }
            return pred();
        }

        template <class lock_type, class Rep, class Period, class Predicate>
        bool
        wait_for(
                lock_type& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
            return wait_until(lock, chrono::steady_clock::now() + d, boost::move(pred));
        }
#endif
    };

        BOOST_THREAD_DECL void notify_all_at_thread_exit(condition_variable& cond, unique_lock<mutex> lk);
}

#include <boost/config/abi_suffix.hpp>

#endif
