// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/arith/range_proof/range_proof.h>
// #include <blsct/arith/range_proof/range_proof_orig.h>

#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(range_proof_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_range_proof_one_value)
{
    std::string msg_str("spagetti meatballs");
    std::vector<unsigned char> message { msg_str.begin(), msg_str.end() };

    std::string nonce_str("nonce");
    G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });

    TokenId token_id(uint256(123));

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    RangeProof rp;
    auto p = rp.Prove(vs, nonce, message, token_id);

    auto is_valid = rp.Verify(std::vector<Proof> { p }, token_id);
    BOOST_CHECK(is_valid);

    // test each invalid value individually
    // test all valid values as a batch
    // test all invalid values as a batch
}

BOOST_AUTO_TEST_CASE(test_range_proof_recovery_one_value)
{
    std::string msg_str("spagetti meatballs");
    std::vector<unsigned char> message { msg_str.begin(), msg_str.end() };

    std::string nonce_str("nonce");
    G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });

    TokenId token_id(uint256(123));

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    RangeProof rp;
    auto p = rp.Prove(vs, nonce, message, token_id);

    AmountRecoveryReq req {
        1,
        p.x,
        p.z,  // Scalar z;
        p.Vs, // G1Points Vs;
        p.Ls, // G1Points Ls;
        p.Rs, // G1Points Rs;
        p.mu, // Scalar mu;
        p.tau_x, // Scalar tau_x;
        nonce
    };
    auto reqs = std::vector<AmountRecoveryReq> { req };
    auto amounts = rp.RecoverAmounts(reqs, token_id);

    BOOST_CHECK(amounts.size() == 1);
    BOOST_CHECK(amounts[0].gamma == nonce.GetHashWithSalt(100));
    BOOST_CHECK(amounts[0].amount == 1);
    BOOST_CHECK(amounts[0].message == msg_str);
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

BOOST_AUTO_TEST_SUITE_END()
