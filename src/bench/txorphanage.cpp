// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <net.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/sign.h>
#include <test/util/setup_common.h>
#include <node/txorphanage.h>
#include <util/check.h>
#include <test/util/transaction_utils.h>

#include <cstdint>
#include <memory>

static constexpr node::TxOrphanage::Usage TINY_TX_WEIGHT{240};
static constexpr int64_t APPROX_WEIGHT_PER_INPUT{200};

// Creates a transaction with num_inputs inputs and 1 output, padded to target_weight. Use this function to maximize m_outpoint_to_orphan_it operations.
// If num_inputs is 0, we maximize the number of inputs.
static CTransactionRef MakeTransactionBulkedTo(unsigned int num_inputs, int64_t target_weight, FastRandomContext& det_rand)
{
    CMutableTransaction tx;
    assert(target_weight >= 40 + APPROX_WEIGHT_PER_INPUT);
    if (!num_inputs) num_inputs = (target_weight - 40) / APPROX_WEIGHT_PER_INPUT;
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

// Constructs a transaction using a subset of inputs[start_input : start_input + num_inputs] up to the weight_limit.
static CTransactionRef MakeTransactionSpendingUpTo(const std::vector<CTxIn>& inputs, unsigned int start_input, unsigned int num_inputs, int64_t weight_limit)
{
    CMutableTransaction tx;
    for (unsigned int i{start_input}; i < start_input + num_inputs; ++i) {
        if (GetTransactionWeight(*MakeTransactionRef(tx)) + APPROX_WEIGHT_PER_INPUT >= weight_limit) break;
        tx.vin.emplace_back(inputs.at(i % inputs.size()));
    }
    assert(tx.vin.size() > 0);
    return MakeTransactionRef(tx);
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
    auto large_tx = MakeTransactionBulkedTo(1, MAX_STANDARD_TX_WEIGHT, det_rand);
    assert(GetTransactionWeight(*large_tx) <= MAX_STANDARD_TX_WEIGHT);

    const auto orphanage{node::MakeTxOrphanage(/*max_global_latency_score=*/node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE, /*reserved_peer_usage=*/node::DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER)};

    // Populate the orphanage. To maximize the number of evictions, first fill up with tiny transactions, then add a huge one.
    NodeId peer{0};
    // Add tiny transactions until we are just about to hit the memory limit, up to the max number of announcements.
    // We use the same tiny transactions for all peers to minimize their contribution to the usage limit.
    int64_t total_weight_to_add{0};
    for (unsigned int txindex{0}; txindex < NUM_TINY_TRANSACTIONS; ++txindex) {
        const auto& tx{tiny_txs.at(txindex)};

        total_weight_to_add += GetTransactionWeight(*tx);
        if (total_weight_to_add > orphanage->MaxGlobalUsage()) break;

        assert(orphanage->AddTx(tx, peer));

        // Sanity check: we should always be exiting at the point of hitting the weight limit.
        assert(txindex < NUM_TINY_TRANSACTIONS - 1);
    }

    // In the real world, we always trim after each new tx.
    // If we need to trim already, that means the benchmark is not representative of what LimitOrphans may do in a single call.
    assert(orphanage->TotalOrphanUsage() <= orphanage->MaxGlobalUsage());
    assert(orphanage->TotalLatencyScore() <= orphanage->MaxGlobalLatencyScore());
    assert(orphanage->TotalOrphanUsage() + TINY_TX_WEIGHT > orphanage->MaxGlobalUsage());

    bench.epochs(1).epochIterations(1).run([&]() NO_THREAD_SAFETY_ANALYSIS {
        // Lastly, add the large transaction.
        const auto num_announcements_before_trim{orphanage->CountAnnouncements()};
        assert(orphanage->AddTx(large_tx, peer));

        // If there are multiple peers, note that they all have the same DoS score. We will evict only 1 item at a time for each new DoSiest peer.
        const auto num_announcements_after_trim{orphanage->CountAnnouncements()};
        const auto num_evicted{num_announcements_before_trim - num_announcements_after_trim};

        // The number of evictions is the same regardless of the number of peers. In both cases, we can exceed the
        // usage limit using 1 maximally-sized transaction.
        assert(num_evicted == MAX_STANDARD_TX_WEIGHT / TINY_TX_WEIGHT);
    });
}
static void OrphanageMultiPeerEviction(benchmark::Bench& bench)
{
    // Best number is just below sqrt(DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE)
    static constexpr unsigned int NUM_PEERS{39};
    // All peers will have the same transactions. We want to be just under the weight limit, so divide the max usage limit by the number of unique transactions.
    static constexpr node::TxOrphanage::Count NUM_UNIQUE_TXNS{node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE / NUM_PEERS};
    static constexpr node::TxOrphanage::Usage TOTAL_USAGE_LIMIT{node::DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER * NUM_PEERS};
    // Subtract 4 because BulkTransaction rounds up and we must avoid going over the weight limit early.
    static constexpr node::TxOrphanage::Usage LARGE_TX_WEIGHT{TOTAL_USAGE_LIMIT / NUM_UNIQUE_TXNS - 4};
    static_assert(LARGE_TX_WEIGHT >= TINY_TX_WEIGHT * 2, "Tx is too small, increase NUM_PEERS");
    // The orphanage does not permit any transactions larger than 400'000, so this test will not work if the large tx is much larger.
    static_assert(LARGE_TX_WEIGHT <= MAX_STANDARD_TX_WEIGHT, "Tx is too large, decrease NUM_PEERS");

    FastRandomContext det_rand{true};
    // Construct large transactions
    std::vector<CTransactionRef> shared_txs;
    shared_txs.reserve(NUM_UNIQUE_TXNS);
    for (unsigned int i{0}; i < NUM_UNIQUE_TXNS; ++i) {
        shared_txs.emplace_back(MakeTransactionBulkedTo(9, LARGE_TX_WEIGHT, det_rand));
    }
    std::vector<size_t> indexes;
    indexes.resize(NUM_UNIQUE_TXNS);
    std::iota(indexes.begin(), indexes.end(), 0);

    const auto orphanage{node::MakeTxOrphanage(/*max_global_latency_score=*/node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE, /*reserved_peer_usage=*/node::DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER)};
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
    assert(max_usage - total_usage <= LARGE_TX_WEIGHT);
    assert(orphanage->TotalLatencyScore() <= orphanage->MaxGlobalLatencyScore());

    auto last_tx = MakeTransactionBulkedTo(0, max_usage - total_usage + 1, det_rand);

    bench.epochs(1).epochIterations(1).run([&]() NO_THREAD_SAFETY_ANALYSIS {
        const auto num_announcements_before_trim{orphanage->CountAnnouncements()};
        // There is a small gap between the total usage and the max usage. Add a transaction to fill it.
        assert(orphanage->AddTx(last_tx, 0));

        // If there are multiple peers, note that they all have the same DoS score. We will evict only 1 item at a time for each new DoSiest peer.
        const auto num_evicted{num_announcements_before_trim - orphanage->CountAnnouncements() + 1};
        // The trimming happens as a round robin. In the first NUM_UNIQUE_TXNS - 2 rounds for each peer, only duplicates are evicted.
        // Once each peer has 2 transactions left, it's possible to select a peer whose oldest transaction is unique.
        assert(num_evicted >= (NUM_UNIQUE_TXNS - 2) * NUM_PEERS);
    });
}

static void OrphanageEraseAll(benchmark::Bench& bench, bool block_or_disconnect)
{
    FastRandomContext det_rand{true};
    const auto orphanage{node::MakeTxOrphanage(/*max_global_latency_score=*/node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE, /*reserved_peer_usage=*/node::DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER)};
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
        int64_t weight_left_for_peer{node::DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER};
        for (unsigned int txnum{0}; txnum < node::DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE / NUM_PEERS; ++txnum) {
            // Transactions must be unique since they use different (though overlapping) inputs.
            const unsigned int start_input = peer * INPUTS_PER_PEER + txnum * 7;

            // Note that we shouldn't be able to hit the weight limit with these small transactions.
            const int64_t weight_limit{std::min<int64_t>(weight_left_for_peer, MAX_STANDARD_TX_WEIGHT)};
            auto ptx = MakeTransactionSpendingUpTo(block_tx->vin, /*start_input=*/start_input, /*num_inputs=*/INPUTS_PER_TX, /*weight_limit=*/weight_limit);

            assert(GetTransactionWeight(*ptx) <= MAX_STANDARD_TX_WEIGHT);
            assert(!orphanage->HaveTx(ptx->GetWitnessHash()));
            assert(orphanage->AddTx(ptx, peer));

            weight_left_for_peer -= GetTransactionWeight(*ptx);
            if (weight_left_for_peer < TINY_TX_WEIGHT * 2) break;
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

BENCHMARK(OrphanageSinglePeerEviction, benchmark::PriorityLevel::LOW);
BENCHMARK(OrphanageMultiPeerEviction, benchmark::PriorityLevel::LOW);
BENCHMARK(OrphanageEraseForBlock, benchmark::PriorityLevel::LOW);
BENCHMARK(OrphanageEraseForPeer, benchmark::PriorityLevel::LOW);
