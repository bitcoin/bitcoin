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

    feeRate = CFeeRate(1000);
    // Must always just return the arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), 121);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), 999);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), 1e3);

    feeRate = CFeeRate(123);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), 14);
    BOOST_CHECK_EQUAL(feeRate.GetFee(122), 15);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), 122);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), 123);

    feeRate = CFeeRate(1000);
    // Must always return the arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0, true), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1, true), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121, true), 121);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999, true), 999);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3, true), 1e3);

    feeRate = CFeeRate(123);
    // Applies ceil to the result
    BOOST_CHECK_EQUAL(feeRate.GetFee(0, true), 0);
    BOOST_CHECK_EQUAL(feeRate.GetFee(8, true), 1);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9, true), 2);
    BOOST_CHECK_EQUAL(feeRate.GetFee(121, true), 15);
    BOOST_CHECK_EQUAL(feeRate.GetFee(122, true), 16);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999, true), 123);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3, true), 123);
}

BOOST_AUTO_TEST_SUITE_END()
