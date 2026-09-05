// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_INDEX_SYNC_UTIL_H
#define BITCOIN_BENCH_INDEX_SYNC_UTIL_H

#include <bench/bench.h>
#include <chain.h>
#include <index/base.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

/** Size of the chain the index benchmarks build. Building it is not timed, but it is most of a run. */
static constexpr uint32_t BENCH_INDEX_NUM_BLOCKS{50};
static constexpr uint32_t BENCH_INDEX_TXS_PER_BLOCK{50};
/** First height added by ExtendChainWithSpends to a fresh TestChain100Setup. */
static constexpr int BENCH_INDEX_FIRST_SPEND_HEIGHT{101};

/**
 * Extends the active chain with `num_blocks` blocks of `num_txs_per_block`
 * chained, validly-signed transactions. Each spends the two outputs of the
 * transaction before it in the same block and creates two new ones, each paying
 * a distinct key.
 */
void ExtendChainWithSpends(TestChain100Setup& test_setup, uint32_t num_blocks, uint32_t num_txs_per_block);

/**
 * Times `make_index()` plus a full Init -> BlockUntilSyncedToCurrentChain ->
 * Sync -> Stop cycle over the chain in `test_setup`. `make_index` runs once per
 * iteration and must return a fresh, not-yet-initialized index, since an index
 * cannot be re-synced after Stop(). Constructing one opens and wipes its
 * database, inside the timed region.
 */
template <typename MakeIndex>
void BenchIndexSync(benchmark::Bench& bench, TestChain100Setup& test_setup, MakeIndex make_index)
{
    // The tip doesn't change during the run, so take it once here to keep the
    // lock out of the measurement.
    const auto expected_tip{WITH_LOCK(::cs_main, return test_setup.m_node.chainman->ActiveTip()->GetBlockHash())};

    bench.minEpochIterations(5).run([&] {
        std::unique_ptr<BaseIndex> index{make_index()};
        assert(index->Init());
        assert(!index->BlockUntilSyncedToCurrentChain());
        index->Sync();

        const IndexSummary summary{index->GetSummary()};
        assert(summary.synced);
        assert(summary.best_block_hash == expected_tip);

        // Shutdown sequence (c.f. Shutdown() in init.cpp)
        index->Stop();
    });
}

/** All txids in the active chain from `from_height` to the tip, in block/tx order. */
std::vector<Txid> CollectChainTxids(TestChain100Setup& test_setup, int from_height);

/** All outpoints spent by non-coinbase transactions from `from_height` to the tip. */
std::vector<COutPoint> CollectChainSpentOutpoints(TestChain100Setup& test_setup, int from_height);

/**
 * Times `lookup_one(key)` over every key in `keys` on an already synced index,
 * reporting time per lookup via `bench.batch()`. `lookup_one` runs one query
 * and returns whether it hit, which is asserted.
 *
 * Two things the number includes: the caches are warm, since the lookups run in
 * the process that just wrote the index, and the lookup APIs do not stop at the
 * database, they also read and deserialize the transaction from the block file.
 */
template <typename Key, typename LookupOne>
void BenchIndexLookup(benchmark::Bench& bench, const std::vector<Key>& keys, LookupOne lookup_one)
{
    assert(!keys.empty());
    bench.batch(keys.size()).unit("lookup").run([&] {
        // Accumulated without short-circuiting, so every key is queried and the
        // reported per-lookup time really divides by keys.size().
        bool all_found{true};
        for (const Key& key : keys) all_found &= lookup_one(key);
        assert(all_found);
    });
}

#endif // BITCOIN_BENCH_INDEX_SYNC_UTIL_H
