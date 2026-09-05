// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <net_processing.h>
#include <node/minisketchwrapper.h>
#include <node/txreconciliation.h>
#include <node/txreconciliation_impl.h>
#include <protocol.h>

#include <test/util/common.h>
#include <test/util/logging.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <test/util/validation.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cmath>

using namespace node;

/** Testing setup with transaction reconciliation enabled, for the tests that drive
 *  announcements through PeerManager rather than the tracker directly. */
struct TxReconciliationRelaySetup : public TestingSetup {
    TxReconciliationRelaySetup()
        : TestingSetup{ChainType::REGTEST,
                       TestOpts{.extra_args = {"-txreconciliation", "-txsendrate=1000"}}}
    {
        // We don't reconcile during IBD, so jump out of it to be able to test reconciliation.
        static_cast<TestChainstateManager&>(*m_node.chainman).JumpOutOfIbd();
    }

    ~TxReconciliationRelaySetup()
    {
        // The connman owns the test nodes and deletes them here, so this has to run before
        // the base fixture tears the connman down.
        static_cast<ConnmanTestMsg&>(*m_node.connman).ClearTestNodes();
    }

    /** Connect an inbound peer and complete a reconciliation handshake with it. */
    CNode& AddReconciliationPeer(NodeId id) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
        auto* node = new CNode{id,
                               /*sock=*/nullptr,
                               CAddress{},
                               /*nKeyedNetGroupIn=*/0,
                               /*nLocalHostNonceIn=*/0,
                               CAddress{},
                               /*addrNameIn=*/"",
                               ConnectionType::INBOUND,
                               /*inbound_onion=*/false,
                               /*network_key=*/0};
        connman.AddTestNode(*node);

        // Version exchange, but stop short of VERACK. The peer has to answer our SENDTXRCNCL
        // before it, otherwise the node forgets the pre-registered state.
        connman.Handshake(*node, /*successfully_connected=*/false,
                          ServiceFlags(NODE_NETWORK | NODE_WITNESS),
                          ServiceFlags(NODE_NETWORK | NODE_WITNESS),
                          PROTOCOL_VERSION, /*relay_txs=*/true);

        connman.ReceiveMsgFrom(*node, NetMsg::Make(NetMsgType::WTXIDRELAY));
        connman.ReceiveMsgFrom(*node, NetMsg::Make(NetMsgType::SENDTXRCNCL,
                                                   TXRECONCILIATION_VERSION, REMOTE_SALT_RELAY));
        connman.ReceiveMsgFrom(*node, NetMsg::Make(NetMsgType::VERACK));
        node->fPauseSend = false;
        // Process all (three) messages, then send our own VERACK to complete the handshake.
        for (int i = 0; i < 3; ++i) connman.ProcessMessagesOnce(*node);
        BOOST_REQUIRE(node->fSuccessfullyConnected);
        m_node.peerman->SendMessages(*node);
        connman.FlushSendBuffer(*node);
        return *node;
    }

    /** The message type the node has queued for this peer, empty if nothing is pending. */
    std::string PendingMsgType(CNode& node)
    {
        LOCK(node.cs_vSend);
        const auto& [to_send, _more, msg_type] = node.m_transport->GetBytesToSend(false);
        return to_send.empty() ? "" : msg_type;
    }

    /** Add a random transaction straight to the mempool, no validation involved. */
    CTransactionRef AddMempoolTx()
    {
        CMutableTransaction mtx;
        mtx.vin.resize(1);
        // A distinct input per transaction, otherwise they all conflict and only one is added.
        mtx.vin[0].prevout = COutPoint{Txid::FromUint256(m_rng.rand256()), 0};
        mtx.vout.resize(1);
        mtx.vout[0].nValue = 1000;
        const auto tx{MakeTransactionRef(mtx)};
        TryAddToMempool(*m_node.mempool, TestMemPoolEntryHelper{}.FromTx(tx));
        return tx;
    }

    /** Announce everything queued, draining the rate limiter by advancing the clock. */
    void DrainAnnouncements(CNode& node, FakeNodeClock& clock) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        // Announcements wait on a randomized trickle deadline, so step the clock until both the
        // global backlog and the peer's queue drain. This should be drained well within 10 iterations.
        // Giving it a 6x marging to prevent flakiness, since the trickle deadline is randomized.
        CNodeStateStats stats;
        PeerManagerInfo info;
        for (int i = 0; i < 60; ++i) {
            clock += 1s;
            m_node.peerman->SendMessages(node);
            BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(node.GetId(), stats));
            info = m_node.peerman->GetInfo();
            if (stats.m_inv_to_send == 0 && info.inbound_bucket.backlog_count == 0) return;
        }
        BOOST_FAIL("announcements did not drain: inv_to_send=" << stats.m_inv_to_send
                   << " backlog=" << info.inbound_bucket.backlog_count);
    }

    static constexpr uint64_t REMOTE_SALT_RELAY{1};
};

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

constexpr uint64_t REMOTE_SALT = 0;

// A pair of wtxids whose short ids collide on a link salted with (2, 1). Precomputed under
// BIP-330's derivation, 1 + (s mod 0xFFFFFFFF); regenerate if that ever changes.
const Wtxid COLLIDING_WTXID{Wtxid::FromUint256(uint256("fdfb332e93ecd3b28835f51f9d999278dba2c10fb45611229a17136a00a4d3c9"))};
const Wtxid COLLIDING_WTXID_2{Wtxid::FromUint256(uint256("8f16f117d30b68c6fcdd1ee033d2b4f8aba719088e5aff78c8f9536aeb1388b4"))};

