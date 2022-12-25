// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NET_PROCESSING_H
#define SYSCOIN_NET_PROCESSING_H

#include <net.h>
#include <validationinterface.h>
// SYSCOIN
#include <headerssync.h>
class AddrMan;
class CChainParams;
class CTxMemPool;
class ChainstateManager;
/** Default for -maxorphantx, maximum number of orphan transactions kept in memory */
static const unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS = 100;
/** Default number of orphan+recently-replaced txn to keep around for block reconstruction */
static const unsigned int DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN = 100;
static const bool DEFAULT_PEERBLOOMFILTERS = false;
static const bool DEFAULT_PEERBLOCKFILTERS = false;
/** Threshold for marking a node to be discouraged, e.g. disconnected and added to the discouragement filter. */
static const int DISCOURAGEMENT_THRESHOLD{100};

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
};
// SYSCOIN
extern RecursiveMutex g_cs_orphans;
/** Data structure for an individual peer. This struct is not protected by
 * cs_main since it does not contain validation-critical data.
 *
 * Memory is owned by shared pointers and this object is destructed when
 * the refcount drops to zero.
 *
 * Mutexes inside this struct must not be held when locking m_peer_mutex.
 *
 * TODO: move most members from CNodeState to this structure.
 * TODO: move remaining application-layer data members from CNode to this structure.
 */
struct Peer {
    /** Same id as the CNode object for this peer */
    const NodeId m_id{0};

    /** Services we offered to this peer.
     *
     *  This is supplied by CConnman during peer initialization. It's const
     *  because there is no protocol defined for renegotiating services
     *  initially offered to a peer. The set of local services we offer should
     *  not change after initialization.
     *
     *  An interesting example of this is NODE_NETWORK and initial block
     *  download: a node which starts up from scratch doesn't have any blocks
     *  to serve, but still advertises NODE_NETWORK because it will eventually
     *  fulfill this role after IBD completes. P2P code is written in such a
     *  way that it can gracefully handle peers who don't make good on their
     *  service advertisements. */
    const ServiceFlags m_our_services;
    /** Services this peer offered to us. */
    std::atomic<ServiceFlags> m_their_services{NODE_NONE};

    /** Protects misbehavior data members */
    Mutex m_misbehavior_mutex;
    /** Accumulated misbehavior score for this peer */
    int m_misbehavior_score GUARDED_BY(m_misbehavior_mutex){0};
    /** Whether this peer should be disconnected and marked as discouraged (unless it has NetPermissionFlags::NoBan permission). */
    bool m_should_discourage GUARDED_BY(m_misbehavior_mutex){false};

    /** Protects block inventory data members */
    Mutex m_block_inv_mutex;
    /** List of blocks that we'll announce via an `inv` message.
     * There is no final sorting before sending, as they are always sent
     * immediately and in the order requested. */
    std::vector<uint256> m_blocks_for_inv_relay GUARDED_BY(m_block_inv_mutex);
    /** Unfiltered list of blocks that we'd like to announce via a `headers`
     * message. If we can't announce via a `headers` message, we'll fall back to
     * announcing via `inv`. */
    std::vector<uint256> m_blocks_for_headers_relay GUARDED_BY(m_block_inv_mutex);
    /** The final block hash that we sent in an `inv` message to this peer.
     * When the peer requests this block, we send an `inv` message to trigger
     * the peer to request the next sequence of block hashes.
     * Most peers use headers-first syncing, which doesn't use this mechanism */
    uint256 m_continuation_block GUARDED_BY(m_block_inv_mutex) {};

    /** This peer's reported block height when we connected */
    std::atomic<int> m_starting_height{-1};

    /** The pong reply we're expecting, or 0 if no pong expected. */
    std::atomic<uint64_t> m_ping_nonce_sent{0};
    /** When the last ping was sent, or 0 if no ping was ever sent */
    std::atomic<std::chrono::microseconds> m_ping_start{0us};
    /** Whether a ping has been requested by the user */
    std::atomic<bool> m_ping_queued{false};

    /** Whether this peer relays txs via wtxid */
    std::atomic<bool> m_wtxid_relay{false};
    /** The feerate in the most recent BIP133 `feefilter` message sent to the peer.
     *  It is *not* a p2p protocol violation for the peer to send us
     *  transactions with a lower fee rate than this. See BIP133. */
    CAmount m_fee_filter_sent GUARDED_BY(NetEventsInterface::g_msgproc_mutex){0};
    /** Timestamp after which we will send the next BIP133 `feefilter` message
      * to the peer. */
    std::chrono::microseconds m_next_send_feefilter GUARDED_BY(NetEventsInterface::g_msgproc_mutex){0};

    struct TxRelay {
        mutable RecursiveMutex m_bloom_filter_mutex;
        /** Whether we relay transactions to this peer. */
        bool m_relay_txs GUARDED_BY(m_bloom_filter_mutex){false};
        /** A bloom filter for which transactions to announce to the peer. See BIP37. */
        std::unique_ptr<CBloomFilter> m_bloom_filter PT_GUARDED_BY(m_bloom_filter_mutex) GUARDED_BY(m_bloom_filter_mutex){nullptr};

