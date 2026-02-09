// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <blockencodings.h>
#include <chain.h>
#include <node/blockdownloadman.h>
#include <node/blockdownloadman_impl.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

using node::BlockDownloadConnectionInfo;
using node::BlockDownloadManager;
using node::BlockDownloadOptions;

BOOST_FIXTURE_TEST_SUITE(blockdownload_tests, TestingSetup)

// Test the full peer lifecycle: registration, sync state, blocks in flight,
// and disconnection with correct counter maintenance. Also covers sync-started
// idempotency and explicit unsetting.
BOOST_AUTO_TEST_CASE(peer_lifecycle)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    const NodeId peer1{0}, peer2{1};
    BlockDownloadConnectionInfo info1{/*m_is_inbound=*/false, /*m_preferred_download=*/true,
                                      /*m_can_serve_witnesses=*/true,
                                      /*m_is_limited_peer=*/false};
    BlockDownloadConnectionInfo info2{/*m_is_inbound=*/true, /*m_preferred_download=*/false,
                                      /*m_can_serve_witnesses=*/true,
                                      /*m_is_limited_peer=*/false};

    // Register peers
    bdm.ConnectedPeer(peer1, info1);
    bdm.ConnectedPeer(peer2, info2);

    // Check initial state — peer1 is preferred, peer2 is not
    BOOST_CHECK_EQUAL(bdm.GetNumSyncStarted(), 0);
    BOOST_CHECK_EQUAL(bdm.GetNumPreferredDownload(), 1);
    BOOST_CHECK_EQUAL(bdm.GetPeersDownloadingFrom(), 0);
    BOOST_CHECK(!bdm.HasBlocksInFlight());

    // Set sync started for both peers
    bdm.SetSyncStarted(peer1, true);
    bdm.SetSyncStarted(peer2, true);
    BOOST_CHECK_EQUAL(bdm.GetNumSyncStarted(), 2);
    BOOST_CHECK(bdm.GetSyncStarted(peer1));
    BOOST_CHECK(bdm.GetSyncStarted(peer2));

    // Setting same value again should not change count (idempotency)
    bdm.SetSyncStarted(peer1, true);
    BOOST_CHECK_EQUAL(bdm.GetNumSyncStarted(), 2);

    // Explicitly unset sync for peer2
    bdm.SetSyncStarted(peer2, false);
    BOOST_CHECK_EQUAL(bdm.GetNumSyncStarted(), 1);
    BOOST_CHECK(!bdm.GetSyncStarted(peer2));

    // Request a block from peer1 — should update in-flight counters
    const CBlockIndex* genesis = m_node.chainman->ActiveChain().Genesis();
    BOOST_REQUIRE(genesis != nullptr);
    bdm.BlockRequested(peer1, *genesis);
    BOOST_CHECK(bdm.HasBlocksInFlight());
    BOOST_CHECK_EQUAL(bdm.GetPeersDownloadingFrom(), 1);
    BOOST_CHECK(bdm.IsBlockRequested(genesis->GetBlockHash()));

    // Disconnect peer1 — should clean up sync, preferred, and in-flight state
    bdm.DisconnectedPeer(peer1);
    BOOST_CHECK_EQUAL(bdm.GetNumSyncStarted(), 0);
    BOOST_CHECK_EQUAL(bdm.GetNumPreferredDownload(), 0);
    BOOST_CHECK_EQUAL(bdm.GetPeersDownloadingFrom(), 0);
    BOOST_CHECK(!bdm.HasBlocksInFlight());

    // Disconnect peer2
    bdm.DisconnectedPeer(peer2);
    bdm.CheckIsEmpty();
}

