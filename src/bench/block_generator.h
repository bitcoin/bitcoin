// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BLOCK_GENERATOR_H
#define BITCOIN_BENCH_BLOCK_GENERATOR_H

#include <kernel/chainparams.h>
#include <primitives/block.h>
#include <uint256.h>

#include <streams.h>

namespace benchmark {
struct ScriptRecipe {
    //! per-script type probabilities - must sum to ~1.0
    double anchor_prob{0}; //!< pay-to-anchor (P2A)
    double multisig_prob{0}; //!< bare multisig
    double null_data_prob{0}; //!< null data (OP_RETURN)
    double pubkey_prob{0}; //!< pay-to-pubkey (P2PK)
    double pubkeyhash_prob{0}; //!< pay-to-pubkey-hash (P2PKH)
    double scripthash_prob{0}; //!< pay-to-script-hash (P2SH)
    double witness_v1_taproot_prob{0}; //!< pay-to-taproot (P2TR)
    double witness_v0_keyhash_prob{0}; //!< pay-to-witness-pubkey-hash (P2WPKH)
    double witness_v0_scripthash_prob{0}; //!< pay-to-witness-script-hash (P2WSH)
    double witness_unknown_prob{0}; //!< unknown witness program
    double nonstandard_prob{0}; //!< bare OP_TRUE
    double random_bytes_prob{0}; //!< raw scripts from random bytes

    size_t tx_count{0}; //!< Does not include the coinbase tx. Leave at 0 to pick a random seeded count.
    double geometric_base_prob{0.5}; //!< Controls geometric distribution for vin/vout/witness sizes.
};

//! mostly legacy outputs, useful for pre-SegWit baselines
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
    .random_bytes_prob = 0.54,
    .tx_count = 1500,
};

//! witness-heavy mix (roughly Taproot-era main chain)
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
    .random_bytes_prob = 0.421,
    .tx_count = 2000,
};

//! Generate benchmark block data in wire form.
//! Any given seed yields deterministic output; the default zero seed provides a stable baseline.
DataStream GenerateBlockData(
    const CChainParams& chain_params = *CChainParams::RegTest(CChainParams::RegTestOptions{}),
    const ScriptRecipe& = kWitness,
    const uint256& seed = {}
);

//! Generate a benchmark block object.
//! Any given seed yields deterministic output; the default zero seed provides a stable baseline.
CBlock GenerateBlock(
    const CChainParams& chain_params = *CChainParams::RegTest(CChainParams::RegTestOptions{}),
    const ScriptRecipe& = kWitness,
    const uint256& seed = {});
} // namespace benchmark

#endif // BITCOIN_BENCH_BLOCK_GENERATOR_H
