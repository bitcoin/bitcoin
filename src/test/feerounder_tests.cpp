// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/fees/block_policy_estimator.h>

#include <boost/test/unit_test.hpp>

#include <set>

BOOST_AUTO_TEST_SUITE(feerounder_tests)

BOOST_AUTO_TEST_CASE(FeeRounder)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    FeeFilterRounder fee_rounder{CFeeRate{CAmount{1000}}, rng};

    // check that 1000 rounds to 974 or 1071
    std::set<CAmount> results;
    while (results.size() < 2) {
        results.emplace(fee_rounder.round(CAmount{1000}));
    }
    BOOST_CHECK_EQUAL(*results.begin(), CAmount{974});
    BOOST_CHECK_EQUAL(*++results.begin(), CAmount{1071});

    // check that negative amounts rounds to 0
    BOOST_CHECK_EQUAL(fee_rounder.round(CAmount{-0}), CAmount{0});
    BOOST_CHECK_EQUAL(fee_rounder.round(CAmount{-1}), CAmount{0});

    // check that MAX_MONEY rounds to 9170997
    BOOST_CHECK_EQUAL(fee_rounder.round(MAX_MONEY), CAmount{9170997});
}

BOOST_AUTO_TEST_SUITE_END()
