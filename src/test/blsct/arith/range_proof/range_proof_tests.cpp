// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/arith/range_proof/range_proof.h>
#include <blsct/arith/range_proof/range_proof_orig.h>

#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(range_proof_tests, MclTestingSetup)

void PrintG1(const char* name, G1Point& p)
{
    printf("%s: %s\n", name, HexStr(p.GetVch()).c_str());
}

void PrintScalar(const char* name, Scalar& s)
{
    printf("%s: %s\n", name, HexStr(s.GetVch()).c_str());
}

// BOOST_AUTO_TEST_CASE(test_range_proof_original_code)
// {
//     G1Point nonce = G1Point::GetBasePoint();
//     auto buf = nonce.GetVch();
//     printf("G: %s\n", HexStr(buf).c_str());

//     Scalar one(1);
//     std::vector<Scalar> vs;
//     vs.push_back(one);

//     BulletproofsRangeproof rp;

//     rp.Prove(vs, nonce, {1, 2, 3, 4});

//     PrintG1("A", rp.A);
//     PrintG1("S", rp.S);
//     PrintG1("T1", rp.T1);
//     PrintG1("T2", rp.T1);
//     PrintScalar("tau_x", rp.taux);
//     PrintScalar("mu", rp.mu);
//     PrintScalar("a", rp.a);
//     PrintScalar("b", rp.b);
//     PrintScalar("t_hat", rp.t);

//     std::vector<G1Point> nonces { nonce };
//     std::pair<int, BulletproofsRangeproof> proof1(0, rp);
//     std::vector<std::pair<int, BulletproofsRangeproof>> proofs { proof1 };
//     RangeproofEncodedData red;
//     std::vector<RangeproofEncodedData> data;

//     auto is_valid = VerifyBulletproof(
//         proofs,
//         data,
//         nonces,
//         false
//     );
//     BOOST_CHECK(is_valid);

//     BOOST_CHECK(data.size() == 1);
//     printf("data gamma=%s\n", data[0].gamma.GetString().c_str());
//     printf("data amount=%ld\n", data[0].amount);
//     printf("data message=%s\n", data[0].message.c_str());
// }

BOOST_AUTO_TEST_CASE(test_range_proof_comarison)
{
    std::string msg_str("spagetti meatballs");
    std::vector<unsigned char> message { msg_str.begin(), msg_str.end() };

    // std::string nonce_str("nonce");
    // G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });

    G1Point nonce = G1Point::GetBasePoint();
    // auto buf = nonce.GetVch();
    // printf("G: %s\n", HexStr(buf).c_str());

    TokenId token_id(uint256(123));

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    {
        printf("\n==== old ====\n");
        Scalar::Rand(true);   // reset the seed
        BulletproofsRangeproof rp;
        rp.Prove(vs_vec, nonce, message);

        // PrintG1("A", rp.A);
        // PrintG1("S", rp.S);
        // PrintG1("T1", rp.T1);
        // PrintG1("T2", rp.T1);
        // PrintScalar("tau_x", rp.taux);
        // PrintScalar("mu", rp.mu);
        // PrintScalar("a", rp.a);
        // PrintScalar("b", rp.b);
        // PrintScalar("t_hat", rp.t);

        std::vector<G1Point> nonces { nonce };
        std::pair<int, BulletproofsRangeproof> proof1(0, rp);
        std::vector<std::pair<int, BulletproofsRangeproof>> proofs { proof1 };
        RangeproofEncodedData red;
        std::vector<RangeproofEncodedData> data;

        Scalar::Rand(true); // reset seed
        auto is_valid = VerifyBulletproof(
            proofs,
            data,
            nonces,
            false
        );
        BOOST_CHECK(is_valid);
        if (is_valid) {
            printf("=====> old impl is working fine!!!!\n");
        }
    }
    {
        printf("\n==== new ====\n");
        Scalar::Rand(true);   // reset the seed
        RangeProof rp;
        auto p = rp.Prove(vs, nonce, message, token_id);

        // PrintG1("A", p.A);
        // PrintG1("S", p.S);
        // PrintG1("T1", p.T1);
        // PrintG1("T2", p.T1);
        // PrintScalar("tau_x", p.tau_x);
        // PrintScalar("mu", p.mu);
        // PrintScalar("a", p.a);
        // PrintScalar("b", p.b);
        // PrintScalar("t_hat", p.t_hat);

        Scalar::Rand(true); // reset seed
        auto is_valid = rp.Verify(std::vector<Proof> { p }, token_id);
        BOOST_CHECK(is_valid);
        if (is_valid) {
            printf("=====> new impl is working fine!!!!\n");
        }
    }

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

BOOST_AUTO_TEST_CASE(test_range_proof_recovery)
{
    /*
    std::string nonce_str("nonce");
    G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });

    std::string msg_str("spagetti meatballs");
    std::vector<unsigned char> message { msg_str.begin(), msg_str.end() };

    TokenId token_id(uint256(123));

    std::vector<Scalar> vs;
    Scalar amount(1);
    vs.push_back(amount);

    //RangeProof range_proof;
    BulletproofsRangeproof range_proof;

    // test one
    {
        range_proof.Prove(vs, nonce, message, token_id);

        std::vector<G1Point> nonces { nonce };
        std::pair<int, BulletproofsRangeproof> proof1(1, range_proof);
        std::vector<std::pair<int, BulletproofsRangeproof>> proofs { proof1 };
        RangeproofEncodedData red;
        std::vector<RangeproofEncodedData> data;

        auto is_valid = VerifyBulletproof(
            proofs,
            data,
            nonces,
            false,
            token_id
        );
        BOOST_CHECK(is_valid);

        BOOST_CHECK(data.size() == 1);
        BOOST_CHECK(data[0].gamma == nonce.GetHashWithSalt(100));
        BOOST_CHECK(data[0].amount == amount);
    }
    */
}

BOOST_AUTO_TEST_SUITE_END()
