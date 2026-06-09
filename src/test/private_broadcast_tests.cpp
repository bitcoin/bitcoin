// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <private_broadcast.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>

#include <algorithm>
#include <ostream>
#include <boost/test/unit_test.hpp>

std::ostream& operator<<(std::ostream& os, PrivateBroadcast::AddResult r)
{
    switch (r) {
    case PrivateBroadcast::AddResult::Added: return os << "Added";
    case PrivateBroadcast::AddResult::AlreadyPresent: return os << "AlreadyPresent";
    case PrivateBroadcast::AddResult::QueueFull: return os << "QueueFull";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

BOOST_FIXTURE_TEST_SUITE(private_broadcast_tests, BasicTestingSetup)

static CTransactionRef MakeDummyTx(uint32_t id, size_t num_witness)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].nSequence = id;
    if (num_witness > 0) {
        mtx.vin[0].scriptWitness = CScriptWitness{};
        mtx.vin[0].scriptWitness.stack.resize(num_witness);
    }
    return MakeTransactionRef(mtx);
}

BOOST_AUTO_TEST_CASE(basic)
{
    FakeNodeClock clock{};

    PrivateBroadcast pb;
    const NodeId recipient1{1};
    in_addr ipv4Addr;
    ipv4Addr.s_addr = 0xa0b0c001;
    const CService addr1{ipv4Addr, 1111};

    // No transactions initially.
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1, /*will_send_to_address=*/addr1).has_value());
    BOOST_CHECK_EQUAL(pb.GetStale().size(), 0);
    BOOST_CHECK(!pb.HavePendingTransactions());
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), 0);

    // Make a transaction and add it.
    const auto tx1{MakeDummyTx(/*id=*/1, /*num_witness=*/0)};

    BOOST_CHECK_EQUAL(pb.Add(tx1), PrivateBroadcast::AddResult::Added);
    BOOST_CHECK_EQUAL(pb.Add(tx1), PrivateBroadcast::AddResult::AlreadyPresent);

    // Make another transaction with same txid, different wtxid and add it.
    const auto tx2{MakeDummyTx(/*id=*/1, /*num_witness=*/1)};
    BOOST_REQUIRE(tx1->GetHash() == tx2->GetHash());
    BOOST_REQUIRE(tx1->GetWitnessHash() != tx2->GetWitnessHash());

    BOOST_CHECK_EQUAL(pb.Add(tx2), PrivateBroadcast::AddResult::Added);
    const auto find_tx_info{[](auto& infos, const CTransactionRef& tx) -> const PrivateBroadcast::TxBroadcastInfo& {
        const auto it{std::ranges::find(infos, tx->GetWitnessHash(), [](const auto& info) { return info.tx->GetWitnessHash(); })};
        BOOST_REQUIRE(it != infos.end());
        return *it;
    }};
    const auto check_peer_counts{[&](size_t tx1_peer_count, size_t tx2_peer_count) {
        const auto infos{pb.GetBroadcastInfo()};
        BOOST_CHECK_EQUAL(infos.size(), 2);
        BOOST_CHECK_EQUAL(find_tx_info(infos, tx1).peers.size(), tx1_peer_count);
        BOOST_CHECK_EQUAL(find_tx_info(infos, tx2).peers.size(), tx2_peer_count);
    }};

    check_peer_counts(/*tx1_peer_count=*/0, /*tx2_peer_count=*/0);

    const auto tx_for_recipient1{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1, /*will_send_to_address=*/addr1).value()};
    BOOST_CHECK((tx_for_recipient1 == tx1 || tx_for_recipient1 == tx2));

    // A second pick must return the other transaction.
    const NodeId recipient2{2};
    const CService addr2{ipv4Addr, 2222};
    const auto tx_for_recipient2{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient2, /*will_send_to_address=*/addr2).value()};
    BOOST_CHECK((tx_for_recipient2 == tx1 || tx_for_recipient2 == tx2));
    BOOST_CHECK_NE(tx_for_recipient1, tx_for_recipient2);

    check_peer_counts(/*tx1_peer_count=*/1, /*tx2_peer_count=*/1);

    const NodeId nonexistent_recipient{0};

    // Confirm transactions <-> recipients mapping is correct.
    BOOST_CHECK(!pb.GetTxForNode(nonexistent_recipient).has_value());
    BOOST_CHECK_EQUAL(pb.GetTxForNode(recipient1).value(), tx_for_recipient1);
    BOOST_CHECK_EQUAL(pb.GetTxForNode(recipient2).value(), tx_for_recipient2);

    // Confirm none of the transactions' reception have been confirmed.
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient1));
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient2));
    BOOST_CHECK(!pb.DidNodeConfirmReception(nonexistent_recipient));

    // 1. Freshly added transactions should NOT be stale yet.
    BOOST_CHECK_EQUAL(pb.GetStale().size(), 0);

    // 2. Fast-forward the mock clock past the INITIAL_STALE_DURATION.
    clock += PrivateBroadcast::INITIAL_STALE_DURATION + 1min;

    // 3. Now that the initial duration has passed, both unconfirmed transactions should be stale.
    BOOST_CHECK_EQUAL(pb.GetStale().size(), 2);

    // Confirm reception by recipient1.
    pb.NodeConfirmedReception(nonexistent_recipient); // Dummy call.
    pb.NodeConfirmedReception(recipient1);

    BOOST_CHECK(pb.DidNodeConfirmReception(recipient1));
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient2));

    const auto infos{pb.GetBroadcastInfo()};
    BOOST_CHECK_EQUAL(infos.size(), 2);
    {
        const auto& peers{find_tx_info(infos, tx_for_recipient1).peers};
        BOOST_CHECK_EQUAL(peers.size(), 1);
        BOOST_CHECK_EQUAL(peers[0].address.ToStringAddrPort(), addr1.ToStringAddrPort());
        BOOST_CHECK(peers[0].received.has_value());
    }
    {
        const auto& peers{find_tx_info(infos, tx_for_recipient2).peers};
        BOOST_CHECK_EQUAL(peers.size(), 1);
        BOOST_CHECK_EQUAL(peers[0].address.ToStringAddrPort(), addr2.ToStringAddrPort());
        BOOST_CHECK(!peers[0].received.has_value());
    }

    const auto stale_state{pb.GetStale()};
    BOOST_CHECK_EQUAL(stale_state.size(), 1);
    BOOST_CHECK_EQUAL(stale_state[0], tx_for_recipient2);

    clock += 10h;

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 2);

    BOOST_CHECK_EQUAL(pb.Remove(tx_for_recipient1).value(), 1);
    BOOST_CHECK(!pb.Remove(tx_for_recipient1).has_value());
    BOOST_CHECK_EQUAL(pb.Remove(tx_for_recipient2).value(), 0);
    BOOST_CHECK(!pb.Remove(tx_for_recipient2).has_value());

    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), 0);
    const CService addr_nonexistent{ipv4Addr, 3333};
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/nonexistent_recipient, /*will_send_to_address=*/addr_nonexistent).has_value());
}

