// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

BOOST_AUTO_TEST_SUITE(util_check_tests)

BOOST_AUTO_TEST_CASE(check_pass)
{
    Assume(true);
    Assert(true);
    CHECK_NONFATAL(true);
}

BOOST_AUTO_TEST_CASE(check_fail)
{
    // Disable aborts for easier testing here
    test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};

    if constexpr (G_ABORT_ON_FAILED_ASSUME) {
        BOOST_CHECK_EXCEPTION(Assume(false), NonFatalCheckError, HasReason{"Internal bug detected: false"});
    } else {
        BOOST_CHECK_NO_THROW(Assume(false));
    }
    BOOST_CHECK_EXCEPTION(Assert(false), NonFatalCheckError, HasReason{"Internal bug detected: false"});
    BOOST_CHECK_EXCEPTION(CHECK_NONFATAL(false), NonFatalCheckError, HasReason{"Internal bug detected: false"});
}

BOOST_AUTO_TEST_SUITE_END()
