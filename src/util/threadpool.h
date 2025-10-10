// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_THREADPOOL_H
#define BITCOIN_UTIL_THREADPOOL_H

#include <sync.h>
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
#include <stdexcept>
#include <utility>
#include <queue>
#include <thread>
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
class ThreadPool {

private:
    std::string m_name;
    Mutex m_mutex;
    std::queue<std::function<void()>> m_work_queue GUARDED_BY(m_mutex);
    std::condition_variable m_cv;
    // Note: m_interrupt must be modified while holding the same mutex used by threads waiting on the condition variable.
    // This ensures threads blocked on m_cv reliably observe the change and proceed correctly without missing signals.
    // Ref: https://en.cppreference.com/w/cpp/thread/condition_variable
    bool m_interrupt GUARDED_BY(m_mutex){false};
    std::vector<std::thread> m_workers GUARDED_BY(m_mutex);

    void WorkerThread() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, wait_lock);
        for (;;) {
            std::function<void()> task;
            {
                // Wait only if needed; avoid sleeping when a new task was submitted while we were processing another one.
                if (!m_interrupt && m_work_queue.empty()) {
                    // Block until the pool is interrupted or a task is available.
                    m_cv.wait(wait_lock,[&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_interrupt || !m_work_queue.empty(); });
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
        for (int i = 0; i < num_workers; i++) {
            m_workers.emplace_back(&util::TraceThread, m_name + "_pool_" + util::ToString(i), [this] { WorkerThread(); });
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
        // Notify workers and join them.
        std::vector<std::thread> threads_to_join;
        {
            LOCK(m_mutex);
            m_interrupt = true;
            threads_to_join.swap(m_workers);
        }
        m_cv.notify_all();
        for (auto& worker : threads_to_join) worker.join();
        {
            // Sanity cleanup: release any std::function captured shared_ptrs
            LOCK(m_mutex);
            std::queue<std::function<void()>> empty;
            m_work_queue.swap(empty);
        }
        // Note: m_interrupt is left true until next Start()
    }

    /**
     * @brief Submit a new task for asynchronous execution.
     *
     * Enqueues a callable to be executed by one of the worker threads.
     * Returns a `std::future` that can be used to retrieve the task’s result.
     */
    template<class T> EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    auto Submit(T task) -> std::future<decltype(task())>
    {
        using TaskType = std::packaged_task<decltype(task())()>;
        auto ptr_task = std::make_shared<TaskType>(std::move(task));
        std::future<decltype(task())> future = ptr_task->get_future();
        {
            LOCK(m_mutex);
            if (m_workers.empty() || m_interrupt) {
                throw std::runtime_error("No active workers; cannot accept new tasks");
            }
            m_work_queue.emplace([ptr_task]() mutable {
                (*ptr_task)();
                ptr_task.reset(); // Explicitly release packaged_task and the stored function obj.
            });
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
        std::function<void()> task;
        {
            LOCK(m_mutex);
            if (m_work_queue.empty()) return;

            // Pop the task
            task = std::move(m_work_queue.front());
            m_work_queue.pop();
        }
        task();
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