BOOST_AUTO_TEST_CASE(stale_unpicked_tx)
{
    FakeNodeClock clock{};

    PrivateBroadcast pb;
    const auto tx{MakeDummyTx(/*id=*/42, /*num_witness=*/0)};
    BOOST_REQUIRE_EQUAL(pb.Add(tx), PrivateBroadcast::AddResult::Added);

    // Unpicked transactions use the longer INITIAL_STALE_DURATION.
    BOOST_CHECK_EQUAL(pb.GetStale().size(), 0);
    clock += PrivateBroadcast::INITIAL_STALE_DURATION - 1min;
    BOOST_CHECK_EQUAL(pb.GetStale().size(), 0);
    clock += 2min;
    const auto stale_state{pb.GetStale()};
    BOOST_REQUIRE_EQUAL(stale_state.size(), 1);
    BOOST_CHECK_EQUAL(stale_state[0], tx);
}

BOOST_AUTO_TEST_CASE(rejection_at_cap)
{
    PrivateBroadcast pb;
    constexpr size_t num_cap{PrivateBroadcast::MAX_TRANSACTIONS};
    constexpr size_t num_over{5};

    // Fill the queue exactly to the cap; every distinct Add() succeeds.
    std::vector<CTransactionRef> txs;
    txs.reserve(num_cap);
    for (size_t i{0}; i < num_cap; ++i) {
        auto tx{MakeDummyTx(/*id=*/static_cast<uint32_t>(i), /*num_witness=*/0)};
        BOOST_REQUIRE_EQUAL(pb.Add(tx), PrivateBroadcast::AddResult::Added);
        txs.push_back(std::move(tx));
    }
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), num_cap);

    // Further distinct transactions are rejected, and the queue is unchanged.
    for (size_t i{0}; i < num_over; ++i) {
        const auto tx{MakeDummyTx(/*id=*/static_cast<uint32_t>(num_cap + i), /*num_witness=*/0)};
        BOOST_CHECK_EQUAL(pb.Add(tx), PrivateBroadcast::AddResult::QueueFull);
    }
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), num_cap);

    // Nothing was evicted: all originally-added transactions are still present.
    const auto infos{pb.GetBroadcastInfo()};
    std::set<uint256> present_wtxids;
    for (const auto& info : infos) {
        present_wtxids.insert(info.tx->GetWitnessHash().ToUint256());
    }
    BOOST_CHECK_EQUAL(present_wtxids.size(), infos.size());
    for (size_t i{0}; i < num_cap; ++i) {
        BOOST_CHECK_MESSAGE(present_wtxids.contains(txs[i]->GetWitnessHash().ToUint256()),
                            "tx index " << i << " should still be present");
    }

    // Re-adding an already-present tx is AlreadyPresent even at the cap (not QueueFull).
    BOOST_CHECK_EQUAL(pb.Add(txs[0]), PrivateBroadcast::AddResult::AlreadyPresent);
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), num_cap);

    // Removing one frees exactly one slot for a new transaction.
    BOOST_REQUIRE(pb.Remove(txs[0]).has_value());
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), num_cap - 1);
    const auto fresh{MakeDummyTx(/*id=*/0xffffffff, /*num_witness=*/0)};
    BOOST_CHECK_EQUAL(pb.Add(fresh), PrivateBroadcast::AddResult::Added);
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), num_cap);

    // A previously-removed tx can be added again as a brand-new entry.
    BOOST_REQUIRE(pb.Remove(fresh).has_value());
    BOOST_CHECK_EQUAL(pb.Add(fresh), PrivateBroadcast::AddResult::Added);
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), num_cap);
}

BOOST_AUTO_TEST_SUITE_END()
