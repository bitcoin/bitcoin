// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bench/bench.h>
#include <bench/index_sync_util.h>
#include <blockfilter.h>
#include <chain.h>
#include <index/base.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <validation.h>

#include <memory>
#include <span>
#include <vector>

using namespace util::hex_literals;

// Very simple block filter index sync benchmark, only using coinbase outputs.
static void BlockFilterIndexSync(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<TestChain100Setup>();

    // Create more blocks
    int CHAIN_SIZE = 600;
    CPubKey pubkey{"02ed26169896db86ced4cbb7b3ecef9859b5952825adbeab998fb5b307e54949c9"_hex_u8};
    CScript script = GetScriptForDestination(WitnessV0KeyHash(pubkey));
    std::vector<CMutableTransaction> noTxns;
    for (int i = 0; i < CHAIN_SIZE - 100; i++) {
        test_setup->CreateAndProcessBlock(noTxns, script);
        test_setup->m_clock += 1s;
    }
    assert(WITH_LOCK(::cs_main, return test_setup->m_node.chainman->ActiveHeight() == CHAIN_SIZE));

    bench.minEpochIterations(5).run([&] {
        BlockFilterIndex filter_index(interfaces::MakeChain(test_setup->m_node), BlockFilterType::BASIC,
                                      /*n_cache_size=*/0, /*f_memory=*/false, /*f_wipe=*/true);
        assert(filter_index.Init());
        assert(!filter_index.BlockUntilSyncedToCurrentChain());
        filter_index.Sync();

        IndexSummary summary = filter_index.GetSummary();
        assert(summary.synced);
        assert(summary.best_block_hash == WITH_LOCK(::cs_main, return test_setup->m_node.chainman->ActiveTip()->GetBlockHash()));

        // Shutdown sequence (c.f. Shutdown() in init.cpp)
        filter_index.Stop();
    });
}

BENCHMARK(BlockFilterIndexSync);

// Returns a fresh, not-yet-initialized BASIC BlockFilterIndex. `f_memory=false`
// exercises the real disk write path, true isolates CPU cost from I/O.
static std::unique_ptr<BlockFilterIndex> MakeBlockFilterIndex(TestChain100Setup& test_setup, bool f_memory)
{
    return std::make_unique<BlockFilterIndex>(interfaces::MakeChain(test_setup.m_node), BlockFilterType::BASIC,
                                              /*n_cache_size=*/1 << 20, f_memory, /*f_wipe=*/true);
}

// Same sync as BlockFilterIndexSync above, but over blocks that carry
// transactions paying to distinct scripts, so the filters hold elements
// proportional to the number of transactions rather than a handful per block.
static void BlockFilterIndexSyncRealistic(benchmark::Bench& bench, bool f_memory)
{
    const auto test_setup = MakeNoLogFileContext<TestChain100Setup>();
    ExtendChainWithSpends(*test_setup, BENCH_INDEX_NUM_BLOCKS, BENCH_INDEX_TXS_PER_BLOCK);

    BenchIndexSync(bench, *test_setup, [&] { return MakeBlockFilterIndex(*test_setup, f_memory); });
}

static void BlockFilterIndexSyncRealisticDisk(benchmark::Bench& bench) { BlockFilterIndexSyncRealistic(bench, /*f_memory=*/false); }
static void BlockFilterIndexSyncRealisticMem(benchmark::Bench& bench) { BlockFilterIndexSyncRealistic(bench, /*f_memory=*/true); }
BENCHMARK(BlockFilterIndexSyncRealisticDisk);
BENCHMARK(BlockFilterIndexSyncRealisticMem);
