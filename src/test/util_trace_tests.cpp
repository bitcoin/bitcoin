// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <util/trace.h>

BOOST_FIXTURE_TEST_SUITE(util_trace_tests, BasicTestingSetup)

// Tests the TRACEPOINT macro and that we can compile tracepoints with 0 to 12 args.
BOOST_AUTO_TEST_CASE(test_tracepoints)
{
    TRACEPOINT(test, zero_args);
    TRACEPOINT(test, one_arg, 1);
    TRACEPOINT(test, six_args, 1, 2, 3, 4, 5, 6);
    TRACEPOINT(test, twelve_args, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()