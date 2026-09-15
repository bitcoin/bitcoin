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
#include <util/byte_units.h> // IWYU pragma: keep
#include <util/check.h>
#include <util/expected.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

// Returns a fresh, not-yet-initialized TxoSpenderIndex. See BenchIndexSync()
// for what f_memory changes. f_wipe empties the database, so the constructor
// regenerates and writes the siphash key on every iteration.
static std::unique_ptr<TxoSpenderIndex> MakeTxoSpenderIndex(TestChain100Setup& test_setup, bool f_memory)
{
    return std::make_unique<TxoSpenderIndex>(interfaces::MakeChain(test_setup.m_node),
                                             /*n_cache_size=*/1_MiB, f_memory, /*f_wipe=*/true);
}

// End-to-end sync of a TxoSpenderIndex: BaseIndex::Sync -> CustomAppend ->
// WriteSpenderInfos.
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
    const int first_spend_height{
        ExtendChainWithSpends(*test_setup, BENCH_INDEX_NUM_BLOCKS, BENCH_INDEX_TXS_PER_BLOCK)};

    auto index{MakeTxoSpenderIndex(*test_setup, /*f_memory=*/false)};
    Assert(index->Init());
    index->Sync();
    Assert(index->GetSummary().synced);

    const std::vector<COutPoint> outpoints{CollectChainSpentOutpoints(*test_setup, first_spend_height)};

    BenchIndexLookup(bench, outpoints, [&](const COutPoint& txo) {
        const auto result{index->FindSpender(txo)};
        return result && result->has_value();
    });

    index->Stop();
}

BENCHMARK(TxoSpenderIndexSyncDisk);
BENCHMARK(TxoSpenderIndexSyncMem);
BENCHMARK(TxoSpenderIndexLookup);
