// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <logging.h>
#include <random.h>
#include <util/string.h>
#include <util/threadpool.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

// General test values
int NUM_WORKERS_DEFAULT = 0;
constexpr char POOL_NAME[] = "test";
constexpr auto WAIT_TIMEOUT = 120s;

struct ThreadPoolFixture {
    ThreadPoolFixture() {
        NUM_WORKERS_DEFAULT = FastRandomContext().randrange(GetNumCores()) + 1;
        LogInfo("thread pool workers count: %d", NUM_WORKERS_DEFAULT);
    }
};

// Test Cases Overview
// 0) Submit task to a non-started pool.
// 1) Submit tasks and verify completion.
// 2) Maintain all threads busy except one.
// 3) Wait for work to finish.
// 4) Wait for result object.
// 5) The task throws an exception, catch must be done in the consumer side.
// 6) Busy workers, help them by processing tasks externally.
// 7) Recursive submission of tasks.
// 8) Submit task when all threads are busy, stop pool and verify task gets executed.
// 9) Congestion test; create more workers than available cores.
// 10) Ensure Interrupt() prevents further submissions.
BOOST_FIXTURE_TEST_SUITE(threadpool_tests, ThreadPoolFixture)

#define WAIT_FOR(futures)                                                         \
    do {                                                                          \
        for (const auto& f : futures) {                                           \
            BOOST_REQUIRE(f.wait_for(WAIT_TIMEOUT) == std::future_status::ready); \
        }                                                                         \
    } while (0)

// Block a number of worker threads by submitting tasks that wait on `blocker_future`.
// Returns the futures of the blocking tasks, ensuring all have started and are waiting.
std::vector<std::future<void>> BlockWorkers(ThreadPool& threadPool, std::shared_future<void>& blocker_future, int num_of_threads_to_block)
{
    // Per-thread ready promises to ensure all workers are actually blocked
    std::vector<std::promise<void>> ready_promises(num_of_threads_to_block);
    std::vector<std::future<void>> ready_futures;
    ready_futures.reserve(num_of_threads_to_block);
    for (auto& p : ready_promises) ready_futures.emplace_back(p.get_future());

    // Fill all workers with blocking tasks
    std::vector<std::future<void>> blocking_tasks;
    for (int i = 0; i < num_of_threads_to_block; i++) {
        std::promise<void>& ready = ready_promises[i];
        blocking_tasks.emplace_back(threadPool.Submit([blocker_future, &ready]() {
            ready.set_value();
            blocker_future.wait();
        }));
    }

    // Wait until all threads are actually blocked
    WAIT_FOR(ready_futures);
    return blocking_tasks;
}

// Test 0, submit task to a non-started pool
BOOST_AUTO_TEST_CASE(submit_task_before_start_fails)
{
    ThreadPool threadPool(POOL_NAME);
    BOOST_CHECK_EXCEPTION((void)threadPool.Submit([]{ return false; }), std::runtime_error, [&](const std::runtime_error& e) {
        BOOST_CHECK_EQUAL(e.what(), "No active workers; cannot accept new tasks");
        return true;
    });
}

// Test 1, submit tasks and verify completion
BOOST_AUTO_TEST_CASE(submit_tasks_complete_successfully)
{
    int num_tasks = 50;

    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::atomic<int> counter = 0;

    // Store futures to ensure completion before checking counter.
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 1; i <= num_tasks; i++) {
        futures.emplace_back(threadPool.Submit([&counter, i]() {
            counter.fetch_add(i, std::memory_order_relaxed);
        }));
    }

    // Wait for all tasks to finish
    WAIT_FOR(futures);
    int expected_value = (num_tasks * (num_tasks + 1)) / 2; // Gauss sum.
    BOOST_CHECK_EQUAL(counter.load(), expected_value);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 0);
}

// Test 2, maintain all threads busy except one
BOOST_AUTO_TEST_CASE(single_available_worker_executes_all_tasks)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    // Single blocking future for all threads
    std::promise<void> blocker;
    std::shared_future<void> blocker_future(blocker.get_future());
    const auto blocking_tasks = BlockWorkers(threadPool, blocker_future, NUM_WORKERS_DEFAULT - 1);

    // Now execute tasks on the single available worker
    // and check that all the tasks are executed.
    int num_tasks = 15;
    int counter = 0;

    // Store futures to wait on
    std::vector<std::future<void>> futures(num_tasks);
    for (auto& f : futures) f = threadPool.Submit([&counter]{ counter++; });

    WAIT_FOR(futures);
    BOOST_CHECK_EQUAL(counter, num_tasks);

    blocker.set_value();
    WAIT_FOR(blocking_tasks);
    threadPool.Stop();
    BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
}

