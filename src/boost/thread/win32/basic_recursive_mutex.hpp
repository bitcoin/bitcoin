#ifndef BOOST_BASIC_RECURSIVE_MUTEX_WIN32_HPP
#define BOOST_BASIC_RECURSIVE_MUTEX_WIN32_HPP

//  basic_recursive_mutex.hpp
//
//  (C) Copyright 2006-8 Anthony Williams
//  (C) Copyright 2011-2012,2017-2018 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/basic_timed_mutex.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace detail
    {
        template<typename underlying_mutex_type>
        struct basic_recursive_mutex_impl
        {
            long recursion_count;
            long locking_thread_id;
            underlying_mutex_type mutex;

            void initialize()
            {
                recursion_count=0;
                locking_thread_id=0;
                mutex.initialize();
            }

            void destroy()
            {
                mutex.destroy();
            }

            bool try_lock() BOOST_NOEXCEPT
            {
                long const current_thread_id=boost::winapi::GetCurrentThreadId();
                return try_recursive_lock(current_thread_id) || try_basic_lock(current_thread_id);
            }

            void lock()
            {
                long const current_thread_id=boost::winapi::GetCurrentThreadId();
                if(!try_recursive_lock(current_thread_id))
                {
                    mutex.lock();
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                }
            }
#if defined BOOST_THREAD_USES_DATETIME
            bool timed_lock(::boost::system_time const& target)
            {
                long const current_thread_id=boost::winapi::GetCurrentThreadId();
                return try_recursive_lock(current_thread_id) || try_timed_lock(current_thread_id,target);
            }
            template<typename Duration>
            bool timed_lock(Duration const& target)
            {
                long const current_thread_id=boost::winapi::GetCurrentThreadId();
                return try_recursive_lock(current_thread_id) || try_timed_lock(current_thread_id,target);
            }
#endif

#ifdef BOOST_THREAD_USES_CHRONO
            template <class Rep, class Period>
            bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
            {
                long const current_thread_id=boost::winapi::GetCurrentThreadId();
                return try_recursive_lock(current_thread_id) || try_timed_lock_for(current_thread_id,rel_time);
            }
            template <class Clock, class Duration>
            bool try_lock_until(const chrono::time_point<Clock, Duration>& t)
            {
                long const current_thread_id=boost::winapi::GetCurrentThreadId();
                return try_recursive_lock(current_thread_id) || try_timed_lock_until(current_thread_id,t);
            }
#endif
            void unlock()
            {
                if(!--recursion_count)
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,0);
                    mutex.unlock();
                }
            }

        private:
            bool try_recursive_lock(long current_thread_id) BOOST_NOEXCEPT
            {
                if(::boost::detail::interlocked_read_acquire(&locking_thread_id)==current_thread_id)
                {
                    ++recursion_count;
                    return true;
                }
                return false;
            }

            bool try_basic_lock(long current_thread_id) BOOST_NOEXCEPT
            {
                if(mutex.try_lock())
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }

#if defined BOOST_THREAD_USES_DATETIME
            bool try_timed_lock(long current_thread_id,::boost::system_time const& target)
            {
                if(mutex.timed_lock(target))
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }
            template<typename Duration>
            bool try_timed_lock(long current_thread_id,Duration const& target)
            {
                if(mutex.timed_lock(target))
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }
#endif
            template <typename TP>
            bool try_timed_lock_until(long current_thread_id,TP const& target)
            {
                if(mutex.try_lock_until(target))
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }
            template <typename D>
            bool try_timed_lock_for(long current_thread_id,D const& target)
            {
                if(mutex.try_lock_for(target))
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }
        };

        typedef basic_recursive_mutex_impl<basic_timed_mutex> basic_recursive_mutex;
        typedef basic_recursive_mutex_impl<basic_timed_mutex> basic_recursive_timed_mutex;
    }
}

#define BOOST_BASIC_RECURSIVE_MUTEX_INITIALIZER {0}

#include <boost/config/abi_suffix.hpp>

#endif
