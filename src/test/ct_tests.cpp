// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_bitcoin.h>

#include <crypto/sha256.h>

#include <secp256k1.h>
#include <secp256k1_rangeproof.h>
#include <inttypes.h>
#include <utilstrencodings.h>

#include <boost/test/unit_test.hpp>

#include <blind.h>

BOOST_FIXTURE_TEST_SUITE(ct_tests, BasicTestingSetup)


class CTxOutValueTest
{
public:
    secp256k1_pedersen_commitment commitment;
    std::vector<uint8_t> vchRangeproof;
    std::vector<uint8_t> vchNonceCommitment;
};


BOOST_AUTO_TEST_CASE(ct_test)
{
    SeedInsecureRand();
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    std::vector<CTxOutValueTest> txins(1);

    std::vector<const uint8_t*> blindptrs;
    uint8_t blindsin[1][32];
    //GetStrongRandBytes(&blindsin[0][0], 32);
    InsecureRandBytes(&blindsin[0][0], 32);
    blindptrs.push_back(&blindsin[0][0]);

    CAmount nValueIn = 45.69 * COIN;
    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txins[0].commitment, &blindsin[0][0], nValueIn, secp256k1_generator_h));

    const int nTxOut = 2;
    std::vector<CTxOutValueTest> txouts(nTxOut);

    std::vector<CAmount> amount_outs(2);
    amount_outs[0] = 5.69 * COIN;
    amount_outs[1] = 40 * COIN;

    std::vector<CKey> kto_outs(2);
    InsecureNewKey(kto_outs[0], true);
    InsecureNewKey(kto_outs[1], true);

    std::vector<CPubKey> pkto_outs(2);
    pkto_outs[0] = kto_outs[0].GetPubKey();
    pkto_outs[1] = kto_outs[1].GetPubKey();

    uint8_t blind[nTxOut][32];

    size_t nBlinded = 0;
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        CTxOutValueTest &txout = txouts[k];

        if (nBlinded + 1 == txouts.size())
        {
            // Last to-be-blinded value: compute from all other blinding factors.
            // sum of output blinding values must equal sum of input blinding values
            BOOST_CHECK(secp256k1_pedersen_blind_sum(ctx, &blind[nBlinded][0], &blindptrs[0], 2, 1));
            blindptrs.push_back(&blind[nBlinded++][0]);
        } else
        {
            //GetStrongRandBytes(&blind[nBlinded][0], 32);
            InsecureRandBytes(&blind[nBlinded][0], 32);
            blindptrs.push_back(&blind[nBlinded++][0]);
        };

        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txout.commitment, (uint8_t*)blindptrs.back(), amount_outs[k], secp256k1_generator_h));

        // Generate ephemeral key for ECDH nonce generation
        CKey ephemeral_key;
        InsecureNewKey(ephemeral_key, true);
        CPubKey ephemeral_pubkey = ephemeral_key.GetPubKey();
        txout.vchNonceCommitment.resize(33);
        memcpy(&txout.vchNonceCommitment[0], &ephemeral_pubkey[0], 33);

        // Generate nonce
        uint256 nonce = ephemeral_key.ECDH(pkto_outs[k]);
        CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());

        // Create range proof
        size_t nRangeProofLen = 5134;
        // TODO: smarter min_value selection

        txout.vchRangeproof.resize(nRangeProofLen);

        uint64_t min_value = 0;
        int ct_exponent = 2;
        int ct_bits = 32;

        const char *message = "narration";
        size_t mlen = strlen(message);

        BOOST_CHECK(secp256k1_rangeproof_sign(ctx,
            &txout.vchRangeproof[0], &nRangeProofLen,
            min_value, &txout.commitment,
            blindptrs.back(), nonce.begin(),
            ct_exponent, ct_bits,
            amount_outs[k],
            (const unsigned char*) message, mlen,
            nullptr, 0,
            secp256k1_generator_h));

        txout.vchRangeproof.resize(nRangeProofLen);
    };

    std::vector<secp256k1_pedersen_commitment*> vpCommitsIn, vpCommitsOut;
    vpCommitsIn.push_back(&txins[0].commitment);

    vpCommitsOut.push_back(&txouts[0].commitment);
    vpCommitsOut.push_back(&txouts[1].commitment);

    BOOST_CHECK(secp256k1_pedersen_verify_tally(ctx, vpCommitsIn.data(), vpCommitsIn.size(), vpCommitsOut.data(), vpCommitsOut.size()));


    for (size_t k = 0; k < txouts.size(); ++k)
    {
        CTxOutValueTest &txout = txouts[k];

        int rexp;
        int rmantissa;
        uint64_t min_value, max_value;

        BOOST_CHECK(secp256k1_rangeproof_info(ctx,
            &rexp, &rmantissa,
            &min_value, &max_value,
            &txout.vchRangeproof[0], txout.vchRangeproof.size()) == 1);


        min_value = 0;
        max_value = 0;
        BOOST_CHECK(1 == secp256k1_rangeproof_verify(ctx, &min_value, &max_value,
            &txout.commitment, txout.vchRangeproof.data(), txout.vchRangeproof.size(),
            nullptr, 0,
            secp256k1_generator_h));


        CPubKey ephemeral_key(txout.vchNonceCommitment);
        BOOST_CHECK(ephemeral_key.IsValid());
        uint256 nonce = kto_outs[k].ECDH(ephemeral_key);
        CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());

        uint8_t blindOut[32];
        unsigned char msg[4096];
        size_t msg_size = sizeof(msg);
        uint64_t amountOut;
        BOOST_CHECK(secp256k1_rangeproof_rewind(ctx,
            blindOut, &amountOut, msg, &msg_size, nonce.begin(),
            &min_value, &max_value,
            &txout.commitment, txout.vchRangeproof.data(), txout.vchRangeproof.size(),
            nullptr, 0,
            secp256k1_generator_h));

        msg[9] = '\0';
        BOOST_CHECK(memcmp(msg, "narration", 9) == 0);
    };

    secp256k1_context_destroy(ctx);
}

