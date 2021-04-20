// Copyright (c) 2014-2020 The Dash Core developers

#include <governance/governance.h>

#include <test/test_dash.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ratecheck_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ratecheck_test)
{
    CRateCheckBuffer buffer;

    BOOST_CHECK(buffer.GetCount() == 0);
    BOOST_CHECK(buffer.GetMinTimestamp() == std::numeric_limits<int64_t>::max());
    BOOST_CHECK(buffer.GetMaxTimestamp() == 0);
    BOOST_CHECK(buffer.GetRate() == 0.0);

    buffer.AddTimestamp(1);

    BOOST_TEST_MESSAGE("buffer.GetMinTimestamp() = " << buffer.GetMinTimestamp());

    BOOST_CHECK(buffer.GetCount() == 1);
    BOOST_CHECK(buffer.GetMinTimestamp() == 1);
    BOOST_CHECK(buffer.GetMaxTimestamp() == 1);
    BOOST_CHECK(buffer.GetRate() == 0.0);

    buffer.AddTimestamp(2);
    BOOST_CHECK(buffer.GetCount() == 2);
    BOOST_CHECK(buffer.GetMinTimestamp() == 1);
    BOOST_CHECK(buffer.GetMaxTimestamp() == 2);
    //BOOST_CHECK(fabs(buffer.GetRate() - 2.0) < 1.0e-9);
    BOOST_CHECK(buffer.GetRate() == 0.0);

    buffer.AddTimestamp(3);
    BOOST_CHECK(buffer.GetCount() == 3);
    BOOST_CHECK(buffer.GetMinTimestamp() == 1);
    BOOST_CHECK(buffer.GetMaxTimestamp() == 3);

    int64_t nMin = buffer.GetMinTimestamp();
    int64_t nMax = buffer.GetMaxTimestamp();
    double dRate = buffer.GetRate();

    BOOST_TEST_MESSAGE("buffer.GetCount() = " << buffer.GetCount());
    BOOST_TEST_MESSAGE("nMin = " << nMin);
    BOOST_TEST_MESSAGE("nMax = " << nMax);
    BOOST_TEST_MESSAGE("buffer.GetRate() = " << dRate);

    //BOOST_CHECK(fabs(buffer.GetRate() - (3.0/2.0)) < 1.0e-9);
    BOOST_CHECK(buffer.GetRate() == 0.0);

    buffer.AddTimestamp(4);
    BOOST_CHECK(buffer.GetCount() == 4);
    BOOST_CHECK(buffer.GetMinTimestamp() == 1);
    BOOST_CHECK(buffer.GetMaxTimestamp() == 4);
    //BOOST_CHECK(fabs(buffer.GetRate() - (4.0/3.0)) < 1.0e-9);
    BOOST_CHECK(buffer.GetRate() == 0.0);

    buffer.AddTimestamp(5);
    BOOST_CHECK(buffer.GetCount() == 5);
    BOOST_CHECK(buffer.GetMinTimestamp() == 1);
    BOOST_CHECK(buffer.GetMaxTimestamp() == 5);
    BOOST_CHECK(fabs(buffer.GetRate() - (5.0/4.0)) < 1.0e-9);

    buffer.AddTimestamp(6);
    BOOST_CHECK(buffer.GetCount() == 5);
    BOOST_CHECK(buffer.GetMinTimestamp() == 2);
    BOOST_CHECK(buffer.GetMaxTimestamp() == 6);
    BOOST_CHECK(fabs(buffer.GetRate() - (5.0/4.0)) < 1.0e-9);

    CRateCheckBuffer buffer2;

    BOOST_TEST_MESSAGE("Before loop tests");
    for(int64_t i = 1; i < 11; ++i)  {
        BOOST_TEST_MESSAGE("In loop: i = " << i);
        buffer2.AddTimestamp(i);
        BOOST_CHECK(buffer2.GetCount() == (i <= 5 ? i : 5));
        BOOST_CHECK(buffer2.GetMinTimestamp() == std::max(int64_t(1), i - 4));
        BOOST_CHECK(buffer2.GetMaxTimestamp() == i);
    }
}

BOOST_AUTO_TEST_SUITE_END()
