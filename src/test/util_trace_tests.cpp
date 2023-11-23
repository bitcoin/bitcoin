// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <util/trace.h>

TRACEPOINT_SEMAPHORE(test, zero_args);
TRACEPOINT_SEMAPHORE(test, one_arg);
TRACEPOINT_SEMAPHORE(test, six_args);
TRACEPOINT_SEMAPHORE(test, twelve_args);
TRACEPOINT_SEMAPHORE(test, check_if_attached);
TRACEPOINT_SEMAPHORE(test, expensive_section);

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

int fail_test_if_executed()
{
    BOOST_CHECK(false);
    return 0;
}

BOOST_AUTO_TEST_CASE(test_tracepoint_check_if_attached)
{
    // TRACEPOINT should check if we are attaching to the tracepoint and only then
    // process arguments. This means, only if we are attached to the
    // `test:check_if_attached` tracepoint, fail_test_if_executed() is executed.
    // Since we don't attach to the tracepoint when running the test, it succeeds.
    TRACEPOINT(test, check_if_attached, fail_test_if_executed());
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(test_tracepoint_manual_tracepoint_active_check)
{
    // We should be able to use the TRACEPOINT_ACTIVE() macro to only
    // execute an 'expensive' code section if we are attached to the
    // tracepoint.
    if (TRACEPOINT_ACTIVE(test, expensive_section)) {
        BOOST_CHECK(false); // expensive_function()
        TRACEPOINT(test, expensive_section);
    }
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