// Test 3, wait for work to finish
BOOST_AUTO_TEST_CASE(wait_for_task_to_finish)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::atomic<bool> flag = false;
    std::future<void> future = threadPool.Submit([&flag]() {
        UninterruptibleSleep(200ms);
        flag.store(true, std::memory_order_release);
    });
    BOOST_CHECK(future.wait_for(WAIT_TIMEOUT) == std::future_status::ready);
    BOOST_CHECK(flag.load(std::memory_order_acquire));
}

// Test 4, obtain result object
BOOST_AUTO_TEST_CASE(get_result_from_completed_task)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::future<bool> future_bool = threadPool.Submit([]() { return true; });
    BOOST_CHECK(future_bool.get());

    std::future<std::string> future_str = threadPool.Submit([]() { return std::string("true"); });
    std::string result = future_str.get();
    BOOST_CHECK_EQUAL(result, "true");
}

// Test 5, throw exception and catch it on the consumer side
BOOST_AUTO_TEST_CASE(task_exception_propagates_to_future)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    int num_tasks = 5;
    std::string err_msg{"something wrong happened"};
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        futures.emplace_back(threadPool.Submit([err_msg, i]() {
            throw std::runtime_error(err_msg + util::ToString(i));
        }));
    }

    for (int i = 0; i < num_tasks; i++) {
        BOOST_CHECK_EXCEPTION(futures.at(i).get(), std::runtime_error, [&](const std::runtime_error& e) {
            BOOST_CHECK_EQUAL(e.what(), err_msg + util::ToString(i));
            return true;
        });
    }
}

// Test 6, all workers are busy, help them by processing tasks from outside
BOOST_AUTO_TEST_CASE(process_tasks_manually_when_workers_busy)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::promise<void> blocker;
    std::shared_future<void> blocker_future(blocker.get_future());
    const auto& blocking_tasks = BlockWorkers(threadPool, blocker_future, NUM_WORKERS_DEFAULT);

    // Now submit tasks and check that none of them are executed.
    int num_tasks = 20;
    std::atomic<int> counter = 0;
    for (int i = 0; i < num_tasks; i++) {
        (void)threadPool.Submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    UninterruptibleSleep(100ms);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), num_tasks);

    // Now process manually
    for (int i = 0; i < num_tasks; i++) {
        threadPool.ProcessTask();
    }
    BOOST_CHECK_EQUAL(counter.load(), num_tasks);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 0);
    blocker.set_value();
    threadPool.Stop();
    WAIT_FOR(blocking_tasks);
}

// Test 7, submit tasks from other tasks
BOOST_AUTO_TEST_CASE(recursive_task_submission)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::promise<void> signal;
    (void)threadPool.Submit([&]() {
        (void)threadPool.Submit([&]() {
            signal.set_value();
        });
    });

    signal.get_future().wait();
    threadPool.Stop();
}

// Test 8, submit task when all threads are busy and then stop the pool
BOOST_AUTO_TEST_CASE(task_submitted_while_busy_completes)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::promise<void> blocker;
    std::shared_future<void> blocker_future(blocker.get_future());
    const auto& blocking_tasks = BlockWorkers(threadPool, blocker_future, NUM_WORKERS_DEFAULT);

    // Submit an extra task that should execute once a worker is free
    std::future<bool> future = threadPool.Submit([]() { return true; });

    // At this point, all workers are blocked, and the extra task is queued
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 1);

    // Wait a short moment before unblocking the threads to mimic a concurrent shutdown
    std::thread thread_unblocker([&blocker]() {
        UninterruptibleSleep(300ms);
        blocker.set_value();
    });

    // Stop the pool while the workers are still blocked
    threadPool.Stop();

    // Expect the submitted task to complete
    BOOST_CHECK(future.get());
    thread_unblocker.join();

    // Obviously all the previously blocking tasks should be completed at this point too
    WAIT_FOR(blocking_tasks);

    // Pool should be stopped and no workers remaining
    BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
}

// Test 9, more workers than available cores (congestion test)
BOOST_AUTO_TEST_CASE(congestion_more_workers_than_cores)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(std::max(1, GetNumCores() * 2)); // Oversubscribe by 2×

    int num_tasks = 200;
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        futures.emplace_back(threadPool.Submit([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    WAIT_FOR(futures);
    BOOST_CHECK_EQUAL(counter.load(), num_tasks);
}

// Test 10, Interrupt() prevents further submissions
BOOST_AUTO_TEST_CASE(interrupt_blocks_new_submissions)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    threadPool.Interrupt();
    BOOST_CHECK_EXCEPTION((void)threadPool.Submit([]{}), std::runtime_error, [&](const std::runtime_error& e) {
        BOOST_CHECK_EQUAL(e.what(), "No active workers; cannot accept new tasks");
        return true;
    });
}

BOOST_AUTO_TEST_SUITE_END()
