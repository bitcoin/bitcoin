// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_THREADPOOL_H
#define BITCOIN_UTIL_THREADPOOL_H

#include <sync.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/string.h>
#include <util/thread.h>
#include <util/threadinterrupt.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

/**
 * @brief Fixed-size thread pool for running arbitrary tasks concurrently.
 *
 * The thread pool maintains a set of worker threads that consume and execute
 * tasks submitted through Submit(). Once started, tasks can be queued and
 * processed asynchronously until Stop() is called.
 *
 * ### Thread-safety and lifecycle
 * - `Start()` and `Stop()` must be called from a controller (non-worker) thread.
 *   Calling `Stop()` from a worker thread will deadlock, as it waits for all
 *   workers to join, including the current one.
 *
 * - `Submit()` can be called from any thread, including workers. It safely
 *   enqueues new work for execution as long as the pool has active workers.
 *
 * - `Stop()` prevents further task submission and wakes all worker threads.
 *   Workers finish processing all remaining queued tasks before exiting,
 *   guaranteeing that no caller waits forever on a pending future.
 */
class ThreadPool
{
private:
    std::string m_name;
    Mutex m_mutex;
    std::queue<std::packaged_task<void()>> m_work_queue GUARDED_BY(m_mutex);
    std::condition_variable m_cv;
    // Note: m_interrupt must be guarded by m_mutex, and cannot be replaced by an unguarded atomic bool.
    // This ensures threads blocked on m_cv reliably observe the change and proceed correctly without missing signals.
    // Ref: https://en.cppreference.com/w/cpp/thread/condition_variable
    bool m_interrupt GUARDED_BY(m_mutex){false};
    std::vector<std::thread> m_workers GUARDED_BY(m_mutex);

    void WorkerThread() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, wait_lock);
        for (;;) {
            std::packaged_task<void()> task;
            {
                // Wait only if needed; avoid sleeping when a new task was submitted while we were processing another one.
                if (!m_interrupt && m_work_queue.empty()) {
                    // Block until the pool is interrupted or a task is available.
                    m_cv.wait(wait_lock, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_interrupt || !m_work_queue.empty(); });
                }

                // If stopped and no work left, exit worker
                if (m_interrupt && m_work_queue.empty()) {
                    return;
                }

                task = std::move(m_work_queue.front());
                m_work_queue.pop();
            }

            {
                // Execute the task without the lock
                REVERSE_LOCK(wait_lock, m_mutex);
                task();
            }
        }
    }

public:
    explicit ThreadPool(const std::string& name) : m_name(name) {}

    ~ThreadPool()
    {
        Stop(); // In case it hasn't been stopped.
    }

    /**
     * @brief Start worker threads.
     *
     * Creates and launches `num_workers` threads that begin executing tasks
     * from the queue. If the pool is already started, throws.
     *
     * Must be called from a controller (non-worker) thread.
     */
    void Start(int num_workers) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        assert(num_workers > 0);
        LOCK(m_mutex);
        if (!m_workers.empty()) throw std::runtime_error("Thread pool already started");
        m_interrupt = false; // Reset

        // Create workers
        m_workers.reserve(num_workers);
        for (int i = 0; i < num_workers; i++) {
            m_workers.emplace_back(&util::TraceThread, strprintf("%s_pool_%d", m_name, i), [this] { WorkerThread(); });
        }
    }

    /**
     * @brief Stop all worker threads and wait for them to exit.
     *
     * Sets the interrupt flag, wakes all waiting workers, and joins them.
     * Any remaining tasks in the queue will be processed before returning.
     *
     * Must be called from a controller (non-worker) thread.
     */
    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        // Notify workers and join them
        std::vector<std::thread> threads_to_join;
        {
            LOCK(m_mutex);
            // Ensure 'Stop()' isn't called from any worker thread to avoid deadlocks
            auto id = std::this_thread::get_id();
            for (const auto& worker : m_workers) assert(worker.get_id() != id);
            // Early shutdown to return right away on any concurrent 'Submit()' call
            m_interrupt = true;
            threads_to_join.swap(m_workers);
        }
        m_cv.notify_all();
        for (auto& worker : threads_to_join) worker.join();
        // Since we currently wait for tasks completion, sanity-check empty queue
        WITH_LOCK(m_mutex, Assume(m_work_queue.empty()));
        // Note: m_interrupt is left true until next Start()
    }

    /**
     * @brief Enqueues a new task for asynchronous execution.
     *
     * Returns a `std::future` that provides the task’s result or propagates
     * any exception it throws.
     * Note: Ignoring the returned future requires guarding the task against
     * uncaught exceptions, as they would otherwise be silently discarded.
     */
    template <class F> EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    auto Submit(F&& fn)
    {
        std::packaged_task task{std::forward<F>(fn)};
        auto future{task.get_future()};
        {
            LOCK(m_mutex);
            if (m_interrupt || m_workers.empty()) {
                throw std::runtime_error("No active workers; cannot accept new tasks");
            }
            m_work_queue.emplace(std::move(task));
        }
        m_cv.notify_one();
        return future;
    }

    /**
     * @brief Execute a single queued task synchronously.
     * Removes one task from the queue and executes it on the calling thread.
     */
    void ProcessTask() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::packaged_task<void()> task;
        {
            LOCK(m_mutex);
            if (m_work_queue.empty()) return;

            // Pop the task
            task = std::move(m_work_queue.front());
            m_work_queue.pop();
        }
        task();
    }

    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WITH_LOCK(m_mutex, m_interrupt = true);
        m_cv.notify_all();
    }

    size_t WorkQueueSize() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        return WITH_LOCK(m_mutex, return m_work_queue.size());
    }

    size_t WorkersCount() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        return WITH_LOCK(m_mutex, return m_workers.size());
    }
};

#endif // BITCOIN_UTIL_THREADPOOL_H