// Build a TxReconciliationState for a given pair of salts
TxReconciliationState StateForSalts(uint64_t local_salt, uint64_t remote_salt)
{
    const uint256 full_salt{ComputeSalt(local_salt, remote_salt)};
    return TxReconciliationState{/*we_initiate=*/false, full_salt.GetUint64(0), full_salt.GetUint64(1)};
}

// Lambda to add random wtxids to a peer's reconciliation set
std::vector<Wtxid> AddTxsToReconSet(TxReconciliationTracker& tracker, NodeId peer_id, int n_txs_to_add)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    auto n_added_txs{0};

    std::vector<Wtxid> added_txs;
    while(n_added_txs < n_txs_to_add) {
        auto wtxid = Wtxid::FromUint256(frc.rand256());
        if (!tracker.AddToSet(peer_id, wtxid).has_value()) {
            added_txs.push_back(wtxid);
            ++n_added_txs;
        }
    }

    return added_txs;
}

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0);

    // Invalid version.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(/*peer_id=*/0, /*is_peer_inbound=*/true,
                                           /*peer_recon_version=*/0, REMOTE_SALT).value(),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // Valid registration (inbound and outbound peers).
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.RegisterPeer(0, true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(!tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(1));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(2));
    tracker.PreRegisterPeer(2);
    BOOST_REQUIRE(!tracker.RegisterPeer(2, true, TXRECONCILIATION_VERSION+1, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(2));

    // Try registering for the second time.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, REMOTE_SALT).value(), ReconciliationError::ALREADY_REGISTERED);

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(100, true, TXRECONCILIATION_VERSION, REMOTE_SALT).value(), ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(100));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Non-registered of simply pre-registered peers not count a registered.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

    // Forgotten peers do not count either.
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerNextToReconcileWithTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // If the peer is not fully registered, the method will return false, doesn't matter the current time
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, GetTime<std::chrono::microseconds>()));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, GetTime<std::chrono::microseconds>()));

    // When the first peer is added to the reconciliation tracker, a full RECON_REQUEST_INTERVAL
    // is given to let transaction pile up, otherwise the node will request an empty reconciliation
    // right away
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    auto current_time = GetTime<std::chrono::microseconds>() + RECON_REQUEST_INTERVAL - 1s;

    // Not enough time passed.
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, current_time));
    // Enough time passed.
    current_time += 1s;
    BOOST_REQUIRE(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));
    // Enough time passed for another round, but the previous reconciliation is still pending.
    // With only one node in the queue, they will still be next.
    current_time += RECON_REQUEST_INTERVAL;
    BOOST_REQUIRE(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Two-peer setup
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));

    // No outbound peers left, so the queue is empty and nobody can be next. Inbound peers are
    // never queued, since we don't initiate reconciliation with them.
    NodeId peer_id_inbound = 3;
    tracker.PreRegisterPeer(peer_id_inbound);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id_inbound, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id_inbound, current_time + RECON_REQUEST_INTERVAL));

    NodeId peer_id1 = 1;
    NodeId peer_id2 = 2;

    // Partially register, check how they are not flagged as next
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());

    // Only one peer can be flagged as next at a time (in insertion order)
    current_time += RECON_REQUEST_INTERVAL;
    bool peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    bool peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    BOOST_REQUIRE(peer1_next && !peer2_next);

    // The frequency depends on the number of peers on re-sampling, we have two
    // so each one is picked every RECON_REQUEST_INTERVAL/2
    current_time += RECON_REQUEST_INTERVAL/2;
    peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    BOOST_REQUIRE(!peer1_next && peer2_next);

    current_time += RECON_REQUEST_INTERVAL/2;
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    BOOST_REQUIRE(peer1_next && !peer2_next);

    // If the peer has pending reconciliation, it doesn't affect the global timer.
    // Here it's peer2's turn, but we are currently reconciling with him, so his turn
    // is immediately passed to the next peer (peer1).
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id2)));
    current_time += RECON_REQUEST_INTERVAL/2;
    bool peer2_skipped = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    BOOST_REQUIRE(peer2_skipped && peer1_next && !peer2_next);

    // A removed peer cannot be next
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id2));
    current_time += RECON_REQUEST_INTERVAL/2;
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    // After removing and resampling, the frequency changes given we have less peers now
    current_time += RECON_REQUEST_INTERVAL;
    bool peer1_next_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    BOOST_REQUIRE(peer1_next && peer1_next_next);

    // It doesn't matter for how long we keep checking
    bool peer2_next_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    bool peer2_next_next_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time + RECON_REQUEST_INTERVAL);
    BOOST_REQUIRE(!peer2_next_next && !peer2_next_next_next);
}

BOOST_AUTO_TEST_CASE(ForgetPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Removing peer after pre-registering works and does not let to register the peer.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).value(), ReconciliationError::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    // Removing a non-existing peer does nothing
    NodeId not_a_peer_id = 1;
    BOOST_REQUIRE(!tracker.IsPeerRegistered(not_a_peer_id));
    BOOST_REQUIRE(!tracker.ForgetPeer(not_a_peer_id));
}

