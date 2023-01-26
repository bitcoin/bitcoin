// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/range_proof/config.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(config_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_config_get_first_power_of_2_greater_or_eq_to)
{
    // 2^0 = 1
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(0) == 1);
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(1) == 1);

    // 2^1 = 2
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(2) == 2);

    // 2^2 = 4
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(3) == 4);
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(4) == 4);

    // 2^3 = 8
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(5) == 8);
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(6) == 8);
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(7) == 8);
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(8) == 8);

    // check boundary of 8
    BOOST_CHECK(Config::GetFirstPowerOf2GreaterOrEqTo(9) == 16);
}

BOOST_AUTO_TEST_SUITE_END()