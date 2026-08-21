// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txorphanage.h>

#include <bench/bench.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <net.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <test/util/transaction_utils.h>
#include <threadsafety.h>
#include <util/check.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <vector>

static constexpr node::TxOrphanage::Usage TINY_TX_WEIGHT{240};
static constexpr int64_t APPROX_WEIGHT_PER_INPUT{200};
// A 1-byte witness stack element costs its slot in the stack vector plus a (minimally-sized) heap allocation.
static constexpr node::TxOrphanage::Usage APPROX_USAGE_PER_ELEMENT{sizeof(std::vector<unsigned char>) + 32};

// Creates a transaction with num_inputs inputs and 1 output, padded to target_weight. Use this function to maximize m_outpoint_to_orphan_it operations.
static CTransactionRef MakeTransactionBulkedTo(unsigned int num_inputs, int64_t target_weight, FastRandomContext& det_rand)
{
    CMutableTransaction tx;
    assert(num_inputs > 0);
    assert(target_weight >= 40 + APPROX_WEIGHT_PER_INPUT);
    for (unsigned int i = 0; i < num_inputs; ++i) {
        tx.vin.emplace_back(Txid::FromUint256(det_rand.rand256()), 0);
    }
    assert(GetTransactionWeight(*MakeTransactionRef(tx)) <= target_weight);

    tx.vout.resize(1);

    // If necessary, pad the transaction to the target weight.
    if (GetTransactionWeight(*MakeTransactionRef(tx)) < target_weight - 4) {
        BulkTransaction(tx, target_weight);
    }
    return MakeTransactionRef(tx);
}

// Constructs a transaction using inputs[start_input : start_input + num_inputs].
static CTransactionRef MakeTransactionSpendingUpTo(const std::vector<CTxIn>& inputs, unsigned int start_input, unsigned int num_inputs)
{
    CMutableTransaction tx;
    for (unsigned int i{start_input}; i < start_input + num_inputs; ++i) {
        tx.vin.emplace_back(inputs.at(i % inputs.size()));
    }
    assert(tx.vin.size() > 0);
    return MakeTransactionRef(tx);
}
// Creates a transaction with num_inputs inputs and 1 output whose usage (see node::GetOrphanUsage) is as close to
// target_usage as possible without exceeding it. The bytes are placed in a witness stack of many 1-byte elements, as
// that is the only way to reach a usage close to MAX_ORPHAN_TX_USAGE while staying within MAX_STANDARD_TX_WEIGHT.
static CTransactionRef MakeTransactionUsing(unsigned int num_inputs, node::TxOrphanage::Usage target_usage, FastRandomContext& det_rand)
{
    assert(num_inputs > 0);
    CMutableTransaction tx;
    for (unsigned int i = 0; i < num_inputs; ++i) {
        tx.vin.emplace_back(Txid::FromUint256(det_rand.rand256()), 0);
    }
    tx.vout.resize(1);
    // Replace the witness stack of the first input with num_elements elements and return the resulting usage.
    const auto set_num_elements = [&](size_t num_elements) {
        tx.vin[0].scriptWitness.stack.assign(num_elements, std::vector<unsigned char>{1});
        tx.vin[0].scriptWitness.stack.shrink_to_fit();
        return node::GetOrphanUsage(MakeTransactionRef(tx));
    };
    // Approach target_usage from below by adding 1-byte witness elements. If the transaction already uses more than
    // that without any witness data, it is returned as is.
    size_t num_elements{0};
    const auto base_usage{set_num_elements(0)};
    if (target_usage > base_usage) {
        num_elements = static_cast<size_t>((target_usage - base_usage) / APPROX_USAGE_PER_ELEMENT);
        // Correct the estimate for allocator rounding.
        while (num_elements > 0 && set_num_elements(num_elements) > target_usage) --num_elements;
        while (set_num_elements(num_elements + 1) <= target_usage) ++num_elements;
    }
    assert(set_num_elements(num_elements) <= target_usage || num_elements == 0);
    auto ptx{MakeTransactionRef(tx)};
    assert(GetTransactionWeight(*ptx) <= MAX_STANDARD_TX_WEIGHT);
    return ptx;
}