BOOST_AUTO_TEST_CASE(AddToSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // If the peer is not registered, adding to the set fails
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    auto error = tracker.AddToSet(peer_id0, wtxid).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    // This holds even if the peer is just pre-registered
    tracker.PreRegisterPeer(peer_id0);
    error = tracker.AddToSet(peer_id0, wtxid).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    // As long as the peer is registered, adding a new wtxid to the set should work
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());

    // If the peer is dropped, adding wtxids to its set should fail
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    Wtxid wtxid2{Wtxid::FromUint256(frc.rand256())};
    error = tracker.AddToSet(peer_id0, wtxid2).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id1));

    // As long as the peer is registered, the transaction is not in the set, and there is no short id
    // collision, adding should work
    size_t added_txs = 0;
    while (added_txs < MAX_RECONSET_SIZE) {
        wtxid = Wtxid::FromUint256(frc.rand256());
        auto r = tracker.AddToSet(peer_id1, wtxid);
        if (!r.has_value()) {
            ++added_txs;
        } else {
            BOOST_REQUIRE_EQUAL(r.value().m_error, ReconciliationError::SHORTID_COLLISION);
            BOOST_REQUIRE(r.value().m_collision.has_value());
        }
    }

    // Adding one item over the limit should fail
    error = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256())).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::FULL_RECON_SET);
    BOOST_REQUIRE(!error.m_collision.has_value());

    // Trying to add the same item twice will just bypass
    BOOST_REQUIRE(!tracker.AddToSet(peer_id1, wtxid).has_value());
}

BOOST_AUTO_TEST_CASE(AddToSetCollisionTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    uint64_t precomputed_remote_salt = 1;

    // Precompute collision
    const Wtxid wtxid{COLLIDING_WTXID};
    const Wtxid collision{COLLIDING_WTXID_2};

    // Register the peer with a predefined salt so we can force the collision
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, precomputed_remote_salt).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

    // Once the peer is registered, we can try to add both transactions and check
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    auto error = tracker.AddToSet(peer_id0, collision);
    BOOST_REQUIRE_EQUAL(error.value().m_error, ReconciliationError::SHORTID_COLLISION);
    BOOST_REQUIRE_EQUAL(error.value().GetCollision(), wtxid);
}

BOOST_AUTO_TEST_CASE(TryRemovingFromSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    const Wtxid wtxid{COLLIDING_WTXID};
    const Wtxid collision{COLLIDING_WTXID_2};

    // If the peer is not registered, removing will fail
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // This holds even if the peer is just pre-registered (register specific salt so we can also check collisions)
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // If the peer is registered but the transaction is not part of the set, this will fail too
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Only if the transaction is found the method will return true
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    // Removing twice won't work
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Adding a transaction but removing a collision won't work
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, collision));
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));

    // And removing after forgetting the peer won't work either
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Failing to remove must leave the short id mapping untouched. Dropping the entry of a
    // transaction that is still in the set would make a later colliding transaction go undetected
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeerWithSalt(peer_id1, 2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, /*remote_salt=*/1).has_value());
    BOOST_REQUIRE(!tracker.AddToSet(peer_id1, wtxid).has_value());
    // collision was never added, so there is nothing to remove.
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id1, collision));
    // wtxid is still in the set, so its short id is still mapped and the collision is caught.
    auto collision_error = tracker.AddToSet(peer_id1, collision);
    BOOST_REQUIRE_EQUAL(collision_error.value().m_error, ReconciliationError::SHORTID_COLLISION);
    BOOST_REQUIRE_EQUAL(collision_error.value().GetCollision(), wtxid);
}

BOOST_AUTO_TEST_CASE(RemoveFromSetsTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0, peer_id1 = 1, peer_id2 = 2;
    FastRandomContext frc{/*fDeterministic=*/true};
    const Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // Two registered peers, both holding the same transaction
    for (auto peer_id : {peer_id0, peer_id1}) {
        tracker.PreRegisterPeer(peer_id);
        BOOST_REQUIRE(!tracker.RegisterPeer(peer_id, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
        BOOST_REQUIRE(!tracker.AddToSet(peer_id, wtxid).has_value());
    }

    // Removal reaches every peer's set, not just one
    tracker.RemoveFromSets({wtxid});
    BOOST_CHECK(!tracker.TryRemovingFromSet(peer_id0, wtxid));
    BOOST_CHECK(!tracker.TryRemovingFromSet(peer_id1, wtxid));

    // A transaction we already sketched stays in the snapshot, so that a sketch extension still
    // describes the same elements, but it must no longer be announced
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    const auto sketched_txs{AddTxsToReconSet(tracker, peer_id2, 5)};
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id2, /*peer_recon_set_size=*/5, Q).has_value());
    std::vector<uint8_t> skdata{};
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id2, skdata));
    BOOST_REQUIRE(!skdata.empty());

    tracker.RemoveFromSets({sketched_txs[0]});
    const std::vector<uint32_t> none{};
    const auto announced{std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id2, /*recon_result=*/false, none))};
    BOOST_CHECK_EQUAL(announced.size(), sketched_txs.size() - 1);
    BOOST_CHECK(std::ranges::find(announced, sketched_txs[0]) == announced.end());
}

