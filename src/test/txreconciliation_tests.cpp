// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
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
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
    NodeId peer_id0 = 0;

    // Removing peer which is not there works.
    tracker.ForgetPeer(peer_id0);

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
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
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
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
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

    // As long as the peer is registered, the transaction is not in the set, and there is no short id
    // collision, adding should work
    size_t added_txs = 0;
    while (added_txs < MAX_RECONSET_SIZE) {
        wtxid = Wtxid::FromUint256(frc.rand256());
        Wtxid collision;

        r = tracker.AddToSet(peer_id1, wtxid);
        if (r.m_succeeded) {
            BOOST_REQUIRE(!r.m_conflict.has_value());
            ++added_txs;
        } else {
            BOOST_REQUIRE_EQUAL(r.m_conflict.value(), collision);
        }
    }

    // Adding one more item will fail due to the set being full
    r = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());

    // Trying to add the same item twice will just bypass
    r = tracker.AddToSet(peer_id1, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());
}

BOOST_AUTO_TEST_CASE(AddToSetCollisionTest)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // Precompute collision
    Wtxid wtxid{Wtxid::FromUint256(uint256("c70d778bccef36a81aed8da0b819d2bd28bd8653e56a5d40903df1a0ade0b876"))};
    Wtxid collision{Wtxid::FromUint256(uint256("ae52a6ecb8733fba1f7af6022a8b9dd327d7825054229fafcad7e03c38ae2a50"))};

    // Register the peer with a predefined salt so we can force the collision
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    // Once the peer is registered, we can try to add both transactions and check
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    auto r = tracker.AddToSet(peer_id0, collision);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE_EQUAL(r.m_conflict.value(), wtxid);
}

BOOST_AUTO_TEST_CASE(IsTransactionInSetTest)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // If the peer is not registered, no transaction can be found
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));

    // Same happens if the peer is only pre-registered
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));
    // Or registered but the transaction hasn't been added
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));

    // Adding the transaction will make it queryable, but only if we set include_delayed,
    // given transactions are placed into the delayed set first
    auto r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/false));
    BOOST_REQUIRE(tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));

    // After a trickle interval, the transaction will be queryable from the regular set
    BOOST_REQUIRE(tracker.ReadyDelayedTransactions(peer_id0));
    BOOST_REQUIRE(tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/false));
}

BOOST_AUTO_TEST_CASE(TryRemovingFromSetTest)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
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
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
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
    TxReconciliationTracker tracker2(TXRECONCILIATION_VERSION, hasher);
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

BOOST_AUTO_TEST_CASE(ReadyAndDelayedTransactionsTest)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // If the peer is not registered, there are no transactions to ready
    BOOST_CHECK(!tracker.ReadyDelayedTransactions(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    // Same happens if the peer is only pre-registered
    BOOST_CHECK(!tracker.ReadyDelayedTransactions(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // Adding a transaction places it in the delayed set until ReadyDelayedTransactions is called
    auto r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/false));
    BOOST_REQUIRE(tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));

    // Reading the transaction will move it to the regular set
    BOOST_REQUIRE(tracker.ReadyDelayedTransactions(peer_id0));
    BOOST_REQUIRE(tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/false));

    // Trying to add the same transaction twice will be bypassed, given both sets are checked
    r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());
    // The transaction is still in the available set
    BOOST_REQUIRE(tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/false));

    // Removing a transaction does so indistinguishably of what internal set they are in
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));
    // Add again to check removing from delayed
    r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_conflict.has_value());
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/false));
    BOOST_REQUIRE(tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    BOOST_REQUIRE(!tracker.IsTransactionInSet(peer_id0, wtxid, /*include_delayed*/true));
}

