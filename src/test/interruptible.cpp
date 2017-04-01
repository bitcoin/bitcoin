// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "interruptible/thread.h"
#include "interruptible/thread_group.h"
#include "interruptible/condition_variable.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

static constexpr std::chrono::seconds wait_time(5);

BOOST_FIXTURE_TEST_SUITE(interruptible_tests, BasicTestingSetup)

void long_loop()
{
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < wait_time) {
        interruptible::this_thread::interruption_point();
        std::this_thread::yield();
    }
}

static void long_sleep()
{
    interruptible::this_thread::sleep_for(std::chrono::seconds(wait_time));
    BOOST_CHECK(false);
}


static void condvar_wait()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    interruptible::condition_variable cond;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < wait_time)
        cond.wait(lock);
    BOOST_CHECK(false);
}

static void condvar_wait_pred()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    interruptible::condition_variable cond;
    auto start = std::chrono::steady_clock::now();
    cond.wait(lock, [&] {return std::chrono::steady_clock::now() - start >= wait_time; });
    BOOST_CHECK(false);
}

static void condvar_wait_for()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    interruptible::condition_variable cond;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < wait_time)
        cond.wait_for(lock, wait_time);
    BOOST_CHECK(false);
}

static void condvar_wait_for_pred()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    interruptible::condition_variable cond;
    auto start = std::chrono::steady_clock::now();
    cond.wait_for(lock, wait_time, [&] {return std::chrono::steady_clock::now() - start >= wait_time; });
    BOOST_CHECK(false);
}

static void condvar_wait_until()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    interruptible::condition_variable cond;
    auto start = std::chrono::steady_clock::now();
    auto end = start + wait_time;
    while (std::chrono::steady_clock::now() < end)
        cond.wait_until(lock, end);
    BOOST_CHECK(false);
}

static void condvar_wait_until_pred()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    interruptible::condition_variable cond;
    auto start = std::chrono::steady_clock::now();
    auto end = start + wait_time;
    cond.wait_until(lock, end, [&] {return std::chrono::steady_clock::now() >= end; });
    BOOST_CHECK(false);
}

static void catcher(std::function<void()>&& func)
{
    BOOST_CHECK_THROW(func(), interruptible::thread_interrupted);
}

template <typename T>
static bool run_test(T&& func)
{
    interruptible::thread runthread(catcher, std::forward<T>(func));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    runthread.interrupt();
    runthread.join();
    return true;
}

template <typename T>
static bool run_group_test(T&& func)
{
    interruptible::thread_group group;
    group.create_thread(catcher, func);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    group.create_thread(catcher, func);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    group.interrupt_all();
    group.join_all();
    return true;
}

BOOST_AUTO_TEST_CASE(interruption_point)
{
    BOOST_CHECK(run_test(long_loop));
    BOOST_CHECK(run_group_test(long_loop));
}


BOOST_AUTO_TEST_CASE(interruptible_sleep)
{
    BOOST_CHECK(run_test(long_sleep));
    BOOST_CHECK(run_group_test(long_sleep));
}

BOOST_AUTO_TEST_CASE(interruptible_condvar)
{
    BOOST_CHECK(run_test(condvar_wait));
    BOOST_CHECK(run_test(condvar_wait_pred));
    BOOST_CHECK(run_test(condvar_wait_for));
    BOOST_CHECK(run_test(condvar_wait_for_pred));
    BOOST_CHECK(run_test(condvar_wait_until));
    BOOST_CHECK(run_test(condvar_wait_until_pred));

    BOOST_CHECK(run_group_test(condvar_wait));
    BOOST_CHECK(run_group_test(condvar_wait_pred));
    BOOST_CHECK(run_group_test(condvar_wait_for));
    BOOST_CHECK(run_group_test(condvar_wait_for_pred));
    BOOST_CHECK(run_group_test(condvar_wait_until));
    BOOST_CHECK(run_group_test(condvar_wait_until_pred));
}

BOOST_AUTO_TEST_SUITE_END()
