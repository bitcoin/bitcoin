// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/index_sync_util.h>
#include <index/base.h>
#include <index/txospenderindex.h>
#include <interfaces/chain.h>
#include <primitives/transaction.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/byte_units.h> // IWYU pragma: keep
#include <util/expected.h>

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Returns a fresh, not-yet-initialized TxoSpenderIndex. `f_memory=false`
// exercises the real disk write path, true isolates CPU cost from I/O.
static std::unique_ptr<TxoSpenderIndex> MakeTxoSpenderIndex(TestChain100Setup& test_setup, bool f_memory)
{
    return std::make_unique<TxoSpenderIndex>(interfaces::MakeChain(test_setup.m_node),
                                             /*n_cache_size=*/1_MiB, f_memory, /*f_wipe=*/true);
}

// End-to-end sync of a TxoSpenderIndex: BaseIndex::Sync -> CustomAppend ->
// BuildSpenderPositions.
static void TxoSpenderIndexSync(benchmark::Bench& bench, bool f_memory)
{
    const auto test_setup = MakeNoLogFileContext<TestChain100Setup>();
    ExtendChainWithSpends(*test_setup, BENCH_INDEX_NUM_BLOCKS, BENCH_INDEX_TXS_PER_BLOCK);

    BenchIndexSync(bench, *test_setup, [&] { return MakeTxoSpenderIndex(*test_setup, f_memory); });
}

static void TxoSpenderIndexSyncDisk(benchmark::Bench& bench) { TxoSpenderIndexSync(bench, /*f_memory=*/false); }
static void TxoSpenderIndexSyncMem(benchmark::Bench& bench) { TxoSpenderIndexSync(bench, /*f_memory=*/true); }

// After a full sync, time FindSpender() over every spent outpoint in the chain.
// See BenchIndexLookup() for what the number covers.
static void TxoSpenderIndexLookup(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<TestChain100Setup>();
    ExtendChainWithSpends(*test_setup, BENCH_INDEX_NUM_BLOCKS, BENCH_INDEX_TXS_PER_BLOCK);

    auto index{MakeTxoSpenderIndex(*test_setup, /*f_memory=*/false)};
    assert(index->Init());
    assert(!index->BlockUntilSyncedToCurrentChain());
    index->Sync();
    assert(index->GetSummary().synced);

    const std::vector<COutPoint> outpoints{CollectChainSpentOutpoints(*test_setup, BENCH_INDEX_FIRST_SPEND_HEIGHT)};

    BenchIndexLookup(bench, outpoints, [&](const COutPoint& txo) {
        const auto result{index->FindSpender(txo)};
        return result && result->has_value();
    });

    index->Stop();
}

BENCHMARK(TxoSpenderIndexSyncDisk);
BENCHMARK(TxoSpenderIndexSyncMem);
BENCHMARK(TxoSpenderIndexLookup);
