// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKDOWNLOADMAN_H
#define BITCOIN_NODE_BLOCKDOWNLOADMAN_H

#include <kernel/cs_main.h>
#include <net.h>
#include <uint256.h>

#include <chrono>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class CBlockIndex;
class ChainstateManager;
class CTxMemPool;
class PartiallyDownloadedBlock;

/** Default time during which a peer must stall block download progress before being disconnected.
 * the actual timeout is increased temporarily if peers are disconnected for hitting the timeout */
static constexpr auto BLOCK_STALLING_TIMEOUT_DEFAULT{2s};
/** Maximum timeout for stalling block download. */
static constexpr auto BLOCK_STALLING_TIMEOUT_MAX{64s};
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and pruning harder). We'll probably
 *  want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
/** Minimum blocks required to signal NODE_NETWORK_LIMITED */
static const unsigned int NODE_NETWORK_LIMITED_MIN_BLOCKS = 288;

namespace node {
class BlockDownloadManagerImpl;

/** Blocks that are in flight, and that are in the queue to be downloaded. */
struct QueuedBlock {
    /** BlockIndex. We must have this since we only request blocks when we've already validated the header. */
    const CBlockIndex* pindex;
    /** Optional, used for CMPCTBLOCK downloads */
    std::unique_ptr<PartiallyDownloadedBlock> partialBlock;
};

struct BlockDownloadOptions {
    /** Reference to ChainstateManager for chain state access and LookupBlockIndex. */
    ChainstateManager& m_chainman;
};

struct BlockDownloadConnectionInfo {
    /** Whether this is an inbound peer. */
    bool m_is_inbound;
    /** Whether this peer is preferred for download. */
    bool m_preferred_download;
    /** Whether this peer can serve witness data (NODE_WITNESS). */
    bool m_can_serve_witnesses;
    /** Whether this peer can only serve limited recent blocks (pruned). */
    bool m_is_limited_peer;
};

/**
 * Class responsible for tracking block download state: which blocks are
 * in-flight, which peers are downloading from, request scheduling, stalling
 * detection, and source tracking.
 *
 * This class is not thread-safe. Access must be synchronized using an
 * external mutex.
 */
class BlockDownloadManager {
    const std::unique_ptr<BlockDownloadManagerImpl> m_impl;

public:
    explicit BlockDownloadManager(const BlockDownloadOptions& options);
    ~BlockDownloadManager();

