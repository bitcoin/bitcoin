#include "random.h"
#include "sync.h"
#include "test/test_bitcoin.h"
#include "utiltime.h"

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

namespace number
{
    int n = 0;
}

namespace locker
{
    CCriticalSection cs_number;
}

static void plusOneThread(int nIterations)
{
    for (int i = 0; i < nIterations; ++i) {
        LOCK(locker::cs_number);
        int n = number::n;
        int nSleep = GetRandInt(10);
        MilliSleep(nSleep);
        number::n = n + 1;
    }
}

BOOST_FIXTURE_TEST_SUITE(omnicore_lock_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(multithread_locking)
{
    int nThreadsNum = 4;
    int nIterations = 20;

    boost::thread_group threadGroup;
    for (int i = 0; i < nThreadsNum; ++i) {
        threadGroup.create_thread(boost::bind(&plusOneThread, nIterations));
    }

    threadGroup.join_all();
    BOOST_CHECK_EQUAL(number::n, (nThreadsNum * nIterations));
}


BOOST_AUTO_TEST_SUITE_END()
