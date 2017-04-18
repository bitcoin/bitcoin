// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_particl.h"

#include "crypto/sha256.h"
#include "key/stealth.h"

#include <secp256k1.h>
#include <secp256k1_rangeproof.h>
#include <secp256k1_mlsag.h>
#include <inttypes.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ringct_tests, BasicTestingSetup)


class CTxOutValueTest
{
public:
    CPubKey pk;
    secp256k1_pedersen_commitment commitment;
    std::vector<unsigned char> vchRangeproof;
    std::vector<unsigned char> vchNonceCommitment;
};

static int memncmp(uint8_t *p, uint8_t c, size_t len)
{
    for (size_t k = 0; k< len; ++k)
    {
        if (*p < c)
            return -1;
        if (*p > c)
            return 1;
    };
    return 0;
}

int testCommitmentSum(secp256k1_context *ctx, CAmount nValueIn,
    std::vector<CAmount> &amountsOut, CAmount nValueOutPlain, size_t nCols,
    bool fPass, bool fUnblindedOutputs=false, bool fUnblindedInputs=false)
{
    BOOST_MESSAGE("nValueIn: " << nValueIn);
    
    int rv;
    std::vector<CTxOutValueTest> txins(1);
    
    std::vector<CKey> kto_outs(amountsOut.size());
    std::vector<CPubKey> pkto_outs(amountsOut.size());
    std::vector<CTxOutValueTest> txouts(amountsOut.size());
    uint8_t blind[txouts.size()][32];
    
    std::vector<CKey> k_ins(1);
    k_ins[0].MakeNewKey(true);
    
    txins[0].pk = k_ins[0].GetPubKey();
    std::vector<const uint8_t*> pBlinds;
    uint8_t blindsin[1][32];
    
    if (fUnblindedInputs)
        memset(&blindsin[0][0], 0, 32);
    else
        GetStrongRandBytes(&blindsin[0][0], 32);
    
    pBlinds.push_back(&blindsin[0][0]);
    
    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txins[0].commitment, &blindsin[0][0], nValueIn, secp256k1_generator_h));
    //BOOST_MESSAGE("C: " << HexStr(&txins[0].commitment.data[0], &txins[0].commitment.data[0]+33));
    
    size_t nBlinded = 0;
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        BOOST_MESSAGE("Output " << k << ": " << amountsOut[k]);
        
        kto_outs[k].MakeNewKey(true);
        pkto_outs[k] = kto_outs[k].GetPubKey();
        
        CTxOutValueTest &txout = txouts[k];
        
        if (fUnblindedOutputs)
            memset(&blind[nBlinded][0], 0, 32);
        else
            GetStrongRandBytes(&blind[nBlinded][0], 32);
        pBlinds.push_back(&blind[nBlinded++][0]);
        
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txout.commitment, (uint8_t*)pBlinds.back(), amountsOut[k], secp256k1_generator_h));
        
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
        
        BOOST_REQUIRE(secp256k1_rangeproof_sign(ctx,
            &txout.vchRangeproof[0], &nRangeProofLen,
            min_value, &txout.commitment,
            pBlinds.back(), nonce.begin(),
            ct_exponent, ct_bits, 
            amountsOut[k],
            NULL, 0,
            NULL, 0,
            secp256k1_generator_h));
        
        txout.vchRangeproof.resize(nRangeProofLen);
    };
    
    size_t nRows = txins.size() + 1; // last row sums commitments
    size_t index = rand() % nCols;
    
    //BOOST_MESSAGE("nCols " << nCols);
    //BOOST_MESSAGE("index " << index);
    
    uint8_t m[nCols * nRows * 33]; // m[(col+(cols*row))*33]
    
    const uint8_t *pcm_in[nCols * txins.size()]; // pcm_in[col+(cols*row)]
    
    size_t haveFee = (nValueOutPlain != 0) ? 1 : 0;
    const uint8_t *pcm_out[txouts.size() + haveFee];
    
    std::vector<CTxOutValueTest> tx_mixin(txins.size() * (nCols-1));
    
    size_t iMix = 0;
    for (size_t k = 0; k < nRows-1; ++k)
    for (size_t i = 0; i < nCols; ++i)
    {
        if (i == index)
        {
            memcpy(&m[(i+k*nCols)*33], txins[k].pk.begin(), 33);
            pcm_in[i+k*nCols] = txins[k].commitment.data;
            continue;
        };
        
        // - make fake input
        CKey key;
        key.MakeNewKey(true);
        
        CTxOutValueTest &mixin = tx_mixin[iMix++];
        mixin.pk = key.GetPubKey();
        CAmount nValue = rand() % (5000 * COIN);
        uint8_t blind[32];
        GetStrongRandBytes(&blind[0], 32);
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &mixin.commitment, blind, nValue, secp256k1_generator_h));
        
        memcpy(&m[(i+k*nCols)*33], mixin.pk.begin(), 33);
        pcm_in[i+k*nCols] = mixin.commitment.data;
    };
    
    uint8_t blindSum[32];
    memset(blindSum, 0, 32);
    
    for (size_t k = 0; k < txouts.size(); ++k)
        pcm_out[k] = txouts[k].commitment.data;
    
    secp256k1_pedersen_commitment feeCommitment;
    if (nValueOutPlain != 0)
    {
        uint8_t zeroBlind[32];
        memset(zeroBlind, 0, 32);
        
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &feeCommitment, &zeroBlind[0], nValueOutPlain, secp256k1_generator_h));
        pcm_out[txouts.size()] = feeCommitment.data;
    };
    
    rv = prepareLastRowMLSAG(txouts.size()+haveFee, txouts.size(), nCols, nRows,
        pcm_in, pcm_out, &pBlinds[0], m, blindSum);
    BOOST_MESSAGE("prepareLastRowMLSAG  " << rv);
    BOOST_REQUIRE(0 == rv);
    
    if (memncmp(blindSum, 0, 32) == 0)
    {
        //BOOST_MESSAGE("m  " << HexStr(&m[(index+(nRows-1)*nCols)*33], &m[(index+(nRows-1)*nCols)*33]+33));
        BOOST_REQUIRE(fPass == (0 == memncmp(&m[(index+(nRows-1)*nCols)*33], 0, 33))); // point at infinity encoded as 0*33
        return 0;
    };
    
    CKey kTest;
    kTest.Set(blindSum, blindSum+32, true);
    BOOST_REQUIRE(kTest.IsValid());
    
    CPubKey pkTestFromCommitmentSum;
    pkTestFromCommitmentSum.Set(&m[(index+(nRows-1)*nCols)*33], &m[(index+(nRows-1)*nCols)*33]+33);
    BOOST_REQUIRE(pkTestFromCommitmentSum.IsValid());
    
    CPubKey pkTestFromBlindKey;
    pkTestFromBlindKey = kTest.GetPubKey();
    BOOST_REQUIRE(pkTestFromBlindKey.IsValid());
    
    //BOOST_MESSAGE("pkTestFromBlindKey       " << HexStr(pkTestFromBlindKey.begin(), pkTestFromBlindKey.end()));
    //BOOST_MESSAGE("pkTestFromCommitmentSum  " << HexStr(pkTestFromCommitmentSum.begin(), pkTestFromCommitmentSum.end()));
    
    BOOST_REQUIRE(fPass == (pkTestFromCommitmentSum == pkTestFromBlindKey));
    return 0;
};

