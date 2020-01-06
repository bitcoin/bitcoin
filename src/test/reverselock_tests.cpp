// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <reverselock.h>
#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(reverselock_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(reverselock_basics)
{
    boost::mutex mutex;
    boost::unique_lock<boost::mutex> lock(mutex);

    BOOST_CHECK(lock.owns_lock());
    {
        reverse_lock<boost::unique_lock<boost::mutex> > rlock(lock);
        BOOST_CHECK(!lock.owns_lock());
    }
    BOOST_CHECK(lock.owns_lock());
}

BOOST_AUTO_TEST_CASE(reverselock_errors)
{
    boost::mutex mutex;
    boost::unique_lock<boost::mutex> lock(mutex);

    // Make sure trying to reverse lock an unlocked lock fails
    lock.unlock();

    BOOST_CHECK(!lock.owns_lock());

    bool failed = false;
    try {
        reverse_lock<boost::unique_lock<boost::mutex> > rlock(lock);
    } catch(...) {
        failed = true;
    }

    BOOST_CHECK(failed);
    BOOST_CHECK(!lock.owns_lock());

    // Locking the original lock after it has been taken by a reverse lock
    // makes no sense. Ensure that the original lock no longer owns the lock
    // after giving it to a reverse one.

    lock.lock();
    BOOST_CHECK(lock.owns_lock());
    {
        reverse_lock<boost::unique_lock<boost::mutex> > rlock(lock);
        BOOST_CHECK(!lock.owns_lock());
    }

    BOOST_CHECK(failed);
    BOOST_CHECK(lock.owns_lock());
}

BOOST_AUTO_TEST_SUITE_END()
