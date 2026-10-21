// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BLOCKINGQUEUE_H
#define BITCOIN_UTIL_BLOCKINGQUEUE_H

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace util {

namespace detail {

/** How many operations one side of the queue performed, and how long it spent
 * blocked on the other. `events` counts every Push or Pop, including
 * ones that did not wait. */
template <bool Measure>
struct WaitStats
{
    size_t events{0};
    std::chrono::microseconds total{0};
    std::chrono::microseconds longest{0};
};

/** When nothing is being measured, store nothing. */
template <>
struct WaitStats<false>
{
};

} // namespace detail

/** A bounded queue with one or more producers and one or more
 * consumers. Push blocks while the queue is full, so a producer that runs
 * ahead cannot exhaust memory, and Pop blocks while it is empty. Pop
 * returns false once Finish() has been called and everything queued
 * before it has been taken.
 *
 * Setting `Measure` allows recording how long each side spent waiting
 * on the other, for debugging and optimisation. */
template <typename T, bool Measure = false>
class BlockingQueue
{
public:
    using WaitStats = detail::WaitStats<Measure>;

    explicit BlockingQueue(size_t capacity) : m_capacity{capacity} {}

    /** Queue `item`, or drop it if Finish() has already been called. */
    void Push(T item)
    {
        std::unique_lock lock{m_mutex};
        if (m_done) return;

        const auto before{Timer()};
        m_not_full.wait(lock, [&] { return m_done || m_items.size() < m_capacity; });
        Record(m_pushed_waits, before);
        if (m_done) return;
        m_items.push_back(std::move(item));
        lock.unlock();
        m_not_empty.notify_one();
    }

    /** Wait for an item to be available, putting it in `item` and
     * returning true; if no item is available and Finish() has been
     * called, return `false`. */
    bool Pop(T& item)
    {
        // Release what the caller handed in here, to avoid it happening
        // under the lock during move assignment.
        item = T{};

        std::unique_lock lock{m_mutex};
        const auto before{Timer()};
        m_not_empty.wait(lock, [&] { return m_done || !m_items.empty(); });
        Record(m_popped_waits, before);
        if (m_items.empty()) return false;
        item = std::move(m_items.front());
        m_items.pop_front();
        lock.unlock();
        m_not_full.notify_one();
        return true;
    }

    /** Mark queue as finished. Wakes all waiters; later Push() calls
     * are dropped, and Pop() returns false once the queue is empty. */
    void Finish()
    {
        std::lock_guard lock{m_mutex};
        m_done = true;
        m_not_empty.notify_all();
        m_not_full.notify_all();
    }

    /** How many operations each side performed, and how long it spent blocked. */
    WaitStats PushWaits() const { std::lock_guard lock{m_mutex}; return m_pushed_waits; }
    WaitStats PopWaits() const { std::lock_guard lock{m_mutex}; return m_popped_waits; }

private:
    static std::chrono::steady_clock::time_point Timer()
    {
        if constexpr (Measure) return std::chrono::steady_clock::now();
        return std::chrono::steady_clock::time_point{};
    }

    static void Record(detail::WaitStats<false>&, std::chrono::steady_clock::time_point) { }

    static void Record(detail::WaitStats<true>& stats, std::chrono::steady_clock::time_point before)
    {
        const auto waited{std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - before)};
        ++stats.events;
        stats.total += waited;
        stats.longest = std::max(stats.longest, waited);
    }

    const size_t m_capacity;
    mutable std::mutex m_mutex;
    std::condition_variable m_not_empty;
    std::condition_variable m_not_full;
    std::deque<T> m_items;
    bool m_done{false};
    WaitStats m_pushed_waits;
    WaitStats m_popped_waits;
};

} // namespace util

#endif // BITCOIN_UTIL_BLOCKINGQUEUE_H
