// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKDOWNLOADMAN_IMPL_H
#define BITCOIN_NODE_BLOCKDOWNLOADMAN_IMPL_H

#include <node/blockdownloadman.h>

#include <chain.h>
#include <kernel/cs_main.h>
#include <uint256.h>

#include <atomic>
#include <chrono>
#include <list>
#include <map>
#include <optional>
#include <vector>

class CBlockIndex;
class ChainstateManager;

namespace node {
class BlockDownloadManagerImpl {
public:
    BlockDownloadOptions m_opts;

    /** Per-peer block download state. */
    struct PeerBlockDownloadState {
        /** Connection info provided at registration time (updated on re-registration). */
        BlockDownloadConnectionInfo m_connection_info;

        /** The best known block we know this peer has announced. */
        const CBlockIndex* pindexBestKnownBlock{nullptr};
        /** The hash of the last unknown block this peer has announced. */
        uint256 hashLastUnknownBlock{};
        /** The last full block we both have. */
        const CBlockIndex* pindexLastCommonBlock{nullptr};
        /** The best header we have sent our peer. */
        const CBlockIndex* pindexBestHeaderSent{nullptr};
        /** Whether we've started headers synchronization with this peer. */
        bool fSyncStarted{false};
        /** Since when we're stalling block download progress (in microseconds), or 0. */
        std::chrono::microseconds m_stalling_since{0us};
        /** Blocks in flight from this peer. */
        std::list<QueuedBlock> vBlocksInFlight;
        /** When the first entry in vBlocksInFlight started downloading. */
        std::chrono::microseconds m_downloading_since{0us};
        /** Whether we consider this a preferred download peer. */
        bool fPreferredDownload{false};

        explicit PeerBlockDownloadState(const BlockDownloadConnectionInfo& info)
            : m_connection_info{info} {}
    };

    /** Per-peer download state map. */
    std::map<NodeId, PeerBlockDownloadState> m_peer_info GUARDED_BY(::cs_main);

    /* Multimap used to preserve insertion order */
    using BlockDownloadMap = std::multimap<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator>>;
    BlockDownloadMap mapBlocksInFlight GUARDED_BY(::cs_main);

    /**
     * Sources of received blocks, saved to be able to punish them when processing
     * happens afterwards.
     * Set mapBlockSource[hash].second to false if the node should not be
     * punished if the block is invalid.
     */
    std::map<uint256, std::pair<NodeId, bool>> mapBlockSource GUARDED_BY(::cs_main);

    /** Number of peers with fSyncStarted. */
    int nSyncStarted GUARDED_BY(::cs_main){0};

    /** Default time during which a peer must stall block download progress before being disconnected. */
    std::atomic<std::chrono::seconds> m_block_stalling_timeout{BLOCK_STALLING_TIMEOUT_DEFAULT};

    /** When our tip was last updated. */
    std::atomic<std::chrono::seconds> m_last_tip_update{0s};

    /** Number of preferable block download peers. */
    int m_num_preferred_download_peers GUARDED_BY(::cs_main){0};

    /** Number of peers from which we're downloading blocks. */
    int m_peers_downloading_from GUARDED_BY(::cs_main){0};

    explicit BlockDownloadManagerImpl(const BlockDownloadOptions& options)
        : m_opts{options} {}

    // Helper to look up per-peer state (returns nullptr if not found).
    PeerBlockDownloadState* GetPeerState(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    const PeerBlockDownloadState* GetPeerState(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void ConnectedPeer(NodeId nodeid, const BlockDownloadConnectionInfo& info) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void DisconnectedPeer(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool IsBlockRequested(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool IsBlockRequestedFromOutbound(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void RemoveBlockRequest(const uint256& hash, std::optional<NodeId> from_peer) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool BlockRequested(NodeId nodeid, const CBlockIndex& block,
                        std::list<QueuedBlock>::iterator** pit,
                        CTxMemPool* mempool) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool TipMayBeStale(std::chrono::seconds now, int64_t n_pow_target_spacing) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void ProcessBlockAvailability(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void UpdateBlockAvailability(NodeId nodeid, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void FindNextBlocksToDownload(NodeId nodeid, unsigned int count,
                                  std::vector<const CBlockIndex*>& vBlocks,
                                  NodeId& nodeStaller) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void TryDownloadingHistoricalBlocks(NodeId nodeid, unsigned int count,
                                        std::vector<const CBlockIndex*>& vBlocks,
                                        const CBlockIndex* from_tip,
                                        const CBlockIndex* target_block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void FindNextBlocks(std::vector<const CBlockIndex*>& vBlocks,
                        NodeId nodeid,
                        PeerBlockDownloadState& state,
                        const CBlockIndex* pindexWalk,
                        unsigned int count, int nWindowEnd,
                        const CChain* activeChain = nullptr,
                        NodeId* nodeStaller = nullptr) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool PeerHasHeader(const PeerBlockDownloadState& state, const CBlockIndex* pindex) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void CheckIsEmpty() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCKDOWNLOADMAN_IMPL_H
