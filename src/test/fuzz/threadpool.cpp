// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <util/threadpool.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <atomic>
#include <future>
#include <queue>

struct ExpectedException : std::runtime_error {
    explicit ExpectedException(const std::string& msg) : std::runtime_error(msg) {}
};

struct ThrowTask {
    void operator()() const { throw ExpectedException("fail"); }
};

struct CounterTask {
    std::atomic_uint32_t& m_counter;
    explicit CounterTask(std::atomic_uint32_t& counter) : m_counter{counter} {}
    void operator()() const { m_counter.fetch_add(1, std::memory_order_relaxed); }
};

// Waits for a future to complete. Increments 'fail_counter' if the expected exception is thrown.
static void GetFuture(std::future<void>& future, uint32_t& fail_counter)
{
    try {
        future.get();
    } catch (const ExpectedException&) {
        fail_counter++;
    } catch (...) {
        assert(false && "Unexpected exception type");
    }
}

// Global thread pool for fuzzing. Persisting it across iterations prevents
// the excessive thread creation/destruction overhead that can lead to
// instability in the fuzzing environment.
// This is also how we use it in the app's lifecycle.
ThreadPool g_pool{"fuzz"};
Mutex g_pool_mutex;
// Global to verify we always have the same number of threads.
size_t g_num_workers = 3;

static void StartPoolIfNeeded() EXCLUSIVE_LOCKS_REQUIRED(!g_pool_mutex)
{
    LOCK(g_pool_mutex);
    if (g_pool.WorkersCount() == g_num_workers) return;
    g_pool.Start(g_num_workers);
}

static void setup_threadpool_test()
{
    // Disable logging entirely. It seems to cause memory leaks.
    LogInstance().DisableLogging();
}

FUZZ_TARGET(threadpool, .init = setup_threadpool_test) EXCLUSIVE_LOCKS_REQUIRED(!g_pool_mutex)
{
    // Because LibAFL calls fork() after calling the init setup function,
    // the child processes end up having one thread active and no workers.
    // To work around this limitation, start thread pool inside the first runner.
    StartPoolIfNeeded();

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const uint32_t num_tasks = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 1024);
    assert(g_pool.WorkersCount() == g_num_workers);
    assert(g_pool.WorkQueueSize() == 0);

    // Counters
    std::atomic_uint32_t task_counter{0};
    uint32_t fail_counter{0};
    uint32_t expected_task_counter{0};
    uint32_t expected_fail_tasks{0};

    std::queue<std::future<void>> futures;
    for (uint32_t i = 0; i < num_tasks; ++i) {
        const bool will_throw = fuzzed_data_provider.ConsumeBool();
        const bool wait_immediately = fuzzed_data_provider.ConsumeBool();

        std::future<void> fut;
        if (will_throw) {
            expected_fail_tasks++;
            fut = g_pool.Submit(ThrowTask{});
        } else {
            expected_task_counter++;
            fut = g_pool.Submit(CounterTask{task_counter});
        }

        // If caller wants to wait immediately, consume the future here (safe).
        if (wait_immediately) {
            // Waits for this task to complete immediately; prior queued tasks may also complete
            // as they were queued earlier.
            GetFuture(fut, fail_counter);
        } else {
            // Store task for a posterior check
            futures.emplace(std::move(fut));
        }
    }

    // Drain remaining futures
    while (!futures.empty()) {
        auto fut = std::move(futures.front());
        futures.pop();
        GetFuture(fut, fail_counter);
    }

    assert(g_pool.WorkQueueSize() == 0);
    assert(task_counter.load() == expected_task_counter);
    assert(fail_counter == expected_fail_tasks);
}
