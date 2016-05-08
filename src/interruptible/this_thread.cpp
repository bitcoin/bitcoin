// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "interruptible/condition_variable.h"
#include "interruptible/this_thread.h"
#include "interruptible/thread.h"
#include "interruptible/thread_data.h"

#include <mutex>
#include <assert.h>

thread_local std::shared_ptr<interruptible::detail::thread_data> t_data;

std::unique_lock<std::mutex> interruptible::this_thread::detail::set_cond(std::condition_variable_any* cond)
{
    assert(t_data);
    return t_data->set_cond(cond);
}

void interruptible::this_thread::detail::unset_cond(std::unique_lock<std::mutex>&& lock)
{
    assert(t_data);
    t_data->unset_cond(std::move(lock));
}

void interruptible::this_thread::detail::set_thread_data(const std::shared_ptr<interruptible::detail::thread_data>& indata)
{
    t_data = indata;
}

void interruptible::this_thread::detail::sleep_until(std::chrono::time_point<std::chrono::steady_clock> endtime)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    interruptible::condition_variable cond;
    cond.wait_until(lock, endtime, [&endtime] { return std::chrono::steady_clock::now() >= endtime; });
}

bool interruptible::this_thread::detail::enable_interruption(bool thread_interruption_enabled)
{
    if(t_data)
        return t_data->enable_interruption(thread_interruption_enabled);
    return false;
}

bool interruptible::this_thread::interruption_enabled()
{
    if(t_data)
        return t_data->interruption_enabled();
    return false;
}

void interruptible::this_thread::interruption_point()
{
    if(t_data && t_data->should_interrupt())
        throw ::interruptible::thread_interrupted();
}