BOOST_AUTO_TEST_CASE(ringct_test_commitment_sums)
{
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    
    std::vector<CAmount> amountsOut(2);
    amountsOut[0] = 30 * COIN;
    amountsOut[1] = -10 * COIN;
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 0, 3, false);
    
    amountsOut[0] = 10 * COIN;
    amountsOut[1] = 10 * COIN;
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 0, 3, true);
    
    amountsOut[0] = 10 * COIN;
    amountsOut[1] = 8 * COIN;
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 2 * COIN, 3, true);
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 3 * COIN, 3, false);
    
    amountsOut[0] = 10 * COIN;
    amountsOut[1] = 8 * COIN;
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 2 * COIN, 3, true, true);
    
    amountsOut.resize(3);
    amountsOut[0] = 5 * COIN;
    amountsOut[1] = 8 * COIN;
    amountsOut[2] = 5 * COIN;
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 2 * COIN, 5, true, true);
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 2 * COIN, 5, true, true, true);
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 2 * COIN, 5, true, false, true);
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 2 * COIN, 5, true);
    
    testCommitmentSum(ctx, 20 * COIN, amountsOut, 1 * COIN, 5, false, true, true);
    
    secp256k1_context_destroy(ctx);
}

BOOST_AUTO_TEST_CASE(ringct_test)
{
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    
    uint32_t rseed;
    GetStrongRandBytes((uint8_t*)&rseed, 4);
    srand(rseed);
    
    std::vector<CTxOutValueTest> txins(1);
    
    std::vector<CKey> k_ins(1);
    k_ins[0].MakeNewKey(true);
    
    txins[0].pk = k_ins[0].GetPubKey();
    std::vector<const uint8_t*> pBlinds;
    uint8_t blindsin[1][32];
    GetStrongRandBytes(&blindsin[0][0], 32);
    pBlinds.push_back(&blindsin[0][0]);
    
    CAmount nValueIn = 45.69 * COIN;
    BOOST_MESSAGE("nValueIn 0: " << nValueIn);
    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txins[0].commitment, &blindsin[0][0], nValueIn, secp256k1_generator_h));
    BOOST_MESSAGE("C: " << HexStr(&txins[0].commitment.data[0], &txins[0].commitment.data[0]+33));
    
    
    std::vector<CStealthAddress> txout_addrs(2);
    std::vector<CTxOutValueTest> txouts(2);
    
    std::vector<CAmount> amount_outs(2);
    amount_outs[0] = 5.69 * COIN;
    amount_outs[1] = 40 * COIN;
    
    std::vector<CKey> kto_outs(2);
    std::vector<CPubKey> pkto_outs(2);
    
    uint8_t blind[txouts.size()][32];
   
    size_t nBlinded = 0;
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        BOOST_MESSAGE("Output " << k << ": " << amount_outs[k]);
        
        kto_outs[k].MakeNewKey(true);
        pkto_outs[k] = kto_outs[k].GetPubKey();
        
        
        CTxOutValueTest &txout = txouts[k];
        
        GetStrongRandBytes(&blind[nBlinded][0], 32);
        pBlinds.push_back(&blind[nBlinded++][0]);
        BOOST_MESSAGE("blind " << HexStr(&blind[nBlinded-1][0], &blind[nBlinded-1][0]+32));
        
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txout.commitment, (uint8_t*)pBlinds.back(), amount_outs[k], secp256k1_generator_h));
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
        
        BOOST_CHECK(secp256k1_rangeproof_sign(ctx,
            &txout.vchRangeproof[0], &nRangeProofLen,
            min_value, &txout.commitment,
            pBlinds.back(), nonce.begin(),
            ct_exponent, ct_bits, 
            amount_outs[k],
            NULL, 0,
            NULL, 0,
            secp256k1_generator_h));
        
        txout.vchRangeproof.resize(nRangeProofLen);
        
        BOOST_MESSAGE("nRangeProofLen: " << nRangeProofLen);
    };
    
    size_t nRows = txins.size() + 1; // last row sums commitments
    //size_t nCols = 3; // ring size
    size_t nCols = 1 + (rand() % 31); // ring size
    
    size_t index = rand() % nCols;
    
    BOOST_MESSAGE("nCols " << nCols);
    BOOST_MESSAGE("index " << index);
    
    uint8_t *m = (uint8_t*) calloc(nCols * nRows, 33); // m[(col+(cols*row))*33]
    BOOST_REQUIRE(m);
    
    const uint8_t *pcm_in[nCols * txins.size()]; // pcm_in[col+(cols*row)]
    const uint8_t *pcm_out[txouts.size()];
    
    std::vector<CTxOutValueTest> tx_mixin(txins.size() * (nCols-1));
    
    size_t iMix = 0;
    for (size_t k = 0; k < nRows-1; ++k)
    for (size_t i = 0; i < nCols; ++i)
    {
        if (i == index)
        {
            memcpy(&m[(i+k*nCols)*33], txins[k].pk.begin(), 33);
            pcm_in[i+k*nCols] = txins[k].commitment.data;
            continue;
        };
        
        // - make fake input
        CKey key;
        key.MakeNewKey(true);
        
        CTxOutValueTest &mixin = tx_mixin[iMix++];
        mixin.pk = key.GetPubKey();
        CAmount nValue = rand() % (5000 * COIN);
        uint8_t blind[32];
        GetStrongRandBytes(&blind[0], 32);
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &mixin.commitment, blind, nValue, secp256k1_generator_h));
        
        memcpy(&m[(i+k*nCols)*33], mixin.pk.begin(), 33);
        pcm_in[i+k*nCols] = mixin.commitment.data;
    };
    
    const uint8_t *sk[nRows]; // pass in pointer to secret keys to keep them in secured memory
    
    for (size_t k = 0; k < txins.size(); ++k)
        sk[k] = k_ins[k].begin();
    uint8_t blindSum[32];
    memset(blindSum, 0, 32);
    sk[nRows-1] = blindSum;
    
    
    for (size_t k = 0; k < txouts.size(); ++k)
        pcm_out[k] = txouts[k].commitment.data;
    BOOST_CHECK(0 == prepareLastRowMLSAG(txouts.size(), txouts.size(), nCols, nRows,
        pcm_in, pcm_out, &pBlinds[0], m, blindSum));
    uint8_t randSeed[32];
    GetStrongRandBytes(randSeed, 32);
    uint8_t preimage[32];
    
    uint8_t ki[33 * txins.size()];
    uint8_t pc[32];
    std::vector<uint8_t> ss(nCols * nRows * 32);
    
    
    BOOST_CHECK(0 == generateMLSAG(ctx, randSeed,
        preimage, nCols, nRows, index, 
        sk, m, ki, pc, &ss[0]));
    /*
    for (size_t k = 0; k < txins.size(); ++k)
        BOOST_MESSAGE("KeyImage " << k << " : " << HexStr(&ki[k], &ki[k]+33));
    
    for (size_t k = 0; k < nRows; ++k)
    for (size_t i = 0; i < nCols; ++i)
    {
        BOOST_MESSAGE("SS: Row " << k << ", Col " << i << " : " 
            << HexStr(&ss[(i+nCols*k)*32], &ss[(i+nCols*k)*32]+32));
    }
    */
    BOOST_CHECK(0 == verifyMLSAG(ctx,
        preimage, nCols, nRows, 
        m, ki, pc, &ss[0]));
    
    free(m);
    
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
    };
    
    secp256k1_context_destroy(ctx);
}



