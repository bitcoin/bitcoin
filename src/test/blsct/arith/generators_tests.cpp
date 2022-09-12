// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>
#include <blsct/arith/generators.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(generators_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_create_instance_before_init)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_SUITE_END()
