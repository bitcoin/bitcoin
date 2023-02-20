// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txpackagetracker.h>
#include <random.h>
#include <txorphanage.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

static inline CTransactionRef make_tx(const std::vector<COutPoint>& inputs, size_t num_outputs)
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(num_outputs);
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout = inputs[i];
        // Add a witness so wtxid != txid
        CScriptWitness witness;
        witness.stack.push_back(std::vector<unsigned char>(i + 10));
        tx.vin[i].scriptWitness = witness;
    }
    for (size_t i = 0; i < num_outputs; ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = COIN;
    }
    return MakeTransactionRef(tx);
}

BOOST_FIXTURE_TEST_SUITE(txpackagetracker_tests, BasicTestingSetup)
BOOST_AUTO_TEST_CASE(pkginfo)
{
    FastRandomContext det_rand{true};
    node::TxPackageTracker tracker;

    // Peer 0: successful handshake
    NodeId peer = 0;
    tracker.ReceivedVersion(peer);
    tracker.ReceivedSendpackages(peer, node::PKG_RELAY_ANCPKG);
    BOOST_CHECK(tracker.ReceivedVerack(peer, /*inbound`*/ true, /*txrelay=*/true, /*wtxidrelay=*/true));

    // Peer 1: unsupported version(s)
    const node::PackageRelayVersions unsupported_package_type{1ULL << 43};
    peer = 1;
    tracker.ReceivedVersion(peer);
    tracker.ReceivedSendpackages(peer, unsupported_package_type);
    BOOST_CHECK(!tracker.ReceivedVerack(peer, /*inbound`*/ true, /*txrelay=*/true, /*wtxidrelay=*/true));

    // Peer 2: no wtxidrelay
    peer = 2;
    tracker.ReceivedVersion(peer);
    tracker.ReceivedSendpackages(peer, node::PKG_RELAY_ANCPKG);
    BOOST_CHECK(!tracker.ReceivedVerack(peer, /*inbound`*/ true, /*txrelay=*/true, /*wtxidrelay=*/false));

    // Peer 3: fRelay=false
    peer = 3;
    tracker.ReceivedVersion(peer);
    tracker.ReceivedSendpackages(peer, node::PKG_RELAY_ANCPKG);
    BOOST_CHECK(!tracker.ReceivedVerack(peer, /*inbound`*/ true, /*txrelay=*/false, /*wtxidrelay=*/true));

    for (NodeId i{0}; i < peer + 1; ++i) {
        BOOST_CHECK_EQUAL(tracker.Count(i), 0);
        BOOST_CHECK_EQUAL(tracker.CountInFlight(i), 0);
    }

    const auto current_time{GetTime<std::chrono::microseconds>()};
    const auto orphan0 = make_tx({COutPoint{det_rand.rand256(), 0}}, 1);
    const auto wtxid_parent1 = det_rand.rand256();
    const auto orphan1 = make_tx({COutPoint{wtxid_parent1, 0}, COutPoint{wtxid_parent1, 1}}, 1);
    const auto orphan2 = make_tx({COutPoint{det_rand.rand256(), 0}, COutPoint{det_rand.rand256(), 0}}, 1);
    tracker.AddOrphanTx(/*nodeid=*/0, orphan0->GetWitnessHash(), orphan0, /*is_preferred=*/false, current_time);
    tracker.AddOrphanTx(/*nodeid=*/1, orphan1->GetWitnessHash(), orphan1, /*is_preferred=*/false, current_time);
    tracker.AddOrphanTx(/*nodeid=*/1, orphan0->GetWitnessHash(), orphan0, /*is_preferred=*/false, current_time + 10s);
    tracker.AddOrphanTx(/*nodeid=*/2, orphan1->GetWitnessHash(), orphan1, /*is_preferred=*/false, current_time + 5s);
    tracker.AddOrphanTx(/*nodeid=*/3, orphan2->GetWitnessHash(), orphan2, /*is_preferred=*/true, current_time + 5s);
    tracker.AddOrphanTx(/*nodeid=*/4, orphan2->GetWitnessHash(), orphan2, /*is_preferred=*/false, current_time + 9s);
    BOOST_CHECK_EQUAL(tracker.Count(0), 1);
    BOOST_CHECK_EQUAL(tracker.Count(1), 2);
    BOOST_CHECK_EQUAL(tracker.Count(2), 1);
    BOOST_CHECK_EQUAL(tracker.Count(3), 1);
    BOOST_CHECK_EQUAL(tracker.Count(4), 1);
    const auto peer0_requests = tracker.GetOrphanRequests(/*nodeid=*/0, current_time + 1s);
    const auto peer1_requests = tracker.GetOrphanRequests(/*nodeid=*/1, current_time + 1s);
    const auto peer2_requests = tracker.GetOrphanRequests(/*nodeid=*/2, current_time + 1s);
    const auto peer3_requests = tracker.GetOrphanRequests(/*nodeid=*/3, current_time);
    const auto peer4_requests = tracker.GetOrphanRequests(/*nodeid=*/4, current_time);
    BOOST_CHECK_EQUAL(peer0_requests.size(), 1);
    BOOST_CHECK(peer0_requests.front().IsWtxid());
    BOOST_CHECK_EQUAL(peer0_requests.front().GetHash(), orphan0->GetWitnessHash());
    BOOST_CHECK_EQUAL(peer1_requests.size(), 1);
    BOOST_CHECK_EQUAL(peer1_requests.front().GetHash(), wtxid_parent1);
    BOOST_CHECK(!peer1_requests.front().IsWtxid());
    BOOST_CHECK(peer2_requests.empty());
    BOOST_CHECK(peer3_requests.empty());
    BOOST_CHECK(peer4_requests.empty());
    BOOST_CHECK_EQUAL(tracker.CountInFlight(0), 1);
    BOOST_CHECK_EQUAL(tracker.CountInFlight(1), 1);
    BOOST_CHECK_EQUAL(tracker.Count(1), 2);
    BOOST_CHECK_EQUAL(tracker.CountInFlight(2), 0);
    BOOST_CHECK_EQUAL(tracker.CountInFlight(3), 0);
    BOOST_CHECK_EQUAL(tracker.CountInFlight(4), 0);
    // After peer1 disconnects, request from peer2
    tracker.DisconnectedPeer(1);
    // After peer3 disconnects, request from peer4
    tracker.DisconnectedPeer(3);
    BOOST_CHECK_EQUAL(tracker.Count(3), 0);
    BOOST_CHECK_EQUAL(tracker.CountInFlight(3), 0);
    const auto peer2_requests_later = tracker.GetOrphanRequests(/*nodeid=*/2, current_time + 5s);
    // 2 inputs but only 1 unique parent
    BOOST_CHECK_EQUAL(peer2_requests_later.size(), 1);
    BOOST_CHECK(!peer2_requests_later.front().IsWtxid());
    BOOST_CHECK_EQUAL(peer2_requests_later.front().GetHash(), wtxid_parent1);
    const auto peer4_requests_later = tracker.GetOrphanRequests(/*nodeid=*/4, current_time + 9s);
    BOOST_CHECK_EQUAL(peer4_requests_later.size(), 2);
    // This counts as 1 in-flight request
    BOOST_CHECK_EQUAL(tracker.CountInFlight(4), 1);
}

BOOST_AUTO_TEST_SUITE_END()
