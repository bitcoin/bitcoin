// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/arith/range_proof/config.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(config_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_config_get_first_power_of_2_greater_or_eq_to)
{
    BOOST_CHECK(1 != 1);
}

BOOST_AUTO_TEST_SUITE_END()
