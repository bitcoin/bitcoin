// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>
#include <util/threadpool.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(threadpool_tests)

constexpr auto TIMEOUT_SECS = std::chrono::seconds(120);

template <typename T>
void WaitFor(const std::vector<std::future<T>>& futures, const std::string& context)
{
    for (size_t i = 0; i < futures.size(); ++i) {
        if (futures[i].wait_for(TIMEOUT_SECS) != std::future_status::ready) {
            throw std::runtime_error("Timeout waiting for: " + context + ", task index " + util::ToString(i));
        }
    }
}

// Block a number of worker threads by submitting tasks that wait on `blocker_future`.
// Returns the futures of the blocking tasks, ensuring all have started and are waiting.
std::vector<std::future<void>> BlockWorkers(ThreadPool& threadPool, std::shared_future<void>& blocker_future, int num_of_threads_to_block, const std::string& context) {
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
    WaitFor(ready_futures, context);
    return blocking_tasks;
}

BOOST_AUTO_TEST_CASE(threadpool_basic)
{
    // Test Cases
    // 0) Submit task to a non-started pool.
    // 1) Submit tasks and verify completion.
    // 2) Maintain all threads busy except one.
    // 3) Wait for work to finish.
    // 4) Wait for result object.
    // 5) The task throws an exception, catch must be done in the consumer side.
    // 6) Busy workers, help them by processing tasks from outside.
    // 7) Recursive submission of tasks.
    // 8) Submit task when all threads are busy, stop pool and verify the task gets executed.

    const int NUM_WORKERS_DEFAULT = 3;
    const std::string POOL_NAME = "test";

    // Test case 0, submit task to a non-started pool
    {
        ThreadPool threadPool(POOL_NAME);
        bool err = false;
        try {
            threadPool.Submit([]() { return false; });
        } catch (const std::runtime_error&) { err = true; }
        BOOST_CHECK(err);
    }

    // Test case 1, submit tasks and verify completion.
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
                counter.fetch_add(i);
            }));
        }

        // Wait for all tasks to finish
        WaitFor(futures, /*context=*/"test1 task");
        int expected_value = (num_tasks * (num_tasks + 1)) / 2; // Gauss sum.
        BOOST_CHECK_EQUAL(counter.load(), expected_value);
        BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 0);
    }

    // Test case 2, maintain all threads busy except one.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);
        // Single blocking future for all threads
        std::promise<void> blocker;
        std::shared_future<void> blocker_future(blocker.get_future());
        const auto& blocking_tasks = BlockWorkers(threadPool, blocker_future, NUM_WORKERS_DEFAULT-1, /*context=*/"test2 blocking tasks enabled");

        // Now execute tasks on the single available worker
        // and check that all the tasks are executed.
        int num_tasks = 15;
        std::atomic<int> counter = 0;

        // Store futures to wait on
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);
        for (int i = 0; i < num_tasks; i++) {
            futures.emplace_back(threadPool.Submit([&counter]() {
                counter.fetch_add(1);
            }));
        }

        WaitFor(futures, /*context=*/"test2 tasks");
        BOOST_CHECK_EQUAL(counter.load(), num_tasks);

        blocker.set_value();
        WaitFor(blocking_tasks, /*context=*/"test2 blocking tasks disabled");
        threadPool.Stop();
        BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
    }

    // Test case 3, wait for work to finish.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<bool> flag = false;
        std::future<void> future = threadPool.Submit([&flag]() {
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            flag.store(true);
        });
        future.wait();
        BOOST_CHECK(flag.load());
    }

    // Test case 4, obtain result object.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::future<bool> future_bool = threadPool.Submit([]() {
            return true;
        });
        BOOST_CHECK(future_bool.get());

        std::future<std::string> future_str = threadPool.Submit([]() {
            return std::string("true");
        });
        std::string result = future_str.get();
        BOOST_CHECK_EQUAL(result, "true");
    }

    // Test case 5, throw exception and catch it on the consumer side.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);

        int ROUNDS = 5;
        std::string err_msg{"something wrong happened"};
        std::vector<std::future<void>> futures;
        futures.reserve(ROUNDS);
        for (int i = 0; i < ROUNDS; i++) {
            futures.emplace_back(threadPool.Submit([err_msg, i]() {
                throw std::runtime_error(err_msg + util::ToString(i));
            }));
        }

        for (int i = 0; i < ROUNDS; i++) {
            try {
                futures.at(i).get();
                BOOST_FAIL("Expected exception not thrown");
            } catch (const std::runtime_error& e) {
                BOOST_CHECK_EQUAL(e.what(), err_msg + util::ToString(i));
            }
        }
    }

    // Test case 6, all workers are busy, help them by processing tasks from outside.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);

        std::promise<void> blocker;
        std::shared_future<void> blocker_future(blocker.get_future());
        const auto& blocking_tasks = BlockWorkers(threadPool, blocker_future, NUM_WORKERS_DEFAULT, /*context=*/"test6 blocking tasks enabled");

        // Now submit tasks and check that none of them are executed.
        int num_tasks = 20;
        std::atomic<int> counter = 0;
        for (int i = 0; i < num_tasks; i++) {
            threadPool.Submit([&counter]() {
                counter.fetch_add(1);
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 20);

        // Now process manually
        for (int i = 0; i < num_tasks; i++) {
            threadPool.ProcessTask();
        }
        BOOST_CHECK_EQUAL(counter.load(), num_tasks);
        blocker.set_value();
        threadPool.Stop();
        WaitFor(blocking_tasks, "Failure waiting for test6 blocking task futures");
    }

    // Test case 7, recursive submission of tasks.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);

        std::promise<void> signal;
        threadPool.Submit([&]() {
            threadPool.Submit([&]() {
                signal.set_value();
            });
        });

        signal.get_future().wait();
        threadPool.Stop();
    }

    // Test case 8, submit a task when all threads are busy and then stop the pool.
    {
        ThreadPool threadPool(POOL_NAME);
        threadPool.Start(NUM_WORKERS_DEFAULT);

        std::promise<void> blocker;
        std::shared_future<void> blocker_future(blocker.get_future());
        const auto& blocking_tasks = BlockWorkers(threadPool, blocker_future, NUM_WORKERS_DEFAULT, /*context=*/"test8 blocking tasks enabled");

        // Submit an extra task that should execute once a worker is free
        std::future<bool> future = threadPool.Submit([]() { return true; });

        // At this point, all workers are blocked, and the extra task is queued
        BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 1);

        // Wait a short moment before unblocking the threads to mimic a concurrent shutdown
        std::thread thread_unblocker([&blocker]() {
            std::this_thread::sleep_for(std::chrono::milliseconds{300});
            blocker.set_value();
        });

        // Stop the pool while the workers are still blocked
        threadPool.Stop();

        // Expect the submitted task to complete
        BOOST_CHECK(future.get());
        thread_unblocker.join();

        // Obviously all the previously blocking tasks should be completed at this point too
        WaitFor(blocking_tasks, "Failure waiting for test8 blocking task futures");

        // Pool should be stopped and no workers remaining
        BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
