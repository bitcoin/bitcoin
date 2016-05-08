// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERRUPTIBLE_THREAD_DATA_H
#define BITCOIN_INTERRUPTIBLE_THREAD_DATA_H

#include <atomic>
#include <assert.h>
#include <mutex>

namespace interruptible
{
namespace detail
{
class thread_data
{
public:
    thread_data() noexcept = default;
    thread_data(thread_data&& t) = default;
    thread_data& operator=(thread_data&& t) = default;
    ~thread_data() = default;

    thread_data(const thread_data&) = delete;
    thread_data& operator=(const thread_data&) = delete;

    template <typename T, typename... Args>
    void start_thread(T&& func, Args&&... args)
    {
        m_thread = std::thread(std::forward<T>(func), std::forward<Args>(args)...);
    }

    void set_interrupted()
    {
        m_interrupted.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(m_wake_mutex);
        if (m_cond_wake_ptr != nullptr)
            m_cond_wake_ptr->notify_all();
    }

    bool is_interrupted() const
    {
        return m_interrupted;
    }

    bool enable_interruption(bool thread_interruption_enabled)
    {
        bool prev = interruption_enabled();
        m_thread_interruption_enabled = thread_interruption_enabled;
        return prev;
    }

    bool interruption_enabled() const
    {
        return m_thread_interruption_enabled;
    }

    bool should_interrupt() const
    {
        return m_thread_interruption_enabled && m_interrupted;
    }

    std::unique_lock<std::mutex> set_cond(std::condition_variable_any* cond)
    {
        std::unique_lock<std::mutex> lock(m_wake_mutex);
        m_cond_wake_ptr = cond;
        return lock;
    }

    void unset_cond(std::unique_lock<std::mutex>&& lock)
    {
        (void)lock; // just let it deconstruct
        m_cond_wake_ptr = nullptr;
    }

    std::thread& get_thread()
    {
        return m_thread;
    }

private:
    std::mutex m_wake_mutex;
    std::condition_variable_any* m_cond_wake_ptr = nullptr;
    bool m_thread_interruption_enabled = true;
    std::atomic<bool> m_interrupted;
    std::thread m_thread;
};
}
}
#endif // BITCOIN_INTERRUPTIBLE_THREAD_DATA_H
