// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <utiltime.h>

#include <blind.h>
#include <random.h>
#include <key.h>

#include <secp256k1_rangeproof.h>

static void Blind(benchmark::State& state)
{
    ECC_Start_Blinding();

    secp256k1_pedersen_commitment commitment;
    std::vector<uint8_t> vchRangeproof;
    uint64_t min_value = 0;
    int ct_exponent = 2;
    int ct_bits = 32;

    CKey ephemeral_key;
    ephemeral_key.MakeNewKey(true);

    CAmount nValue = 1 * COIN;

    std::vector<uint8_t> vBlind(32);
    GetStrongRandBytes(&vBlind[0], 32);

    assert(secp256k1_pedersen_commit(secp256k1_ctx_blind, &commitment, &vBlind[0], (uint64_t)nValue, secp256k1_generator_h));

    // Create range proof
    size_t nRangeProofLen = 5134;
    vchRangeproof.resize(nRangeProofLen);
    assert(secp256k1_rangeproof_sign(secp256k1_ctx_blind,
        &vchRangeproof[0], &nRangeProofLen,
        min_value, &commitment,
        &vBlind[0], ephemeral_key.begin(),
        ct_exponent, ct_bits,
        nValue,
        nullptr, 0,
        nullptr, 0,
        secp256k1_generator_h));

    vchRangeproof.resize(nRangeProofLen);

    uint64_t max_value = 0;

    while (state.KeepRunning())
    {
        assert(1 == secp256k1_rangeproof_verify(secp256k1_ctx_blind, &min_value, &max_value,
            &commitment, vchRangeproof.data(), vchRangeproof.size(),
            nullptr, 0, secp256k1_generator_h));
    };

    ECC_Stop_Blinding();
}

BENCHMARK(Blind, 10);
