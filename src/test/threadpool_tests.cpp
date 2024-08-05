// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/threadpool.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(threadpool_tests)

BOOST_AUTO_TEST_CASE(threadpool_basic)
{
    // Test Cases
    // 1) Submit tasks and verify completion.
    // 2) Maintain all threads busy except one.
    // 3) Wait for work to finish.
    // 4) Wait for result object.
    // 5) The task throws an exception, catch must be done in the consumer side.
    // 6) Busy workers, help them by processing tasks from outside.

    const int NUM_WORKERS_DEFAULT = 3;

    // Test case 1, submit tasks and verify completion.
    {
        int num_tasks = 50;

        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<int> counter = 0;
        for (int i = 1; i < num_tasks + 1; i++) {
            threadPool.Submit([&counter, i]() {
                counter += i;
            });
        }

        while (threadPool.WorkQueueSize() != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
        int expected_value = (num_tasks * (num_tasks + 1)) / 2; // Gauss sum.
        BOOST_CHECK_EQUAL(counter, expected_value);
    }

    // Test case 2, maintain all threads busy except one.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<bool> stop = false;
        for (int i=0; i < NUM_WORKERS_DEFAULT - 1; i++) {
            threadPool.Submit([&stop]() {
                while (!stop) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{10});
                }
            });
        }

        // Now execute tasks on the single available worker
        // and check that all the tasks are executed.
        int num_tasks = 15;
        std::atomic<int> counter = 0;
        for (int i=0; i < num_tasks; i++) {
            threadPool.Submit([&counter]() {
                counter++;
            });
        }
        while (threadPool.WorkQueueSize() != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
        BOOST_CHECK_EQUAL(counter, num_tasks);

        stop = true;
        threadPool.Stop();
    }

    // Test case 3, wait for work to finish.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<bool> flag = false;
        std::future<void> future = threadPool.Submit([&flag]() {
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            flag = true;
        });
        future.get();
        BOOST_CHECK(flag.load());
    }

    // Test case 4, wait for result object.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::future<bool> future_bool = threadPool.Submit([]() {
            return true;
        });
        BOOST_CHECK(future_bool.get());

        std::future<std::string> future_str = threadPool.Submit([]() {
            return std::string("true");
        });
        BOOST_CHECK_EQUAL(future_str.get(), "true");
    }

    // Test case 5
    // Throw exception and catch it on the consumer side.
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);

        int ROUNDS = 5;
        std::string err_msg{"something wrong happened"};
        std::vector<std::future<void>> futures;
        futures.reserve(ROUNDS);
        for (int i = 0; i < ROUNDS; i++) {
            futures.emplace_back(threadPool.Submit([err_msg, i]() {
                throw std::runtime_error(err_msg + ToString(i));
            }));
        }

        for (int i = 0; i < ROUNDS; i++) {
            try {
                futures.at(i).get();
                BOOST_ASSERT_MSG(false, "Error: did not throw an exception");
            } catch (const std::runtime_error& e) {
                BOOST_CHECK_EQUAL(e.what(), err_msg + ToString(i));
            }
        }
    }

    // Test case 6
    // All workers are busy, help them by processing tasks from outside
    {
        ThreadPool threadPool;
        threadPool.Start(NUM_WORKERS_DEFAULT);
        std::atomic<bool> stop = false;
        for (int i=0; i < NUM_WORKERS_DEFAULT; i++) {
            threadPool.Submit([&stop]() {
                while (!stop) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{10});
                }
            });
        }

        // Now submit tasks and check that none of them are executed.
        int num_tasks = 20;
        std::atomic<int> counter = 0;
        for (int i=0; i < num_tasks; i++) {
            threadPool.Submit([&counter]() {
                counter++;
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 20);

        // Now process them manually
        for (int i=0; i<num_tasks; i++) {
            threadPool.ProcessTask();
        }
        BOOST_CHECK_EQUAL(counter, num_tasks);
        stop = true;
        threadPool.Stop();
    }

}

BOOST_AUTO_TEST_SUITE_END()
