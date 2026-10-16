// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <bench/bench.h>
#include <chain.h>
#include <node/blockstorage.h>
#include <random.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

static void CompareBlockIndexWork(benchmark::Bench& bench, int work_bits)
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
            block->nChainWork = UintToArith256(rng.rand256()) >> (256 - work_bits);
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

static void CBlockIndexWorkComparator12(benchmark::Bench& bench)
{
    CompareBlockIndexWork(bench, 12);
}

static void CBlockIndexWorkComparator96(benchmark::Bench& bench)
{
    CompareBlockIndexWork(bench, 96);
}

static void CBlockIndexWorkComparator(benchmark::Bench& bench)
{
    CompareBlockIndexWork(bench, 256);
}

BENCHMARK(CBlockIndexWorkComparator12);
BENCHMARK(CBlockIndexWorkComparator96);
BENCHMARK(CBlockIndexWorkComparator);
