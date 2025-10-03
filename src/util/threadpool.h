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

class ThreadPool {

private:
    std::string m_name;
    Mutex m_mutex;
    std::queue<std::function<void()>> m_work_queue GUARDED_BY(m_mutex);
    std::condition_variable m_cv;
    std::atomic<bool> m_interrupt{false};
    std::vector<std::thread> m_workers;

    void WorkerThread() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, wait_lock);
        for (;;) {
            std::function<void()> task;
            {
                // Wait only if needed; avoid sleeping when a new task was submitted while we were processing another one.
                if (!m_interrupt.load() && m_work_queue.empty()) {
                    // Block until the pool is interrupted or a task is available.
                    m_cv.wait(wait_lock,[&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_interrupt.load() || !m_work_queue.empty(); });
                }

                // If stopped and no work left, exit worker
                if (m_interrupt.load() && m_work_queue.empty()) {
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
    ThreadPool(const std::string& name) : m_name(name) {}

    ~ThreadPool()
    {
        Stop(); // In case it hasn't been stopped.
    }

    void Start(int num_workers)
    {
        if (!m_workers.empty()) throw std::runtime_error("Thread pool already started");
        m_interrupt.store(false); // Reset

        // Create workers
        for (int i = 0; i < num_workers; i++) {
            m_workers.emplace_back(&util::TraceThread, m_name + "_pool_" + util::ToString(i), [this] { WorkerThread(); });
        }
    }

    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        // Notify workers and join them.
        // Note: even when m_interrupt is atomic, it must be modified while holding the same mutex
        // used by threads waiting on the condition variable. This ensures threads blocked on m_cv
        // reliably observe the change and proceed correctly without missing signals.
        // Ref: https://en.cppreference.com/w/cpp/thread/condition_variable
        WITH_LOCK(m_mutex, m_interrupt.store(true));
        m_cv.notify_all();
        for (auto& worker : m_workers) {
            worker.join();
        }
        m_workers.clear();
        // m_interrupt is left true until next Start()
    }

    template<class T> EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    auto Submit(T task) -> std::future<decltype(task())>
    {
        if (m_workers.empty() || m_interrupt.load()) throw std::runtime_error("No active workers; cannot accept new tasks");
        using TaskType = std::packaged_task<decltype(task())()>;
        auto ptr_task = std::make_shared<TaskType>(std::move(task));
        std::future<decltype(task())> future = ptr_task->get_future();
        {
            LOCK(m_mutex);
            m_work_queue.emplace([ptr_task]() {
                (*ptr_task)();
            });
        }
        m_cv.notify_one();
        return future;
    }

    // Synchronous processing
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

    size_t WorkersCount() const
    {
        return m_workers.size();
    }
};

#endif // BITCOIN_UTIL_THREADPOOL_H
