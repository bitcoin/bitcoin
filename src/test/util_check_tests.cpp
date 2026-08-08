// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>

#include <boost/test/unit_test.hpp>
#include <test/util/common.h>

BOOST_AUTO_TEST_SUITE(util_check_tests)

namespace {
void TriggerNonFatalUnreachable()
{
    NONFATAL_UNREACHABLE();
}
} // namespace

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
        BOOST_CHECK_EXCEPTION(Assume(false), NonFatalCheckError, HasReason{"Internal bug detected: 'false' check failed"});
    } else {
        BOOST_CHECK_NO_THROW(Assume(false));
    }
    BOOST_CHECK_EXCEPTION(Assert(false), NonFatalCheckError, HasReason{"Internal bug detected: 'false' check failed"});
    BOOST_CHECK_EXCEPTION(CHECK_NONFATAL(false), NonFatalCheckError, HasReason{"Internal bug detected: 'false' check failed"});

    // Repeat with a more realistic assert condition
    void* this_should_be_nonnull{nullptr};
    BOOST_CHECK_EXCEPTION(Assert(this_should_be_nonnull != nullptr), NonFatalCheckError, HasReason{"Internal bug detected: 'this_should_be_nonnull != nullptr' check failed"});
}

BOOST_AUTO_TEST_CASE(unreachable_diagnostics)
{
    BOOST_CHECK_EXCEPTION(TriggerNonFatalUnreachable(), NonFatalCheckError, HasReason{"Internal bug detected: Unreachable code reached (non-fatal)"});
}

BOOST_AUTO_TEST_SUITE_END()
