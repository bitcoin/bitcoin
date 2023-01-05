// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/config.h>
#include <blsct/range_proof/generators.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

using T = Mcl;
using Point = T::Point;
using Scalar = T::Scalar;

BOOST_FIXTURE_TEST_SUITE(generators_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_generators_get_instance)
{
    GeneratorsFactory<T> gf;

    TokenId token_id_1(uint256(1), 11ULL);
    Generators<T> gens1 = gf.GetInstance(token_id_1);
    Generators<T> gens1_2 = gf.GetInstance(token_id_1);

    TokenId token_id_2(uint256(2), 22ULL);
    Generators<T> gens2 = gf.GetInstance(token_id_2);

    auto max_size = Config::m_max_input_value_vec_len;

    // regardless of the token_id, the same Gi and Hi should be returned
    //// same token_id
    BOOST_CHECK(gens1.GetGiSubset(max_size) == gens1_2.GetGiSubset(max_size));
    BOOST_CHECK(gens1.GetHiSubset(max_size) == gens1_2.GetHiSubset(max_size));

    //// different token_ids
    BOOST_CHECK(gens1.GetGiSubset(max_size) == gens2.GetGiSubset(max_size));
    BOOST_CHECK(gens1.GetHiSubset(max_size) == gens2.GetHiSubset(max_size));
}

BOOST_AUTO_TEST_CASE(test_generators_h_static)
{
    GeneratorsFactory<T> gf;

    TokenId token_id_1(uint256(1), 11ULL);
    Generators<T> gens1 = gf.GetInstance(token_id_1);

    TokenId token_id_2(uint256(2), 22ULL);
    Generators<T> gens2 = gf.GetInstance(token_id_2);

    // regardless of token_id, the same H should be returned
    BOOST_CHECK(gens1.H.get() == gens2.H.get());

    // H should be equal to the base point
    BOOST_CHECK(gens1.H.get() == Point::GetBasePoint());
}

BOOST_AUTO_TEST_CASE(test_generators_g_derived_from_token_id)
{
    GeneratorsFactory<T> gf;

    TokenId token_id_1(uint256(1), 11ULL);
    Generators<T> gens1 = gf.GetInstance(token_id_1);

    TokenId token_id_2(uint256(2), 22ULL);
    Generators<T> gens2 = gf.GetInstance(token_id_2);

    // G should differ if token_id differs
    BOOST_CHECK(gens1.G != gens2.G);

    // the same G should be derived for the same token_id
    Generators<T> gens1_2 = gf.GetInstance(token_id_1);
    BOOST_CHECK(gens1.G == gens1_2.G);
}

BOOST_AUTO_TEST_CASE(test_generators_get_gihi_subset)
{
    TokenId token_id(uint256(1), 11ULL);
    GeneratorsFactory<T> gf;

    Generators<T> gens = gf.GetInstance(token_id);

    // should be able to get empty Gi and Hi
    BOOST_CHECK_NO_THROW(gens.GetGiSubset(0));
    BOOST_CHECK_NO_THROW(gens.GetHiSubset(0));

    auto max_size = Config::m_max_input_value_vec_len;

    // should be able to get Gi and Hi up to of size max_input_value_vec_len
    BOOST_CHECK_NO_THROW(gens.GetGiSubset(max_size));
    BOOST_CHECK_NO_THROW(gens.GetHiSubset(max_size));

    // should not be able to get Gi and Hi above the size of size max_input_value_vec_len
    BOOST_CHECK_THROW(gens.GetGiSubset(max_size + 1), std::runtime_error);
    BOOST_CHECK_THROW(gens.GetHiSubset(max_size + 1), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()