// A transaction leaving the mempool should be removed from the reconciliation sets, but it should not affect the extensions.
BOOST_AUTO_TEST_CASE(RemoveFromSetsKeepsExtensionConsistentTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    const std::vector<uint32_t> none{};
    std::vector<uint8_t> extensions[2];

    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());

    // Two rounds over the same transactions. One of the rounds triggers a removal after the snapshop is created,
    // while the first does not.
    for (int round = 0; round < 2; ++round) {
        const auto txs{AddTxsToReconSet(tracker, peer_id0, 5)};
        BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size=*/5, Q).has_value());
        std::vector<uint8_t> base_skdata{};
        BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, base_skdata));
        BOOST_REQUIRE(!base_skdata.empty());

        // The node should have snapsotted the set. Trigger removal for one of the iters.
        if (round == 1) tracker.RemoveFromSets({txs[0]});

        BOOST_REQUIRE(!tracker.HandleExtensionRequest(peer_id0).has_value());
        BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, extensions[round]));
        BOOST_REQUIRE(!extensions[round].empty());

        // Finish the round so the next one starts from a clean state
        BOOST_REQUIRE(std::holds_alternative<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, none)));
    }

    // Removing after the snapshop should have no effect on the extension.
    BOOST_CHECK(extensions[1] == extensions[0]);
}

// Once a peer's reconciliation set is full, further transactions have to be fanned out instead.
BOOST_FIXTURE_TEST_CASE(AnnounceTxsFullReconciliationSet, TxReconciliationRelaySetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    FakeNodeClock clock{};
    CNode& recon{AddReconciliationPeer(0)};
    BOOST_REQUIRE(!recon.fDisconnect);
    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);

    {
        ASSERT_DEBUG_LOG("to the reconciliation set");
        for (uint32_t i = 0; i < MAX_RECONSET_SIZE; ++i) {
            m_node.peerman->InitiateTxBroadcastToAll(AddMempoolTx()->GetWitnessHash());
        }
        DrainAnnouncements(recon, clock);
        BOOST_REQUIRE(!recon.fDisconnect);
    }

    // All of them were reconciled, so nothing should have been announced.
    connman.FlushSendBuffer(recon);
    BOOST_CHECK_EQUAL(PendingMsgType(recon), "");

    // The next one no longer fits in the set, so it must be fanned out.
    {
        ASSERT_DEBUG_LOG("Reconciliation set maximum size reached");
        m_node.peerman->InitiateTxBroadcastToAll(AddMempoolTx()->GetWitnessHash());
        DrainAnnouncements(recon, clock);
    }
    BOOST_CHECK_EQUAL(PendingMsgType(recon), "inv");
}

BOOST_AUTO_TEST_CASE(InitiateReconciliationRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // A reconciliation request cannot be initiated with a non-fully registered peer
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::NOT_FOUND);

    // For a registered peer with no pending transactions, we expect the set size to be zero, and the
    // reconciliation params to match the default
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    auto [local_set_size, local_q_formatted] = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_set_size, 0);
    BOOST_REQUIRE_EQUAL(local_q_formatted, static_cast<uint16_t>(std::ceil(Q * Q_PRECISION)));

    // We only initiate reconciliation with outbound peers. For inbounds, we expect them to initiate
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id1)), ReconciliationError::WRONG_ROLE);

    // Start fresh
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());

    // After adding some transactions to a registered peer we expect its set to contain that amount of transactions
    int n_added_txs = 0;
    int n_target_txs = frc.randrange(42) + 1;
    while (n_added_txs < n_target_txs) {
        auto wtxid = Wtxid::FromUint256(frc.rand256());
        if (!tracker.AddToSet(peer_id0, wtxid).has_value()) ++n_added_txs;
    }
    std::tie(local_set_size, local_q_formatted) = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_set_size, n_added_txs);
    BOOST_REQUIRE_EQUAL(local_q_formatted, static_cast<uint16_t>(std::ceil(Q * Q_PRECISION)));

    // Once we are reconciling with a peer, a new reconciliation cannot be initiated until the previous is completed
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::WRONG_PHASE);
}

BOOST_AUTO_TEST_CASE(InitiateReconciliationRequestQCeilTest)
{
    // BIP-330 requires the formatted q to be rounded up when narrowed from a
    // floating-point coefficient to the wire format. Using Q = 0.25 and
    // Q_PRECISION = 32767, the exact product is 8191.75 so we should expect 8192.
    static_assert(Q == 0.25);
    static_assert(Q_PRECISION == 32767);

    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    auto [local_set_size, local_q_formatted] = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_q_formatted, 8192);
}