// Test that block source attribution (which peer sent a block) can be
// set, queried, and erased, and that unrelated hashes are not affected.
BOOST_AUTO_TEST_CASE(block_source_tracking)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    uint256 hash1 = uint256::ONE;
    uint256 hash2{ArithToUint256(arith_uint256{2})};
    NodeId peer1{0};

    // Initially no source
    BOOST_CHECK(!bdm.GetBlockSource(hash1).has_value());

    // Set source
    bdm.SetBlockSource(hash1, peer1, true);
    auto source = bdm.GetBlockSource(hash1);
    BOOST_CHECK(source.has_value());
    BOOST_CHECK_EQUAL(source->first, peer1);
    BOOST_CHECK_EQUAL(source->second, true);

    // No source for different hash
    BOOST_CHECK(!bdm.GetBlockSource(hash2).has_value());

    // Erase source
    bdm.EraseBlockSource(hash1);
    BOOST_CHECK(!bdm.GetBlockSource(hash1).has_value());
}

// Test last tip update get/set and TipMayBeStale logic: lazy initialization,
// recent vs old tip, and staleness threshold.
BOOST_AUTO_TEST_CASE(stalling_and_tip_staleness)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    int64_t pow_spacing = 600; // 10 minutes
    auto now = GetTime<std::chrono::seconds>();

    // Last tip update: initially zero, then set/read back
    BOOST_CHECK(bdm.GetLastTipUpdate() == std::chrono::seconds{0});

    // First call with last_tip_update at 0 initializes it to now and returns false
    BOOST_CHECK(!bdm.TipMayBeStale(now, pow_spacing));

    // Set a recent tip update — not stale
    bdm.SetLastTipUpdate(now);
    BOOST_CHECK(bdm.GetLastTipUpdate() == now);
    BOOST_CHECK(!bdm.TipMayBeStale(now, pow_spacing));

    // Set an old tip update — stale (no blocks in flight)
    bdm.SetLastTipUpdate(now - std::chrono::seconds{pow_spacing * 4});
    BOOST_CHECK(bdm.TipMayBeStale(now, pow_spacing));
}

// Test basic block request flow: requesting a block marks it in-flight,
// updates counters, detects outbound requests, rejects duplicate requests
// from the same peer, and cleans up on removal.
BOOST_AUTO_TEST_CASE(block_requested_basic)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    NodeId peer1{0};
    bdm.ConnectedPeer(peer1, {/*m_is_inbound=*/false, /*m_preferred_download=*/true,
                               /*m_can_serve_witnesses=*/true,
                               /*m_is_limited_peer=*/false});

    // Use the genesis block as our test block
    const CBlockIndex* genesis = m_node.chainman->ActiveChain().Genesis();
    BOOST_REQUIRE(genesis != nullptr);

    // Initially not requested
    BOOST_CHECK(!bdm.IsBlockRequested(genesis->GetBlockHash()));

    // Request it
    bool first = bdm.BlockRequested(peer1, *genesis);
    BOOST_CHECK(first);
    BOOST_CHECK(bdm.IsBlockRequested(genesis->GetBlockHash()));
    // peer1 is NOT inbound (m_is_inbound=false), so it IS outbound
    BOOST_CHECK(bdm.IsBlockRequestedFromOutbound(genesis->GetBlockHash()));
    BOOST_CHECK_EQUAL(bdm.GetPeersDownloadingFrom(), 1);
    BOOST_CHECK_EQUAL(bdm.CountBlocksInFlight(genesis->GetBlockHash()), 1u);

    // Request same block from same peer — returns false
    bool second = bdm.BlockRequested(peer1, *genesis);
    BOOST_CHECK(!second);

    // Remove the request
    bdm.RemoveBlockRequest(genesis->GetBlockHash(), std::nullopt);
    BOOST_CHECK(!bdm.IsBlockRequested(genesis->GetBlockHash()));
    BOOST_CHECK_EQUAL(bdm.GetPeersDownloadingFrom(), 0);

    bdm.DisconnectedPeer(peer1);
    bdm.CheckIsEmpty();
}

