// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txrelay.h>
#include <primitives/block.h>
#include <random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

BOOST_FIXTURE_TEST_SUITE(txrelay_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(txrelay_bloom_filter_lifecycle)
{
    node::TxRelay tx_relay;
    const std::vector<unsigned char> data{1, 2, 3};

    BOOST_CHECK(!tx_relay.GetRelayTxs());
    BOOST_CHECK(!tx_relay.AddToBloomFilter(data));

    tx_relay.SetBloomFilter(CBloomFilter{10, 0.001, 0, BLOOM_UPDATE_NONE});
    BOOST_CHECK(tx_relay.GetRelayTxs());
    BOOST_CHECK(tx_relay.AddToBloomFilter(data));

    tx_relay.ClearBloomFilter();
    BOOST_CHECK(tx_relay.GetRelayTxs());
    BOOST_CHECK(!tx_relay.AddToBloomFilter(data));
}

BOOST_AUTO_TEST_CASE(txrelay_inventory_gated_until_send_scheduled)
{
    node::TxRelay tx_relay;
    const uint256 hash{uint256::ONE};
    const Wtxid wtxid{Wtxid::FromUint256(hash)};

    BOOST_CHECK(tx_relay.IsInventoryPristine());
    tx_relay.PushInventory(hash, wtxid);
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 0U);
    BOOST_CHECK(tx_relay.IsInventoryPristine());

    tx_relay.SetNextInvSendTime(1us);
    BOOST_CHECK(!tx_relay.IsInventoryPristine());

    tx_relay.PushInventory(hash, wtxid);
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);
}

BOOST_AUTO_TEST_CASE(txrelay_known_inventory_not_requeued)
{
    node::TxRelay tx_relay;
    tx_relay.SetRelayTxs(true);
    tx_relay.SetNextInvSendTime(1us);

    const uint256 first_hash{uint256::ONE};
    tx_relay.PushInventory(first_hash, Wtxid::FromUint256(first_hash));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);

    const uint256 known_hash{uint256{2}};
    tx_relay.AddKnownTx(known_hash);
    tx_relay.PushInventory(known_hash, Wtxid::FromUint256(known_hash));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);
}

BOOST_AUTO_TEST_CASE(txrelay_state_accessors)
{
    node::TxRelay tx_relay;

    BOOST_CHECK_EQUAL(tx_relay.GetLastInvSequence(), 1U);
    tx_relay.SetLastInvSequence(42);
    BOOST_CHECK_EQUAL(tx_relay.GetLastInvSequence(), 42U);

    BOOST_CHECK(!tx_relay.StartTxInventoryBatch(/*send_trickle=*/true).SendMempool());
    tx_relay.SetSendMempool();
    BOOST_CHECK(tx_relay.StartTxInventoryBatch(/*send_trickle=*/true).SendMempool());
    BOOST_CHECK(!tx_relay.StartTxInventoryBatch(/*send_trickle=*/true).SendMempool());

    tx_relay.SetFeeFilterReceived(123);
    BOOST_CHECK_EQUAL(tx_relay.GetFeeFilterReceived(), 123);
}

BOOST_AUTO_TEST_CASE(txrelay_inventory_batch_schedules_before_snapshot)
{
    node::TxRelay tx_relay;

    auto batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/false, 1us)};
    BOOST_CHECK(batch.SendTrickle());
    BOOST_CHECK(batch.InvSendTimeReached());
    tx_relay.SetNextInvSendTime(2us);

    const uint256 hash{uint256::ONE};
    tx_relay.PushInventory(hash, Wtxid::FromUint256(hash));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);

    auto early_batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/false, 1us)};
    BOOST_CHECK(!early_batch.SendTrickle());
    BOOST_CHECK(!early_batch.InvSendTimeReached());
}

// StartTxInventoryBatch drops queued inventory at snapshot time when the peer
// has disabled tx relay. This exercises the nested bloom-lock branch (the only
// place both mutexes are held, in the m_tx_inventory_mutex -> m_bloom_filter_mutex
// order).
BOOST_AUTO_TEST_CASE(txrelay_inventory_batch_cleared_when_relay_disabled)
{
    node::TxRelay tx_relay;
    tx_relay.SetNextInvSendTime(1us); // open the gate (PushInventory ignores the relay flag)

    const uint256 hash{uint256::ONE};
    tx_relay.PushInventory(hash, Wtxid::FromUint256(hash));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);

    tx_relay.SetRelayTxs(false); // peer asked us not to relay transactions
    auto batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/true)};
    BOOST_CHECK_EQUAL(batch.QueuedCandidates().size(), 0U);
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 0U);
}

BOOST_AUTO_TEST_CASE(txrelay_inventory_batch_moves_and_returns_queued_inventory)
{
    node::TxRelay tx_relay;
    tx_relay.SetRelayTxs(true);
    tx_relay.SetNextInvSendTime(1us);

    const uint256 hash{uint256::ONE};
    tx_relay.PushInventory(hash, Wtxid::FromUint256(hash));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);

    auto batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/true)};
    BOOST_CHECK(batch.SendTrickle());
    BOOST_CHECK_EQUAL(batch.QueuedCandidates().size(), 1U);
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 0U);

    tx_relay.ReturnTxInventory(std::move(batch));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 1U);

    auto processed_batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/true)};
    processed_batch.EraseQueued(Wtxid::FromUint256(hash));
    tx_relay.ReturnTxInventory(std::move(processed_batch));
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 0U);
}