BOOST_AUTO_TEST_CASE(HandleReconciliationRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // A reconciliation request cannot be initiated with a non-fully registered peer
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id0, 0, Q).value(), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id0, 0, Q).value(), ReconciliationError::NOT_FOUND);

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, 0, Q).has_value());

    // If a reconciliation flow is ongoing for a given peer, we won't start another with him.
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id0, 0, Q).value(), ReconciliationError::WRONG_PHASE);

    // Other peers can request reconciliation as long as we are not currently reconciling with them.
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id1, 0, Q).has_value());
    // Already reconciling with the peer
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id1, 0, Q).value(), ReconciliationError::WRONG_PHASE);

    // We only respond to reconciliation requests if they are the initiator (peer is inbound)
    NodeId peer_id2 = 2;
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id2, 0, Q).value(), ReconciliationError::WRONG_ROLE);

    // A BIP-330 compliant peer rounds q up when narrowing it to the wire format, so the largest value it can hold its Q_PRECISION.
    // Check the boundary.
    NodeId peer_id3 = 3;
    tracker.PreRegisterPeer(peer_id3);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id3, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id3, /*peer_recon_set_size=*/0, /*peer_q=*/Q_PRECISION).has_value());

    NodeId peer_id4 = 4;
    tracker.PreRegisterPeer(peer_id4);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id4, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id4, /*peer_recon_set_size=*/0,
                                                           /*peer_q=*/static_cast<uint16_t>(Q_PRECISION + 1)).value(),
                        ReconciliationError::PROTOCOL_VIOLATION);
}

// A peer cannot make us answer with a bigger sketch than our own set limit justifies by claiming an
// inflated set size: the claimed size is clamped to MAX_RECONSET_SIZE before it reaches the estimate.
BOOST_AUTO_TEST_CASE(HandleReconciliationRequestClampsSetSizeTest)
{
    // Returns the capacity of the sketch we answer a request of the given claimed set size with.
    const auto responded_capacity = [](uint16_t claimed_set_size) {
        TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
        NodeId peer_id0 = 0;
        tracker.PreRegisterPeer(peer_id0);
        BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
        AddTxsToReconSet(tracker, peer_id0, 10);
        BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, claimed_set_size, Q).has_value());
        std::vector<uint8_t> skdata{};
        BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
        return skdata.size() / BYTES_PER_SKETCH_CAPACITY;
    };

    // Claiming far more than we would ever hold is answered exactly as if the peer had claimed our
    // own limit, rather than driving the estimate up to MAX_SKETCH_CAPACITY.
    const auto at_limit{responded_capacity(static_cast<uint16_t>(MAX_RECONSET_SIZE))};
    BOOST_CHECK_EQUAL(responded_capacity(65535), at_limit);
    BOOST_CHECK_LT(at_limit, MAX_SKETCH_CAPACITY);
}

BOOST_AUTO_TEST_CASE(ShouldRespondToReconciliationRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};
    std::vector<uint8_t> skdata{};

    // We cannot respond to partially registered peers
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());

    // We only respond to reconciliation requests after receiving one
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());

    // The sketch data depends on the transactions we had on the peer's local set
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(skdata.empty());

    // After responding, the reconciliation phase have moved from INIT_REQUESTED to INIT_RESPONDED, so we cannot respond again
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));

    // Reset the peer
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/0, Q).has_value());

    // If the received request had size zero (peer_recon_set_size == 0), we will respond but shortcut, since reconciling is pointless
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(skdata.empty());

    // Reset the peer
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());

    // If the peer has data to reconcile, adding some transactions to the peers local set will affect the serialized
    // sketch data we receive (which we're supposed to send to our peer)
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(!skdata.empty());

    // We only respond to reconciliation requests if they are the initiator (peer is inbound)
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id1, skdata));

    // Test extension requests
    NodeId peer_id2 = 2;
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    // There needs to be data in both sets for the phase to move towards extension, otherwise we'll shortcut
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id2, /*peer_recon_set_size*/10, Q).has_value());
    tracker.AddToSet(peer_id2, Wtxid::FromUint256(frc.rand256()));

    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id2, skdata));
    BOOST_REQUIRE(!tracker.HandleExtensionRequest(peer_id2).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id2, skdata));
    // Once the phase has advanced, we won't respond again
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id2, skdata));
}

BOOST_AUTO_TEST_CASE(HandleSketchBasicFlowTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    std::vector<uint8_t> skdata{};

    // We cannot respond to partially registered peers
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::NOT_FOUND);

    // Only reply if we have initiated a request
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::WRONG_PHASE);
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0)));
    BOOST_REQUIRE(std::holds_alternative<HandleSketchResult>(tracker.HandleSketch(peer_id0, skdata)));

    // Only reply if we are the initiator (peer is outbound)
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::WRONG_ROLE);

    // We cannot reply to a forgotten peer
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::NOT_FOUND);
}

