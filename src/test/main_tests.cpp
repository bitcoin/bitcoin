// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    uint64_t nSum = 0;
    for (int nHeight = 0; nHeight < 14000000; nHeight += 1000) {
        /* @TODO fix subsidity, add nBits */
        uint64_t nSubsidy = GetBlockValue(0, nHeight, 0);
        BOOST_CHECK(nSubsidy <= 50 * COIN);
        nSum += nSubsidy * 1000;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK(nSum == 2099999997690000ULL);
}

BOOST_AUTO_TEST_SUITE_END()
