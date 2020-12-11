// Copyright (c) 2012-2017 The Bitcoin Core developers
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
    BOOST_CHECK(LockStackEmpty());
    bool error_thrown = false;
    try {
        LOCK2(mutex2, mutex1);
    } catch (const std::logic_error& e) {
        BOOST_CHECK_EQUAL(e.what(), "potential deadlock detected: mutex1 -> mutex2 -> mutex1");
        error_thrown = true;
    }
    BOOST_CHECK(LockStackEmpty());
    #ifdef DEBUG_LOCKORDER
    BOOST_CHECK(error_thrown);
    #else
    BOOST_CHECK(!error_thrown);
    #endif
}

template <typename MutexType>
void TestInconsistentLockOrderDetected(MutexType& mutex1, MutexType& mutex2) NO_THREAD_SAFETY_ANALYSIS
{
    ENTER_CRITICAL_SECTION(mutex1);
    ENTER_CRITICAL_SECTION(mutex2);
#ifdef DEBUG_LOCKORDER
    BOOST_CHECK_EXCEPTION(LEAVE_CRITICAL_SECTION(mutex1), std::logic_error, HasReason("mutex1 was not most recent critical section locked"));
#endif // DEBUG_LOCKORDER
    LEAVE_CRITICAL_SECTION(mutex2);
    LEAVE_CRITICAL_SECTION(mutex1);
    BOOST_CHECK(LockStackEmpty());
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(sync_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(potential_deadlock_detected)
{
    #ifdef DEBUG_LOCKORDER
    bool prev = g_debug_lockorder_abort;
    g_debug_lockorder_abort = false;
    #endif

    CCriticalSection rmutex1, rmutex2;
    TestPotentialDeadLockDetected(rmutex1, rmutex2);
    // The second test ensures that lock tracking data have not been broken by exception.
    TestPotentialDeadLockDetected(rmutex1, rmutex2);

    Mutex mutex1, mutex2;
    TestPotentialDeadLockDetected(mutex1, mutex2);
    // The second test ensures that lock tracking data have not been broken by exception.
    TestPotentialDeadLockDetected(mutex1, mutex2);

    #ifdef DEBUG_LOCKORDER
    g_debug_lockorder_abort = prev;
    #endif
}

BOOST_AUTO_TEST_CASE(inconsistent_lock_order_detected)
{
#ifdef DEBUG_LOCKORDER
    bool prev = g_debug_lockorder_abort;
    g_debug_lockorder_abort = false;
#endif // DEBUG_LOCKORDER

    RecursiveMutex rmutex1, rmutex2;
    TestInconsistentLockOrderDetected(rmutex1, rmutex2);
    // By checking lock order consistency (CheckLastCritical) before any unlocking (LeaveCritical)
    // the lock tracking data must not have been broken by exception.
    TestInconsistentLockOrderDetected(rmutex1, rmutex2);

    Mutex mutex1, mutex2;
    TestInconsistentLockOrderDetected(mutex1, mutex2);
    // By checking lock order consistency (CheckLastCritical) before any unlocking (LeaveCritical)
    // the lock tracking data must not have been broken by exception.
    TestInconsistentLockOrderDetected(mutex1, mutex2);

#ifdef DEBUG_LOCKORDER
    g_debug_lockorder_abort = prev;
#endif // DEBUG_LOCKORDER
}

BOOST_AUTO_TEST_SUITE_END()
