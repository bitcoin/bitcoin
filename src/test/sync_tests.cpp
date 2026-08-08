// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <test/util/common.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <mutex>
#include <stdexcept>

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

#ifdef DEBUG_LOCKORDER
template <typename MutexType>
void TestDoubleLock2(MutexType& m)
{
    LOCK(m);
}

template <typename MutexType>
void TestDoubleLock(bool should_throw)
{
    const bool prev = g_debug_lockorder_abort;
    g_debug_lockorder_abort = false;

    MutexType m;
    {
        LOCK(m);
        if (should_throw) {
            BOOST_CHECK_EXCEPTION(TestDoubleLock2(m), std::logic_error,
                              HasReason("double lock detected"));
        } else {
            BOOST_CHECK_NO_THROW(TestDoubleLock2(m));
        }
    }
    BOOST_CHECK(LockStackEmpty());

    g_debug_lockorder_abort = prev;
}
#endif /* DEBUG_LOCKORDER */

template <typename MutexType>
void TestInconsistentLockOrderDetected(MutexType& mutex1, MutexType& mutex2)
{
    {
        WAIT_LOCK(mutex1, lock1);
        LOCK(mutex2);
#ifdef DEBUG_LOCKORDER
        BOOST_CHECK_EXCEPTION(REVERSE_LOCK(lock1, mutex1), std::logic_error, HasReason("mutex1 was not most recent critical section locked"));
#endif // DEBUG_LOCKORDER
    }
    BOOST_CHECK(LockStackEmpty());
}
} // namespace

BOOST_AUTO_TEST_SUITE(sync_tests)

BOOST_AUTO_TEST_CASE(potential_deadlock_detected)
{
    #ifdef DEBUG_LOCKORDER
    bool prev = g_debug_lockorder_abort;
    g_debug_lockorder_abort = false;
    #endif

    RecursiveMutex rmutex1, rmutex2;
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

/* Double lock would produce an undefined behavior. Thus, we only do that if
 * DEBUG_LOCKORDER is activated to detect it. We don't want non-DEBUG_LOCKORDER
 * build to produce tests that exhibit known undefined behavior. */
#ifdef DEBUG_LOCKORDER
BOOST_AUTO_TEST_CASE(double_lock_mutex)
{
    TestDoubleLock<Mutex>(/*should_throw=*/true);
}

BOOST_AUTO_TEST_CASE(double_lock_recursive_mutex)
{
    TestDoubleLock<RecursiveMutex>(/*should_throw=*/false);
}
#endif /* DEBUG_LOCKORDER */

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

BOOST_AUTO_TEST_CASE(shared_mutex)
{
    struct {
        SharedMutex mutex;
        int value GUARDED_BY(mutex){0};
    } shared;

    AssertLockNotHeld(shared.mutex);
    WITH_LOCK(shared.mutex, AssertLockHeld(shared.mutex); shared.value = 1); // Exclusive lock permits guarded writes
    {
        READ_LOCK(shared.mutex); // Shared lock permits the guarded read below
#ifdef DEBUG_LOCKORDER
        BOOST_CHECK(!LockStackEmpty());
#endif
        BOOST_CHECK_EQUAL(shared.value, 1);
    }
    AssertLockNotHeld(shared.mutex);
    BOOST_CHECK(LockStackEmpty());
}

BOOST_AUTO_TEST_CASE(shared_lock_outlives_outer_lock)
{
#ifdef DEBUG_LOCKORDER
    const bool prev{g_debug_lockorder_abort};
    g_debug_lockorder_abort = false;
#endif

    {
        Mutex outer_mutex;
        auto outer_lock{std::make_unique<UniqueLock<Mutex>>(LOCK_ARGS(outer_mutex))};
        SharedMutex shared_mutex;
        SharedLock shared_lock{LOCK_ARGS(shared_mutex)};
        outer_lock.reset();
        const auto relock{[&] { LOCK(outer_mutex); }};
#ifdef DEBUG_LOCKORDER
        BOOST_CHECK_EXCEPTION(relock(), std::logic_error, HasReason("potential deadlock detected"));
#else
        BOOST_CHECK_NO_THROW(relock());
#endif
    }
    BOOST_CHECK(LockStackEmpty());

#ifdef DEBUG_LOCKORDER
    g_debug_lockorder_abort = prev;
#endif
}

BOOST_AUTO_TEST_SUITE_END()
