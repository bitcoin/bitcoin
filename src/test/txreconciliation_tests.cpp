// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    const uint64_t salt = 0;

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0);

    // Invalid version.
    BOOST_CHECK_EQUAL(tracker.RegisterPeer(/*peer_id=*/0, /*is_peer_inbound=*/true,
                                           /*peer_recon_version=*/0, salt),
                      ReconciliationRegisterResult::PROTOCOL_VIOLATION);

    // Valid registration (inbound and outbound peers).
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(0, true, 1, salt), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(tracker.RegisterPeer(1, false, 1, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(1));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(2));
    tracker.PreRegisterPeer(2);
    BOOST_REQUIRE(tracker.RegisterPeer(2, true, 2, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(2));

    // Try registering for the second time.
    BOOST_REQUIRE(tracker.RegisterPeer(1, false, 1, salt) == ReconciliationRegisterResult::ALREADY_REGISTERED);

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(100, true, 1, salt), ReconciliationRegisterResult::NOT_FOUND);
    BOOST_CHECK(!tracker.IsPeerRegistered(100));
}

BOOST_AUTO_TEST_CASE(ForgetPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Removing peer after pre-registring works and does not let to register the peer.
    tracker.PreRegisterPeer(peer_id0);
    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));
    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(AddToSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // If the peer is not registered, adding to the set fails
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    auto r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());

    // As long as the peer is registered, adding a new wtxid to the set should work
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());

    // If the peer is dropped, adding wtxids to its set should fail
    tracker.ForgetPeer(peer_id0);
    Wtxid wtxid2{Wtxid::FromUint256(frc.rand256())};
    r = tracker.AddToSet(peer_id0, wtxid2);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());

    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id1));

    // As long as the peer is registered and the transaction is not in the set, adding it should succeed
    for (size_t i = 0; i < MAX_RECONSET_SIZE; ++i)
        r = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256()));
        BOOST_REQUIRE(r.m_succeeded);
        BOOST_REQUIRE(!r.m_conflict.has_value());

    // Trying to add the same item twice should fail
    r = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());
}

BOOST_AUTO_TEST_CASE(TryRemovingFromSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));
}

BOOST_AUTO_TEST_CASE(SortPeersByFewestParentsTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    FastRandomContext frc{/*fDeterministic=*/true};

    std::vector<NodeId> peers = {0, 1, 2, 3, 4, 5, 6, 7};
    std::vector<Wtxid> parents;

    for (auto &peer_id: peers) {
        tracker.PreRegisterPeer(peer_id);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id, true, 1, 1), ReconciliationRegisterResult::SUCCESS);

        // Add a number of parents equal to the peer_id to each peer's reconciliation set.
        // We purposely want to have a peer with zero parent to cover that edge case
        parents.push_back(Wtxid::FromUint256(frc.rand256()));
        for (auto i=0; i<peer_id; i++) {
            tracker.AddToSet(peer_id, parents[i]);
        }
    }

    // We gave each peer a number of parents equal to its peer_id, so we expect the ordering
    // to match peer_id in ascending order (which matches the insertion order for the peers vector)
    BOOST_REQUIRE(tracker.SortPeersByFewestParents(parents) == peers);

    // Lets check ties now. Leave the tree first peers with no parent (so they tie)
    // plus add some to the last two
    TxReconciliationTracker tracker2(TXRECONCILIATION_VERSION);
    peers = {0, 1, 2, 3, 4};

    for (auto &peer_id: peers) {
        tracker2.PreRegisterPeer(peer_id);
        BOOST_REQUIRE_EQUAL(tracker2.RegisterPeer(peer_id, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
        if (peer_id > 2) {
            // Lets make the last two peers be sorted in reverse insertion order
            for (size_t i=peer_id; i<parents.size(); i++) {
                tracker2.AddToSet(peer_id, parents[i]);
            }
        }
    }

    auto sorted_peers = tracker2.SortPeersByFewestParents(parents);
    auto best_peers = std::vector<int>(sorted_peers.begin(), sorted_peers.begin() + 3);

    // best_peers should contain the first three peers in an unknown order, so sorting it should match the head of peers
    std::sort(best_peers.begin(), best_peers.end());
    BOOST_REQUIRE(std::equal(peers.begin(), peers.begin() + 3, best_peers.begin()));
    // The last two peers should be sorted in reverse insertion order
    BOOST_REQUIRE(std::equal(sorted_peers.begin() + 3, sorted_peers.end(), peers.rbegin(), peers.rend() - 3));
}

BOOST_AUTO_TEST_SUITE_END()