// Conservation invariant (single-threaded): across repeated snapshot/drain
// rounds, the inventory the batch hands out is exactly what is pending, and the
// union of everything "sent" (erased from a batch) with everything still queued
// always equals the originally queued set. No wtxid is invented or dropped by
// the StartTxInventoryBatch swap / ReturnTxInventory merge.
BOOST_AUTO_TEST_CASE(txrelay_inventory_batch_conserves_inventory)
{
    node::TxRelay tx_relay;
    tx_relay.SetRelayTxs(true);
    tx_relay.SetNextInvSendTime(1us); // open the announcement gate

    constexpr size_t N{200};
    std::set<Wtxid> queued; // ground truth of what is still pending
    for (size_t i = 0; i < N; ++i) {
        const uint256 hash{m_rng.rand256()};
        tx_relay.PushInventory(hash, Wtxid::FromUint256(hash));
        queued.insert(Wtxid::FromUint256(hash));
    }
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, queued.size());

    std::set<Wtxid> sent;
    for (int round = 0; round < 8 && !queued.empty(); ++round) {
        auto batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/true)};

        // The swap empties the live queue and hands us exactly what was pending.
        BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 0U);
        const auto candidates{batch.QueuedCandidates()};
        BOOST_CHECK(std::set<Wtxid>(candidates.begin(), candidates.end()) == queued);

        // "Send" a random subset by erasing it from the batch; the remainder is
        // returned to the live queue.
        for (const auto& wtxid : candidates) {
            if (m_rng.randbool()) {
                batch.EraseQueued(wtxid);
                BOOST_CHECK(sent.insert(wtxid).second); // never sent twice
                queued.erase(wtxid);
            }
        }
        tx_relay.ReturnTxInventory(std::move(batch));

        // Conservation after the round: live queue == ground-truth remainder.
        BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, queued.size());
    }

    // The sent set and the remaining queue partition the original, no overlap.
    auto final_batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/true)};
    const auto remaining{final_batch.QueuedCandidates()};
    BOOST_CHECK(std::set<Wtxid>(remaining.begin(), remaining.end()) == queued);
    for (const auto& wtxid : remaining) BOOST_CHECK(!sent.contains(wtxid));
}

// Conservation invariant under concurrency: one thread keeps queuing inventory
// via PushInventory() while another keeps draining via the snapshot/merge batch
// (as SendMessages() does, holding no TxRelay mutex during the drain). Every
// queued wtxid must be drained exactly once -- none lost to the swap, none
// duplicated by the merge. Run under ThreadSanitizer to also check that the
// only synchronization is TxRelay's own mutex (no data race introduced by
// moving work out of the lock).
BOOST_AUTO_TEST_CASE(txrelay_inventory_batch_conserves_under_concurrent_push)
{
    node::TxRelay tx_relay;
    tx_relay.SetRelayTxs(true);
    tx_relay.SetNextInvSendTime(1us); // open the announcement gate

    // A fixed universe of distinct wtxids, each queued exactly once.
    constexpr size_t N{2000};
    std::set<Wtxid> expected;
    while (expected.size() < N) expected.insert(Wtxid::FromUint256(m_rng.rand256()));
    const std::vector<Wtxid> universe{expected.begin(), expected.end()};

    std::atomic<bool> producer_done{false};

    // Producer thread: queue every wtxid once, concurrently with the drainer.
    // Performs no Boost.Test assertions (those run only on the main thread).
    std::thread producer{[&] {
        for (const auto& wtxid : universe) {
            tx_relay.PushInventory(wtxid.ToUint256(), wtxid);
        }
        producer_done.store(true);
    }};

    // Drainer (main thread): repeatedly snapshot the queue and "send" every
    // candidate by erasing it, recording what was drained. After the producer
    // is done, keep draining until a post-join snapshot comes back empty --
    // i.e. nothing is left pending or in flight.
    std::set<Wtxid> drained;
    auto drain_once = [&] {
        auto batch{tx_relay.StartTxInventoryBatch(/*send_trickle=*/true)};
        const auto candidates{batch.QueuedCandidates()};
        for (const auto& wtxid : candidates) {
            BOOST_CHECK(drained.insert(wtxid).second); // each drained at most once
            batch.EraseQueued(wtxid);
        }
        tx_relay.ReturnTxInventory(std::move(batch)); // nothing left to return
        return candidates.empty();
    };

    while (!producer_done.load()) drain_once();

    producer.join();

    while (!drain_once()) {}

    // Every queued wtxid was drained exactly once; queue is empty at the end.
    BOOST_CHECK(drained == expected);
    BOOST_CHECK_EQUAL(drained.size(), N);
    BOOST_CHECK_EQUAL(tx_relay.GetInventoryStats().m_inv_to_send, 0U);
}

// MakeMerkleBlock returns nullopt when no bloom filter is loaded, and a
// CMerkleBlock filtered through the peer's bloom filter when one is.
BOOST_AUTO_TEST_CASE(txrelay_make_merkle_block)
{
    node::TxRelay tx_relay;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    CBlock block;
    block.vtx.push_back(MakeTransactionRef(coinbase));
    const Txid txid{block.vtx[0]->GetHash()};

    // No filter loaded -> no merkle block.
    BOOST_CHECK(!tx_relay.MakeMerkleBlock(block).has_value());

    // A filter matching the coinbase -> a merkle block containing it.
    CBloomFilter filter{10, 0.001, 0, BLOOM_UPDATE_ALL};
    filter.insert(txid.ToUint256());
    tx_relay.SetBloomFilter(filter);

    const auto merkle_block{tx_relay.MakeMerkleBlock(block)};
    BOOST_REQUIRE(merkle_block.has_value());
    BOOST_REQUIRE_EQUAL(merkle_block->vMatchedTxn.size(), 1U);
    BOOST_CHECK(merkle_block->vMatchedTxn[0].second == txid);
}

BOOST_AUTO_TEST_SUITE_END()
