// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

namespace {
template <typename MutexType>
void TestPotentialDeadLockDetected(MutexType& mutex1, MutexType& mutex2)
{
    {
        LOCK2(mutex1, mutex2);
    }
    bool error_thrown = false;
    try {
        LOCK2(mutex2, mutex1);
    } catch (const std::logic_error& e) {
        BOOST_CHECK_EQUAL(e.what(), "potential deadlock detected");
        error_thrown = true;
    }
    #ifdef DEBUG_LOCKORDER
    BOOST_CHECK(error_thrown);
    #else
    BOOST_CHECK(!error_thrown);
    #endif
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(sync_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(potential_deadlock_detected)
{
    #ifdef DEBUG_LOCKORDER
    bool prev = g_debug_lockorder_abort;
    g_debug_lockorder_abort = false;
    #endif

    RecursiveMutex rmutex1, rmutex2;
    TestPotentialDeadLockDetected(rmutex1, rmutex2);

    Mutex mutex1, mutex2;
    TestPotentialDeadLockDetected(mutex1, mutex2);

    #ifdef DEBUG_LOCKORDER
    g_debug_lockorder_abort = prev;
    #endif
}

BOOST_AUTO_TEST_SUITE_END()
