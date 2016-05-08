// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "interruptible/thread.h"
#include "interruptible/thread_data.h"
#include "interruptible/this_thread.h"

void interruptible::thread::start_thread(std::function<void()>&& func)
{
    m_data = std::make_shared<interruptible::detail::thread_data>();
    auto run_thread_func = [func, this] {
        interruptible::this_thread::detail::set_thread_data(m_data);
        try {
            func();
        }
        catch (const thread_interrupted&) {
        }
    };
    m_data->start_thread(run_thread_func);
}

void interruptible::thread::interrupt()
{
    m_data->set_interrupted();
}

void interruptible::thread::join()
{
    if (m_data->get_thread().joinable()) {
        m_data->get_thread().join();
    }
}

void interruptible::thread::swap(interruptible::thread& rhs) noexcept
{
    m_data.swap(rhs.m_data);
}

bool interruptible::thread::joinable() const noexcept
{
    return m_data->get_thread().joinable();
}

void interruptible::thread::detach()
{
    m_data->get_thread().detach();
}

interruptible::thread::id interruptible::thread::get_id() const noexcept
{
    return m_data->get_thread().get_id();
}

interruptible::thread::native_handle_type interruptible::thread::native_handle()
{
    return m_data->get_thread().native_handle();
}

unsigned interruptible::thread::hardware_concurrency() noexcept
{
    return thread_type::hardware_concurrency();
}
