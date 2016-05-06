// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERRUPTIBLE_LOCK_H
#define BITCOIN_INTERRUPTIBLE_LOCK_H

#include "interruptible/this_thread.h"

#include <condition_variable>
#include <mutex>
#include <assert.h>

/*
In order to avoid exposing the internals of thread_data, the semantics here are a bit funky.
thread_data's mutex must be locked when setting/unsetting the condition pointer, and should
also lock/unlock along with the user's mutex.

To facilitate that, set_cond returns a (locked) unique_lock which the caller should hold until
releasing the cond with unset_cond, which moves back the lock, which is then destroyed.

*/

namespace interruptible
{
template <typename Lockable>
class int_lock
{
public:
    int_lock(const int_lock&) = delete;
    int_lock& operator=(const int_lock&) = delete;

    int_lock(std::condition_variable_any& cond, Lockable& user_lock) : m_user_lock(user_lock), m_cond_lock(this_thread::detail::set_cond(&cond))
    {
        assert(m_user_lock.owns_lock());
        assert(m_cond_lock.owns_lock());
    }
    ~int_lock()
    {
        assert(m_user_lock.owns_lock());
        assert(m_cond_lock.owns_lock());
        this_thread::detail::unset_cond(std::move(m_cond_lock));
    }
    inline void lock()
    {
        assert(!m_user_lock.owns_lock());
        assert(!m_cond_lock.owns_lock());
        std::lock(m_user_lock, m_cond_lock);
    }
    inline void unlock()
    {
        assert(m_user_lock.owns_lock());
        assert(m_cond_lock.owns_lock());
        m_cond_lock.unlock();
        m_user_lock.unlock();
    }

private:
    Lockable& m_user_lock;
    std::unique_lock<std::mutex> m_cond_lock;
};
}

#endif // BITCOIN_INTERRUPTIBLE_LOCK_H
