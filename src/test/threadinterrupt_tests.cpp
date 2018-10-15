// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <threadinterrupt.h>

#include <boost/test/unit_test.hpp>
#include <future>
#include <random.h>

BOOST_AUTO_TEST_SUITE(threadinterrupt_tests)

BOOST_AUTO_TEST_CASE(test_InterruptibleSleep)
{
    std::packaged_task<bool()> task([] {
        try {
            InterruptibleSleep(std::chrono::seconds(20));
        } catch (const ThreadInterrupted&) {
            return true;
        }
        return false;
    });
    std::future<bool> interrupted = task.get_future();
    InterruptibleThread thread(std::move(task));
    thread.interrupt();
    BOOST_CHECK(interrupted.wait_for(std::chrono::seconds(10)) == std::future_status::ready); // Wait at most 10 seconds
    thread.join();
    BOOST_CHECK(interrupted.get());
}

BOOST_AUTO_TEST_CASE(test_InterruptionPoint)
{
    std::promise<void> promise; // Make sure we call InterruptionPoint after interrupted
    std::packaged_task<bool(std::promise<void>&)> task([] (std::promise<void>& promise) {
        try {
            promise.get_future().get();
            InterruptionPoint();
        } catch (const ThreadInterrupted&) {
            return true;
        }
        return false;
    });
    std::future<bool> interrupted = task.get_future();
    InterruptibleThread thread(std::move(task), std::ref(promise));
    thread.interrupt();
    promise.set_value();
    BOOST_CHECK(interrupted.wait_for(std::chrono::seconds(10)) == std::future_status::ready); // Wait at most 10 seconds
    thread.join();
    BOOST_CHECK(interrupted.get());
}

BOOST_AUTO_TEST_CASE(test_InterruptionPoint_std_thread)
{
    std::packaged_task<bool()> task([] {
        try {
            InterruptionPoint();
        } catch (const ThreadInterrupted&) {
            return false;
        }
        return true;
    });
    std::future<bool> interrupted = task.get_future();
    std::thread thread(std::move(task));
    BOOST_CHECK(interrupted.wait_for(std::chrono::seconds(10)) == std::future_status::ready); // Wait at most 10 seconds
    thread.join();
    BOOST_CHECK(interrupted.get());
}

BOOST_AUTO_TEST_CASE(test_InterruptibleSleep_std_thread)
{
    std::packaged_task<bool()> task([] {
        try {
            InterruptibleSleep(std::chrono::milliseconds(100));
        } catch (const ThreadInterrupted&) {
            return false;
        }
        return true;
    });
    std::future<bool> interrupted = task.get_future();
    std::thread thread(std::move(task));
    BOOST_CHECK(interrupted.wait_for(std::chrono::seconds(10)) == std::future_status::ready); // Wait at most 10 seconds
    thread.join();
    BOOST_CHECK(interrupted.get());
}

BOOST_AUTO_TEST_CASE(test_InterruptibleThread_args)
{
    std::packaged_task<int(int, int, int)> task([](int a, int b, int c) {
        try {
            InterruptibleSleep(std::chrono::seconds(20));
        } catch (const ThreadInterrupted&) {
            return a + b + c;
        }
        return -1;
    });
    std::future<int> result = task.get_future();
    int a = GetRandInt(100);
    int b = GetRandInt(100);
    int c = GetRandInt(100);
    InterruptibleThread thread(std::move(task), a, b, c);
    thread.interrupt();
    BOOST_CHECK(result.wait_for(std::chrono::seconds(10)) == std::future_status::ready); // Wait at most 10 seconds
    thread.join();
    BOOST_CHECK_EQUAL(result.get(), a + b + c);
}

BOOST_AUTO_TEST_SUITE_END()
