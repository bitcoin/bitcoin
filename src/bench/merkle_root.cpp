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

    constexpr size_t block_vtx_size{9001}; // Must be odd to test the special case of the last hash being duplicated.
    std::vector<uint256> leaves;
    leaves.resize(block_vtx_size);
    for (auto& h : leaves) h = rng.rand256();

    constexpr size_t iterations{1000};
    std::vector<std::vector<uint256>> cases(iterations);
    for (auto& h : cases) {
        // TODO reserve to even
        h = leaves; // copies the same vector for each case
    }

    constexpr uint256 expected_root{"d8d4dfd014a533bc3941b8663fa6e7f3a8707af124f713164d75b0c3179ecb08"};
    size_t i{0};
    bench.epochs(1).epochIterations(iterations).batch(block_vtx_size).unit("leaf").run([&] {
        assert(cases[i].size() == block_vtx_size);
        bool mutation{false};
        const uint256 hash{ComputeMerkleRoot(std::move(cases[i]), &mutation)};
        assert(i++ < iterations && !mutation && hash == expected_root);
    });
}

BENCHMARK(MerkleRoot, benchmark::PriorityLevel::HIGH);
