// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <btcsignals.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <semaphore>

namespace {


struct MoveOnlyData {
    MoveOnlyData(int data) : m_data(data) {}
    MoveOnlyData(MoveOnlyData&&) = default;

    MoveOnlyData& operator=(MoveOnlyData&&) = delete;
    MoveOnlyData(const MoveOnlyData&) = delete;
    MoveOnlyData& operator=(const MoveOnlyData&) = delete;

    int m_data;
};

MoveOnlyData MoveOnlyReturnCallback(int val)
{
    return {val};
}

void IncrementCallback(int& val)
{
    val++;
}
void SquareCallback(int& val)
{
    val *= val;
}

bool ReturnTrue()
{
    return true;
}
bool ReturnFalse()
{
    return false;
}

} // anonymous namespace

BOOST_FIXTURE_TEST_SUITE(btcsignals_tests, BasicTestingSetup)

/* Callbacks should always be executed in the order in which they were added
 */
BOOST_AUTO_TEST_CASE(callback_order)
{
    btcsignals::signal<void(int&)> sig0;
    sig0.connect(IncrementCallback);
    sig0.connect(SquareCallback);
    int val{3};
    sig0(val);
    BOOST_CHECK_EQUAL(val, 16);
    BOOST_CHECK(!sig0.empty());
}

BOOST_AUTO_TEST_CASE(disconnects)
{
    btcsignals::signal<void(int&)> sig0;
    auto conn0 = sig0.connect(IncrementCallback);
    auto conn1 = sig0.connect(SquareCallback);
    conn1.disconnect();
    BOOST_CHECK(!sig0.empty());
    int val{3};
    sig0(val);
    BOOST_CHECK_EQUAL(val, 4);

    BOOST_CHECK(!sig0.empty());
    conn0.disconnect();
    BOOST_CHECK(sig0.empty());
    sig0(val);
    BOOST_CHECK_EQUAL(val, 4);

    conn0 = sig0.connect(IncrementCallback);
    conn1 = sig0.connect(IncrementCallback);
    BOOST_CHECK(!sig0.empty());
    sig0(val);
    BOOST_CHECK_EQUAL(val, 6);
    conn1.disconnect();

    BOOST_CHECK(conn0.connected());
    {
        btcsignals::scoped_connection scope(conn0);
    }
    BOOST_CHECK(!conn0.connected());
    BOOST_CHECK(sig0.empty());
    sig0(val);
    BOOST_CHECK_EQUAL(val, 6);
}

/* Check that move-only return types work correctly
 */
BOOST_AUTO_TEST_CASE(moveonly_return)
{
    btcsignals::signal<MoveOnlyData(int)> sig0;
    sig0.connect(MoveOnlyReturnCallback);
    int data{3};
    auto ret = sig0(data);
    BOOST_CHECK_EQUAL(ret->m_data, 3);
}

/* The result of the signal invocation should always be the result of the last
 * enabled callback.
 */
BOOST_AUTO_TEST_CASE(return_value)
{
    btcsignals::signal<bool()> sig0;
    decltype(sig0)::result_type ret;
    ret = sig0();
    BOOST_CHECK(!ret);
    {
        btcsignals::scoped_connection conn0 = sig0.connect(ReturnTrue);
        ret = sig0();
        BOOST_CHECK(ret && *ret == true);
    }
    ret = sig0();
    BOOST_CHECK(!ret);
    {
        btcsignals::scoped_connection conn1 = sig0.connect(ReturnTrue);
        btcsignals::scoped_connection conn0 = sig0.connect(ReturnFalse);
        ret = sig0();
        BOOST_CHECK(ret && *ret == false);
        conn0.disconnect();
        ret = sig0();
        BOOST_CHECK(ret && *ret == true);
    }
    ret = sig0();
    BOOST_CHECK(!ret);
}

/* Test the thread-safety of connect/disconnect/empty/connected/callbacks.
 * Connect sig0 to an incrementor function and loop in a thread.
 * Meanwhile, in another thread, inject and call new increment callbacks.
 * Both threads are constantly calling empty/connected.
 * Though the end-result is undefined due to a non-deterministic number of
 * total callbacks executed, this should all be completely threadsafe.
 * Sanitizers should pick up any buggy data race behavior (if present).
 */
