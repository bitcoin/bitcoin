// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_INDEX_SYNC_UTIL_H
#define BITCOIN_BENCH_INDEX_SYNC_UTIL_H

#include <bench/bench.h>
#include <chain.h>
#include <index/base.h>
#include <primitives/block.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/check.h>
#include <validation.h>

#include <memory>
#include <vector>

/**
 * Extends the active chain with blocks of chained transactions.
 * The first transaction spends a mature coinbase, and each later transaction spends the two outputs of its predecessor.
 * Returns the blocks added.
 *
 * Only callable once per setup: it always spends m_coinbase_txns[0].
 */
std::vector<CBlock> ExtendChainWithSpends(TestChain100Setup& test_setup);

/**
 * Times `make_index()` plus a full Init -> Sync -> Stop cycle over the chain in
 * `test_setup`. `make_index` must return a fresh index, wiped and constructed
 * inside the timed region: a second Sync() on a synced index is a no-op.
 *
 * `f_memory=false` keeps the index database on the filesystem.
 * The index stays in LevelDB's write buffer here, so Disk mostly adds the synced writes that create the database.
 * The default data directory may be a RAM-backed tmpfs. See doc/benchmarking.md.
 */
template <typename MakeIndex>
void BenchIndexSync(benchmark::Bench& bench, TestChain100Setup& test_setup, MakeIndex make_index)
{
    // BaseIndex::Commit() writes nothing until the index tip is an ancestor of
    // the last flushed block, and a test setup never flushes on its own.
    test_setup.m_node.chainman->ActiveChainstate().ForceFlushStateToDisk();
    const auto expected_tip{WITH_LOCK(::cs_main, return test_setup.m_node.chainman->ActiveTip()->GetBlockHash())};

    bench.minEpochIterations(5).run([&] {
        std::unique_ptr<BaseIndex> index{make_index()};
        Assert(index->Init());
        index->Sync();

        const IndexSummary summary{index->GetSummary()};
        Assert(summary.synced);
        Assert(summary.best_block_hash == expected_tip);

        StopIndex(*index, test_setup.m_node);
    });
}

/**
 * Syncs `index`, then times `lookup_one(key)` over every key in `keys`,
 * reporting time per lookup via `bench.batch()`. `lookup_one` runs one query
 * and returns whether it hit, which is asserted. What the number measures is
 * mostly the block file read and deserialization the lookup APIs do on top of
 * the database query, which at this chain size is answered from memory.
 */
template <typename Key, typename LookupOne>
void BenchIndexLookup(benchmark::Bench& bench, TestChain100Setup& test_setup, BaseIndex& index,
                      const std::vector<Key>& keys, LookupOne lookup_one)
{
    Assert(!keys.empty());
    Assert(index.Init());
    index.Sync();
    Assert(index.GetSummary().synced);

    bench.batch(keys.size()).minEpochIterations(5).unit("lookup").run([&] {
        // Accumulated without short-circuiting, so every key is queried and the
        // reported per-lookup time really divides by keys.size().
        bool all_found{true};
        for (const Key& key : keys) all_found &= lookup_one(key);
        Assert(all_found);
    });

    StopIndex(index, test_setup.m_node);
}

#endif // BITCOIN_BENCH_INDEX_SYNC_UTIL_H
