// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/arith/range_proof/range_proof.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(range_proof_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_range_proof_input_param_validation)
{
    Scalar one(1);
    Scalar two(2);
    Scalar twoPow64 = two << 63; // = 2^64

    // valid range is [0, 2**64)
    Scalar lower_bound(0);
    Scalar upper_bound = twoPow64 - one;

    std::vector<Scalar> in_range {
        lower_bound,
        lower_bound + one,
        upper_bound - one,
        upper_bound
    };
    std::vector<Scalar> out_of_range {
        upper_bound + one,
        upper_bound + two,
        upper_bound * two,
        one.Negate()
    };

    std::string nonce_str("nonce");
    G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });

    std::string msg_str("spagetti meatballs");
    std::vector<unsigned char> message { msg_str.begin(), msg_str.end() };

    TokenId token_id(uint256(123));
    RangeProof range_proof;

    // test one
    {
        Scalars vs;
        vs.Add(one);
        auto proof = range_proof.Prove(vs, nonce, message, token_id);
        auto is_valid = range_proof.Verify(std::vector<Proof> { proof }, token_id);
        BOOST_CHECK(is_valid);
    }

    // // test each valid value individually
    // int i = 1;
    // for (Scalar v: in_range) {
    //     printf("=====> TEST %d/%ld\n", i++, in_range.size());
    //     Scalars vs;
    //     vs.Add(v);
    //     auto proof = range_proof.Prove(vs, nonce, message, token_id);
    //     auto is_valid = range_proof.Verify(std::vector<Proof> { proof }, token_id);
    //     BOOST_CHECK(is_valid);
    // }

    // test each invalid value individually
    // test all valid values as a batch
    // test all invalid values as a batch
}

BOOST_AUTO_TEST_CASE(test_range_proof_inner_product_argument)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_get_first_power_of_2_greater_or_eq_to)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_prove)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_verify)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_validate_proofs_by_sizes)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_recover_amounts)
{
}

BOOST_AUTO_TEST_SUITE_END()
