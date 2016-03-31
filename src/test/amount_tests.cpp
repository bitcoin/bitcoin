// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(amount_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(GetFeeTest)
{
    CFeeRate feeRate;

    feeRate = CFeeRate(0);
    // Must always return 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e5), 0);

    feeRate = CFeeRate(1000);
    // Must always just return the arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), 121);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), 999);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), 1e3);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), 9e3);

    feeRate = CFeeRate(-1000);
    // Must always just return -1 * arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), -1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), -121);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), -999);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), -1e3);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), -9e3);

    feeRate = CFeeRate(123);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), 1); // Special case: returns 1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), 14);
    BOOST_CHECK_EQUAL(feeRate.GetFee(122), 15);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), 122);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), 123);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), 1107);

    feeRate = CFeeRate(-123);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), -1); // Special case: returns -1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), -1);

    // Check full constructor
    // default value
    BOOST_CHECK(CFeeRate(CAmount(-1), 1000) == CFeeRate(-1));
    BOOST_CHECK(CFeeRate(CAmount(0), 1000) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(1), 1000) == CFeeRate(1));
    // lost precision (can only resolve satoshis per kB)
    BOOST_CHECK(CFeeRate(CAmount(1), 1001) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(2), 1001) == CFeeRate(1));
    // some more integer checks
    BOOST_CHECK(CFeeRate(CAmount(26), 789) == CFeeRate(32));
    BOOST_CHECK(CFeeRate(CAmount(27), 789) == CFeeRate(34));
    // Maximum size in bytes, should not crash
    CFeeRate(MAX_MONEY, std::numeric_limits<size_t>::max() >> 1).GetFeePerK();

    // With full constructor (allows to test internal precission)
    feeRate = CFeeRate(1000, KB * 2);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB - 1), 499);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB), 500);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB + 1), 500);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * 2), 1000);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * 10), 5000);

    feeRate = CFeeRate(1000 * 1000, KB * KB * KB);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB - 1), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee((KB * 2) - 1), 1); // Lost precision
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * 2), 2);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * 10), 10);
    BOOST_CHECK_EQUAL(feeRate.GetFee((KB * 10) - 1), 9);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * 100), 100);
    BOOST_CHECK_EQUAL(feeRate.GetFee((KB * KB) - 1), 999);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * KB), 1000);
    BOOST_CHECK_EQUAL(feeRate.GetFee((KB * KB * KB) - 1), 999999);
    BOOST_CHECK_EQUAL(feeRate.GetFee(KB * KB * KB), 1000000);
}

BOOST_AUTO_TEST_SUITE_END()
