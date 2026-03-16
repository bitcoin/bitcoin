// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_THREADPOOL_H
#define BITCOIN_UTIL_THREADPOOL_H

#include <sync.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/expected.h>
#include <util/thread.h>

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <ranges>
#include <thread>
#include <type_traits>
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
 * - `Interrupt()` stops new task submission and lets queued ones drain
 *   in the background. Callers can continue other shutdown steps and call
 *   Stop() at the end to ensure no remaining tasks are left to execute.
 *
 * - `Stop()` prevents further task submission and blocks until all the
 *   queued ones are completed.
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
        if (m_interrupt) throw std::runtime_error("Thread pool has been interrupted or is stopping");
        if (!m_workers.empty()) throw std::runtime_error("Thread pool already started");

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
     * Concurrent calls to Start() will be rejected while Stop() is in progress.
     */
    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        // Notify workers and join them
        std::vector<std::thread> threads_to_join;
        {
            LOCK(m_mutex);
            // Ensure Stop() is not called from a worker thread while workers are still registered,
            // otherwise a self-join deadlock would occur.
            auto id = std::this_thread::get_id();
            for (const auto& worker : m_workers) assert(worker.get_id() != id);
            // Early shutdown to return right away on any concurrent Submit() call
            m_interrupt = true;
            threads_to_join.swap(m_workers);
        }
        m_cv.notify_all();
        // Help draining queue
        while (ProcessTask()) {}
        // Free resources
        for (auto& worker : threads_to_join) worker.join();

        // Since we currently wait for tasks completion, sanity-check empty queue
        LOCK(m_mutex);
        Assume(m_work_queue.empty());
        // Re-allow Start() now that all workers have exited
        m_interrupt = false;
    }

    enum class SubmitError {
        Inactive,
        Interrupted,
    };

    template <class F>
    using Future = std::future<std::invoke_result_t<F>>;

    template <class R>
    using RangeFuture = Future<std::ranges::range_reference_t<R>>;

    template <class F>
    using PackagedTask = std::packaged_task<std::invoke_result_t<F>()>;

    /**
     * @brief Enqueues a new task for asynchronous execution.
     *
     * @param  fn Callable to execute asynchronously.
     * @return On success, a future containing fn's result.
     *         On failure, an error indicating why the task was rejected:
     *         - SubmitError::Inactive: Pool has no workers (never started or already stopped).
     *         - SubmitError::Interrupted: Pool task acceptance has been interrupted.
     *
     * Thread-safe: Can be called from any thread, including within the provided 'fn' callable.
     *
     * @warning Ignoring the returned future requires guarding the task against
     *          uncaught exceptions, as they would otherwise be silently discarded.
     */
    template <class F>
    [[nodiscard]] util::Expected<Future<F>, SubmitError> Submit(F&& fn) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        PackagedTask<F> task{std::forward<F>(fn)};
        auto future{task.get_future()};
        {
            LOCK(m_mutex);
            if (m_workers.empty()) return util::Unexpected{SubmitError::Inactive};
            if (m_interrupt) return util::Unexpected{SubmitError::Interrupted};

            m_work_queue.emplace(std::move(task));
        }
        m_cv.notify_one();
        return {std::move(future)};
    }

    /**
     * @brief Enqueues a range of tasks for asynchronous execution.
     *
     * @param  fns Callables to execute asynchronously.
     * @return On success, a vector of futures containing each element of fns's result in order.
     *         On failure, an error indicating why the range was rejected:
     *         - SubmitError::Inactive: Pool has no workers (never started or already stopped).
     *         - SubmitError::Interrupted: Pool task acceptance has been interrupted.
     *
     * This is more efficient when submitting many tasks at once, since
     * the queue lock is only taken once internally and all worker threads are
     * notified. For single tasks, Submit() is preferred since only one worker
     * thread is notified.
     *
     * Thread-safe: Can be called from any thread, including within submitted callables.
     *
     * @warning Ignoring the returned futures requires guarding tasks against
     *          uncaught exceptions, as they would otherwise be silently discarded.
     */
    template <std::ranges::sized_range R>
        requires(!std::is_lvalue_reference_v<R>)
    [[nodiscard]] util::Expected<std::vector<RangeFuture<R>>, SubmitError> Submit(R&& fns) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::vector<RangeFuture<R>> futures;
        futures.reserve(std::ranges::size(fns));

        {
            LOCK(m_mutex);
            if (m_workers.empty()) return util::Unexpected{SubmitError::Inactive};
            if (m_interrupt) return util::Unexpected{SubmitError::Interrupted};
            for (auto&& fn : fns) {
                PackagedTask<std::ranges::range_reference_t<R>> task{std::move(fn)};
                futures.emplace_back(task.get_future());
                m_work_queue.emplace(std::move(task));
            }
        }
        m_cv.notify_all();
        return {std::move(futures)};
    }

    /**
     * @brief Execute a single queued task synchronously.
     * Removes one task from the queue and executes it on the calling thread.
     */
    bool ProcessTask() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::packaged_task<void()> task;
        {
            LOCK(m_mutex);
            if (m_work_queue.empty()) return false;

            // Pop the task
            task = std::move(m_work_queue.front());
            m_work_queue.pop();
        }
        task();
        return true;
    }

    /**
     * @brief Stop accepting new tasks and begin asynchronous shutdown.
     *
     * Wakes all worker threads so they can drain the queue and exit.
     * Unlike Stop(), this function does not wait for threads to finish.
     *
     * Note: The next step in the pool lifecycle is calling Stop(), which
     *       releases any dangling resources and resets the pool state
     *       for shutdown or restart.
     */
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

constexpr std::string_view SubmitErrorString(const ThreadPool::SubmitError err) noexcept {
    switch (err) {
        case ThreadPool::SubmitError::Inactive:
            return "No active workers";
        case ThreadPool::SubmitError::Interrupted:
            return "Interrupted";
    }
    Assume(false); // Unreachable
    return "Unknown error";
}

#endif // BITCOIN_UTIL_THREADPOOL_H
