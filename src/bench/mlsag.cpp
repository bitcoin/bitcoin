// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <utiltime.h>

#include <blind.h>
#include <random.h>
#include <key.h>
#include <amount.h>

#include <secp256k1_rangeproof.h>
#include <secp256k1_mlsag.h>

static void Mlsag(benchmark::State& state)
{
    ECC_Start_Blinding();

    const size_t nInputs = 2;
    const size_t nCols = 4;
    const size_t nRows = nInputs+1;
    const size_t nRealCol = 1;
    const size_t nOutputs = 2;
    const size_t nBlinded = 1;
    uint8_t tmp32[32], preimage[32];

    int64_t nValues[] = {1234 * COIN, 1234 * COIN, 2467 * COIN, 1 * COIN};

    std::vector<CKey> vKeys(2);
    std::vector<CKey> vBlindsOut(1);
    std::vector<CKey> vBlindsIn(2);

    const uint8_t *pkeys[nInputs+1];
    uint8_t m[nRows * nCols * 33];
    const uint8_t *pcm_in[nInputs * nCols], *pcm_out[nOutputs], *pblinds[nInputs + nOutputs];
    secp256k1_pedersen_commitment cm_in[nInputs * nCols];
    secp256k1_pedersen_commitment cm_out[nOutputs];
    uint8_t pc[32];
    uint8_t ki[nInputs * 33];
    uint8_t ss[(nInputs+1) * nCols * 33]; // max_rows * max_cols




    for (size_t k = 0; k < nBlinded; ++k)
    {
        vBlindsOut[0].MakeNewKey(true);
        pblinds[nInputs + k] = vBlindsOut[k].begin();

        assert(secp256k1_pedersen_commit(secp256k1_ctx_blind, &cm_out[k], pblinds[nInputs + k], nValues[nInputs + k], secp256k1_generator_h));
        pcm_out[k] = cm_out[k].data;
    };

    memset(tmp32, 0, 32); // Use tmp32 for zero
    for (size_t k = nBlinded; k < nOutputs; ++k)
    {
        // NOTE: fails if value <= 0
        assert(secp256k1_pedersen_commit(secp256k1_ctx_blind, &cm_out[k], tmp32, nValues[nInputs + k], secp256k1_generator_h));
        pcm_out[k] = cm_out[k].data;
    };


    for (size_t k = 0; k < nInputs; ++k)
    for (size_t i = 0; i < nCols; ++i)
    {
        if (i == nRealCol)
        {
            vKeys[k].MakeNewKey(true);
            pkeys[k] = vKeys[k].begin();

            CPubKey pk = vKeys[k].GetPubKey();
            memcpy(&m[(i+k*nCols)*33], pk.begin(), 33);


            vBlindsIn[k].MakeNewKey(true);
            pblinds[k] = vBlindsIn[k].begin();

            assert(secp256k1_pedersen_commit(secp256k1_ctx_blind, &cm_in[i+k*nCols], pblinds[k], nValues[k], secp256k1_generator_h));
            pcm_in[i+k*nCols] = cm_in[i+k*nCols].data;
            continue;
        };

        // Fake input
        CKey key;
        key.MakeNewKey(true);
        CPubKey pk = key.GetPubKey();
        memcpy(&m[(i+k*nCols)*33], pk.begin(), 33);

        CAmount v = 10;
        key.MakeNewKey(true);
        assert(secp256k1_pedersen_commit(secp256k1_ctx_blind, &cm_in[i+k*nCols], key.begin(), v, secp256k1_generator_h));
        pcm_in[i+k*nCols] = cm_in[i+k*nCols].data;
    };

    uint8_t blindSum[32];
    pkeys[nInputs] = blindSum;

    assert(0 == secp256k1_prepare_mlsag(m, blindSum,
        nOutputs, nBlinded, nCols, nRows,
        pcm_in, pcm_out, pblinds));

    GetRandBytes(tmp32, 32);
    GetRandBytes(preimage, 32);

    assert(0 == secp256k1_generate_mlsag(secp256k1_ctx_blind, ki, pc, ss,
        tmp32, preimage, nCols, nRows, nRealCol,
        pkeys, m));



    while (state.KeepRunning())
    {
        assert(0 == secp256k1_verify_mlsag(secp256k1_ctx_blind,
            preimage, nCols, nRows,
            m, ki, pc, ss));
    };

    ECC_Stop_Blinding();
}

BENCHMARK(Mlsag, 10);
