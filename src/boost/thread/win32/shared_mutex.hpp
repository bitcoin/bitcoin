#ifndef BOOST_THREAD_WIN32_SHARED_MUTEX_HPP
#define BOOST_THREAD_WIN32_SHARED_MUTEX_HPP

//  (C) Copyright 2006-8 Anthony Williams
//  (C) Copyright 2011-2012,2017-2018 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <boost/assert.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/static_assert.hpp>
#include <limits.h>
#include <boost/thread/thread_time.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/platform_time.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    class shared_mutex
    {
    private:
        struct state_data
        {
            unsigned long shared_count:11,
                shared_waiting:11,
                exclusive:1,
                upgrade:1,
                exclusive_waiting:7,
                exclusive_waiting_blocked:1;

            friend bool operator==(state_data const& lhs,state_data const& rhs)
            {
                return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
            }
        };

        static state_data interlocked_compare_exchange(state_data* target, state_data new_value, state_data comparand)
        {
            BOOST_STATIC_ASSERT(sizeof(state_data) == sizeof(long));
            long new_val, comp;
            std::memcpy(&new_val, &new_value, sizeof(new_value));
            std::memcpy(&comp, &comparand, sizeof(comparand));
            long const res=BOOST_INTERLOCKED_COMPARE_EXCHANGE(reinterpret_cast<long*>(target),
                                                              new_val,
                                                              comp);
            state_data result;
            std::memcpy(&result, &res, sizeof(result));
            return result;
        }

        enum
        {
            unlock_sem = 0,
            exclusive_sem = 1
        };

        state_data state;
        detail::win32::handle semaphores[2];
        detail::win32::handle upgrade_sem;

        void release_waiters(state_data old_state)
        {
            if(old_state.exclusive_waiting)
            {
                BOOST_VERIFY(winapi::ReleaseSemaphore(semaphores[exclusive_sem],1,0)!=0);
            }

            if(old_state.shared_waiting || old_state.exclusive_waiting)
            {
                BOOST_VERIFY(winapi::ReleaseSemaphore(semaphores[unlock_sem],old_state.shared_waiting + (old_state.exclusive_waiting?1:0),0)!=0);
            }
        }
        void release_shared_waiters(state_data old_state)
        {
            if(old_state.shared_waiting || old_state.exclusive_waiting)
            {
                BOOST_VERIFY(winapi::ReleaseSemaphore(semaphores[unlock_sem],old_state.shared_waiting + (old_state.exclusive_waiting?1:0),0)!=0);
            }
        }

    public:
        BOOST_THREAD_NO_COPYABLE(shared_mutex)
        shared_mutex()
        {
            semaphores[unlock_sem]=detail::win32::create_anonymous_semaphore(0,LONG_MAX);
            semaphores[exclusive_sem]=detail::win32::create_anonymous_semaphore_nothrow(0,LONG_MAX);
            if (!semaphores[exclusive_sem])
            {
              detail::win32::release_semaphore(semaphores[unlock_sem],LONG_MAX);
              boost::throw_exception(thread_resource_error());
            }
            upgrade_sem=detail::win32::create_anonymous_semaphore_nothrow(0,LONG_MAX);
            if (!upgrade_sem)
            {
              detail::win32::release_semaphore(semaphores[unlock_sem],LONG_MAX);
              detail::win32::release_semaphore(semaphores[exclusive_sem],LONG_MAX);
              boost::throw_exception(thread_resource_error());
            }
            state_data state_={0,0,0,0,0,0};
            state=state_;
        }

        ~shared_mutex()
        {
            winapi::CloseHandle(upgrade_sem);
            winapi::CloseHandle(semaphores[unlock_sem]);
            winapi::CloseHandle(semaphores[exclusive_sem]);
        }

        bool try_lock_shared()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                if(!new_state.exclusive && !new_state.exclusive_waiting_blocked)
                {
                    ++new_state.shared_count;
                    if(!new_state.shared_count)
                    {
                        return false;
                    }
                }

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            return !(old_state.exclusive| old_state.exclusive_waiting_blocked);
        }

        void lock_shared()
        {
            for(;;)
            {
                state_data old_state=state;
                for(;;)
                {
                    state_data new_state=old_state;
                    if(new_state.exclusive || new_state.exclusive_waiting_blocked)
                    {
                        ++new_state.shared_waiting;
                        if(!new_state.shared_waiting)
                        {
                            boost::throw_exception(boost::lock_error());
                        }
                    }
                    else
                    {
                        ++new_state.shared_count;
                        if(!new_state.shared_count)
                        {
                            boost::throw_exception(boost::lock_error());
                        }
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }

                if(!(old_state.exclusive| old_state.exclusive_waiting_blocked))
                {
                    return;
                }

                BOOST_VERIFY(winapi::WaitForSingleObjectEx(semaphores[unlock_sem],::boost::detail::win32::infinite,0)==0);
            }
        }

    private:
        unsigned long getMs(detail::platform_duration const& d)
        {
            return static_cast<unsigned long>(d.getMs());
        }

        template <typename Duration>
        unsigned long getMs(Duration const& d)
        {
            return static_cast<unsigned long>(chrono::ceil<chrono::milliseconds>(d).count());
        }

        template <typename Clock, typename Timepoint, typename Duration>
        bool do_lock_shared_until(Timepoint const& t, Duration const& max)
        {
            for(;;)
            {
                state_data old_state=state;
                for(;;)
                {
                    state_data new_state=old_state;
                    if(new_state.exclusive || new_state.exclusive_waiting_blocked)
                    {
                        ++new_state.shared_waiting;
                        if(!new_state.shared_waiting)
                        {
                            boost::throw_exception(boost::lock_error());
                        }
                    }
                    else
                    {
                        ++new_state.shared_count;
                        if(!new_state.shared_count)
                        {
                            boost::throw_exception(boost::lock_error());
                        }
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }

                if(!(old_state.exclusive| old_state.exclusive_waiting_blocked))
                {
                    return true;
                }

                // If the clock is the system clock, it may jump while this function
                // is waiting. To compensate for this and time out near the correct
                // time, we call WaitForSingleObjectEx() in a loop with a short
                // timeout and recheck the time remaining each time through the loop.
                unsigned long res=0;
                for(;;)
                {
                    Duration d(t - Clock::now());
                    if(d <= Duration::zero()) // timeout occurred
                    {
                        res=detail::win32::timeout;
                        break;
                    }
                    if(max != Duration::zero())
                    {
                        d = (std::min)(d, max);
                    }
                    res=winapi::WaitForSingleObjectEx(semaphores[unlock_sem],getMs(d),0);
                    if(res!=detail::win32::timeout) // semaphore released
                    {
                        break;
                    }
                }

                if(res==detail::win32::timeout)
                {
                    for(;;)
                    {
                        state_data new_state=old_state;
                        if(new_state.exclusive || new_state.exclusive_waiting_blocked)
                        {
                            if(new_state.shared_waiting)
                            {
                                --new_state.shared_waiting;
                            }
                        }
                        else
                        {
                            ++new_state.shared_count;
                            if(!new_state.shared_count)
                            {
                                return false;
                            }
                        }

                        state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                        if(current_state==old_state)
                        {
                            break;
                        }
                        old_state=current_state;
                    }

                    if(!(old_state.exclusive| old_state.exclusive_waiting_blocked))
                    {
                        return true;
                    }
                    return false;
                }

                BOOST_ASSERT(res==0);
            }
        }
    public:

#if defined BOOST_THREAD_USES_DATETIME
        template<typename TimeDuration>
        bool timed_lock_shared(TimeDuration const & relative_time)
        {
            const detail::mono_platform_timepoint t(detail::mono_platform_clock::now() + detail::platform_duration(relative_time));
            // The reference clock is steady and so no need to poll periodically, thus 0 ms max (i.e. no max)
            return do_lock_shared_until<detail::mono_platform_clock>(t, detail::platform_duration::zero());
        }
        bool timed_lock_shared(boost::system_time const& wait_until)
        {
            const detail::real_platform_timepoint t(wait_until);
            return do_lock_shared_until<detail::real_platform_clock>(t, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        bool try_lock_shared_for(const chrono::duration<Rep, Period>& rel_time)
        {
            const chrono::steady_clock::time_point t(chrono::steady_clock::now() + rel_time);
            typedef typename chrono::duration<Rep, Period> Duration;
            typedef typename common_type<Duration, typename chrono::steady_clock::duration>::type common_duration;
            // The reference clock is steady and so no need to poll periodically, thus 0 ms max (i.e. no max)
            return do_lock_shared_until<chrono::steady_clock>(t, common_duration::zero());
        }
        template <class Duration>
        bool try_lock_shared_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
        {
            typedef typename common_type<Duration, typename chrono::steady_clock::duration>::type common_duration;
            // The reference clock is steady and so no need to poll periodically, thus 0 ms max (i.e. no max)
            return do_lock_shared_until<chrono::steady_clock>(t, common_duration::zero());
        }
        template <class Clock, class Duration>
        bool try_lock_shared_until(const chrono::time_point<Clock, Duration>& t)
        {
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            return do_lock_shared_until<Clock>(t, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
        }
#endif

        void unlock_shared()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                bool const last_reader=!--new_state.shared_count;

                if(last_reader)
                {
                    if(new_state.upgrade)
                    {
                        new_state.upgrade=false;
                        new_state.exclusive=true;
                    }
                    else
                    {
                        if(new_state.exclusive_waiting)
                        {
                            --new_state.exclusive_waiting;
                            new_state.exclusive_waiting_blocked=false;
                        }
                        new_state.shared_waiting=0;
                    }
                }

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    if(last_reader)
                    {
                        if(old_state.upgrade)
                        {
                            BOOST_VERIFY(winapi::ReleaseSemaphore(upgrade_sem,1,0)!=0);
                        }
                        else
                        {
                            release_waiters(old_state);
                        }
                    }
                    break;
                }
                old_state=current_state;
            }
        }

        bool try_lock()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                if(new_state.shared_count || new_state.exclusive)
                {
                    return false;
                }
                else
                {
                    new_state.exclusive=true;
                }

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            return true;
        }

        void lock()
        {
            for(;;)
            {
                state_data old_state=state;
                for(;;)
                {
                    state_data new_state=old_state;
                    if(new_state.shared_count || new_state.exclusive)
                    {
                        ++new_state.exclusive_waiting;
                        if(!new_state.exclusive_waiting)
                        {
                            boost::throw_exception(boost::lock_error());
                        }

                        new_state.exclusive_waiting_blocked=true;
                    }
                    else
                    {
                        new_state.exclusive=true;
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }

                if(!old_state.shared_count && !old_state.exclusive)
                {
                    return;
                }

                #ifndef UNDER_CE
                const bool wait_all = true;
                #else
                const bool wait_all = false;
                #endif
                BOOST_VERIFY(winapi::WaitForMultipleObjectsEx(2,semaphores,wait_all,::boost::detail::win32::infinite,0)<2);
            }
        }

    private:
        template <typename Clock, typename Timepoint, typename Duration>
        bool do_lock_until(Timepoint const& t, Duration const& max)
        {
            for(;;)
            {
                state_data old_state=state;
                for(;;)
                {
                    state_data new_state=old_state;
                    if(new_state.shared_count || new_state.exclusive)
                    {
                        ++new_state.exclusive_waiting;
                        if(!new_state.exclusive_waiting)
                        {
                            boost::throw_exception(boost::lock_error());
                        }

                        new_state.exclusive_waiting_blocked=true;
                    }
                    else
                    {
                        new_state.exclusive=true;
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }

                if(!old_state.shared_count && !old_state.exclusive)
                {
                    return true;
                }

                // If the clock is the system clock, it may jump while this function
                // is waiting. To compensate for this and time out near the correct
                // time, we call WaitForMultipleObjectsEx() in a loop with a short
                // timeout and recheck the time remaining each time through the loop.
                unsigned long wait_res=0;
                for(;;)
                {
                    Duration d(t - Clock::now());
                    if(d <= Duration::zero()) // timeout occurred
                    {
                        wait_res=detail::win32::timeout;
                        break;
                    }
                    if(max != Duration::zero())
                    {
                        d = (std::min)(d, max);
                    }
                    #ifndef UNDER_CE
                    wait_res=winapi::WaitForMultipleObjectsEx(2,semaphores,true,getMs(d),0);
                    #else
                    wait_res=winapi::WaitForMultipleObjectsEx(2,semaphores,false,getMs(d),0);
                    #endif
                    //wait_res=winapi::WaitForMultipleObjectsEx(2,semaphores,wait_all,getMs(d), 0);

                    if(wait_res!=detail::win32::timeout) // semaphore released
                    {
                        break;
                    }
                }

                if(wait_res==detail::win32::timeout)
                {
                    for(;;)
                    {
                        bool must_notify = false;
                        state_data new_state=old_state;
                        if(new_state.shared_count || new_state.exclusive)
                        {
                            if(new_state.exclusive_waiting)
                            {
                                if(!--new_state.exclusive_waiting)
                                {
                                    new_state.exclusive_waiting_blocked=false;
                                    must_notify = true;
                                }
                            }
                        }
                        else
                        {
                            new_state.exclusive=true;
                        }

                        state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                        if (must_notify)
                        {
                          BOOST_VERIFY(winapi::ReleaseSemaphore(semaphores[unlock_sem],1,0)!=0);
                        }

                        if(current_state==old_state)
                        {
                            break;
                        }
                        old_state=current_state;
                    }
                    if(!old_state.shared_count && !old_state.exclusive)
                    {
                        return true;
                    }
                    return false;
                }

                BOOST_ASSERT(wait_res<2);
            }
        }
    public:

#if defined BOOST_THREAD_USES_DATETIME
        bool timed_lock(boost::system_time const& wait_until)
        {
            const detail::real_platform_timepoint t(wait_until);
            return do_lock_until<detail::real_platform_clock>(t, detail::platform_milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
        }
        template<typename TimeDuration>
        bool timed_lock(TimeDuration const & relative_time)
        {
            const detail::mono_platform_timepoint t(detail::mono_platform_clock::now() + detail::platform_duration(relative_time));
            // The reference clock is steady and so no need to poll periodically, thus 0 ms max (i.e. no max)
            return do_lock_until<detail::mono_platform_clock>(t, detail::platform_duration::zero());
        }
#endif
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
        {
            const chrono::steady_clock::time_point t(chrono::steady_clock::now() + rel_time);
            typedef typename chrono::duration<Rep, Period> Duration;
            typedef typename common_type<Duration, typename chrono::steady_clock::duration>::type common_duration;
            // The reference clock is steady and so no need to poll periodically, thus 0 ms max (i.e. no max)
            return do_lock_until<chrono::steady_clock>(t, common_duration::zero());
        }
        template <class Duration>
        bool try_lock_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
        {
            typedef typename common_type<Duration, typename chrono::steady_clock::duration>::type common_duration;
            // The reference clock is steady and so no need to poll periodically, thus 0 ms max (i.e. no max)
            return do_lock_until<chrono::steady_clock>(t, common_duration::zero());
        }
        template <class Clock, class Duration>
        bool try_lock_until(const chrono::time_point<Clock, Duration>& t)
        {
            typedef typename common_type<Duration, typename Clock::duration>::type common_duration;
            return do_lock_until<Clock>(t, common_duration(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
        }
#endif

        void unlock()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                new_state.exclusive=false;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            release_waiters(old_state);
        }

        void lock_upgrade()
        {
            for(;;)
            {
                state_data old_state=state;
                for(;;)
                {
                    state_data new_state=old_state;
                    if(new_state.exclusive || new_state.exclusive_waiting_blocked || new_state.upgrade)
                    {
                        ++new_state.shared_waiting;
                        if(!new_state.shared_waiting)
                        {
                            boost::throw_exception(boost::lock_error());
                        }
                    }
                    else
                    {
                        ++new_state.shared_count;
                        if(!new_state.shared_count)
                        {
                            boost::throw_exception(boost::lock_error());
                        }
                        new_state.upgrade=true;
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }

                if(!(old_state.exclusive|| old_state.exclusive_waiting_blocked|| old_state.upgrade))
                {
                    return;
                }

                BOOST_VERIFY(winapi::WaitForSingleObjectEx(semaphores[unlock_sem],winapi::infinite,0)==0);
            }
        }

        bool try_lock_upgrade()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                if(new_state.exclusive || new_state.exclusive_waiting_blocked || new_state.upgrade)
                {
                    return false;
                }
                else
                {
                    ++new_state.shared_count;
                    if(!new_state.shared_count)
                    {
                        return false;
                    }
                    new_state.upgrade=true;
                }

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            return true;
        }

        void unlock_upgrade()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                new_state.upgrade=false;
                bool const last_reader=!--new_state.shared_count;

                new_state.shared_waiting=0;
                if(last_reader)
                {
                    if(new_state.exclusive_waiting)
                    {
                        --new_state.exclusive_waiting;
                        new_state.exclusive_waiting_blocked=false;
                    }
                }

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    if(last_reader)
                    {
                        release_waiters(old_state);
                    }
                    else {
                        release_shared_waiters(old_state);
                    }
                    // #7720
                    //else {
                    //    release_waiters(old_state);
                    //}
                    break;
                }
                old_state=current_state;
            }
        }

        void unlock_upgrade_and_lock()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                bool const last_reader=!--new_state.shared_count;

                if(last_reader)
                {
                    new_state.upgrade=false;
                    new_state.exclusive=true;
                }

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    if(!last_reader)
                    {
                        BOOST_VERIFY(winapi::WaitForSingleObjectEx(upgrade_sem,detail::win32::infinite,0)==0);
                    }
                    break;
                }
                old_state=current_state;
            }
        }

        void unlock_and_lock_upgrade()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                new_state.exclusive=false;
                new_state.upgrade=true;
                ++new_state.shared_count;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            release_waiters(old_state);
        }

        void unlock_and_lock_shared()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                new_state.exclusive=false;
                ++new_state.shared_count;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            release_waiters(old_state);
        }
        void unlock_upgrade_and_lock_shared()
        {
            state_data old_state=state;
            for(;;)
            {
                state_data new_state=old_state;
                new_state.upgrade=false;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            release_waiters(old_state);
        }

    };
    typedef shared_mutex upgrade_mutex;

}

#include <boost/config/abi_suffix.hpp>

#endif