BOOST_AUTO_TEST_CASE(HandleSketchTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // Since we do not have access to the TxReconciliationState from outside TxReconciliationTracker we'll
    // need to register the peer with a pre-defined salt, so we can compute the same (salted) short_ids
    uint64_t local_salt = 1;
    uint64_t remote_salt = 2;
    const TxReconciliationState state{StateForSalts(local_salt, remote_salt)};

    // Setup a peer we have initiated a reconciliation with
    tracker.PreRegisterPeerWithSalt(peer_id0, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, remote_salt).has_value());
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0)));

    // If we have nothing to reconcile with the peer, shortcut and send all transactions.
    // This will trigger the peer sending all their pending transactions to us
    std::vector<uint8_t> skdata(BYTES_PER_SKETCH_CAPACITY, 0);
    auto result = std::get<HandleSketchResult>(tracker.HandleSketch(peer_id0, skdata));
    BOOST_REQUIRE_EQUAL(result.m_succeeded, std::optional(false));
    BOOST_REQUIRE(result.m_txs_to_announce.empty());
    BOOST_REQUIRE(result.m_txs_to_request.empty());

    // If their sketch is empty, reconciliation shortcuts and we announce all pending transactions
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0)));
    skdata.clear();
    auto n_txs_to_add = frc.randrange(42) + 1;
    std::vector<Wtxid> added_txs = AddTxsToReconSet(tracker, peer_id0, n_txs_to_add);

    result = std::get<HandleSketchResult>(tracker.HandleSketch(peer_id0, skdata));
    BOOST_REQUIRE_EQUAL(result.m_succeeded, std::optional(false));
    BOOST_REQUIRE(std::is_permutation(result.m_txs_to_announce.begin(), result.m_txs_to_announce.end(), added_txs.begin(), added_txs.end()));
    BOOST_REQUIRE(result.m_txs_to_request.empty());
     // After a successful reconciliation, the sets are emptied
    added_txs = AddTxsToReconSet(tracker, peer_id0, n_txs_to_add);

    // After successfully handling a Sketch the peer's phase is reset to NONE (even if we shortcut), so we won't handle another one
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::WRONG_PHASE);

    // If the peer provided a non-empty sketch, its size need to be valid (multiple of the element size and within bounds)
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0))); // Re set the peer's phase

    // Not multiple of the element size
    skdata.push_back(0);
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::PROTOCOL_VIOLATION);

    // Over the limit
    skdata.resize((MAX_SKETCH_CAPACITY + 1) * BYTES_PER_SKETCH_CAPACITY, 0);
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, skdata)), ReconciliationError::PROTOCOL_VIOLATION);

    // Exactly at the limit is still valid, so it must be handled rather than rejected. Use a
    // separate peer so peer_id0's reconciliation state is left untouched.
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeerWithSalt(peer_id1, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, remote_salt).has_value());
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id1)));
    std::vector<uint8_t> at_limit(MAX_SKETCH_CAPACITY * BYTES_PER_SKETCH_CAPACITY, 0);
    BOOST_REQUIRE(std::holds_alternative<HandleSketchResult>(tracker.HandleSketch(peer_id1, at_limit)));

    // If the peer sketch has data and we also have data for the peer, we reconcile normally:
    // Create a valid sketch with part of the data we have for the peer. Give enough capacity for
    // up to `n_txs_to_add` to differ.
    std::vector<Wtxid> expected_txs_to_announce{};
    std::vector<uint32_t> expected_txs_to_request{};
    std::set<uint32_t> added_shortids{};

    Minisketch remote_sketch = node::MakeMinisketch32(n_txs_to_add);
    // Add a few transaction we already know to the peer's sketch
    for (size_t i=0; i < added_txs.size(); i++) {
        if (i % 2 == 0) {
            auto short_id = state.ComputeShortID(added_txs[i]);
            // Make sure there are no collisions
            if (added_shortids.insert(short_id).second) {
                remote_sketch.Add(short_id);
            }
        } else {
            expected_txs_to_announce.push_back(added_txs[i]);
        }
    }
    // Also add a few that we don't know
    for (size_t i=0; i < added_txs.size() / 2; i++) {
        auto short_id = state.ComputeShortID(Wtxid::FromUint256(frc.rand256()));
        // Make sure there are no collisions
        if (!added_shortids.contains(short_id)) {
            remote_sketch.Add(short_id);
            expected_txs_to_request.push_back(short_id);
        }
    }

    result = std::get<HandleSketchResult>(tracker.HandleSketch(peer_id0, remote_sketch.Serialize()));
    BOOST_REQUIRE(result.m_succeeded == std::optional(true));
    BOOST_REQUIRE(std::is_permutation(result.m_txs_to_request.begin(), result.m_txs_to_request.end(), expected_txs_to_request.begin(), expected_txs_to_request.end()));
    BOOST_REQUIRE(std::is_permutation(result.m_txs_to_announce.begin(), result.m_txs_to_announce.end(), expected_txs_to_announce.begin(), expected_txs_to_announce.end()));

    // The phase has also reset when succeeding and not short-cutting
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleSketch(peer_id0, remote_sketch.Serialize())), ReconciliationError::WRONG_PHASE);

    // If the difference in sketches is over the sketch capacity, reconciliation will fail and the node will prepare for an extension
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0))); // Re set the peer's phase
    added_txs = AddTxsToReconSet(tracker, peer_id0, n_txs_to_add); // Sets are cleared after reconciliation, so add more txs
    // Adding a single more transaction will trigger an extension
    remote_sketch.Add(frc.rand32());
    result = std::get<HandleSketchResult>(tracker.HandleSketch(peer_id0, remote_sketch.Serialize()));
    BOOST_REQUIRE(!result.m_succeeded.has_value());
    BOOST_REQUIRE(result.m_txs_to_announce.empty());
    BOOST_REQUIRE(result.m_txs_to_request.empty());
    // Trying to initiate again will fail because we are waiting for an extension
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::WRONG_PHASE);
}