static void OrphanageSinglePeerEviction(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};

    // Fill up announcements slots with tiny txns, followed by a single large one
    unsigned int NUM_TINY_TRANSACTIONS((node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE));

    // Construct transactions to submit to orphanage: 1-in-1-out tiny transactions
    std::vector<CTransactionRef> tiny_txs;
    tiny_txs.reserve(NUM_TINY_TRANSACTIONS);
    for (unsigned int i{0}; i < NUM_TINY_TRANSACTIONS; ++i) {
        tiny_txs.emplace_back(MakeTransactionBulkedTo(1, TINY_TX_WEIGHT, det_rand));
    }
    // All of the tiny transactions are accounted the same usage.
    const auto TINY_TX_USAGE{node::GetOrphanUsage(tiny_txs.front())};
    assert(std::all_of(tiny_txs.begin(), tiny_txs.end(), [&](const auto& tx) { return node::GetOrphanUsage(tx) == TINY_TX_USAGE; }));
    // Use the largest orphan that may be stored, to maximize the number of evictions it triggers.
    auto large_tx = MakeTransactionUsing(1, node::MAX_ORPHAN_TX_USAGE, det_rand);
    const auto LARGE_TX_USAGE{node::GetOrphanUsage(large_tx)};

    const auto orphanage{node::MakeTxOrphanage(/*max_global_latency_score=*/node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE, /*reserved_peer_usage=*/node::DEFAULT_RESERVED_ORPHAN_USAGE_PER_PEER)};

    // Populate the orphanage. To maximize the number of evictions, first fill up with tiny transactions, then add a huge one.
    NodeId peer{0};
    // Add tiny transactions until we are just about to hit the memory limit, up to the max number of announcements.
    // We use the same tiny transactions for all peers to minimize their contribution to the usage limit.
    node::TxOrphanage::Usage total_usage_to_add{0};
    for (unsigned int txindex{0}; txindex < NUM_TINY_TRANSACTIONS; ++txindex) {
        const auto& tx{tiny_txs.at(txindex)};

        total_usage_to_add += TINY_TX_USAGE;
        if (total_usage_to_add > orphanage->MaxGlobalUsage()) break;

        assert(orphanage->AddTx(tx, peer));

        // Sanity check: we should always be exiting at the point of hitting the usage limit.
        assert(txindex < NUM_TINY_TRANSACTIONS - 1);
    }

    // In the real world, we always trim after each new tx.
    // If we need to trim already, that means the benchmark is not representative of what LimitOrphans may do in a single call.
    assert(orphanage->TotalOrphanUsage() <= orphanage->MaxGlobalUsage());
    assert(orphanage->TotalLatencyScore() <= orphanage->MaxGlobalLatencyScore());
    assert(orphanage->TotalOrphanUsage() + TINY_TX_USAGE > orphanage->MaxGlobalUsage());
    const auto total_usage_before{orphanage->TotalOrphanUsage()};
    // Adding the large transaction must evict exactly enough tiny ones to get back within the global limit.
    const auto expected_evicted{(total_usage_before + LARGE_TX_USAGE - orphanage->MaxGlobalUsage() + TINY_TX_USAGE - 1) / TINY_TX_USAGE};

    bench.epochs(1).epochIterations(1).run([&]() NO_THREAD_SAFETY_ANALYSIS {
        // Lastly, add the large transaction.
        const auto num_announcements_before_trim{orphanage->CountAnnouncements()};
        assert(orphanage->AddTx(large_tx, peer));

        // If there are multiple peers, note that they all have the same DoS score. We will evict only 1 item at a time for each new DoSiest peer.
        const auto num_announcements_after_trim{orphanage->CountAnnouncements()};
        // The large transaction itself was added before trimming, hence the +1.
        const auto num_evicted{num_announcements_before_trim + 1 - num_announcements_after_trim};

        // The number of evictions is the same regardless of the number of peers. In both cases, we can exceed the
        // usage limit using 1 maximally-sized transaction.
        assert(num_evicted == expected_evicted);
    });
}
static void OrphanageMultiPeerEviction(benchmark::Bench& bench)
{
    // Best number is just below sqrt(DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE)
    static constexpr unsigned int NUM_PEERS{39};
    // All peers will have the same transactions. We want to be just under the usage limit, so divide the max usage limit by the number of unique transactions.
    static constexpr node::TxOrphanage::Count NUM_UNIQUE_TXNS{node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE / NUM_PEERS};
    static constexpr node::TxOrphanage::Usage TOTAL_USAGE_LIMIT{node::DEFAULT_RESERVED_ORPHAN_USAGE_PER_PEER * NUM_PEERS};
    static constexpr node::TxOrphanage::Usage LARGE_TX_USAGE_TARGET{TOTAL_USAGE_LIMIT / NUM_UNIQUE_TXNS};
    // The orphanage does not store transactions using more memory than MAX_ORPHAN_TX_USAGE.
    static_assert(LARGE_TX_USAGE_TARGET <= node::MAX_ORPHAN_TX_USAGE, "Tx is too large, decrease NUM_PEERS");

    FastRandomContext det_rand{true};
    // Construct large transactions. Transactions with 9 inputs maximize the number of m_outpoint_to_orphan_wtxids
    // operations per unit of latency score.
    std::vector<CTransactionRef> shared_txs;
    shared_txs.reserve(NUM_UNIQUE_TXNS);
    for (unsigned int i{0}; i < NUM_UNIQUE_TXNS; ++i) {
        shared_txs.emplace_back(MakeTransactionUsing(9, LARGE_TX_USAGE_TARGET, det_rand));
    }
    // All of the shared transactions are accounted the same usage.
    const auto LARGE_TX_USAGE{node::GetOrphanUsage(shared_txs.front())};
    assert(std::all_of(shared_txs.begin(), shared_txs.end(), [&](const auto& tx) { return node::GetOrphanUsage(tx) == LARGE_TX_USAGE; }));
    std::vector<size_t> indexes;
    indexes.resize(NUM_UNIQUE_TXNS);
    std::iota(indexes.begin(), indexes.end(), 0);

    const auto orphanage{node::MakeTxOrphanage(/*max_global_latency_score=*/node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE, /*reserved_peer_usage=*/node::DEFAULT_RESERVED_ORPHAN_USAGE_PER_PEER)};
    // Every peer sends the same transactions, all from shared_txs.
    // Each peer has 1 or 2 assigned transactions, which they must place as the last and second-to-last positions.
    // The assignments ensure that every transaction is in some peer's last 2 transactions, and is thus remains in the orphanage until the end of LimitOrphans.
    static_assert(NUM_UNIQUE_TXNS <= NUM_PEERS * 2);

    // We need each peer to send some transactions so that the global limit (which is a function of the number of peers providing at least 1 announcement) rises.
    for (unsigned int i{0}; i < NUM_UNIQUE_TXNS; ++i) {
        for (NodeId peer{0}; peer < NUM_PEERS; ++peer) {
            const CTransactionRef& reserved_last_tx{shared_txs.at(peer)};
            CTransactionRef reserved_second_to_last_tx{peer < NUM_UNIQUE_TXNS - NUM_PEERS ? shared_txs.at(peer + NUM_PEERS) : nullptr};

            const auto& tx{shared_txs.at(indexes.at(i))};
            if (tx == reserved_last_tx) {
                // Skip
            } else if (reserved_second_to_last_tx && tx == reserved_second_to_last_tx) {
                // Skip
            } else {
                orphanage->AddTx(tx, peer);
            }
        }
    }

    // Now add the final reserved transactions.
    for (NodeId peer{0}; peer < NUM_PEERS; ++peer) {
        const CTransactionRef& reserved_last_tx{shared_txs.at(peer)};
        CTransactionRef reserved_second_to_last_tx{peer < NUM_UNIQUE_TXNS - NUM_PEERS ? shared_txs.at(peer + NUM_PEERS) : nullptr};
        // Add the final reserved transactions.
        if (reserved_second_to_last_tx) {
            orphanage->AddTx(reserved_second_to_last_tx, peer);
        }
        orphanage->AddTx(reserved_last_tx, peer);
    }

    assert(orphanage->CountAnnouncements() == NUM_PEERS * NUM_UNIQUE_TXNS);
    const auto total_usage{orphanage->TotalOrphanUsage()};
    const auto max_usage{orphanage->MaxGlobalUsage()};
    assert(max_usage >= total_usage);
    assert(max_usage - total_usage <= LARGE_TX_USAGE);
    assert(orphanage->TotalLatencyScore() <= orphanage->MaxGlobalLatencyScore());

    // Construct a transaction that fills the remaining gap and exceeds the limit. MakeTransactionUsing() never exceeds
    // its target and can only be sized in increments of one witness element, so aim a few elements higher.
    auto last_tx = MakeTransactionUsing(1, max_usage - total_usage + 4 * APPROX_USAGE_PER_ELEMENT, det_rand);
    assert(total_usage + node::GetOrphanUsage(last_tx) > max_usage);
    assert(node::GetOrphanUsage(last_tx) <= node::MAX_ORPHAN_TX_USAGE);

    bench.epochs(1).epochIterations(1).run([&]() NO_THREAD_SAFETY_ANALYSIS {
        const auto num_announcements_before_trim{orphanage->CountAnnouncements()};
        // There is a small gap between the total usage and the max usage. Add a transaction to fill it.
        assert(orphanage->AddTx(last_tx, 0));

        // If there are multiple peers, note that they all have the same DoS score. We will evict only 1 item at a time for each new DoSiest peer.
        // The filler transaction itself was added before trimming, hence the +1.
        const auto num_evicted{num_announcements_before_trim + 1 - orphanage->CountAnnouncements()};
        // The trimming happens as a round robin. In the first NUM_UNIQUE_TXNS - 2 rounds for each peer, only duplicates are evicted.
        // Once each peer has 2 transactions left, it's possible to select a peer whose oldest transaction is unique.
        assert(num_evicted >= (NUM_UNIQUE_TXNS - 2) * NUM_PEERS);
    });
}

