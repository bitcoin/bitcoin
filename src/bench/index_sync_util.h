// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_INDEX_SYNC_UTIL_H
#define BITCOIN_BENCH_INDEX_SYNC_UTIL_H

#include <bench/bench.h>
#include <index/base.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <validation.h>

#include <cstdint>
#include <memory>
#include <vector>

/** Blocks of chained spends the benchmarks build. ExtendChainWithSpends() mines
 *  one more on top of these to spend a coinbase, over TestChain100Setup's 100.
 *  Building the chain is not timed, but it is most of the wall time of a run. */
static constexpr uint32_t BENCH_INDEX_NUM_BLOCKS{50};
/** Chained transactions in each of those blocks. */
static constexpr uint32_t BENCH_INDEX_TXS_PER_BLOCK{50};

/**
 * Extends the active chain with `num_blocks + 1` blocks: one spending a mature
 * coinbase, then `num_blocks` of `num_txs_per_block` chained transactions, each
 * spending the two outputs of the previous one. Returns the first height added.
 *
 * Only callable once per setup: it always spends m_coinbase_txns[0]. Flushes
 * the chainstate, without which BaseIndex::Commit() writes nothing.
 */
int ExtendChainWithSpends(TestChain100Setup& test_setup, uint32_t num_blocks, uint32_t num_txs_per_block);

/**
 * Times `make_index()` plus a full Init -> Sync -> Stop cycle over the chain in
 * `test_setup`. `make_index` must return a fresh index, wiped and constructed
 * inside the timed region: a second Sync() on a synced index is a no-op.
 *
 * Disk vs Mem: f_memory=false keeps the index database on the filesystem, true
 * keeps it in memory. The data directory defaults to fs::temp_directory_path(),
 * a tmpfs on many Linux systems where Disk writes to RAM too, so the Disk
 * figure only means what its name says when the directory is on real storage:
 *
 *     build/bin/bench_bitcoin -filter='<benchmark>' -testdatadir=/path/on/disk
 *
 * The chain is small enough that the index never leaves LevelDB's write buffer,
 * so what Disk adds over Mem is page cache traffic. See doc/benchmarking.md.
 */
template <typename MakeIndex>
void BenchIndexSync(benchmark::Bench& bench, TestChain100Setup& test_setup, MakeIndex make_index)
{
    // The tip is fixed for the whole run, and cs_main stays out of the loop.
    const auto expected_tip{WITH_LOCK(::cs_main, return test_setup.m_node.chainman->ActiveTip()->GetBlockHash())};

    bench.minEpochIterations(5).run([&] {
        std::unique_ptr<BaseIndex> index{make_index()};
        Assert(index->Init());
        index->Sync();

        const IndexSummary summary{index->GetSummary()};
        Assert(summary.synced);
        Assert(summary.best_block_hash == expected_tip);

        // Shutdown sequence (c.f. Shutdown() in init.cpp)
        index->Stop();
    });
}

/** All txids in the active chain from `from_height` (>= 0) to the tip, in block/tx order. */
std::vector<Txid> CollectChainTxids(TestChain100Setup& test_setup, int from_height);

/** All outpoints spent by non-coinbase transactions from `from_height` (>= 0) to the tip. */
std::vector<COutPoint> CollectChainSpentOutpoints(TestChain100Setup& test_setup, int from_height);

/**
 * Times `lookup_one(key)` over every key in `keys` on an already synced index,
 * reporting time per lookup via `bench.batch()`. `lookup_one` runs one query
 * and returns whether it hit, which is asserted. What the number measures is
 * mostly the block file read and deserialization the lookup APIs do on top of
 * the database query, which at this chain size is answered from memory.
 */
template <typename Key, typename LookupOne>
void BenchIndexLookup(benchmark::Bench& bench, const std::vector<Key>& keys, LookupOne lookup_one)
{
    Assert(!keys.empty());
    bench.batch(keys.size()).minEpochIterations(5).unit("lookup").run([&] {
        // Accumulated without short-circuiting, so every key is queried and the
        // reported per-lookup time really divides by keys.size().
        bool all_found{true};
        for (const Key& key : keys) all_found &= lookup_one(key);
        Assert(all_found);
    });
}

#endif // BITCOIN_BENCH_INDEX_SYNC_UTIL_H
