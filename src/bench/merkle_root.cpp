// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <uint256.h>
#include <random.h>
#include <consensus/merkle.h>

static void MerkleRoot(benchmark::State& state)
{
    FastRandomContext rng(true);
    std::vector<uint256> leaves;
    leaves.resize(9001);
    for (auto& item : leaves) {
        item = rng.rand256();
    }
    while (state.KeepRunning()) {
        bool mutation = false;
        uint256 hash = ComputeMerkleRoot(std::vector<uint256>(leaves), &mutation);
        leaves[mutation] = hash;
    }
}

BENCHMARK(MerkleRoot, 800);
