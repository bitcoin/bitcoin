// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <private_broadcast.h>
#include <test/util/setup_common.h>
#include <util/time.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>

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
    SetMockTime(Now<NodeSeconds>());

    PrivateBroadcast pb;
    const NodeId recipient1{1};
    in_addr ipv4Addr;
    ipv4Addr.s_addr = 0xa0b0c001;
    const CService addr1(ipv4Addr, 1111);

    // No transactions initially.
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1, /*will_send_to_address=*/addr1).has_value());
    BOOST_CHECK_EQUAL(pb.GetStale().size(), 0);
    BOOST_CHECK(!pb.HavePendingTransactions());
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), 0);

    // Make a transaction and add it.
    const auto tx1{MakeDummyTx(/*id=*/1, /*num_witness=*/0)};

    BOOST_CHECK(pb.Add(tx1));
    BOOST_CHECK(!pb.Add(tx1));

    // Make another transaction with same txid, different wtxid and add it.
    const auto tx2{MakeDummyTx(/*id=*/1, /*num_witness=*/1)};
    BOOST_REQUIRE(tx1->GetHash() == tx2->GetHash());
    BOOST_REQUIRE(tx1->GetWitnessHash() != tx2->GetWitnessHash());

    BOOST_CHECK(pb.Add(tx2));

    {
        const auto infos{pb.GetBroadcastInfo()};
        BOOST_CHECK_EQUAL(infos.size(), 2);

        const auto it1{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx1->GetWitnessHash(); })};
        const auto it2{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx2->GetWitnessHash(); })};
        BOOST_REQUIRE(it1 != infos.end());
        BOOST_REQUIRE(it2 != infos.end());
        const auto& info1{*it1};
        const auto& info2{*it2};

        BOOST_CHECK_EQUAL(info1.peers.size(), 0);
        BOOST_CHECK_EQUAL(info2.peers.size(), 0);
    }

    const auto tx_for_recipient1{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1, /*will_send_to_address=*/addr1).value()};
    BOOST_CHECK(tx_for_recipient1 == tx1 || tx_for_recipient1 == tx2);

    // A second pick must return the other transaction.
    const NodeId recipient2{2};
    const CService addr2(ipv4Addr, 2222);
    const auto tx_for_recipient2{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient2, /*will_send_to_address=*/addr2).value()};
    BOOST_CHECK(tx_for_recipient2 == tx1 || tx_for_recipient2 == tx2);
    BOOST_CHECK_NE(tx_for_recipient1, tx_for_recipient2);

    {
        const auto infos{pb.GetBroadcastInfo()};
        BOOST_CHECK_EQUAL(infos.size(), 2);

        const auto it1{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx1->GetWitnessHash(); })};
        const auto it2{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx2->GetWitnessHash(); })};
        BOOST_REQUIRE(it1 != infos.end());
        BOOST_REQUIRE(it2 != infos.end());
        const auto& info1{*it1};
        const auto& info2{*it2};

        BOOST_CHECK_EQUAL(info1.peers.size(), 1);
        BOOST_CHECK_EQUAL(info2.peers.size(), 1);
    }

    const NodeId nonexistent_recipient{0};

    // Confirm transactions <-> recipients mapping is correct.
    BOOST_CHECK(!pb.GetTxForNode(nonexistent_recipient).has_value());
    BOOST_CHECK_EQUAL(pb.GetTxForNode(recipient1).value(), tx_for_recipient1);
    BOOST_CHECK_EQUAL(pb.GetTxForNode(recipient2).value(), tx_for_recipient2);

    // Confirm none of the transactions' reception have been confirmed.
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient1));
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient2));
    BOOST_CHECK(!pb.DidNodeConfirmReception(nonexistent_recipient));

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 2);

    // Confirm reception by recipient1.
    pb.NodeConfirmedReception(nonexistent_recipient); // Dummy call.
    pb.NodeConfirmedReception(recipient1);

    BOOST_CHECK(pb.DidNodeConfirmReception(recipient1));
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient2));

    {
        const auto infos{pb.GetBroadcastInfo()};
        BOOST_CHECK_EQUAL(infos.size(), 2);

        const auto it_confirmed{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx_for_recipient1->GetWitnessHash(); })};
        const auto it_unconfirmed{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx_for_recipient2->GetWitnessHash(); })};
        BOOST_REQUIRE(it_confirmed != infos.end());
        BOOST_REQUIRE(it_unconfirmed != infos.end());
        const auto& confirmed_tx{*it_confirmed};
        const auto& unconfirmed_tx{*it_unconfirmed};

        BOOST_CHECK_EQUAL(confirmed_tx.peers.size(), 1);
        BOOST_CHECK_EQUAL(confirmed_tx.peers[0].address.ToStringAddrPort(), addr1.ToStringAddrPort());
        BOOST_CHECK_EQUAL(unconfirmed_tx.peers.size(), 1);
        BOOST_CHECK_EQUAL(unconfirmed_tx.peers[0].address.ToStringAddrPort(), addr2.ToStringAddrPort());
        BOOST_CHECK(confirmed_tx.peers[0].received.has_value());
        BOOST_CHECK(!unconfirmed_tx.peers[0].received.has_value());
    }

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 1);
    BOOST_CHECK_EQUAL(pb.GetStale()[0], tx_for_recipient2);

    SetMockTime(Now<NodeSeconds>() + 10h);

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 2);

    BOOST_CHECK_EQUAL(pb.StopBroadcasting(tx_for_recipient1, "test").value(), 1);
    BOOST_CHECK(!pb.StopBroadcasting(tx_for_recipient1, "test").has_value());
    BOOST_CHECK_EQUAL(pb.StopBroadcasting(tx_for_recipient2, "test").value(), 0);
    BOOST_CHECK(!pb.StopBroadcasting(tx_for_recipient2, "test").has_value());

    // Stopped transactions remain stored and available in GetBroadcastInfo().
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), 2);
    BOOST_CHECK(!pb.HavePendingTransactions());
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/nonexistent_recipient, /*will_send_to_address=*/CService{}).has_value());
}

