// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/g1point.h>
#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(generators_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_generators_get_instance)
{
    TokenId token_id_1(uint256(1), 11ULL);
    TokenId token_id_2(uint256(2), 22ULL);
    GeneratorsFactory gf;

    Generators gens1 = gf.GetInstance(token_id_1);
    Generators gens2 = gf.GetInstance(token_id_2);

    // G should always be the same and equal to G1Point base point
    BOOST_CHECK(gens1.G.get() == gens2.G.get());
    BOOST_CHECK(gens1.G.get() == G1Point::GetBasePoint());

    // size of Gi and Hi should be the same and equal to max_input_value_vec_len in config
    BOOST_CHECK(gens1.Hi.get().Size() == gens1.Gi.get().Size());
    BOOST_CHECK(gens1.Hi.get().Size() == Config::m_max_input_value_vec_len);

    // same Gi and Hi should always be returned
    BOOST_CHECK(gens1.Hi.get() == gens2.Hi.get());
    BOOST_CHECK(gens1.Gi.get() == gens2.Gi.get());

    // H should differ if token_id differs
    BOOST_CHECK(gens1.H != gens2.H);

    // H should be identical if Generator is created from the same token_id
    Generators gens3 = gf.GetInstance(token_id_1);
    BOOST_CHECK(gens1.H == gens3.H);
}

BOOST_AUTO_TEST_SUITE_END()
