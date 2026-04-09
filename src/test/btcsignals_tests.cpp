// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <btcsignals.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <semaphore>

namespace {


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

BOOST_AUTO_TEST_CASE(any_of_combiner)
{
    btcsignals::signal<bool(), btcsignals::any_of> sig0;
    decltype(sig0)::result_type ret;
    ret = sig0();
    BOOST_CHECK_EQUAL(ret, false);
    {
        btcsignals::scoped_connection conn0{sig0.connect(ReturnTrue)};
        ret = sig0();
        BOOST_CHECK_EQUAL(ret, true);
    }
    ret = sig0();
    BOOST_CHECK_EQUAL(ret, false);
    {
        btcsignals::scoped_connection conn0{sig0.connect(ReturnTrue)};
        btcsignals::scoped_connection conn1{sig0.connect(ReturnFalse)};
        ret = sig0();
        BOOST_CHECK_EQUAL(ret, true);
        conn0.disconnect();
        ret = sig0();
        BOOST_CHECK_EQUAL(ret, false);
    }
    ret = sig0();
    BOOST_CHECK_EQUAL(ret, false);
}

/* Test the thread-safety of connect/disconnect/empty/connected/callbacks.
 * Connect sig0 to an incrementor function and loop in a thread.
 * Meanwhile, in another thread, inject and call new increment callbacks.
 * Both threads are constantly calling empty/connected.
 * The end-result must be deterministic for the atomic modified by conn0.
 * Though, the end-result for the atomic modified by the extra connections is
 * undefined due to a non-deterministic number of total callbacks executed.
 * In any case, this should all be completely threadsafe.
 * Sanitizers should pick up any buggy data race behavior (if present).
 */
BOOST_AUTO_TEST_CASE(thread_safety)
{
    btcsignals::signal<void()> sig0;
    std::atomic<uint32_t> val_det{0};
    std::atomic<uint32_t> val_non_det{0};
    auto conn0 = sig0.connect([&val_det] { val_det++; });

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

    std::thread extra_increment_injector([&conn0, &sig0, &val_non_det] {
        static constexpr size_t num_extra_conns{1000};
        std::vector<btcsignals::scoped_connection> extra_conns;
        extra_conns.reserve(num_extra_conns);
        for (size_t i = 0; i < num_extra_conns; i++) {
            BOOST_CHECK(!sig0.empty());
            BOOST_CHECK(conn0.connected());
            btcsignals::scoped_connection extra{sig0.connect([&val_non_det] { val_non_det++; })};
            if (i % 2 == 0) {
                extra_conns.emplace_back(std::move(extra));
            }
            sig0();
        }
    });
    incrementor.join();
    extra_increment_injector.join();
    conn0.disconnect();
    BOOST_CHECK(sig0.empty());

    // sig0 will have been called 2000 times, and only the first connection did
    // increment val_det, so it must be 2000.
    BOOST_CHECK_EQUAL(val_det.load(), 2000);
    // The number of connections that increment val_non_det is growing from 1
    // to 500, where 500 are disconnected immediately again after the step.
    // Before the end of each step the connections are called at least once.
    // However, it is unknown how often the connections have been called
    // exactly. The 500th Triangular Number gives a lower estimate.
    // T_n=n(n+1)/2
    BOOST_CHECK_GE(val_non_det.load(), 500 * 501 / 2);
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
