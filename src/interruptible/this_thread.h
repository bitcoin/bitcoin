// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERRUPTIBLE_THIS_THREAD_H
#define BITCOIN_INTERRUPTIBLE_THIS_THREAD_H

#include <condition_variable>
#include <mutex>
#include <thread>

namespace interruptible
{
namespace detail
{
class thread_data;
}

namespace this_thread
{
namespace detail
{
std::unique_lock<std::mutex> set_cond(std::condition_variable_any* cond);
void unset_cond(std::unique_lock<std::mutex>&& lock);
void set_thread_data(const std::shared_ptr<::interruptible::detail::thread_data>& indata);
bool enable_interruption(bool thread_interruption_enabled);
void sleep_until(std::chrono::time_point<std::chrono::steady_clock> endtime);
}


void interruption_point();
bool interruption_enabled();

class disable_interruption
{
    friend class restore_interruption;

public:
    disable_interruption()
    {
        m_was_enabled = detail::enable_interruption(false);
    }
    ~disable_interruption()
    {
        detail::enable_interruption(m_was_enabled);
    }
    disable_interruption(const disable_interruption&) = delete;
    disable_interruption& operator=(const disable_interruption&) = delete;

private:
    bool m_was_enabled;
};

class restore_interruption
{
public:
    explicit restore_interruption(disable_interruption& disabler)
    {
        detail::enable_interruption(disabler.m_was_enabled);
    }
    ~restore_interruption()
    {
        detail::enable_interruption(false);
    }
    restore_interruption(const restore_interruption&) = delete;
    restore_interruption& operator=(const restore_interruption&) = delete;
};

template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration)
{
    detail::sleep_until(std::chrono::steady_clock::now() + sleep_duration);
}
}
}

#endif // BITCOIN_INTERRUPTIBLE_THIS_THREAD_H
