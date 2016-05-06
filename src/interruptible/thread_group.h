// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERRUPTIBLE_THREAD_GROUP_H
#define BITCOIN_INTERRUPTIBLE_THREAD_GROUP_H

#include "interruptible/thread.h"

#include <list>
#include <memory>

namespace interruptible
{
class thread;
class thread_group
{
public:
    thread_group() = default;
    thread_group(const thread_group&) = delete;
    thread_group& operator=(const thread_group&) = delete;

    template <typename T, typename... Args>
    void create_thread(T&& func, Args&&... args)
    {
        auto thread_ptr(new interruptible::thread(std::forward<T>(func), std::forward<Args>(args)...));
        m_threads.emplace_back(thread_ptr);
    }

    void add_thread(interruptible::thread* rhs);
    void remove_thread(interruptible::thread* rhs);
    void interrupt_all();
    void join_all();
    size_t size() const;

private:
    mutable std::mutex m_mutex;
    std::list<std::unique_ptr<interruptible::thread> > m_threads;
};
}

#endif // BITCOIN_INTERRUPTIBLE_THREAD_GROUP_H
