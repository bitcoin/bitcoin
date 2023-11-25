// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <kernel/disconnected_transactions.h>
#include <primitives/block.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

constexpr size_t BLOCK_VTX_COUNT{4000};
constexpr size_t BLOCK_VTX_COUNT_10PERCENT{400};

using BlockTxns = decltype(CBlock::vtx);

/** Reorg where 1 block is disconnected and 2 blocks are connected. */
struct ReorgTxns {
    /** Disconnected block. */
    BlockTxns disconnected_txns;
    /** First connected block. */
    BlockTxns connected_txns_1;
    /** Second connected block, new chain tip. Has no overlap with disconnected_txns. */
    BlockTxns connected_txns_2;
    /** Transactions shared between disconnected_txns and connected_txns_1. */
    size_t num_shared;
};

static BlockTxns CreateRandomTransactions(size_t num_txns)
{
    // Ensure every transaction has a different txid by having each one spend the previous one.
    static Txid prevout_hash{};

    BlockTxns txns;
    txns.reserve(num_txns);
    // Simplest spk for every tx
    CScript spk = CScript() << OP_TRUE;
    for (uint32_t i = 0; i < num_txns; ++i) {
        CMutableTransaction tx;
        tx.vin.emplace_back(COutPoint{prevout_hash, 0});
        tx.vout.emplace_back(CENT, spk);
        auto ptx{MakeTransactionRef(tx)};
        txns.emplace_back(ptx);
        prevout_hash = ptx->GetHash();
    }
    return txns;
}

/** Creates blocks for a Reorg, each with BLOCK_VTX_COUNT transactions. Between the disconnected
 * block and the first connected block, there will be num_not_shared transactions that are
 * different, and all other transactions the exact same. The second connected block has all unique
 * transactions. This is to simulate a reorg in which all but num_not_shared transactions are
 * confirmed in the new chain. */
static ReorgTxns CreateBlocks(size_t num_not_shared)
{
    auto num_shared{BLOCK_VTX_COUNT - num_not_shared};
    const auto shared_txns{CreateRandomTransactions(/*num_txns=*/num_shared)};

    // Create different sets of transactions...
    auto disconnected_block_txns{CreateRandomTransactions(/*num_txns=*/num_not_shared)};
    std::copy(shared_txns.begin(), shared_txns.end(), std::back_inserter(disconnected_block_txns));

    auto connected_block_txns{CreateRandomTransactions(/*num_txns=*/num_not_shared)};
    std::copy(shared_txns.begin(), shared_txns.end(), std::back_inserter(connected_block_txns));

    assert(disconnected_block_txns.size() == BLOCK_VTX_COUNT);
    assert(connected_block_txns.size() == BLOCK_VTX_COUNT);

    return ReorgTxns{/*disconnected_txns=*/disconnected_block_txns,
                     /*connected_txns_1=*/connected_block_txns,
                     /*connected_txns_2=*/CreateRandomTransactions(BLOCK_VTX_COUNT),
                     /*num_shared=*/num_shared};
}

static void Reorg(const ReorgTxns& reorg)
{
    DisconnectedBlockTransactions disconnectpool{MAX_DISCONNECTED_TX_POOL_BYTES};
    // Disconnect block
    const auto evicted = disconnectpool.AddTransactionsFromBlock(reorg.disconnected_txns);
    assert(evicted.empty());

    // Connect first block
    disconnectpool.removeForBlock(reorg.connected_txns_1);
    // Connect new tip
    disconnectpool.removeForBlock(reorg.connected_txns_2);

    // Sanity Check
    assert(disconnectpool.size() == BLOCK_VTX_COUNT - reorg.num_shared);

    disconnectpool.clear();
}

/** Add transactions from DisconnectedBlockTransactions, remove all but one (the disconnected
 * block's coinbase transaction) of them, and then pop from the front until empty. This is a reorg
 * in which all of the non-coinbase transactions in the disconnected chain also exist in the new
 * chain. */
static void AddAndRemoveDisconnectedBlockTransactionsAll(benchmark::Bench& bench)
{
    const auto chains{CreateBlocks(/*num_not_shared=*/1)};
    assert(chains.num_shared == BLOCK_VTX_COUNT - 1);

    bench.minEpochIterations(10).run([&]() {
        Reorg(chains);
    });
}

/** Add transactions from DisconnectedBlockTransactions, remove 90% of them, and then pop from the front until empty. */
static void AddAndRemoveDisconnectedBlockTransactions90(benchmark::Bench& bench)
{
    const auto chains{CreateBlocks(/*num_not_shared=*/BLOCK_VTX_COUNT_10PERCENT)};
    assert(chains.num_shared == BLOCK_VTX_COUNT - BLOCK_VTX_COUNT_10PERCENT);

    bench.minEpochIterations(10).run([&]() {
        Reorg(chains);
    });
}

/** Add transactions from DisconnectedBlockTransactions, remove 10% of them, and then pop from the front until empty. */
static void AddAndRemoveDisconnectedBlockTransactions10(benchmark::Bench& bench)
{
    const auto chains{CreateBlocks(/*num_not_shared=*/BLOCK_VTX_COUNT - BLOCK_VTX_COUNT_10PERCENT)};
    assert(chains.num_shared == BLOCK_VTX_COUNT_10PERCENT);

    bench.minEpochIterations(10).run([&]() {
        Reorg(chains);
    });
}

BENCHMARK(AddAndRemoveDisconnectedBlockTransactionsAll, benchmark::PriorityLevel::HIGH);
BENCHMARK(AddAndRemoveDisconnectedBlockTransactions90, benchmark::PriorityLevel::HIGH);
BENCHMARK(AddAndRemoveDisconnectedBlockTransactions10, benchmark::PriorityLevel::HIGH);
