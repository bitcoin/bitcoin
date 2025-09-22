// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <logging.h>
#include <util/threadpool.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <iostream>

struct ExpectedException : std::runtime_error {
    ExpectedException(const std::string &msg) : std::runtime_error(msg) {}
};
struct ThrowTask {
    void operator()() const { throw ExpectedException("fail"); }
};

struct CounterTask {
    std::atomic_uint32_t& m_counter;
    explicit CounterTask(std::atomic_uint32_t& counter) : m_counter{counter} {}
    void operator()() const { m_counter.fetch_add(1); }
};

// Increases 'fail_counter' if the expected exception is thrown
static void get_future(std::future<void>& future, uint32_t& fail_counter) {
    try {
        future.get();
    } catch (const ExpectedException&) {
        fail_counter++;
    } catch (...) {
        assert(false);
    }
}

static void setup_threadpool_test()
{
    // Disable logging entirely. It seems to cause memory leaks.
    LogInstance().DisableLogging();
}

FUZZ_TARGET(threadpool, .init = setup_threadpool_test)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const uint32_t num_tasks = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 1024);
    const uint32_t num_workers = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, 4);
    if (GetNumCores() < (int) num_workers) {
        std::cout << "WARNING: Running more threads than available cores" << std::endl;
    }
    ThreadPool pool{"fuzz"};
    pool.Start(num_workers);
    assert(pool.WorkersCount() == num_workers);
    assert(pool.WorkQueueSize() == 0);

    // Counters
    std::atomic_uint32_t task_counter{0};
    uint32_t fail_counter{0};
    uint32_t expected_task_counter{0};
    uint32_t expected_fail_tasks{0};

    std::queue<std::future<void>> futures;
    for (uint32_t i = 0; i < num_tasks; ++i) {
        bool will_throw = fuzzed_data_provider.ConsumeBool();
        bool wait_immediately = fuzzed_data_provider.ConsumeBool();

        std::future<void> fut;
        if (will_throw) {
            expected_fail_tasks++;
            fut = pool.Submit(ThrowTask{});
        } else {
            expected_task_counter++;
            fut = pool.Submit(CounterTask{task_counter});
        }

        // If caller wants to wait immediately, consume the future here (safe).
        if (wait_immediately) {
            // This waits for this task to complete immediately; prior queued tasks may also complete
            // as they were queued earlier.
            get_future(fut, fail_counter);
        } else {
            // Store task for a posterior check
            futures.emplace(std::move(fut));
        }
    }

    // Drain remaining futures
    while (!futures.empty()) {
        auto fut = std::move(futures.front());
        futures.pop();
        get_future(fut, fail_counter);
    }

    assert(pool.WorkQueueSize() == 0);
    assert(task_counter.load() == expected_task_counter);
    assert(fail_counter == expected_fail_tasks);
    pool.Stop();
}