// Test that RemoveBlockRequest with a specific peer only removes that
// peer's request, leaving requests from other peers intact.
BOOST_AUTO_TEST_CASE(remove_block_request_from_specific_peer)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    NodeId peer1{0}, peer2{1};
    bdm.ConnectedPeer(peer1, {false, false, true, false});
    bdm.ConnectedPeer(peer2, {true, false, true, false});

    const CBlockIndex* genesis = m_node.chainman->ActiveChain().Genesis();

    // Request from peer1
    bdm.BlockRequested(peer1, *genesis);
    BOOST_CHECK(bdm.IsBlockRequested(genesis->GetBlockHash()));

    // Try to remove from peer2 — should not remove
    bdm.RemoveBlockRequest(genesis->GetBlockHash(), peer2);
    BOOST_CHECK(bdm.IsBlockRequested(genesis->GetBlockHash()));

    // Remove from peer1 — should remove
    bdm.RemoveBlockRequest(genesis->GetBlockHash(), peer1);
    BOOST_CHECK(!bdm.IsBlockRequested(genesis->GetBlockHash()));

    bdm.DisconnectedPeer(peer1);
    bdm.DisconnectedPeer(peer2);
    bdm.CheckIsEmpty();
}

// Test FindBlockInFlight: verify all BlockInFlightInfo fields across
// different scenarios — no in-flight blocks, single request without
// partialBlock, and multi-peer requests with partialBlock.
BOOST_AUTO_TEST_CASE(find_block_in_flight)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    NodeId peer1{0}, peer2{1}, peer3{2};
    bdm.ConnectedPeer(peer1, {/*m_is_inbound=*/false, /*m_preferred_download=*/false,
                               /*m_can_serve_witnesses=*/true,
                               /*m_is_limited_peer=*/false});
    bdm.ConnectedPeer(peer2, {true, false, true, false});
    bdm.ConnectedPeer(peer3, {true, false, true, false});

    const CBlockIndex* genesis = m_node.chainman->ActiveChain().Genesis();
    BOOST_REQUIRE(genesis != nullptr);
    const uint256& hash = genesis->GetBlockHash();

    // No block in flight — already_in_flight is 0, first_in_flight is true (vacuous)
    auto info = bdm.FindBlockInFlight(hash, peer1);
    BOOST_CHECK_EQUAL(info.already_in_flight, 0u);
    BOOST_CHECK(info.first_in_flight);
    BOOST_CHECK(!info.requested_from_peer);
    BOOST_CHECK(info.queued_block == nullptr);

    // Request from peer1 without pit (no partialBlock)
    bdm.BlockRequested(peer1, *genesis);

    info = bdm.FindBlockInFlight(hash, peer1);
    BOOST_CHECK_EQUAL(info.already_in_flight, 1u);
    BOOST_CHECK(info.first_in_flight);
    BOOST_CHECK(info.requested_from_peer);
    BOOST_CHECK(info.queued_block == nullptr);

    // Request from peer2 with pit — creates a partialBlock
    std::list<node::QueuedBlock>::iterator* pit = nullptr;
    bdm.BlockRequested(peer2, *genesis, &pit, /*mempool=*/nullptr);

    // From peer1's perspective: still first in flight, no partialBlock
    info = bdm.FindBlockInFlight(hash, peer1);
    BOOST_CHECK_EQUAL(info.already_in_flight, 2u);
    BOOST_CHECK(info.first_in_flight);
    BOOST_CHECK(info.requested_from_peer);
    BOOST_CHECK(info.queued_block == nullptr);

    // From peer2's perspective: not first in flight, has partialBlock
    info = bdm.FindBlockInFlight(hash, peer2);
    BOOST_CHECK_EQUAL(info.already_in_flight, 2u);
    BOOST_CHECK(!info.first_in_flight);
    BOOST_CHECK(info.requested_from_peer);
    BOOST_CHECK(info.queued_block != nullptr);

    // From peer3's perspective: not involved at all
    info = bdm.FindBlockInFlight(hash, peer3);
    BOOST_CHECK_EQUAL(info.already_in_flight, 2u);
    BOOST_CHECK(!info.first_in_flight);
    BOOST_CHECK(!info.requested_from_peer);
    BOOST_CHECK(info.queued_block == nullptr);

    bdm.DisconnectedPeer(peer1);
    bdm.DisconnectedPeer(peer2);
    bdm.DisconnectedPeer(peer3);
    bdm.CheckIsEmpty();
}