BOOST_AUTO_TEST_CASE(HandleExtensionRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    std::vector<uint8_t> skdata{};

    // An extension request cannot be initiated with a non-fully registered peer
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::NOT_FOUND);

    // If the peer is registered but the reconciliation flow has not reached the extension phase we cannot reply to an extension
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::WRONG_PHASE);

    // Extensions are not allowed if the peer didn't have anything to send us
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, 0, Q).has_value());
    std::vector<Wtxid> added_txs = AddTxsToReconSet(tracker, peer_id0, 10);
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::PROTOCOL_VIOLATION);
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));

    // Nor if we didn't have anything to send them
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, 10, Q).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::PROTOCOL_VIOLATION);
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));

    // If there is data for both, and we are in the right phase, extension should be allowed
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(skdata.empty());
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, 10, Q).has_value());
    added_txs = AddTxsToReconSet(tracker, peer_id0, 10);
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(!skdata.empty());
    BOOST_REQUIRE(!tracker.HandleExtensionRequest(peer_id0).has_value());

    // Once the request is dealt with, the phase moves to EXT_REQUESTED, an a subsequent requests are not allowed
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::WRONG_PHASE);

    // We only handle extension requests if they are the initiator
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(tracker.HandleExtensionRequest(peer_id0).value(), ReconciliationError::WRONG_ROLE);
}

// Test that a peer can only ask for short ids it learned from the sketch we sent
BOOST_AUTO_TEST_CASE(HandleReconcilDiffBoundsShortIDsTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    const uint64_t local_salt{2}, remote_salt{1};
    const TxReconciliationState state{StateForSalts(local_salt, remote_salt)};

    // Get the peer to the phase where it may finalize a round
    tracker.PreRegisterPeerWithSalt(peer_id0, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, remote_salt).has_value());
    const auto added_txs{AddTxsToReconSet(tracker, peer_id0, 10)};
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size=*/10, Q).has_value());
    std::vector<uint8_t> skdata{};
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));

    // The sketch we just sent bounds what the peer can possibly have decoded
    const size_t sketched_capacity{skdata.size() / BYTES_PER_SKETCH_CAPACITY};
    BOOST_REQUIRE(sketched_capacity > 1);

    // Asking for more short ids than that sketch could have yielded is a protocol violation
    const std::vector<uint32_t> too_many(sketched_capacity + 1, state.ComputeShortID(added_txs[0]));
    BOOST_CHECK_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/true, too_many)),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // Within the bound, a repeated short id is only announced once
    const std::vector<uint32_t> repeated(sketched_capacity, state.ComputeShortID(added_txs[0]));
    const auto result{std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/true, repeated))};
    BOOST_CHECK_EQUAL(result.size(), 1U);
    BOOST_CHECK_EQUAL(result[0], added_txs[0]);

    // A peer that reported an empty set is sent no sketch at all, so it cannot have decoded any
    // short id. Asking for one is a protocol violation.
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeerWithSalt(peer_id1, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, remote_salt).has_value());
    const auto peer1_txs{AddTxsToReconSet(tracker, peer_id1, 10)};
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id1, /*peer_recon_set_size=*/0, Q).has_value());
    std::vector<uint8_t> empty_skdata{};
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id1, empty_skdata));
    // Nothing was sketched, so nothing could be decoded
    BOOST_REQUIRE(empty_skdata.empty());
    const std::vector<uint32_t> asked{state.ComputeShortID(peer1_txs[0])};
    BOOST_CHECK_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id1, /*recon_result=*/true, asked)),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // After an extension we sent a sketch of twice the original capacity, so the peer may ask for
    // twice as many short ids
    NodeId peer_id2 = 2;
    tracker.PreRegisterPeerWithSalt(peer_id2, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, remote_salt).has_value());
    const auto peer2_txs{AddTxsToReconSet(tracker, peer_id2, 10)};
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id2, /*peer_recon_set_size=*/10, Q).has_value());
    std::vector<uint8_t> base_skdata{};
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id2, base_skdata));
    const size_t base_capacity{base_skdata.size() / BYTES_PER_SKETCH_CAPACITY};
    BOOST_REQUIRE(base_capacity > 0);
    BOOST_REQUIRE(!tracker.HandleExtensionRequest(peer_id2).has_value());
    std::vector<uint8_t> ext_skdata{};
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id2, ext_skdata));

    // Beyond the doubled capacity is still a protocol violation
    const std::vector<uint32_t> beyond_doubled(2 * base_capacity + 1, state.ComputeShortID(peer2_txs[0]));
    BOOST_CHECK_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id2, /*recon_result=*/true, beyond_doubled)),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // But more than the original capacity is now allowed
    const std::vector<uint32_t> over_base(base_capacity + 1, state.ComputeShortID(peer2_txs[0]));
    BOOST_CHECK(std::holds_alternative<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id2, /*recon_result=*/true, over_base)));
}

BOOST_AUTO_TEST_CASE(HandleReconcilDiffEmptySketchTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    const uint64_t local_salt{2}, remote_salt{1};

    // A peer reporting an empty set is sent no sketch at all
    tracker.PreRegisterPeerWithSalt(peer_id0, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, remote_salt).has_value());
    const auto added_txs{AddTxsToReconSet(tracker, peer_id0, 10)};
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size=*/0, Q).has_value());
    std::vector<uint8_t> skdata{};
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(skdata.empty());

    // It cannot have decoded anything, so claiming success is a protocol violation
    const std::vector<uint32_t> none{};
    BOOST_CHECK_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/true, none)),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // Terminating with a failure is the expected behaviour, and announces everything we held
    const auto announced{std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, none))};
    BOOST_CHECK_EQUAL(announced.size(), added_txs.size());
}

