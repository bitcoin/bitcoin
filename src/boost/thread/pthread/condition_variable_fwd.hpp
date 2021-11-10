#ifndef BOOST_THREAD_PTHREAD_CONDITION_VARIABLE_FWD_HPP
#define BOOST_THREAD_PTHREAD_CONDITION_VARIABLE_FWD_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <pthread.h>
#include <boost/thread/cv_status.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/detail/platform_time.hpp>
#include <boost/thread/pthread/pthread_helpers.hpp>

#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif

#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <algorithm>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    class condition_variable
    {
    private:
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        pthread_mutex_t internal_mutex;
//#endif
        pthread_cond_t cond;

    public:
    //private: // used by boost::thread::try_join_until

        bool do_wait_until(
            unique_lock<mutex>& lock,
            detail::internal_platform_timepoint const &timeout);

    public:
      BOOST_THREAD_NO_COPYABLE(condition_variable)
        condition_variable()
        {
            int res;
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            // Even if it is not used, the internal_mutex exists (see
            // above) and must be initialized (etc) in case some
            // compilation units provide interruptions and others
            // don't.
            res=posix::pthread_mutex_init(&internal_mutex);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost::condition_variable::condition_variable() constructor failed in pthread_mutex_init"));
            }
//#endif
            res = posix::pthread_cond_init(&cond);
            if (res)
            {
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                // ditto
                BOOST_VERIFY(!posix::pthread_mutex_destroy(&internal_mutex));
//#endif
                boost::throw_exception(thread_resource_error(res, "boost::condition_variable::condition_variable() constructor failed in pthread_cond_init"));
            }
        }
        ~condition_variable()
        {
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            // ditto
            BOOST_VERIFY(!posix::pthread_mutex_destroy(&internal_mutex));
//#endif
            BOOST_VERIFY(!posix::pthread_cond_destroy(&cond));
        }

        void wait(unique_lock<mutex>& m);

        template<typename predicate_type>
        void wait(unique_lock<mutex>& m,predicate_type pred)
        {
            while (!pred())
            {
                wait(m);
            }
        }

#if defined BOOST_THREAD_USES_DATETIME
        bool timed_wait(
            unique_lock<mutex>& m,
            boost::system_time const& abs_time)
        {
#if defined BOOST_THREAD_WAIT_BUG
            const detail::real_platform_timepoint ts(abs_time + BOOST_THREAD_WAIT_BUG);
#else
            const detail::real_platform_timepoint ts(abs_time);
#endif
#if defined BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
            // The system time may jump while this function is waiting. To compensate for this and time
            // out near the correct time, we could call do_wait_until() in a loop with a short timeout
            // and recheck the time remaining each time through the loop. However, because we can't
            // check the predicate each time do_wait_until() completes, this introduces the possibility
            // of not exiting the function when a notification occurs, since do_wait_until() may report
            // that it timed out even though a notification was received. The best this function can do
            // is report correctly whether or not it reached the timeout time.
            const detail::platform_duration d(ts - detail::real_platform_clock::now());
            do_wait_until(m, detail::internal_platform_clock::now() + d);
            return ts > detail::real_platform_clock::now();
#else
            return do_wait_until(m, ts);
#endif
        }
        bool timed_wait(
            unique_lock<mutex>& m,
            ::boost::xtime const& abs_time)
        {
            return timed_wait(m,system_time(abs_time));
        }

        template<typename duration_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            duration_type const& wait_duration)
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
            detail::platform_duration d(wait_duration);
#if defined(BOOST_THREAD_HAS_MONO_CLOCK) && !defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
            // The system time may jump while this function is waiting. To compensate for this and time
            // out near the correct time, we could call do_wait_until() in a loop with a short timeout
            // and recheck the time remaining each time through the loop. However, because we can't
            // check the predicate each time do_wait_until() completes, this introduces the possibility
            // of not exiting the function when a notification occurs, since do_wait_until() may report
            // that it timed out even though a notification was received. The best this function can do
            // is report correctly whether or not it reached the timeout time.
            const detail::mono_platform_timepoint ts(detail::mono_platform_clock::now() + d);
            do_wait_until(m, detail::internal_platform_clock::now() + d);
            return ts > detail::mono_platform_clock::now();
#else
            return do_wait_until(m, detail::internal_platform_clock::now() + d);
#endif
        }

        template<typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            boost::system_time const& abs_time,predicate_type pred)
        {
#if defined BOOST_THREAD_WAIT_BUG
            const detail::real_platform_timepoint ts(abs_time + BOOST_THREAD_WAIT_BUG);
#else
            const detail::real_platform_timepoint ts(abs_time);
#endif
            while (!pred())
            {
#if defined BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
                // The system time may jump while this function is waiting. To compensate for this
                // and time out near the correct time, we call do_wait_until() in a loop with a
                // short timeout and recheck the time remaining each time through the loop.
                detail::platform_duration d(ts - detail::real_platform_clock::now());
                if (d <= detail::platform_duration::zero()) break; // timeout occurred
                d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
                do_wait_until(m, detail::internal_platform_clock::now() + d);
#else
                if (!do_wait_until(m, ts)) break; // timeout occurred
#endif
            }
            return pred();
        }

        template<typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            ::boost::xtime const& abs_time,predicate_type pred)
        {
            return timed_wait(m,system_time(abs_time),pred);
        }

        template<typename duration_type,typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            duration_type const& wait_duration,predicate_type pred)
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
            detail::platform_duration d(wait_duration);
#if defined(BOOST_THREAD_HAS_MONO_CLOCK) && !defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
            // The system time may jump while this function is waiting. To compensate for this
            // and time out near the correct time, we call do_wait_until() in a loop with a
            // short timeout and recheck the time remaining each time through the loop.
            const detail::mono_platform_timepoint ts(detail::mono_platform_clock::now() + d);
            while (!pred())
            {
                if (d <= detail::platform_duration::zero()) break; // timeout occurred
                d = (std::min)(d, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
                do_wait_until(m, detail::internal_platform_clock::now() + d);
                d = ts - detail::mono_platform_clock::now();
            }
#else
            const detail::internal_platform_timepoint ts(detail::internal_platform_clock::now() + d);
            while (!pred())
            {
                if (!do_wait_until(m, ts)) break; // timeout occurred
            }
#endif
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

#define BOOST_THREAD_DEFINES_CONDITION_VARIABLE_NATIVE_HANDLE
        typedef pthread_cond_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &cond;
        }

        void notify_one() BOOST_NOEXCEPT;
        void notify_all() BOOST_NOEXCEPT;
    };

    BOOST_THREAD_DECL void notify_all_at_thread_exit(condition_variable& cond, unique_lock<mutex> lk);
}

#include <boost/config/abi_suffix.hpp>

#endif