static int GetDeterministicBytes(uint8_t *p, size_t len)
{
    for (size_t k = 0; k < len; ++k)
        *(p+k) = rand() % 256;
    return 0;
};

static int GetBytes(uint8_t *p, size_t len, bool fDeterministic)
{
    if (fDeterministic)
        GetDeterministicBytes(p, len);
    else
        GetStrongRandBytes(p, len);
    return 0;
};

int doTest(secp256k1_context *ctx, size_t nInputs, size_t nOutputs, CAmount nFee, bool fDeterministic)
{
    BOOST_MESSAGE("nFee: " << nFee);
    
    uint8_t tmp[64];
    std::vector<const uint8_t*> pBlinds;
    std::vector<CTxOutValueTest> txins(nInputs);
    std::vector<CKey> kIns(nInputs);
    
    uint8_t blindsIn[nInputs][32];
    
    // Make inputs
    CAmount nInputSum = 0;
    for (size_t k = 0; k < nInputs; ++k)
    {
        CAmount nValueIn = rand() % 4 == 0 ? 0 : nFee * (rand() % 20000);
        BOOST_MESSAGE("Input " << k << ": " << nValueIn);
        
        // Can't use MakeNewKey if fDeterministic
        GetBytes(tmp, 32, fDeterministic);
        kIns[k].Set(tmp, true);
        txins[k].pk = kIns[k].GetPubKey();
        
        GetBytes(&blindsIn[k][0], 32, fDeterministic);
        pBlinds.push_back(&blindsIn[k][0]);
        
        // Make sure there's always enough coin
        if (k == nInputs-1
            && nInputSum < nFee * 2)
            nValueIn = nFee * 2 + nFee * (rand() % 20000);
        nInputSum += nValueIn;
        
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txins[k].commitment, &blindsIn[k][0], nValueIn, secp256k1_generator_h));
        //BOOST_MESSAGE("C: " << HexStr(&txins[0].vchCommitment[0], &txins[0].vchCommitment[0]+33)); 
    };


    std::vector<CStealthAddress> txoutAddrs(nOutputs);
    std::vector<CTxOutValueTest> txouts(nOutputs);

    std::vector<CAmount> amountsOut(nOutputs);
    CAmount nOutputLeft = nInputSum - nFee;
    for (size_t k = 0; k < nOutputs-1; ++k)
    {
        amountsOut[k] = rand() % 4 == 0 ? 0 : (rand() % nOutputLeft);
        nOutputLeft -= amountsOut[k];
    };
    amountsOut[nOutputs-1] = nOutputLeft;
    
    std::vector<CKey> kto_outs(nOutputs);
    std::vector<CPubKey> pkto_outs(nOutputs);
    
    uint8_t blind[txouts.size()][32];
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        BOOST_MESSAGE("Output " << k << ": " << amountsOut[k]);
        
        GetBytes(tmp, 32, fDeterministic);
        kto_outs[k].Set(tmp, true);
        pkto_outs[k] = kto_outs[k].GetPubKey();
        
        CTxOutValueTest &txout = txouts[k];
        
        GetBytes(&blind[k][0], 32, fDeterministic);
        pBlinds.push_back(&blind[k][0]);
        //BOOST_MESSAGE("blind " << HexStr(&blind[k][0], &blind[k][0]+32));
        
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &txout.commitment, (uint8_t*)pBlinds.back(), amountsOut[k], secp256k1_generator_h));
        BOOST_MESSAGE("C " << k << ": " << HexStr(&txout.commitment.data[0], &txout.commitment.data[0]+33));
        
        // Generate ephemeral key for ECDH nonce generation
        CKey ephemeral_key;
        GetBytes(tmp, 32, fDeterministic);
        ephemeral_key.Set(tmp, true);
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

        BOOST_CHECK(secp256k1_rangeproof_sign(ctx,
            &txout.vchRangeproof[0], &nRangeProofLen,
            min_value, &txout.commitment,
            pBlinds.back(), nonce.begin(),
            ct_exponent, ct_bits, 
            amountsOut[k],
            NULL, 0,
            NULL, 0,
            secp256k1_generator_h));
        
        txout.vchRangeproof.resize(nRangeProofLen);

        //BOOST_MESSAGE("nRangeProofLen: " << nRangeProofLen);
    };
    
    size_t nRows = txins.size() + 1; // last row sums commitments
    size_t nCols = 1 + (rand() % 128); // ring size
    size_t index = rand() % nCols;
    BOOST_MESSAGE("nCols " << nCols);
    BOOST_MESSAGE("index " << index);

    uint8_t *m = (uint8_t*) calloc(nCols * nRows, 33); // m[(col+(cols*row))*33]
    BOOST_REQUIRE(m);

    size_t haveFee = (nFee != 0) ? 1 : 0;
    const uint8_t *pcm_in[nCols * txins.size()]; // pcm_in[col+(cols*row)]
    const uint8_t *pcm_out[txouts.size() + haveFee];

    std::vector<CTxOutValueTest> tx_mixin(txins.size() * (nCols-1));

    size_t iMix = 0;
    for (size_t k = 0; k < nRows-1; ++k)
    for (size_t i = 0; i < nCols; ++i)
    {
        if (i == index)
        {
            memcpy(&m[(i+k*nCols)*33], txins[k].pk.begin(), 33);
            pcm_in[i+k*nCols] = txins[k].commitment.data;
            continue;
        };
        
        // - make fake input
        CKey key;
        GetBytes(tmp, 32, fDeterministic);
        key.Set(tmp, true);
        
        CTxOutValueTest &mixin = tx_mixin[iMix++];
        mixin.pk = key.GetPubKey();
        CAmount nValue = rand() % (5000 * COIN);
        
        uint8_t blind[32];
        GetBytes(&blind[0], 32, fDeterministic);
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &mixin.commitment, blind, nValue, secp256k1_generator_h));
        
        memcpy(&m[(i+k*nCols)*33], mixin.pk.begin(), 33);
        pcm_in[i+k*nCols] = mixin.commitment.data;
    };

    const uint8_t *sk[nRows]; // pass in pointer to secret keys to keep them in secured memory

    for (size_t k = 0; k < txins.size(); ++k)
        sk[k] = kIns[k].begin();
    uint8_t blindSum[32]; // memory for last key (sum of blinding factors)
    memset(blindSum, 0, 32);
    sk[nRows-1] = blindSum;

    for (size_t k = 0; k < txouts.size(); ++k)
        pcm_out[k] = txouts[k].commitment.data;
    
    // Add commitment for fee
    secp256k1_pedersen_commitment feeCommitment;
    if (haveFee)
    {
        uint8_t zeroBlind[32];
        memset(zeroBlind, 0, 32);
        
        BOOST_CHECK(secp256k1_pedersen_commit(ctx, &feeCommitment, &zeroBlind[0], nFee, secp256k1_generator_h));
        pcm_out[txouts.size()] = feeCommitment.data;
    };
    
    BOOST_CHECK(0 == prepareLastRowMLSAG(txouts.size()+haveFee, txouts.size(), nCols, nRows,
        pcm_in, pcm_out, &pBlinds[0], m, blindSum));
    
    
    uint8_t randSeed[32], preimage[32], ki[33 * txins.size()], pc[32];
    std::vector<uint8_t> ss(nCols * nRows * 32);
    
    GetBytes(randSeed, 32, fDeterministic);
    
    BOOST_CHECK(0 == generateMLSAG(ctx, randSeed,
        preimage, nCols, nRows, index, 
        sk, m, ki, pc, &ss[0]));
    
    BOOST_CHECK(0 == verifyMLSAG(ctx,
        preimage, nCols, nRows, 
        m, ki, pc, &ss[0]));
    
    free(m);
    /*
    BOOST_MESSAGE("nFee " << nFee);
    BOOST_MESSAGE("nInputSum " << nInputSum);
    
    CAmount nSumOut = 0;
    for (size_t k = 0; k < amountsOut.size(); ++k)
    {
        BOOST_MESSAGE("amountsOut " << k << amountsOut[k]);
        nSumOut += amountsOut[k];
    }
    BOOST_MESSAGE("nSumOut " << nSumOut);
    BOOST_MESSAGE("nSumOut+ nFee " << nSumOut+ nFee);
    */
    
    for (size_t k = 0; k < txouts.size(); ++k)
    {
        //BOOST_MESSAGE("output recover: " << k);
        CTxOutValueTest &txout = txouts[k];
        
        int rexp;
        int rmantissa;
        uint64_t min_value;
        uint64_t max_value;
        
        BOOST_CHECK(1 == secp256k1_rangeproof_info(ctx,
            &rexp, &rmantissa,
            &min_value, &max_value,
            &txout.vchRangeproof[0], txout.vchRangeproof.size()));
        
        BOOST_MESSAGE("rexp " << rexp);
        BOOST_MESSAGE("rmantissa " << rmantissa);
        BOOST_MESSAGE("min_value " << min_value);
        BOOST_MESSAGE("max_value " << max_value);

        BOOST_CHECK(1 == secp256k1_rangeproof_verify(ctx, &min_value, &max_value,
            &txout.commitment, txout.vchRangeproof.data(), txout.vchRangeproof.size(),
            NULL, 0,
            secp256k1_generator_h));
        
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
        
        BOOST_CHECK(amountsOut[k] == (CAmount) amountOut);
        BOOST_MESSAGE("amountOut " << amountOut);
    };
    
    return 0;
}

BOOST_AUTO_TEST_CASE(ringct_test_2in_fee)
{
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    
    uint32_t rseed = 1;
    srand(rseed);
    
    doTest(ctx, 2, 3, 0.001 * COIN, true);
    
    GetStrongRandBytes((uint8_t*)&rseed, 4);
    srand(rseed);
    for (size_t k = 0; k < 10; ++k)
    {
        doTest(ctx, 1 + (rand() % 15), 1 + (rand() % 15), 0.001 * COIN, false);
    };
    
    secp256k1_context_destroy(ctx);
}

BOOST_AUTO_TEST_SUITE_END()
