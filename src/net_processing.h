// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_PROCESSING_H
#define BITCOIN_NET_PROCESSING_H

#include <net.h>
#include <validationinterface.h>

#include <chrono>

class AddrMan;
class CChainParams;
class CTxMemPool;
class ChainstateManager;

namespace node {
class Warnings;
} // namespace node

/** Whether transaction reconciliation protocol should be enabled by default. */
static constexpr bool DEFAULT_TXRECONCILIATION_ENABLE{false};
/** Default for -maxorphantx, maximum number of orphan transactions kept in memory */
static const uint32_t DEFAULT_MAX_ORPHAN_TRANSACTIONS{100};
/** Default number of non-mempool transactions to keep around for block reconstruction. Includes
    orphan, replaced, and rejected transactions. */
static const uint32_t DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN{100};
static const bool DEFAULT_PEERBLOOMFILTERS = false;
static const bool DEFAULT_PEERBLOCKFILTERS = false;
/** Maximum number of outstanding CMPCTBLOCK requests for the same block. */
static const unsigned int MAX_CMPCTBLOCKS_INFLIGHT_PER_BLOCK = 3;

struct CNodeStateStats {
    int nSyncHeight = -1;
    int nCommonHeight = -1;
    int m_starting_height = -1;
    std::chrono::microseconds m_ping_wait;
    std::vector<int> vHeightInFlight;
    bool m_relay_txs;
    CAmount m_fee_filter_received;
    uint64_t m_addr_processed = 0;
    uint64_t m_addr_rate_limited = 0;
    bool m_addr_relay_enabled{false};
    ServiceFlags their_services;
    int64_t presync_height{-1};
    std::chrono::seconds time_offset{0};
};

struct PeerManagerInfo {
    std::chrono::seconds median_outbound_time_offset{0s};
    bool ignores_incoming_txs{false};
};

class PeerManager : public CValidationInterface, public NetEventsInterface
{
public:
    struct Options {
        //! Whether this node is running in -blocksonly mode
        bool ignore_incoming_txs{DEFAULT_BLOCKSONLY};
        //! Whether transaction reconciliation protocol is enabled
        bool reconcile_txs{DEFAULT_TXRECONCILIATION_ENABLE};
        //! Maximum number of orphan transactions kept in memory
        uint32_t max_orphan_txs{DEFAULT_MAX_ORPHAN_TRANSACTIONS};
        //! Number of non-mempool transactions to keep around for block reconstruction. Includes
        //! orphan, replaced, and rejected transactions.
        uint32_t max_extra_txs{DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN};
        //! Whether all P2P messages are captured to disk
        bool capture_messages{false};
        //! Whether or not the internal RNG behaves deterministically (this is
        //! a test-only option).
        bool deterministic_rng{false};
    };

    static std::unique_ptr<PeerManager> make(CConnman& connman, AddrMan& addrman,
                                             BanMan* banman, ChainstateManager& chainman,
                                             CTxMemPool& pool, node::Warnings& warnings, Options opts);
    virtual ~PeerManager() = default;

    /**
     * Attempt to manually fetch block from a given peer. We must already have the header.
     *
     * @param[in]  peer_id      The peer id
     * @param[in]  block_index  The blockindex
     * @returns std::nullopt if a request was successfully made, otherwise an error message
     */
    virtual std::optional<std::string> FetchBlock(NodeId peer_id, const CBlockIndex& block_index) = 0;

    /** Begin running background tasks, should only be called once */
    virtual void StartScheduledTasks(CScheduler& scheduler) = 0;

    /** Get statistics from node state */
    virtual bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) const = 0;

    /** Get peer manager info. */
    virtual PeerManagerInfo GetInfo() const = 0;

    /** Relay transaction to all peers. */
    virtual void RelayTransaction(const uint256& txid, const uint256& wtxid) = 0;

    /** Send ping message to all peers */
    virtual void SendPings() = 0;

    /** Set the height of the best block and its time (seconds since epoch). */
    virtual void SetBestBlock(int height, std::chrono::seconds time) = 0;

    /* Public for unit testing. */
    virtual void UnitTestMisbehaving(NodeId peer_id) = 0;

    /**
     * Evict extra outbound peers. If we think our tip may be stale, connect to an extra outbound.
     * Public for unit testing.
     */
    virtual void CheckForStaleTipAndEvictPeers() = 0;

    /** Process a single message from a peer. Public for fuzz testing */
    virtual void ProcessMessage(CNode& pfrom, const std::string& msg_type, DataStream& vRecv,
                                const std::chrono::microseconds time_received, const std::atomic<bool>& interruptMsgProc) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;

    /** This function is used for testing the stale tip eviction logic, see denialofservice_tests.cpp */
    virtual void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds) = 0;

    /**
     * Gets the set of service flags which are "desirable" for a given peer.
     *
     * These are the flags which are required for a peer to support for them
     * to be "interesting" to us, ie for us to wish to use one of our few
     * outbound connection slots for or for us to wish to prioritize keeping
     * their connection around.
     *
     * Relevant service flags may be peer- and state-specific in that the
     * version of the peer may determine which flags are required (eg in the
     * case of NODE_NETWORK_LIMITED where we seek out NODE_NETWORK peers
     * unless they set NODE_NETWORK_LIMITED and we are out of IBD, in which
     * case NODE_NETWORK_LIMITED suffices).
     *
     * Thus, generally, avoid calling with 'services' == NODE_NONE, unless
     * state-specific flags must absolutely be avoided. When called with
     * 'services' == NODE_NONE, the returned desirable service flags are
     * guaranteed to not change dependent on state - ie they are suitable for
     * use when describing peers which we know to be desirable, but for which
     * we do not have a confirmed set of service flags.
    */
    virtual ServiceFlags GetDesirableServiceFlags(ServiceFlags services) const = 0;
};

#endif // BITCOIN_NET_PROCESSING_H
