// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <blsct/arith/generators.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(generators_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_generators_get_generator)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_get_init)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_constructor)
{
    TokenId token_id(uint256(51), 123ULL);
    Generators gen(token_id);


    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_get_instance)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_getter_g)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_getter_h)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_getter_gi)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_CASE(test_generators_getter_hi)
{
    BOOST_CHECK(1 == 2);
}

BOOST_AUTO_TEST_SUITE_END()
