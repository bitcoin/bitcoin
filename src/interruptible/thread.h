// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERRUPTIBLE_THREAD_H
#define BITCOIN_INTERRUPTIBLE_THREAD_H

#include "interruptible/this_thread.h"

#include <thread>
#include <memory>

namespace interruptible
{
namespace detail
{
class thread_data;
}

class thread
{
public:
    typedef std::thread thread_type;
    typedef thread_type::native_handle_type native_handle_type;
    typedef thread_type::id id;

    thread() noexcept = default;

    template <typename T, typename... Args>
    explicit thread(T&& func, Args&&... args)
    {
        start_thread(std::bind(std::forward<T>(func), std::forward<Args>(args)...));
    }

    thread(const thread&) = delete;
    thread(thread&& t) noexcept(true) = default;

    thread& operator=(const thread&) = delete;
    thread& operator=(thread&& t) noexcept(true) = default;

    void join();
    void swap(thread& rhs) noexcept;
    bool joinable() const noexcept;
    void detach();
    id get_id() const noexcept;
    native_handle_type native_handle();
    static unsigned hardware_concurrency() noexcept;

    // interruptible additions
    void interrupt();
    bool interruption_requested() const;

private:
    std::shared_ptr<detail::thread_data> m_data;
    void start_thread(std::function<void()>&& func);
};

class thread_interrupted
{
};
}

#endif // BITCOIN_INTERRUPTIBLE_THREAD_H
