// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERRUPTIBLE_CONDITION_VARIABLE_H
#define BITCOIN_INTERRUPTIBLE_CONDITION_VARIABLE_H

#include "interruptible/lock.h"
#include "interruptible/this_thread.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace interruptible
{
class condition_variable
{
public:
    typedef int_lock<std::unique_lock<std::mutex> > lock_type;

    condition_variable() = default;
    ~condition_variable() = default;

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

    inline void notify_one() noexcept
    {
        m_condvar.notify_one();
    }

    inline void notify_all() noexcept
    {
        m_condvar.notify_all();
    }

    template <class Lockable>
    inline void wait(Lockable& user_lock)
    {
        lock_type lock(m_condvar, user_lock);
        this_thread::interruption_point();
        m_condvar.wait(lock);
        this_thread::interruption_point();
    }

    template <class Lockable, class Predicate>
    void wait(Lockable& user_lock, Predicate pred)
    {
        lock_type lock(m_condvar, user_lock);
        this_thread::interruption_point();
        while (!pred()) {
            m_condvar.wait(lock);
            this_thread::interruption_point();
        }
    }

    template <class Lockable, class Clock, class Duration>
    std::cv_status wait_until(Lockable& user_lock, const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        lock_type lock(m_condvar, user_lock);
        this_thread::interruption_point();
        std::cv_status ret = m_condvar.wait_until(lock, abs_time);
        this_thread::interruption_point();
        return ret;
    }

    template <class Lockable, class Clock, class Duration, class Predicate>
    bool wait_until(Lockable& user_lock, const std::chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {
        lock_type lock(m_condvar, user_lock);
        this_thread::interruption_point();
        while (!pred()) {
            auto ret = m_condvar.wait_until(lock, abs_time);
            this_thread::interruption_point();
            if (ret == std::cv_status::timeout)
                return pred();
        }
        return true;
    }

    template <class Lockable, class Rep, class Period>
    std::cv_status wait_for(Lockable& user_lock, const std::chrono::duration<Rep, Period>& rel_time)
    {
        lock_type lock(m_condvar, user_lock);
        this_thread::interruption_point();
        std::cv_status ret = m_condvar.wait_for(lock, rel_time);
        this_thread::interruption_point();
        return ret;
    }

    template <class Lockable, class Rep, class Period, class Predicate>
    bool wait_for(Lockable& user_lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        return wait_until(user_lock, std::chrono::steady_clock::now() + rel_time, pred);
    }

private:
    std::condition_variable_any m_condvar;
};
}

#endif // BITCOIN_INTERRUPTIBLE_CONDITION_VARIABLE_H
