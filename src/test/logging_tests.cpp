// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <logging/timer.h>
#include <test/util/setup_common.h>

#include <chrono>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(logging_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(logging_timer)
{

    SetMockTime(1);
    auto sec_timer = BCLog::Timer<std::chrono::seconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(sec_timer.LogMsg("test secs"), "tests: test secs (1.00s)");

    SetMockTime(1);
    auto ms_timer = BCLog::Timer<std::chrono::milliseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(ms_timer.LogMsg("test ms"), "tests: test ms (1000.00ms)");

    SetMockTime(1);
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("test micros"), "tests: test micros (1000000.00μs)");

    SetMockTime(0);
}

BOOST_AUTO_TEST_SUITE_END()