static void OrphanageEraseAll(benchmark::Bench& bench, bool block_or_disconnect)
{
    FastRandomContext det_rand{true};
    const auto orphanage{node::MakeTxOrphanage(/*max_global_latency_score=*/node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE, /*reserved_peer_usage=*/node::DEFAULT_RESERVED_ORPHAN_USAGE_PER_PEER)};
    // This is an unrealistically large number of inputs for a block, as there is almost no room given to witness data,
    // outputs, and overhead for individual transactions. The entire block is 1 transaction with 20,000 inputs.
    constexpr unsigned int NUM_BLOCK_INPUTS{MAX_BLOCK_WEIGHT / APPROX_WEIGHT_PER_INPUT};
    const auto block_tx{MakeTransactionBulkedTo(NUM_BLOCK_INPUTS, MAX_BLOCK_WEIGHT - 4000, det_rand)};
    CBlock block;
    block.vtx.push_back(block_tx);

    // Transactions with 9 inputs maximize the computation / LatencyScore ratio.
    constexpr unsigned int INPUTS_PER_TX{9};
    constexpr unsigned int NUM_PEERS{125};
    constexpr unsigned int NUM_TXNS_PER_PEER = node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE / NUM_PEERS;

    // Divide the block's inputs evenly among the peers.
    constexpr unsigned int INPUTS_PER_PEER = NUM_BLOCK_INPUTS / NUM_PEERS;
    static_assert(INPUTS_PER_PEER > 0);
    // All the block inputs are spent by the orphanage transactions. Each peer is assigned 76 of them.
    // Each peer has 24 transactions spending 9 inputs each, so jumping by 3 ensures we cover all of the inputs.
    static_assert(7 * NUM_TXNS_PER_PEER + INPUTS_PER_TX - 1 >= INPUTS_PER_PEER);

    for (NodeId peer{0}; peer < NUM_PEERS; ++peer) {
        node::TxOrphanage::Usage usage_left_for_peer{node::DEFAULT_RESERVED_ORPHAN_USAGE_PER_PEER};
        for (unsigned int txnum{0}; txnum < node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE / NUM_PEERS; ++txnum) {
            // Transactions must be unique since they use different (though overlapping) inputs.
            const unsigned int start_input = peer * INPUTS_PER_PEER + txnum * 7;

            auto ptx = MakeTransactionSpendingUpTo(block_tx->vin, /*start_input=*/start_input, /*num_inputs=*/INPUTS_PER_TX);

            assert(GetTransactionWeight(*ptx) <= MAX_STANDARD_TX_WEIGHT);
            assert(!orphanage->HaveTx(ptx->GetWitnessHash()));
            assert(orphanage->AddTx(ptx, peer));

            // Note that we shouldn't be able to hit the usage limit with these small transactions.
            usage_left_for_peer -= node::GetOrphanUsage(ptx);
            assert(usage_left_for_peer > 0);
        }
    }

    // If these fail, it means this benchmark is not realistic because the orphanage would have been trimmed already.
    assert(orphanage->TotalLatencyScore() <= orphanage->MaxGlobalLatencyScore());
    assert(orphanage->TotalOrphanUsage() <= orphanage->MaxGlobalUsage());

    // 3000 announcements (and unique transactions) in the orphanage.
    // They spend a total of 27,000 inputs (20,000 unique ones)
    assert(orphanage->CountAnnouncements() == NUM_PEERS * NUM_TXNS_PER_PEER);
    assert(orphanage->TotalLatencyScore() == orphanage->CountAnnouncements());

    bench.epochs(1).epochIterations(1).run([&]() NO_THREAD_SAFETY_ANALYSIS {
        if (block_or_disconnect) {
            // Erase everything through EraseForBlock.
            // Every tx conflicts with this block.
            orphanage->EraseForBlock(block);
            assert(orphanage->CountAnnouncements() == 0);
        } else {
            // Erase everything through EraseForPeer.
            for (NodeId peer{0}; peer < NUM_PEERS; ++peer) {
                orphanage->EraseForPeer(peer);
            }
            assert(orphanage->CountAnnouncements() == 0);
        }
    });
}

static void OrphanageEraseForBlock(benchmark::Bench& bench)
{
    OrphanageEraseAll(bench, /*block_or_disconnect=*/true);
}
static void OrphanageEraseForPeer(benchmark::Bench& bench)
{
    OrphanageEraseAll(bench, /*block_or_disconnect=*/false);
}

BENCHMARK(OrphanageSinglePeerEviction);
BENCHMARK(OrphanageMultiPeerEviction);
BENCHMARK(OrphanageEraseForBlock);
BENCHMARK(OrphanageEraseForPeer);
