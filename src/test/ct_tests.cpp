// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_particl.h"

#include "crypto/sha256.h"

#include <secp256k1.h>
#include <secp256k1_rangeproof.h>
#include <inttypes.h>

#include <boost/test/unit_test.hpp>

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
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    
    
    std::vector<CTxOutValueTest> txins(1);
    
    std::vector<const uint8_t*> blindptrs;
    uint8_t blindsin[1][32];
    GetStrongRandBytes(&blindsin[0][0], 32);
    blindptrs.push_back(&blindsin[0][0]);
    
    CAmount nValueIn = 45.69 * COIN;
    BOOST_MESSAGE("nValueIn 0: " << nValueIn);
    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txins[0].commitment, &blindsin[0][0], nValueIn, secp256k1_generator_h));
    BOOST_MESSAGE("C: " << HexStr(&txins[0].commitment.data[0], &txins[0].commitment.data[0]+33));
    
    std::vector<CTxOutValueTest> txouts(2);
    
    std::vector<CAmount> amount_outs(2);
    amount_outs[0] = 5.69 * COIN;
    amount_outs[1] = 40 * COIN;
    
    std::vector<CKey> kto_outs(2);
    kto_outs[0].MakeNewKey(true);
    kto_outs[1].MakeNewKey(true);
    
    std::vector<CPubKey> pkto_outs(2);
    pkto_outs[0] = kto_outs[0].GetPubKey();
    pkto_outs[1] = kto_outs[1].GetPubKey();
    
    uint8_t blind[txouts.size()][32];
    
    size_t nBlinded = 0;
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        BOOST_MESSAGE("output: " << k);
        CTxOutValueTest &txout = txouts[k];
        
        if (nBlinded + 1 == txouts.size())
        {
            // Last to-be-blinded value: compute from all other blinding factors.
            // sum of output blinding values must equal sum of input blinding values
            BOOST_CHECK(secp256k1_pedersen_blind_sum(ctx, &blind[nBlinded][0], &blindptrs[0], 2, 1));
            blindptrs.push_back(&blind[nBlinded++][0]);
            BOOST_MESSAGE("secp256k1_pedersen_blind_sum " << HexStr(&blind[nBlinded-1][0], &blind[nBlinded-1][0]+32));
        } else
        {
            GetStrongRandBytes(&blind[nBlinded][0], 32);
            blindptrs.push_back(&blind[nBlinded++][0]);
            BOOST_MESSAGE("GetStrongRandBytes " << HexStr(&blind[nBlinded-1][0], &blind[nBlinded-1][0]+32));
        };
        
        BOOST_MESSAGE("output " << k << ": " << amount_outs[k]);
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txout.commitment, (uint8_t*)blindptrs.back(), amount_outs[k], secp256k1_generator_h));
        BOOST_MESSAGE("C " << k << ": " << HexStr(&txout.commitment.data[0], &txout.commitment.data[0]+33));
        
        // Generate ephemeral key for ECDH nonce generation
        CKey ephemeral_key;
        ephemeral_key.MakeNewKey(true);
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
            NULL, 0,
            secp256k1_generator_h));
        
        txout.vchRangeproof.resize(nRangeProofLen);
        
        BOOST_MESSAGE("nRangeProofLen: " << nRangeProofLen);
    };
    
    std::vector<secp256k1_pedersen_commitment*> vpCommitsIn, vpCommitsOut;
    vpCommitsIn.push_back(&txins[0].commitment);
    BOOST_MESSAGE("Cin 1: " << HexStr(&txins[0].commitment.data[0], &txins[0].commitment.data[0]+33));
    
    vpCommitsOut.push_back(&txouts[0].commitment);
    vpCommitsOut.push_back(&txouts[1].commitment);
    BOOST_MESSAGE("C 1: " << HexStr(&txouts[0].commitment.data[0], &txouts[0].commitment.data[0]+33));
    BOOST_MESSAGE("C 2: " << HexStr(&txouts[1].commitment.data[0], &txouts[1].commitment.data[0]+33));
    
    BOOST_CHECK(secp256k1_pedersen_verify_tally(ctx, vpCommitsIn.data(), vpCommitsIn.size(), vpCommitsOut.data(), vpCommitsOut.size()));
    
    
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        BOOST_MESSAGE("output recover: " << k);
        CTxOutValueTest &txout = txouts[k];
        
        int rexp;
        int rmantissa;
        uint64_t min_value;
        uint64_t max_value;
        
        BOOST_CHECK(secp256k1_rangeproof_info(ctx,
            &rexp, &rmantissa,
            &min_value, &max_value,
            &txout.vchRangeproof[0], txout.vchRangeproof.size()) == 1);
        
        BOOST_MESSAGE("rexp " << rexp);
        BOOST_MESSAGE("rmantissa " << rmantissa);
        BOOST_MESSAGE("min_value " << min_value);
        BOOST_MESSAGE("max_value " << max_value);
        
        
        min_value = 0;
        max_value = 0;
        BOOST_CHECK(1 == secp256k1_rangeproof_verify(ctx, &min_value, &max_value,
            &txout.commitment, txout.vchRangeproof.data(), txout.vchRangeproof.size(),
            NULL, 0,
            secp256k1_generator_h));
        BOOST_MESSAGE("min_value " << min_value);
        BOOST_MESSAGE("max_value " << max_value);
        
        
        CPubKey ephemeral_key(txout.vchNonceCommitment);
        BOOST_CHECK(ephemeral_key.IsValid());
        uint256 nonce = kto_outs[k].ECDH(ephemeral_key);
        CSHA256().Write(nonce.begin(), 32).Finalize(nonce.begin());
        
        uint8_t blindOut[32];
        unsigned char msg[4096];
        size_t msg_size;
        uint64_t amountOut;
        BOOST_CHECK(secp256k1_rangeproof_rewind(ctx,
            blindOut, &amountOut, msg, &msg_size, nonce.begin(),
            &min_value, &max_value, 
            &txout.commitment, txout.vchRangeproof.data(), txout.vchRangeproof.size(),
            NULL, 0,
            secp256k1_generator_h));
        
        BOOST_MESSAGE("amountOut " << amountOut);
        BOOST_MESSAGE("msg_size " << msg_size);
        msg[9] = '\0';
        BOOST_MESSAGE("msg " << msg);
        BOOST_CHECK(memcmp(msg, "narration", 9) == 0);
    };
    
    secp256k1_context_destroy(ctx);
}

BOOST_AUTO_TEST_SUITE_END()
