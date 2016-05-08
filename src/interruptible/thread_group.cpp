// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "interruptible/thread_group.h"

namespace interruptible
{

thread_group::~thread_group()
{
    for (auto&& thread : m_threads)
        delete thread;
}

void thread_group::add_thread(interruptible::thread* rhs)
{
    if (rhs != nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threads.push_back(rhs);
    }
}

void thread_group::remove_thread(interruptible::thread* rhs)
{
    if (rhs != nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threads.remove_if([rhs](const interruptible::thread* it) { return rhs->get_id() == it->get_id(); });
    }
}

void thread_group::interrupt_all()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto&& thread : m_threads)
        thread->interrupt();
}

void thread_group::join_all()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto&& thread : m_threads)
        thread->join();
}

size_t thread_group::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_threads.size();
}
}
