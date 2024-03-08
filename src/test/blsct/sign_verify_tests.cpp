// Copyright (c) 2022 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <blsct/common.h>
#include <blsct/private_key.h>
#include <blsct/public_key.h>
#include <blsct/public_keys.h>
#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

namespace blsct {

BOOST_FIXTURE_TEST_SUITE(sign_verify_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_compatibility_bet_bls_keys_and_blsct_keys)
{
    // secret key
    blsSecretKey bls_sk{1};
    blsct::PrivateKey blsct_sk(bls_sk.v);
    auto a = blsct_sk.GetScalar().m_scalar;
    auto b = bls_sk.v;
    BOOST_CHECK(mclBnFr_isEqual(&a, &b) == 1);

    // public key
    blsct::PublicKey blsct_pk = blsct_sk.GetPublicKey();
    MclG1Point blsct_g1_point = blsct_pk.GetG1Point();

    blsPublicKey bls_pk;
    blsGetPublicKey(&bls_pk, &bls_sk);
    auto c = blsct_g1_point.m_point;
    auto d = bls_pk.v;
    BOOST_CHECK(mclBnG1_isEqual(&c, &d) == 1);
}

BOOST_AUTO_TEST_CASE(test_sign_verify_balance)
{
    blsct::PrivateKey sk(1);

    auto pk = sk.GetPublicKey();
    auto sig = sk.SignBalance();

    auto res = pk.VerifyBalance(sig);
    BOOST_CHECK(res == true);
}

BOOST_AUTO_TEST_CASE(test_sign_verify_balance_batch)
{
    blsct::PrivateKey sk1(1);
    blsct::PrivateKey sk2(12345);

    std::vector<blsct::Signature> sigs{
        sk1.SignBalance(),
        sk2.SignBalance(),
    };
    PublicKeys pks(std::vector<blsct::PublicKey>{
        sk1.GetPublicKey(),
        sk2.GetPublicKey(),
    });
    auto aggr_sig = blsct::Signature::Aggregate(sigs);
    auto res = pks.VerifyBalanceBatch(aggr_sig);

    BOOST_CHECK(res == true);
}

BOOST_AUTO_TEST_CASE(test_sign_verify_balance_batch_bad_inputs)
{
    PublicKeys pks(std::vector<blsct::PublicKey>{});
    blsct::Signature aggr_sig;
    BOOST_CHECK_THROW(pks.VerifyBalanceBatch(aggr_sig), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_augment_message)
{
    auto pk = MclG1Point::GetBasePoint();
    std::vector<uint8_t> pk_data = pk.GetVch();
    auto msg = std::vector<uint8_t>{1, 2, 3, 4, 5};
    auto act = PublicKey(pk).AugmentMessage(msg);

    auto exp = std::vector<uint8_t>(pk_data);
    exp.insert(exp.end(), msg.begin(), msg.end());
    BOOST_CHECK(act == exp);
}

BOOST_AUTO_TEST_CASE(test_sign_verify)
{
    blsct::PrivateKey sk(1);
    auto pk = sk.GetPublicKey();
    std::vector<uint8_t> msg{'m', 's', 'g'};

    auto sig = sk.Sign(msg);
    auto res = pk.Verify(msg, sig);
    BOOST_CHECK(res == true);
}

BOOST_AUTO_TEST_CASE(test_verify_batch)
{
    blsct::PrivateKey sk1(1);
    blsct::PrivateKey sk2(12345);

    PublicKeys pks(std::vector<blsct::PublicKey>{
        sk1.GetPublicKey(),
        sk2.GetPublicKey(),
    });
    std::vector<std::vector<uint8_t>> msgs{
        std::vector<uint8_t>{'m', 's', 'g', '1'},
        std::vector<uint8_t>{'m', 's', 'g', '2'},
    };
    std::vector<blsct::Signature> sigs{
        sk1.Sign(msgs[0]),
        sk2.Sign(msgs[1]),
    };
    auto aggr_sig = blsct::Signature::Aggregate(sigs);

    auto res = pks.VerifyBatch(msgs, aggr_sig);
    BOOST_CHECK(res == true);
}

BOOST_AUTO_TEST_CASE(test_verify_batch_bad_inputs)
{
    blsct::Signature sig;
    {
        // empty pks
        PublicKeys empty_pks(std::vector<PublicKey>{});
        std::vector<std::vector<uint8_t>> msgs{
            std::vector<uint8_t>{'m', 's', 'g'},
        };
        BOOST_CHECK_THROW(empty_pks.VerifyBatch(msgs, sig), std::runtime_error);
    }
    {
        // empty msgs
        PublicKeys pks(std::vector<PublicKey>{
            PublicKey(),
        });
        std::vector<std::vector<uint8_t>> empty_msgs;
        BOOST_CHECK_THROW(pks.VerifyBatch(empty_msgs, sig), std::runtime_error);
    }
    {
        // numbers of pks and msgs don't match
        PublicKeys pks(std::vector<PublicKey>{
            PublicKey(),
        });
        std::vector<std::vector<uint8_t>> msgs{
            std::vector<uint8_t>{'m', 's', 'g', '1'},
            std::vector<uint8_t>{'m', 's', 'g', '2'},
        };
        BOOST_CHECK_THROW(pks.VerifyBatch(msgs, sig), std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace blsct