// Test that TipMayBeStale returns false when blocks are in flight,
// even if the last tip update is old enough to trigger staleness.
BOOST_AUTO_TEST_CASE(tip_not_stale_with_blocks_in_flight)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    NodeId peer1{0};
    bdm.ConnectedPeer(peer1, {false, false, true, false});

    int64_t pow_spacing = 600;
    auto now = GetTime<std::chrono::seconds>();

    // Initialize lazy tip update
    bdm.TipMayBeStale(now, pow_spacing);

    // Set an old tip update — stale when no blocks are in flight
    bdm.SetLastTipUpdate(now - std::chrono::seconds{pow_spacing * 4});
    BOOST_CHECK(bdm.TipMayBeStale(now, pow_spacing));

    // Put a block in flight
    const CBlockIndex* genesis = m_node.chainman->ActiveChain().Genesis();
    bdm.BlockRequested(peer1, *genesis);
    BOOST_CHECK(bdm.HasBlocksInFlight());

    // Tip should NOT be stale — blocks in flight means we're making progress
    BOOST_CHECK(!bdm.TipMayBeStale(now, pow_spacing));

    // Remove the in-flight block — staleness returns
    bdm.RemoveBlockRequest(genesis->GetBlockHash(), std::nullopt);
    BOOST_CHECK(bdm.TipMayBeStale(now, pow_spacing));

    bdm.DisconnectedPeer(peer1);
    bdm.CheckIsEmpty();
}

// Test two-phase peer registration: ConnectedPeer is called first with
// preferred=false (from InitializeNode), then re-called with preferred=true
// (from VERSION handler). Verify the preferred counter does not double-count.
BOOST_AUTO_TEST_CASE(connected_peer_reregistration)
{
    LOCK(cs_main);
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    NodeId peer1{0};

    // Phase 1: initial registration with preferred=false
    bdm.ConnectedPeer(peer1, {/*m_is_inbound=*/false, /*m_preferred_download=*/false,
                               /*m_can_serve_witnesses=*/true,
                               /*m_is_limited_peer=*/false});
    BOOST_CHECK_EQUAL(bdm.GetNumPreferredDownload(), 0);

    // Phase 2: re-registration with preferred=true (VERSION handler)
    bdm.ConnectedPeer(peer1, {false, /*m_preferred_download=*/true, true, false});
    BOOST_CHECK_EQUAL(bdm.GetNumPreferredDownload(), 1);

    // Phase 3: re-registration again with preferred=true — should be idempotent
    bdm.ConnectedPeer(peer1, {false, true, true, false});
    BOOST_CHECK_EQUAL(bdm.GetNumPreferredDownload(), 1);

    bdm.DisconnectedPeer(peer1);
    bdm.CheckIsEmpty();
}

// Test CompareExchangeBlockStallingTimeout CAS semantics: fails when the
// expected value does not match current (updating expected to actual),
// succeeds when it does match.
BOOST_AUTO_TEST_CASE(compare_exchange_stalling_timeout)
{
    BlockDownloadManager bdm(BlockDownloadOptions{*m_node.chainman});

    // Initial value is default
    BOOST_CHECK(bdm.GetBlockStallingTimeout() == BLOCK_STALLING_TIMEOUT_DEFAULT);

    // CAS with wrong expected value — should fail
    auto wrong = std::chrono::seconds{999};
    BOOST_CHECK(!bdm.CompareExchangeBlockStallingTimeout(wrong, std::chrono::seconds{10}));
    // On failure, expected is updated to the actual current value
    BOOST_CHECK(wrong == BLOCK_STALLING_TIMEOUT_DEFAULT);
    // Value unchanged
    BOOST_CHECK(bdm.GetBlockStallingTimeout() == BLOCK_STALLING_TIMEOUT_DEFAULT);

    // CAS with correct expected value — should succeed
    auto expected = BLOCK_STALLING_TIMEOUT_DEFAULT;
    BOOST_CHECK(bdm.CompareExchangeBlockStallingTimeout(expected, std::chrono::seconds{4}));
    BOOST_CHECK(bdm.GetBlockStallingTimeout() == std::chrono::seconds{4});
}

BOOST_AUTO_TEST_SUITE_END()
