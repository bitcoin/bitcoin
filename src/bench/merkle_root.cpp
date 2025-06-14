// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/merkle.h>
#include <random.h>
#include <uint256.h>

#include <vector>

static void MerkleRoot(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    std::vector<uint256> leaves(9000); // even number of leaves
    for (auto& item : leaves) {
        item = rng.rand256();
    }
    bench.batch(leaves.size()).unit("leaf").run([&] {
        bool mutation{false};
        const uint256 hash{ComputeMerkleRoot(std::vector(leaves), &mutation)};
        assert(mutation == false && hash == uint256{"18ff9b2bef46a834b2625871b2839fec3b67fc1243920c424858324fbde1a8de"});
    });
}

BENCHMARK(MerkleRoot, benchmark::PriorityLevel::HIGH);