        mutable RecursiveMutex m_tx_inventory_mutex;
        /** A filter of all the txids and wtxids that the peer has announced to
         *  us or we have announced to the peer. We use this to avoid announcing
         *  the same txid/wtxid to a peer that already has the transaction. */
        CRollingBloomFilter m_tx_inventory_known_filter GUARDED_BY(m_tx_inventory_mutex){50000, 0.000001};
        /** Set of transaction ids we still have to announce (txid for
         *  non-wtxid-relay peers, wtxid for wtxid-relay peers). We use the
         *  mempool to sort transactions in dependency order before relay, so
         *  this does not have to be sorted. */
        std::set<uint256> m_tx_inventory_to_send GUARDED_BY(m_tx_inventory_mutex);
        /** Whether the peer has requested us to send our complete mempool. Only
         *  permitted if the peer has NetPermissionFlags::Mempool. See BIP35. */
        bool m_send_mempool GUARDED_BY(m_tx_inventory_mutex){false};
        /** The last time a BIP35 `mempool` request was serviced. */
        std::atomic<std::chrono::seconds> m_last_mempool_req{0s};
        /** The next time after which we will send an `inv` message containing
         *  transaction announcements to this peer. */
        std::chrono::microseconds m_next_inv_send_time GUARDED_BY(m_tx_inventory_mutex){0};

        /** Minimum fee rate with which to filter transaction announcements to this node. See BIP133. */
        std::atomic<CAmount> m_fee_filter_received{0};
        // SYSCOIN
        std::set<CInv> m_tx_inventory_to_send_other;
    };

    /* Initializes a TxRelay struct for this peer. Can be called at most once for a peer. */
    TxRelay* SetTxRelay() EXCLUSIVE_LOCKS_REQUIRED(!m_tx_relay_mutex)
    {
        LOCK(m_tx_relay_mutex);
        Assume(!m_tx_relay);
        m_tx_relay = std::make_unique<Peer::TxRelay>();
        return m_tx_relay.get();
    };

    TxRelay* GetTxRelay() EXCLUSIVE_LOCKS_REQUIRED(!m_tx_relay_mutex)
    {
        return WITH_LOCK(m_tx_relay_mutex, return m_tx_relay.get());
    };

    /** A vector of addresses to send to the peer, limited to MAX_ADDR_TO_SEND. */
    std::vector<CAddress> m_addrs_to_send GUARDED_BY(NetEventsInterface::g_msgproc_mutex);
    /** Probabilistic filter to track recent addr messages relayed with this
     *  peer. Used to avoid relaying redundant addresses to this peer.
     *
     *  We initialize this filter for outbound peers (other than
     *  block-relay-only connections) or when an inbound peer sends us an
     *  address related message (ADDR, ADDRV2, GETADDR).
     *
     *  Presence of this filter must correlate with m_addr_relay_enabled.
     **/
    std::unique_ptr<CRollingBloomFilter> m_addr_known GUARDED_BY(NetEventsInterface::g_msgproc_mutex);
    /** Whether we are participating in address relay with this connection.
     *
     *  We set this bool to true for outbound peers (other than
     *  block-relay-only connections), or when an inbound peer sends us an
     *  address related message (ADDR, ADDRV2, GETADDR).
     *
     *  We use this bool to decide whether a peer is eligible for gossiping
     *  addr messages. This avoids relaying to peers that are unlikely to
     *  forward them, effectively blackholing self announcements. Reasons
     *  peers might support addr relay on the link include that they connected
     *  to us as a block-relay-only peer or they are a light client.
     *
     *  This field must correlate with whether m_addr_known has been
     *  initialized.*/
    std::atomic_bool m_addr_relay_enabled{false};
    /** Whether a getaddr request to this peer is outstanding. */
    bool m_getaddr_sent GUARDED_BY(NetEventsInterface::g_msgproc_mutex){false};
    /** Guards address sending timers. */
    mutable Mutex m_addr_send_times_mutex;
    /** Time point to send the next ADDR message to this peer. */
    std::chrono::microseconds m_next_addr_send GUARDED_BY(m_addr_send_times_mutex){0};
    /** Time point to possibly re-announce our local address to this peer. */
    std::chrono::microseconds m_next_local_addr_send GUARDED_BY(m_addr_send_times_mutex){0};
    /** Whether the peer has signaled support for receiving ADDRv2 (BIP155)
     *  messages, indicating a preference to receive ADDRv2 instead of ADDR ones. */
    std::atomic_bool m_wants_addrv2{false};
    /** Whether this peer has already sent us a getaddr message. */
    bool m_getaddr_recvd GUARDED_BY(NetEventsInterface::g_msgproc_mutex){false};
    /** Number of addresses that can be processed from this peer. Start at 1 to
     *  permit self-announcement. */
    double m_addr_token_bucket GUARDED_BY(NetEventsInterface::g_msgproc_mutex){1.0};
    /** When m_addr_token_bucket was last updated */
    std::chrono::microseconds m_addr_token_timestamp GUARDED_BY(NetEventsInterface::g_msgproc_mutex){GetTime<std::chrono::microseconds>()};
    /** Total number of addresses that were dropped due to rate limiting. */
    std::atomic<uint64_t> m_addr_rate_limited{0};
    /** Total number of addresses that were processed (excludes rate-limited ones). */
    std::atomic<uint64_t> m_addr_processed{0};

