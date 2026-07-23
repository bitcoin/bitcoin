// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <bench/bench.h>
#include <chain.h>
#include <node/blockstorage.h>
#include <random.h>
#include <uint256.h>

#include <algorithm>
#include <memory>
#include <vector>

static void CBlockIndexWorkComparator(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};

    constexpr size_t n{1'000};
    std::vector<std::unique_ptr<CBlockIndex>> blocks;
    blocks.reserve(n);
    for (size_t i{0}; i < n; ++i) {
        auto block{std::make_unique<CBlockIndex>()};

        if (i % 10 == 1) {
            // Have some duplicates
            if (rng.randbool()) block->nChainWork = blocks.back()->nChainWork;
            if (rng.randbool()) block->nSequenceId = blocks.back()->nSequenceId;
        } else {
            block->nChainWork = UintToArith256(rng.rand256());
            block->nSequenceId = int32_t(rng.rand32());
        }

        blocks.push_back(std::move(block));
    }
    std::ranges::shuffle(blocks, rng);

    bench.batch(n * n).unit("cmp").run([&] {
        for (size_t i{0}; i < n; ++i) {
            for (size_t j{0}; j < n; ++j) {
                constexpr node::CBlockIndexWorkComparator comparator;
                bool result{comparator(&*blocks[i], &*blocks[j])};
                ankerl::nanobench::doNotOptimizeAway(result);
            }
        }
    });
}

BENCHMARK(CBlockIndexWorkComparator);
