// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <blsct/arith/g1point.h>
#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(generators_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_generators_G_Gi_Hi)
{
    TokenId token_id_1(uint256(51), 123ULL);
    TokenId token_id_2(uint256(62), 234ULL);
    Generators gens1, gens2;

    // G should always be the same and equal to G1Point base point
    BOOST_CHECK(gens1.G() == gens2.G());
    BOOST_CHECK(gens1.G() == G1Point::GetBasePoint());

    // size of Gi and Hi should be the same and equal to max_input_value_vec_len in config
    BOOST_CHECK(gens1.Hi().Size() == gens1.Gi().Size());
    BOOST_CHECK(gens1.Hi().Size() == Config::m_max_input_value_vec_len);

    // same Gi and Hi should always be returned
    BOOST_CHECK(gens1.Hi() == gens2.Hi());
    BOOST_CHECK(gens1.Gi() == gens2.Gi());
}

BOOST_AUTO_TEST_CASE(test_generators_H)
{
    TokenId token_id_1(uint256(51), 123ULL);
    TokenId token_id_2(uint256(62), 234ULL);
    Generators gens1, gens2;

    auto H1 = gens1.H(token_id_1);
    auto H2 = gens1.H(token_id_2);
    auto H3 = gens1.H(token_id_1);

    // H should differ if created from different token_id
    BOOST_CHECK(H1 != H2);

    // H should be identical if created from the same token_id
    BOOST_CHECK(H1 != H3);

    // the same H should be generated regardless of the instance used
    auto H1_again = gens2.H(token_id_1);
    auto H2_again = gens2.H(token_id_2);

    BOOST_CHECK(H1 == H1_again);
    BOOST_CHECK(H2 == H2_again);
}

BOOST_AUTO_TEST_SUITE_END()