BOOST_AUTO_TEST_CASE(ct_parameters_test)
{
    //for (size_t k = 0; k < 10000; ++k)
    for (size_t k = 0; k < 100; ++k)
    {
        CAmount nValue = (GetRand((MAX_MONEY / (k+1))) / COIN) * COIN;
        uint64_t min_value = 0;
        int ct_exponent = 0;
        int ct_bits = 32;

        SelectRangeProofParameters(nValue, min_value, ct_exponent, ct_bits);
    };
}

BOOST_AUTO_TEST_CASE(ct_commitment_test)
{
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    secp256k1_pedersen_commitment commitment1, commitment2, commitment3;
    uint8_t blind[32];
    memset(blind, 0, 32);

    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &commitment1, blind, 10, secp256k1_generator_h));
    BOOST_CHECK(HexStr(commitment1.data, commitment1.data+33) == "093806b3e479859dc6dd508eca22257d796bba3e32a6616cc97b51723b50a5f429");

    memset(blind, 1, 32);
    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &commitment2, blind, 10, secp256k1_generator_h));
    BOOST_CHECK(HexStr(commitment2.data, commitment2.data+33) == "09badd85325926c329aa62f5a7d37d0a015aabfb52608052d277530bd025ddc971");

    secp256k1_pedersen_commitment *pc[2];
    pc[0] = &commitment1;
    pc[1] = &commitment2;
    BOOST_CHECK(secp256k1_pedersen_commitment_sum(ctx, &commitment3, pc, 2));
    BOOST_CHECK(HexStr(commitment3.data, commitment3.data+33) == "09e922a6c61aecd734d79ce41dbf09f71779bfcca6d3f30e4495923eb9801fb9a2");

    secp256k1_context_destroy(ctx);
}

BOOST_AUTO_TEST_SUITE_END()
