// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/merkle.h>
#include <random.h>
#include <uint256.h>

#include <cassert>
#include <vector>

static void MerkleRoot(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};

    std::vector<uint256> hashes;
    hashes.resize(9001);
    for (auto& item : hashes) {
        item = rng.rand256();
    }

    constexpr uint256 expected_root{"d8d4dfd014a533bc3941b8663fa6e7f3a8707af124f713164d75b0c3179ecb08"};
    bench.batch(hashes.size()).unit("leaf").run([&] {
        auto leaves{ToMerkleLeaves(hashes, [&](bool, const auto& h) { return h; })};
        const uint256 root{ComputeMerkleRoot(std::move(leaves))};
        assert(root == expected_root);
    });
}

BENCHMARK(MerkleRoot, benchmark::PriorityLevel::HIGH);