BOOST_AUTO_TEST_CASE(prune_finished)
{
    SetMockTime(Now<NodeSeconds>());
    PrivateBroadcast pb;

    static constexpr uint32_t MAX_STORED_TRANSACTIONS{1'000};

    // Add one transaction that will be the oldest finished entry.
    const auto oldest{MakeDummyTx(/*id=*/1, /*num_witness=*/0)};
    BOOST_CHECK(pb.Add(oldest));
    SetMockTime(Now<NodeSeconds>() - 1h);
    BOOST_CHECK(pb.StopBroadcasting(oldest, "test").has_value());

    // Fill up to the cap with other finished transactions that all have newer finish times.
    SetMockTime(Now<NodeSeconds>() + 1h);
    for (uint32_t i = 0; i < MAX_STORED_TRANSACTIONS - 1; ++i) {
        auto tx{MakeDummyTx(/*id=*/i + 2, /*num_witness=*/0)};
        BOOST_CHECK(pb.Add(tx));
        BOOST_CHECK(pb.StopBroadcasting(tx, "test").has_value());
    }
    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), MAX_STORED_TRANSACTIONS);

    // Adding one more transaction should evict the oldest finished transaction.
    const auto extra{MakeDummyTx(/*id=*/MAX_STORED_TRANSACTIONS + 1, /*num_witness=*/0)};
    BOOST_CHECK(pb.Add(extra));
    const auto infos1{pb.GetBroadcastInfo()};
    BOOST_CHECK_EQUAL(infos1.size(), MAX_STORED_TRANSACTIONS);
    BOOST_CHECK(std::ranges::find_if(infos1, [&](const auto& info) {
                    return info.tx->GetWitnessHash() == oldest->GetWitnessHash();
                }) == infos1.end());
    BOOST_CHECK(std::ranges::find_if(infos1, [&](const auto& info) {
                    return info.tx->GetWitnessHash() == extra->GetWitnessHash();
                }) != infos1.end());

    // If all entries are unfinished, do not prune even above the cap.
    PrivateBroadcast pb_unfinished;
    for (uint32_t i = 0; i < MAX_STORED_TRANSACTIONS + 1; ++i) {
        BOOST_CHECK(pb_unfinished.Add(MakeDummyTx(/*id=*/i + 1, /*num_witness=*/0)));
    }
    const auto infos2{pb_unfinished.GetBroadcastInfo()};
    BOOST_CHECK_EQUAL(infos2.size(), MAX_STORED_TRANSACTIONS + 1);
    BOOST_CHECK(std::ranges::all_of(infos2, [](const auto& info) { return !info.final_state.has_value(); }));
}