    /** Register a new peer for block download tracking. */
    void ConnectedPeer(NodeId nodeid, const BlockDownloadConnectionInfo& info) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Clean up all block download state for a disconnected peer. */
    void DisconnectedPeer(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Have we requested this block from any peer? */
    bool IsBlockRequested(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Have we requested this block from an outbound peer? */
    bool IsBlockRequestedFromOutbound(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Remove this block from our tracked requested blocks.
     *  If from_peer is specified, only remove the block if it is in flight from that peer. */
    void RemoveBlockRequest(const uint256& hash, std::optional<NodeId> from_peer) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Mark a block as in flight from a given peer.
     *  Returns false (still setting pit) if the block was already in flight from the same peer.
     *  pit will only be valid as long as the same cs_main lock is being held. */
    bool BlockRequested(NodeId nodeid, const CBlockIndex& block,
                        std::list<QueuedBlock>::iterator** pit = nullptr,
                        CTxMemPool* mempool = nullptr) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Check whether the tip might be stale based on last update time and in-flight state.
     *  nPowTargetSpacing is used to determine the staleness threshold. */
    bool TipMayBeStale(std::chrono::seconds now, int64_t n_pow_target_spacing) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Record that the tip was updated. */
    void SetLastTipUpdate(std::chrono::seconds time);

    /** Get the last tip update time. */
    std::chrono::seconds GetLastTipUpdate() const;

    /** Check whether the last unknown block a peer advertised is not yet known. */
    void ProcessBlockAvailability(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Update tracking information about which blocks a peer is assumed to have. */
    void UpdateBlockAvailability(NodeId nodeid, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Calculate which blocks to download from a given peer, given our current tip.
     *  Update pindexLastCommonBlock and add not-in-flight missing successors to vBlocks.
     *  Sets nodeStaller to a stalling peer NodeId if applicable, or -1. */
    void FindNextBlocksToDownload(NodeId nodeid, unsigned int count,
                                  std::vector<const CBlockIndex*>& vBlocks,
                                  NodeId& nodeStaller) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Request blocks for the background chainstate, if one is in use. */
    void TryDownloadingHistoricalBlocks(NodeId nodeid, unsigned int count,
                                        std::vector<const CBlockIndex*>& vBlocks,
                                        const CBlockIndex* from_tip,
                                        const CBlockIndex* target_block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the best known block for a peer (or nullptr). */
    const CBlockIndex* GetBestKnownBlock(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the best header we have sent a peer (or nullptr). */
    const CBlockIndex* GetBestHeaderSent(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Set the best header we have sent a peer. */
    void SetBestHeaderSent(NodeId nodeid, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the last common block with a peer (or nullptr). */
    const CBlockIndex* GetLastCommonBlock(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get whether we have started syncing headers with this peer. */
    bool GetSyncStarted(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Mark that we've started syncing headers with this peer, incrementing the global counter. */
    void SetSyncStarted(NodeId nodeid, bool started) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the global number of peers with sync started. */
    int GetNumSyncStarted() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the stalling timeout for blocks. */
    std::chrono::seconds GetBlockStallingTimeout() const;

    /** Atomically compare-and-exchange the stalling timeout.
     *  Returns true on success (value was expected, now set to desired). */
    bool CompareExchangeBlockStallingTimeout(std::chrono::seconds& expected, std::chrono::seconds desired);

    /** Get the number of preferred download peers. */
    int GetNumPreferredDownload() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the number of peers we are downloading blocks from. */
    int GetPeersDownloadingFrom() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Whether blocks are in flight from any peer. */
    bool HasBlocksInFlight() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get total number of blocks in flight (across all hashes and peers). */
    size_t GetTotalBlocksInFlight() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the blocks-in-flight list for a peer. */
    const std::list<QueuedBlock>& GetBlocksInFlight(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the downloading-since time for a peer. */
    std::chrono::microseconds GetDownloadingSince(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the stalling-since time for a peer. */
    std::chrono::microseconds GetStallingSince(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Set the stalling-since time for a peer. */
    void SetStallingSince(NodeId nodeid, std::chrono::microseconds time) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get whether a peer is a preferred download peer. */
    bool IsPreferredDownload(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get block source information for a given block hash. */
    std::optional<std::pair<NodeId, bool>> GetBlockSource(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Record the source of a received block. */
    void SetBlockSource(const uint256& hash, NodeId nodeid, bool punish_on_invalid) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Remove a block source entry. */
    void EraseBlockSource(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the count of blocks in flight matching a hash. */
    size_t CountBlocksInFlight(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Check that all data structures are empty. Used for post-disconnect assertions. */
    void CheckIsEmpty() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Check whether a peer has a particular header. */
    bool PeerHasHeader(NodeId nodeid, const CBlockIndex* pindex) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Result of looking up in-flight block information. */
    struct BlockInFlightInfo {
        /** How many peers have this block in flight. */
        size_t already_in_flight{0};
        /** Whether the first entry (if any) is from the specified peer. */
        bool first_in_flight{false};
        /** Whether the specified peer has this block in flight (with a partialBlock). */
        bool requested_from_peer{false};
        /** If requested_from_peer, pointer to the QueuedBlock for this peer (valid while cs_main held). */
        QueuedBlock* queued_block{nullptr};
    };

    /** Find detailed in-flight information for a block hash + specific peer. */
    BlockInFlightInfo FindBlockInFlight(const uint256& hash, NodeId peer_id) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
} // namespace node
#endif // BITCOIN_NODE_BLOCKDOWNLOADMAN_H