BOOST_AUTO_TEST_CASE(GetFanoutTargetsTest)
{

    auto should_fanout_to = [](NodeId peer_id, std::vector<NodeId> fanout_targets) {
        return std::find(fanout_targets.begin(), fanout_targets.end(), peer_id) != fanout_targets.end();
    };

    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, hasher);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};
    std::vector<NodeId> fanout_targets;

    // Registered peers should be selected to receive some transaction via flooding.
    // Since there is only one reconciling peer, it will be selected for all transactions.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/false, 1, 1), ReconciliationRegisterResult::SUCCESS);
    for (int i = 0; i < 100; ++i) {
        fanout_targets = tracker.GetFanoutTargets(Wtxid::FromUint256(frc.rand256()), /*inbounds_fanout_tx_relay=*/0, /*outbounds_fanout_tx_relay=*/0);
        BOOST_CHECK(should_fanout_to(peer_id0, fanout_targets));
    }

    // Don't select a fanout target if we already have enough targets
    for (int i = 0; i < 100; ++i) {
        fanout_targets = tracker.GetFanoutTargets(Wtxid::FromUint256(frc.rand256()), /*inbounds_fanout_tx_relay=*/0, /*outbounds_fanout_tx_relay=*/2);
        BOOST_CHECK(!should_fanout_to(peer_id0, fanout_targets));
        fanout_targets = tracker.GetFanoutTargets(Wtxid::FromUint256(frc.rand256()), /*inbounds_fanout_tx_relay=*/0, /*outbounds_fanout_tx_relay=*/100);
        BOOST_CHECK(!should_fanout_to(peer_id0, fanout_targets));
    }


    // Now for inbound connections.
    // Initialize a new instance with a new hasher to be used later on.
    CSipHasher hasher2(0x0706050403020100ULL, 0x4F0E0D0C0B0A0908ULL);
    TxReconciliationTracker tracker2(TXRECONCILIATION_VERSION, hasher2);
    int inbound_peers = 36;
    for (int i = 1; i < inbound_peers; ++i) {
        tracker.PreRegisterPeer(i);
        tracker2.PreRegisterPeer(i);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(i, /*is_peer_inbound=*/true, 1, 1), ReconciliationRegisterResult::SUCCESS);
        BOOST_REQUIRE_EQUAL(tracker2.RegisterPeer(i, /*is_peer_inbound=*/true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    }

    // Relay to a fraction of registered inbound peers.
    // For 35 peers we will choose 3.5 flooding targets, which means that it's either 3 or 4 with
    // 50% chance. Make sure the randomness actually works by checking against a different hasher.
    size_t total_fanouted1 = 0;
    size_t total_fanouted2 = 0;
    auto wtxid = Wtxid::FromUint256(uint256(1)); // determinism is required.
    std::vector<NodeId> fanout_targets_t2;
    fanout_targets = tracker.GetFanoutTargets(Wtxid::FromUint256(wtxid), /*inbounds_fanout_tx_relay=*/0, /*outbounds_fanout_tx_relay=*/0);
    fanout_targets_t2 = tracker2.GetFanoutTargets(Wtxid::FromUint256(wtxid), /*inbounds_fanout_tx_relay=*/0, /*outbounds_fanout_tx_relay=*/0);
    for (int i = 1; i < inbound_peers; ++i) {
        total_fanouted1 += should_fanout_to(i, fanout_targets);
        total_fanouted2 += should_fanout_to(i, fanout_targets_t2);
    }
    BOOST_CHECK_EQUAL(total_fanouted1, 3);
    BOOST_CHECK_EQUAL(total_fanouted2, 4);

    // Don't relay if there is sufficient non-reconciling peers
    fanout_targets = tracker.GetFanoutTargets(Wtxid::FromUint256(frc.rand256()), /*inbounds_fanout_tx_relay=*/4, /*outbounds_fanout_tx_relay=*/0);
    for (int j = 0; j < 100; ++j) {
        size_t total_fanouted = 0;
        for (int i = 1; i < inbound_peers; ++i) {
            total_fanouted += should_fanout_to(i, fanout_targets);
        }
        BOOST_CHECK_EQUAL(total_fanouted, 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