BOOST_AUTO_TEST_CASE(HandleReconcilDiffBasicFlowTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    std::vector<uint8_t> skdata(BYTES_PER_SKETCH_CAPACITY, 0);
    std::vector<uint32_t> missing_short_ids{};

    // We cannot respond to partially registered peers
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)), ReconciliationError::NOT_FOUND);

    // Only handle diff if we have: received a recon request and already replied with a sketch
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)), ReconciliationError::WRONG_PHASE);
    // Handle a diff from the inital request
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(std::holds_alternative<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)));

    // Same applies to extensions, we only handle a diff after an extension if we are in the proper phase
    // (we made the previous diff fail so we don't need to re-register)
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());
    std::vector<Wtxid> added_txs = AddTxsToReconSet(tracker, peer_id0, 10);
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    // Handle a diff from extension. This requires data to be in both sets during the initial request
    BOOST_REQUIRE(!tracker.HandleExtensionRequest(peer_id0).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(std::holds_alternative<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/true, missing_short_ids)));

    // Only handle diffs if they are the initiator (peer is inbound)
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)), ReconciliationError::WRONG_ROLE);

    // Don't handle a recon diff of a forgotten peer
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)), ReconciliationError::NOT_FOUND);
}

BOOST_AUTO_TEST_CASE(HandleReconcilDiffTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // We need to fix the salts so we can compute the same short IDs as the node will, in order to test the results
    uint64_t local_salt = 1;
    uint64_t remote_salt = 2;
    const TxReconciliationState state{StateForSalts(local_salt, remote_salt)};

    std::vector<uint8_t> skdata(BYTES_PER_SKETCH_CAPACITY, 0);
    std::vector<uint32_t> missing_short_ids{};

    tracker.PreRegisterPeerWithSalt(peer_id0, local_salt);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, remote_salt).has_value());

    // Neither our node nor the peer have anything for each other. In this case HandleReconcilDiff will return nothing
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/0, Q).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    auto result = std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids));
    BOOST_REQUIRE(result.empty());

    // After a successful handling, the phase is reset to NONE, so we won't be allowed to handle twice
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids)), ReconciliationError::WRONG_PHASE);

    // If the peer doesn't have anything but we do, we will reply with everything (shortcut)
    int tx_count = frc.randrange(90) + 10;
    std::vector<Wtxid> added_txs = AddTxsToReconSet(tracker, peer_id0, tx_count);
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/0, Q).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    result = std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/false, missing_short_ids));
    BOOST_REQUIRE_EQUAL(result.size(), tx_count);

    // If reconciliation succeeds and the peer requests some transactions by short ID, we will reply with their corresponding wtxids
    added_txs = AddTxsToReconSet(tracker, peer_id0, tx_count);

    std::vector<Wtxid> peer_missing_wtxid{};
    std::vector<uint32_t> peer_missing_short_ids{};
    for (size_t i=0; i < added_txs.size(); i++) {
        if (i % 2 == 0) {
            auto short_id = state.ComputeShortID(added_txs[i]);
            // Make sure there are no collisions
            if (auto it = std::find(peer_missing_short_ids.begin(), peer_missing_short_ids.end(), short_id); it == peer_missing_short_ids.end()) {
                peer_missing_short_ids.push_back(short_id);
                peer_missing_wtxid.push_back(added_txs[i]);
            }
        }
    }

    // Here, recon_result must be true, otherwise we will reply with all known wtxids, as in the previous test.
    // The peer must also report a non-zero set size, otherwise we would send no sketch at all and it
    // could not have learned any short id to ask for (same reasoning as the extension case below).
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/1, Q).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    result = std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/true, peer_missing_short_ids));
    BOOST_REQUIRE(std::is_permutation(result.begin(), result.end(), peer_missing_wtxid.begin(), peer_missing_wtxid.end()));

    // We may also need to handle a reconciliation difference in an extension phase.
    // For this to work, we need both recon_result being true and the peer needs to have reported a non-zero set size,
    // otherwise, we would have never created (and snapshotted) an original sketch, and the extension would be rejected
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());

    // Add a couple of transactions to missing so we have something to compare to
    added_txs = AddTxsToReconSet(tracker, peer_id0, tx_count);
    peer_missing_wtxid.clear();
    peer_missing_short_ids.clear();
    for (size_t i=0; i<5; i++) { // We know tx_count is at least 10, so half should do
        auto short_id = state.ComputeShortID(added_txs[i]);
        // Make sure there are no collisions
        if (auto it = std::find(peer_missing_short_ids.begin(), peer_missing_short_ids.end(), short_id); it == peer_missing_short_ids.end()) {
            peer_missing_short_ids.push_back(short_id);
            peer_missing_wtxid.push_back(added_txs[i]);
        }
    }

    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    BOOST_REQUIRE(!tracker.HandleExtensionRequest(peer_id0).has_value());
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata));
    result = std::get<std::vector<Wtxid>>(tracker.HandleReconcilDiff(peer_id0, /*recon_result=*/true, peer_missing_short_ids));
    BOOST_REQUIRE(std::is_permutation(result.begin(), result.end(), peer_missing_wtxid.begin(), peer_missing_wtxid.end()));
}
BOOST_AUTO_TEST_SUITE_END()