    /** Whether we've sent this peer a getheaders in response to an inv prior to initial-headers-sync completing */
    bool m_inv_triggered_getheaders_before_sync GUARDED_BY(NetEventsInterface::g_msgproc_mutex){false};
    
    /** Protects m_getdata_requests **/
    Mutex m_getdata_requests_mutex;
    /** Work queue of items requested by this peer **/
    std::deque<CInv> m_getdata_requests GUARDED_BY(m_getdata_requests_mutex);

    /** Time of the last getheaders message to this peer */
    NodeClock::time_point m_last_getheaders_timestamp GUARDED_BY(NetEventsInterface::g_msgproc_mutex){};
    /** Protects m_headers_sync **/
    Mutex m_headers_sync_mutex;
    /** Headers-sync state for this peer (eg for initial sync, or syncing large
     * reorgs) **/
    std::unique_ptr<HeadersSyncState> m_headers_sync PT_GUARDED_BY(m_headers_sync_mutex) GUARDED_BY(m_headers_sync_mutex) {};

    /** Whether we've sent our peer a sendheaders message. **/
    std::atomic<bool> m_sent_sendheaders{false};
    // SYSCOIN
    /** This peer's a masternode connection */
    std::atomic<bool> m_masternode_connection{false};
    explicit Peer(NodeId id, ServiceFlags our_services)
        : m_id{id}
        , m_our_services{our_services}
    {}

private:
    Mutex m_tx_relay_mutex;

    /** Transaction relay data. May be a nullptr. */
    std::unique_ptr<TxRelay> m_tx_relay GUARDED_BY(m_tx_relay_mutex);
};

using PeerRef = std::shared_ptr<Peer>;

class PeerManager : public CValidationInterface, public NetEventsInterface
{
public:
    static std::unique_ptr<PeerManager> make(CConnman& connman, AddrMan& addrman,
                                             BanMan* banman, ChainstateManager& chainman,
                                             CTxMemPool& pool, bool ignore_incoming_txs);
    virtual ~PeerManager() { }

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

    /** Whether this node ignores txs received over p2p. */
    virtual bool IgnoresIncomingTxs() = 0;

    /** Relay transaction to all peers. */
    virtual void RelayTransaction(const uint256& txid, const uint256& wtxid) = 0;

    /** Send ping message to all peers */
    virtual void SendPings() = 0;

    /** Set the best height */
    virtual void SetBestHeight(int height) = 0;

    // SYSCOIN
    virtual size_t GetRequestedCount(NodeId nodeId) const = 0;
    virtual void ReceivedResponse(NodeId nodeId, const uint256& hash) = 0;
    virtual void ForgetTxHash(NodeId nodeId, const uint256& hash) = 0;
    virtual void _RelayTransaction(const uint256& txid, const uint256& wtxid) = 0;
    virtual void PushTxInventory(Peer& peer, const uint256& txid, const uint256& wtxid) = 0;
    virtual void RelayTransactionOther(const CInv& inv) = 0;
    virtual void _RelayTransactionOther(const CInv& inv) = 0;
    virtual void PushTxInventoryOther(Peer& peer, const CInv& inv) = 0;
    virtual PeerRef GetPeerRef(NodeId id) const = 0;
    virtual void AddKnownTx(Peer& peer, const uint256& hash) = 0;
    virtual bool IsBanned(NodeId nodeid, BanMan& banman) = 0;
    /* Public for unit testing. */
    virtual void UnitTestMisbehaving(NodeId peer_id, int howmuch) = 0;
    // SYSCOIN
    virtual void Misbehaving(Peer& peer, int howmuch, const std::string& message) = 0;

    /**
     * Evict extra outbound peers. If we think our tip may be stale, connect to an extra outbound.
     * Public for unit testing.
     */
    virtual void CheckForStaleTipAndEvictPeers() = 0;

    /** Process a single message from a peer. Public for fuzz testing */
    virtual void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                                const std::chrono::microseconds time_received, const std::atomic<bool>& interruptMsgProc) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;

    /** This function is used for testing the stale tip eviction logic, see denialofservice_tests.cpp */
    virtual void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds) = 0;
};

// SYSCOIN
// Upstream moved this into net_processing.cpp (13417), however since we use Misbehaving in a number of syscoin specific
// files such as mnauth.cpp and governance.cpp it makes sense to keep it in the header
/** Increase a node's misbehavior score. */
bool IsBanned(NodeId nodeid, BanMan& banman);
unsigned int GetMaxInv();
bool CanServeBlocks(const Peer& peer);
#endif // SYSCOIN_NET_PROCESSING_H