BOOST_AUTO_TEST_CASE(update_aborted_to_received)
{
    SetMockTime(Now<NodeSeconds>());

    PrivateBroadcast pb;
    const auto tx{MakeDummyTx(/*id=*/1, /*num_witness=*/0)};
    BOOST_CHECK(pb.Add(tx));

    in_addr ipv4Addr;
    ipv4Addr.s_addr = 0xa0b0c002;

    // Stop broadcasting as aborted.
    BOOST_CHECK_EQUAL(pb.StopBroadcasting(tx, "aborted").value(), 0);
    int64_t time_aborted{0};
    {
        const auto infos{pb.GetBroadcastInfo()};
        const auto it{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx->GetWitnessHash(); })};
        BOOST_REQUIRE(it != infos.end());
        BOOST_REQUIRE(it->final_state.has_value());
        BOOST_CHECK(!it->final_state->result.has_value());
        BOOST_CHECK_EQUAL(it->final_state->result.error(), "aborted");
        time_aborted = TicksSinceEpoch<std::chrono::seconds>(it->final_state->time);
    }

    // Further abort attempts should return nullopt and must not overwrite the aborted final state.
    SetMockTime(Now<NodeSeconds>() + 1s);
    BOOST_CHECK(!pb.StopBroadcasting(tx, "aborted again").has_value());
    {
        const auto infos{pb.GetBroadcastInfo()};
        const auto it{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx->GetWitnessHash(); })};
        BOOST_REQUIRE(it != infos.end());
        BOOST_REQUIRE(it->final_state.has_value());
        BOOST_CHECK(!it->final_state->result.has_value());
        BOOST_CHECK_EQUAL(it->final_state->result.error(), "aborted");
        BOOST_CHECK_EQUAL(TicksSinceEpoch<std::chrono::seconds>(it->final_state->time), time_aborted);
    }

    // If we later receive it back from the network, update final_state to received (but return nullopt).
    SetMockTime(Now<NodeSeconds>() + 1s);
    const CService addr_received(ipv4Addr, 2222);
    BOOST_CHECK(!pb.StopBroadcasting(tx, addr_received).has_value());
    int64_t time_received{0};
    {
        const auto infos{pb.GetBroadcastInfo()};
        const auto it{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx->GetWitnessHash(); })};
        BOOST_REQUIRE(it != infos.end());
        BOOST_REQUIRE(it->final_state.has_value());
        BOOST_REQUIRE(it->final_state->result.has_value());
        BOOST_CHECK_EQUAL(it->final_state->result.value().ToStringAddrPort(), addr_received.ToStringAddrPort());
        time_received = TicksSinceEpoch<std::chrono::seconds>(it->final_state->time);
    }

    // Further received-from updates should return nullopt and must not overwrite the received final state.
    SetMockTime(Now<NodeSeconds>() + 1s);
    const CService addr_received2(ipv4Addr, 3333);
    BOOST_CHECK(!pb.StopBroadcasting(tx, addr_received2).has_value());
    {
        const auto infos{pb.GetBroadcastInfo()};
        const auto it{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx->GetWitnessHash(); })};
        BOOST_REQUIRE(it != infos.end());
        BOOST_REQUIRE(it->final_state.has_value());
        BOOST_REQUIRE(it->final_state->result.has_value());
        BOOST_CHECK_EQUAL(it->final_state->result.value().ToStringAddrPort(), addr_received.ToStringAddrPort());
        BOOST_CHECK_EQUAL(TicksSinceEpoch<std::chrono::seconds>(it->final_state->time), time_received);
    }

    // Further attempts to abort should return nullopt and must not overwrite the received final state.
    BOOST_CHECK(!pb.StopBroadcasting(tx, "aborted after received").has_value());
    {
        const auto infos{pb.GetBroadcastInfo()};
        const auto it{std::ranges::find_if(infos, [&](const auto& info) { return info.tx->GetWitnessHash() == tx->GetWitnessHash(); })};
        BOOST_REQUIRE(it != infos.end());
        BOOST_REQUIRE(it->final_state.has_value());
        BOOST_REQUIRE(it->final_state->result.has_value());
        BOOST_CHECK_EQUAL(it->final_state->result.value().ToStringAddrPort(), addr_received.ToStringAddrPort());
    }
}

BOOST_AUTO_TEST_SUITE_END()
