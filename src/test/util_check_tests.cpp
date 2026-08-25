// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>

#include <boost/test/unit_test.hpp>
#include <test/util/common.h>

BOOST_AUTO_TEST_SUITE(util_check_tests)

BOOST_AUTO_TEST_CASE(check_pass)
{
    Assume(true);
    Assert(true);
    CHECK_NONFATAL(true);
}

BOOST_AUTO_TEST_CASE(check_fail)
{
    constexpr int zero{};
    // Disable aborts for easier testing here
    test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};

    if constexpr (G_ABORT_ON_FAILED_ASSUME) {
        BOOST_CHECK_EXCEPTION(Assume(zero), NonFatalCheckError, HasReason{"Internal bug detected: zero"});
    } else {
        BOOST_CHECK_NO_THROW(Assume(zero));
    }
    BOOST_CHECK_EXCEPTION(Assert(zero), NonFatalCheckError, HasReason{"Internal bug detected: zero"});
    BOOST_CHECK_EXCEPTION(AssertUnreachable(), NonFatalCheckError, HasReason{"Internal bug detected: Unreachable"});
    BOOST_CHECK_EXCEPTION(CHECK_NONFATAL(zero), NonFatalCheckError, HasReason{"Internal bug detected: zero"});
}

BOOST_AUTO_TEST_SUITE_END()
