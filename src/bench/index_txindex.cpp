// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/index_sync_util.h>
#include <index/base.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <primitives/transaction.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/byte_units.h> // IWYU pragma: keep

#include <cassert>
#include <memory>
#include <vector>

// Returns a fresh, not-yet-initialized TxIndex. `f_memory=false` exercises the
// real disk write path, true isolates CPU cost from I/O.
static std::unique_ptr<TxIndex> MakeTxIndex(TestChain100Setup& test_setup, bool f_memory)
{
    return std::make_unique<TxIndex>(interfaces::MakeChain(test_setup.m_node),
                                     /*n_cache_size=*/1_MiB, f_memory, /*f_wipe=*/true);
}

// End-to-end sync of a TxIndex: BaseIndex::Sync -> TxIndex::CustomAppend ->
// TxIndex::DB::WriteTxs.
static void TxIndexSync(benchmark::Bench& bench, bool f_memory)
{
    const auto test_setup = MakeNoLogFileContext<TestChain100Setup>();
    ExtendChainWithSpends(*test_setup, BENCH_INDEX_NUM_BLOCKS, BENCH_INDEX_TXS_PER_BLOCK);

    BenchIndexSync(bench, *test_setup, [&] { return MakeTxIndex(*test_setup, f_memory); });
}

static void TxIndexSyncDisk(benchmark::Bench& bench) { TxIndexSync(bench, /*f_memory=*/false); }
static void TxIndexSyncMem(benchmark::Bench& bench) { TxIndexSync(bench, /*f_memory=*/true); }

// After a full sync, time FindTx() over every txid in the chain. See
// BenchIndexLookup() for what the number covers.
static void TxIndexLookup(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<TestChain100Setup>();
    ExtendChainWithSpends(*test_setup, BENCH_INDEX_NUM_BLOCKS, BENCH_INDEX_TXS_PER_BLOCK);

    // Build and fully sync a persistent txindex, kept alive for the lookups.
    auto index{MakeTxIndex(*test_setup, /*f_memory=*/false)};
    assert(index->Init());
    assert(!index->BlockUntilSyncedToCurrentChain());
    index->Sync();
    assert(index->GetSummary().synced);

    const std::vector<Txid> txids{CollectChainTxids(*test_setup, BENCH_INDEX_FIRST_SPEND_HEIGHT)};

    BenchIndexLookup(bench, txids, [&](const Txid& id) {
        uint256 block_hash;
        CTransactionRef tx;
        return index->FindTx(id, block_hash, tx);
    });

    index->Stop();
}

BENCHMARK(TxIndexSyncDisk);
BENCHMARK(TxIndexSyncMem);
BENCHMARK(TxIndexLookup);
