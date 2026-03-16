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
    const CService addr1{ipv4Addr, 1111};

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
    BOOST_CHECK(tx_for_recipient1 == tx1 || tx_for_recipient1 == tx2);

    // A second pick must return the other transaction.
    const NodeId recipient2{2};
    const CService addr2{ipv4Addr, 2222};
    const auto tx_for_recipient2{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient2, /*will_send_to_address=*/addr2).value()};
    BOOST_CHECK(tx_for_recipient2 == tx1 || tx_for_recipient2 == tx2);
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

    BOOST_CHECK_EQUAL(pb.GetStale().size(), 2);

    // Confirm reception by recipient1.
    pb.NodeConfirmedReception(nonexistent_recipient); // Dummy call.
    pb.NodeConfirmedReception(recipient1);

    BOOST_CHECK(pb.DidNodeConfirmReception(recipient1));
    BOOST_CHECK(!pb.DidNodeConfirmReception(recipient2));

    const auto infos{pb.GetBroadcastInfo()};
    BOOST_CHECK_EQUAL(infos.size(), 2);
    {
        const auto& [tx, peers]{find_tx_info(infos, tx_for_recipient1)};
        BOOST_CHECK_EQUAL(peers.size(), 1);
        BOOST_CHECK_EQUAL(peers[0].address.ToStringAddrPort(), addr1.ToStringAddrPort());
        BOOST_CHECK(peers[0].received.has_value());
    }
    {
        const auto& [tx, peers]{find_tx_info(infos, tx_for_recipient2)};
        BOOST_CHECK_EQUAL(peers.size(), 1);
        BOOST_CHECK_EQUAL(peers[0].address.ToStringAddrPort(), addr2.ToStringAddrPort());
        BOOST_CHECK(!peers[0].received.has_value());
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
    const CService addr_nonexistent{ipv4Addr, 3333};
    BOOST_CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/nonexistent_recipient, /*will_send_to_address=*/addr_nonexistent).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
