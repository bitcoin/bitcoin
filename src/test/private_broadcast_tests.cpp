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

    // No transactions initially.
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1).has_value());
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

        BOOST_CHECK_EQUAL(info1.num_sent, 0);
        BOOST_CHECK(!info1.last_sent.has_value());
        BOOST_CHECK_EQUAL(info1.num_peer_reception_acks, 0);
        BOOST_CHECK(!info1.last_peer_reception_ack.has_value());

        BOOST_CHECK_EQUAL(info2.num_sent, 0);
        BOOST_CHECK(!info2.last_sent.has_value());
        BOOST_CHECK_EQUAL(info2.num_peer_reception_acks, 0);
        BOOST_CHECK(!info2.last_peer_reception_ack.has_value());
    }

    const auto tx_for_recipient1{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1).value()};
    BOOST_CHECK(tx_for_recipient1 == tx1 || tx_for_recipient1 == tx2);

    // A second pick must return the other transaction.
    const NodeId recipient2{2};
    const auto tx_for_recipient2{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient2).value()};
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

        BOOST_CHECK_EQUAL(info1.num_sent, 1);
        BOOST_CHECK(info1.last_sent.has_value());
        BOOST_CHECK_EQUAL(info2.num_sent, 1);
        BOOST_CHECK(info2.last_sent.has_value());
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

        BOOST_CHECK_EQUAL(confirmed_tx.num_peer_reception_acks, 1);
        BOOST_CHECK(confirmed_tx.last_peer_reception_ack.has_value());
        BOOST_CHECK_EQUAL(unconfirmed_tx.num_peer_reception_acks, 0);
        BOOST_CHECK(!unconfirmed_tx.last_peer_reception_ack.has_value());
    }

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 1);
    BOOST_CHECK_EQUAL(pb.GetStale()[0], tx_for_recipient2);

    SetMockTime(Now<NodeSeconds>() + 10h);

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 2);

    BOOST_CHECK_EQUAL(pb.Remove(tx_for_recipient1).value(), 1);
    BOOST_CHECK(!pb.Remove(tx_for_recipient1).has_value());
    BOOST_CHECK_EQUAL(pb.Remove(tx_for_recipient2).value(), 0);
    BOOST_CHECK(!pb.Remove(tx_for_recipient2).has_value());

    BOOST_CHECK_EQUAL(pb.GetBroadcastInfo().size(), 0);
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/nonexistent_recipient).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
