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
//! Return benchmark block bytes in wire format.
DataStream GenerateBlockData(
    const CChainParams& chain_params = *CChainParams::RegTest(CChainParams::RegTestOptions{}),
    const uint256& seed = {}
);

//! Return benchmark block object.
CBlock GenerateBlock(
    const CChainParams& chain_params = *CChainParams::RegTest(CChainParams::RegTestOptions{}),
    const uint256& seed = {});
} // namespace benchmark

#endif // BITCOIN_BENCH_BLOCK_GENERATOR_H
