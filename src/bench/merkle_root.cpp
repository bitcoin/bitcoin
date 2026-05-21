// Copyright (c) 2016-present The Bitcoin Core developers
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

    std::vector<uint256> hashes{};
    hashes.resize(9001);
    for (auto& item : hashes) {
        item = rng.rand256();
    }

    constexpr uint256 expected_root{"d8d4dfd014a533bc3941b8663fa6e7f3a8707af124f713164d75b0c3179ecb08"};
    for (bool mutate : {false, true}) {
        bench.name(mutate ? "MerkleRootWithMutation" : "MerkleRoot").batch(hashes.size()).unit("leaf").run([&] {
            std::vector<uint256> leaves;
            leaves.reserve((hashes.size() + 1) & ~1ULL); // capacity rounded up to even
            for (const auto& hash : hashes) {
                leaves.push_back(hash);
            }

            bool mutated{false};
            const uint256 root{ComputeMerkleRoot(std::move(leaves), mutate ? &mutated : nullptr)};
            assert(root == expected_root);
        });
    }
}

BENCHMARK(MerkleRoot);
