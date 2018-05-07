// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)
/*
BOOST_AUTO_TEST_CASE(subsidy_limit_test, * boost::unit_test::disabled())
{
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < 14000000; nHeight += 1000) {
        // @TODO fix subsidity, add nBits 
        CAmount nSubsidy = GetBlockValue(nHeight, 0);
        BOOST_CHECK(nSubsidy <= 25 * COIN);
        nSum += nSubsidy * 1000;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK(nSum == 1350824726649000ULL);
}
*/
BOOST_AUTO_TEST_SUITE_END()