BOOST_AUTO_TEST_CASE(thread_safety)
{
    btcsignals::signal<void()> sig0;
    std::atomic<uint32_t> val{0};
    auto conn0 = sig0.connect([&val] {
        val++;
    });

    std::thread incrementor([&conn0, &sig0] {
        for (int i = 0; i < 1000; i++) {
            sig0();
        }
        // Because these calls are purposely happening on both threads at the
        // same time, these must be asserts rather than BOOST_CHECKs to prevent
        // a race inside of BOOST_CHECK itself (writing to the log).
        assert(!sig0.empty());
        assert(conn0.connected());
    });

    std::thread extra_increment_injector([&conn0, &sig0, &val] {
        static constexpr size_t num_extra_conns{1000};
        std::vector<btcsignals::scoped_connection> extra_conns;
        extra_conns.reserve(num_extra_conns);
        for (size_t i = 0; i < num_extra_conns; i++) {
            BOOST_CHECK(!sig0.empty());
            BOOST_CHECK(conn0.connected());
            extra_conns.emplace_back(sig0.connect([&val] {
                val++;
            }));
            sig0();
        }
    });
    incrementor.join();
    extra_increment_injector.join();
    conn0.disconnect();
    BOOST_CHECK(sig0.empty());

    // sig will have been called 2000 times, and at least 1000 of those will
    // have been executing multiple incrementing callbacks. So while val is
    // probably MUCH bigger, it's guaranteed to be at least 3000.
    BOOST_CHECK_GE(val.load(), 3000);
}

/* Test that connection and disconnection works from within signal
 * callbacks.
 */
BOOST_AUTO_TEST_CASE(recursion_safety)
{
    btcsignals::connection conn0, conn1, conn2;
    btcsignals::signal<void()> sig0;
    bool nonrecursive_callback_ran{false};
    bool recursive_callback_ran{false};

    conn0 = sig0.connect([&] {
        BOOST_CHECK(!sig0.empty());
        nonrecursive_callback_ran = true;
    });
    BOOST_CHECK(!nonrecursive_callback_ran);
    sig0();
    BOOST_CHECK(nonrecursive_callback_ran);
    BOOST_CHECK(conn0.connected());

    nonrecursive_callback_ran = false;
    conn1 = sig0.connect([&] {
        nonrecursive_callback_ran = true;
        conn1.disconnect();
    });
    BOOST_CHECK(!nonrecursive_callback_ran);
    BOOST_CHECK(conn0.connected());
    BOOST_CHECK(conn1.connected());
    sig0();
    BOOST_CHECK(nonrecursive_callback_ran);
    BOOST_CHECK(conn0.connected());
    BOOST_CHECK(!conn1.connected());

    nonrecursive_callback_ran = false;
    conn1 = sig0.connect([&] {
        conn2 = sig0.connect([&] {
            BOOST_CHECK(conn0.connected());
            recursive_callback_ran = true;
            conn0.disconnect();
            conn2.disconnect();
        });
        nonrecursive_callback_ran = true;
        conn1.disconnect();
    });
    BOOST_CHECK(!nonrecursive_callback_ran);
    BOOST_CHECK(!recursive_callback_ran);
    BOOST_CHECK(conn0.connected());
    BOOST_CHECK(conn1.connected());
    BOOST_CHECK(!conn2.connected());
    sig0();
    BOOST_CHECK(nonrecursive_callback_ran);
    BOOST_CHECK(!recursive_callback_ran);
    BOOST_CHECK(conn0.connected());
    BOOST_CHECK(!conn1.connected());
    BOOST_CHECK(conn2.connected());
    sig0();
    BOOST_CHECK(recursive_callback_ran);
    BOOST_CHECK(!conn0.connected());
    BOOST_CHECK(!conn1.connected());
    BOOST_CHECK(!conn2.connected());
}

/* Test that disconnection from another thread works in real time
 */
BOOST_AUTO_TEST_CASE(disconnect_thread_safety)
{
    btcsignals::connection conn0, conn1, conn2;
    btcsignals::signal<void(int&)> sig0;
    std::binary_semaphore done1{0};
    std::binary_semaphore done2{0};
    int val{0};

    conn0 = sig0.connect([&](int&) {
        conn1.disconnect();
        done1.release();
        done2.acquire();
    });
    conn1 = sig0.connect(IncrementCallback);
    conn2 = sig0.connect(IncrementCallback);
    std::thread thr([&] {
        done1.acquire();
        conn2.disconnect();
        done2.release();
    });
    sig0(val);
    thr.join();
    BOOST_CHECK_EQUAL(val, 0);
}


BOOST_AUTO_TEST_SUITE_END()
