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

    // peer0 is allowed to send ancpkginfo for orphan0, but not for any other tx or version
    BOOST_CHECK(tracker.PkgInfoAllowed(/*nodeid=*/0, orphan0->GetWitnessHash(), node::PKG_RELAY_ANCPKG));
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/0, orphan0->GetWitnessHash(), unsupported_package_type));
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/0, orphan1->GetWitnessHash(), node::PKG_RELAY_ANCPKG));
    // No other peers are allowed to send ancpkginfo (they disconnected or aren't registered for it)
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/1, orphan1->GetWitnessHash(), node::PKG_RELAY_ANCPKG));
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/2, orphan1->GetWitnessHash(), node::PKG_RELAY_ANCPKG));
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/3, orphan2->GetWitnessHash(), node::PKG_RELAY_ANCPKG));
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/4, orphan2->GetWitnessHash(), node::PKG_RELAY_ANCPKG));

    // After receiving ancpkginfo, a second ancpkginfo is not allowed
    const auto missing_wtxid{det_rand.rand256()};
    std::vector<std::pair<uint256, bool>> txdata_status;
    txdata_status.emplace_back(missing_wtxid, true);
    txdata_status.emplace_back(orphan0->GetWitnessHash(), false);
    const auto ancpkginfo_result = tracker.ReceivedAncPkgInfo(/*nodeid=*/0, orphan0->GetWitnessHash(), txdata_status,
                                            current_time + 100s);
    BOOST_CHECK_EQUAL(std::get<std::vector<uint256>>(ancpkginfo_result).size(), 1);
    BOOST_CHECK_EQUAL(std::get<std::vector<uint256>>(ancpkginfo_result).front(), missing_wtxid);
    BOOST_CHECK(!tracker.PkgInfoAllowed(/*nodeid=*/0, orphan0->GetWitnessHash(), node::PKG_RELAY_ANCPKG));
}
void RegisterPeerForAncestorPackages(node::TxPackageTracker& tracker, NodeId peer)
{
    tracker.ReceivedVersion(peer);
    tracker.ReceivedSendpackages(peer, node::PKG_RELAY_ANCPKG);
    tracker.ReceivedVerack(peer, true, true, true);
}
BOOST_AUTO_TEST_CASE(txdata_download)
{
    FastRandomContext det_rand{true};
    node::TxPackageTracker tracker;

    // 2 parents 1 child
    const auto tx_parent1 = make_tx({COutPoint{det_rand.rand256(), 0}}, 1);
    const auto tx_parent2 = make_tx({COutPoint{det_rand.rand256(), 0}}, 1);
    const auto tx_child = make_tx({COutPoint{tx_parent1->GetHash(), 0}, COutPoint{tx_parent2->GetHash(), 0}}, 1);
    const Package package_2p1c{tx_parent1, tx_parent2, tx_child};

    const auto current_time{GetTime<std::chrono::microseconds>()};
    {
    NodeId peer = 0;
    RegisterPeerForAncestorPackages(tracker, peer);

    tracker.AddOrphanTx(peer, tx_child->GetWitnessHash(), tx_child, /*is_preferred=*/true, current_time);
    const auto requests = tracker.GetOrphanRequests(peer, current_time + 1s);
    BOOST_CHECK_EQUAL(1, requests.size());
    BOOST_CHECK(tracker.PkgInfoAllowed(peer, tx_child->GetWitnessHash(), node::PKG_RELAY_ANCPKG));

    const std::vector<uint256> missing_wtxids{tx_parent1->GetWitnessHash(), tx_parent2->GetWitnessHash()};
    std::vector<std::pair<uint256, bool>> txdata_status;
    txdata_status.emplace_back(tx_parent1->GetWitnessHash(), true);
    txdata_status.emplace_back(tx_parent2->GetWitnessHash(), true);
    txdata_status.emplace_back(tx_child->GetWitnessHash(), false);
    const auto ancpkginfo_result = tracker.ReceivedAncPkgInfo(peer, tx_child->GetWitnessHash(), txdata_status,
                                            current_time + 100s);
    const auto request_list = std::get<std::vector<uint256>>(ancpkginfo_result);
    BOOST_CHECK(std::find(request_list.cbegin(), request_list.cend(), tx_parent1->GetWitnessHash()) != request_list.end());
    BOOST_CHECK(std::find(request_list.cbegin(), request_list.cend(), tx_parent2->GetWitnessHash()) != request_list.end());

    // Nodeid and exact missing transactions must match.
    BOOST_CHECK(!tracker.ReceivedPkgTxns(peer, {tx_parent1}).has_value());
    BOOST_CHECK(!tracker.ReceivedPkgTxns(peer, {tx_parent2}).has_value());
    BOOST_CHECK(!tracker.ReceivedPkgTxns(2, {tx_parent1, tx_parent2}).has_value());

    const auto validate_2p1c = tracker.ReceivedPkgTxns(peer, {tx_parent1, tx_parent2});
    BOOST_CHECK(validate_2p1c.has_value());
    BOOST_CHECK_EQUAL(validate_2p1c.value().m_info_provider, peer);
    BOOST_CHECK_EQUAL(validate_2p1c->m_rep_wtxid, tx_child->GetWitnessHash());
    BOOST_CHECK_EQUAL(validate_2p1c->m_pkginfo_hash, GetPackageHash(package_2p1c));
    BOOST_CHECK(validate_2p1c->m_unvalidated_txns == package_2p1c);
    }
}

BOOST_AUTO_TEST_SUITE_END()
