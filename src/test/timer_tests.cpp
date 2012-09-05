#include <boost/test/unit_test.hpp>

#include "timer.h"

BOOST_AUTO_TEST_SUITE(timer_tests)

static void actor(CTimer *t, void *p)
{
}

static void actor2(CTimer *t, void *p)
{
    t->expire_time += 10;
}

BOOST_AUTO_TEST_CASE(timer_basics)
{
    CTimerList cl;
    BOOST_CHECK(cl.size() == 0);

    int64 nNow = GetTime();

    CTimer t1 = { (char *) "t1", nNow - 1, actor };
    CTimer t2 = { (char *) "t2", nNow, actor };
    CTimer t3 = { (char *) "t3", nNow + 1, actor };
    CTimer t4 = { (char *) "t4", nNow, actor2 };

    BOOST_CHECK(cl.addTimer(t1) == true);
    BOOST_CHECK(cl.addTimer(t2) == true);
    BOOST_CHECK(cl.addTimer(t3) == true);

    BOOST_CHECK(cl.size() == 3);
    BOOST_CHECK(cl.nextExpire() == (nNow - 1));

    cl.clear();
    BOOST_CHECK(cl.size() == 0);

    BOOST_CHECK(cl.addTimer(t1) == true);
    BOOST_CHECK(cl.addTimer(t2) == true);
    BOOST_CHECK(cl.addTimer(t3) == true);

    BOOST_CHECK(cl.size() == 3);
    BOOST_CHECK(cl.nextExpire() == (nNow - 1));

    int nRun = cl.runTimers();
    BOOST_CHECK(nRun == 2);
    BOOST_CHECK(cl.size() == 1);

    BOOST_CHECK(cl.addTimer(t4) == true);
    nRun = cl.runTimers();
    BOOST_CHECK(nRun == 1);
    BOOST_CHECK(cl.size() == 2);
}

BOOST_AUTO_TEST_SUITE_END()

