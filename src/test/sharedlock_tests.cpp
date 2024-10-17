// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <thread>

BOOST_AUTO_TEST_SUITE(sharedlock_tests)

void run_task_in_parallel(const std::function<void()>& task, int num_threads)
{
    std::vector<std::thread> workers;
    for (int i=0; i<num_threads; i++) {
        workers.emplace_back(std::thread(task));
    }
    std::for_each(workers.begin(), workers.end(), [](std::thread &t) { t.join(); });
}

BOOST_AUTO_TEST_CASE(sharedlock_basics)
{
    SharedMutex mutex;
    {
        // 1) Acquire shared lock and check that the writer lock cannot be acquired
        LOCK_SHARED(mutex);
        BOOST_CHECK(!mutex.try_lock());
    }

    {
        // 2) Acquire exclusive lock and check that the reader lock cannot be acquired
        LOCK(mutex);
        BOOST_CHECK(!mutex.try_lock_shared());
    }

    {
        // 3) Acquire exclusive lock and verify that no other thread can acquire it.
        LOCK(mutex);
        run_task_in_parallel([&](){
            assert(!mutex.try_lock());
            assert(!mutex.try_lock_shared());
        }, /*num_threads=*/10);
    }

    {
        // 4) Acquire shared lock and verify that multiple reader threads can acquire the shared mutex at the same time.
        LOCK_SHARED(mutex);
        run_task_in_parallel([&](){
            LOCK_SHARED(mutex);
            assert(!mutex.try_lock());
        }, /*num_threads=*/10);
    }
}

BOOST_AUTO_TEST_SUITE_END()
