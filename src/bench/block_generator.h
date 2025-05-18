// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BLOCK_GENERATOR_H
#define BITCOIN_BENCH_BLOCK_GENERATOR_H

#include <kernel/chainparams.h>
#include <primitives/block.h>
#include <uint256.h>

#include <streams.h>

namespace benchmark {
struct ScriptRecipe
{
    //! per-output type probabilities ─ must sum ≤ 1.0
    double anchor_prob{0};
    double multisig_prob{0};
    double null_data_prob{0};
    double pubkey_prob{0};
    double pubkeyhash_prob{0};
    double scripthash_prob{0};
    double witness_v1_taproot_prob{0};
    double witness_v0_keyhash_prob{0};
    double witness_v0_scripthash_prob{0};
    double witness_unknown_prob{0};
    double nonstandard_prob{0};

    //! tuning
    size_t tx_count{0};
    double geometric_base_prob{0.5};
};

/* purely legacy outputs, useful for pre-SegWit baselines */
inline constexpr ScriptRecipe kLegacy{
    .anchor_prob = 0.00,
    .multisig_prob = 0.05,
    .null_data_prob = 0.005,
    .pubkey_prob = 0.10,
    .pubkeyhash_prob = 0.20,
    .scripthash_prob = 0.10,
    .witness_v1_taproot_prob = 0.00,
    .witness_v0_keyhash_prob = 0.00,
    .witness_v0_scripthash_prob = 0.00,
    .witness_unknown_prob = 0.00,
    .nonstandard_prob = 0.005,

    .tx_count = 1500,
};

/* witness-heavy mix (roughly Taproot-era main chain) */
inline constexpr ScriptRecipe kWitness{
    .anchor_prob = 0.001,
    .multisig_prob = 0.005,
    .null_data_prob = 0.002,
    .pubkey_prob = 0.001,
    .pubkeyhash_prob = 0.02,
    .scripthash_prob = 0.01,
    .witness_v1_taproot_prob = 0.20,
    .witness_v0_keyhash_prob = 0.25,
    .witness_v0_scripthash_prob = 0.05,
    .witness_unknown_prob = 0.02,
    .nonstandard_prob = 0.02,

    .tx_count = 2000,
};

DataStream GetBlockData(
    const CChainParams& chain_params = *CChainParams::RegTest(CChainParams::RegTestOptions{}),
    const ScriptRecipe& = kWitness,
    const uint256& seed = {}
);
CBlock GetBlock(
    const CChainParams& chain_params = *CChainParams::RegTest(CChainParams::RegTestOptions{}),
    const ScriptRecipe& = kWitness,
    const uint256& seed = {});
} // namespace benchmark
#endif // BITCOIN_BENCH_BLOCK_GENERATOR_H
