// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_processing.h>

#include <addrman.h>
#include <banman.h>
#include <blockencodings.h>
#include <blockfilter.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <index/blockfilterindex.h>
#include <merkleblock.h>
#include <netbase.h>
#include <netmessagemaker.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <reverse_iterator.h>
#include <scheduler.h>
#include <streams.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <txorphanage.h>
#include <txrequest.h>
#include <util/check.h> // For NDEBUG compile time check
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>

#include <memory>
#include <optional>
#include <typeinfo>

/** How long to cache transactions in mapRelay for normal relay */
static constexpr auto RELAY_TX_CACHE_TIME = 15min;
/** How long a transaction has to be in the mempool before it can unconditionally be relayed (even when not in mapRelay). */
static constexpr auto UNCONDITIONAL_RELAY_DELAY = 2min;
/** Headers download timeout.
 *  Timeout = base + per_header * (expected number of headers) */
static constexpr auto HEADERS_DOWNLOAD_TIMEOUT_BASE = 15min;
static constexpr auto HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER = 1ms;
/** Protect at least this many outbound peers from disconnection due to slow/
 * behind headers chain.
 */
static constexpr int32_t MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT = 4;
/** Timeout for (unprotected) outbound peers to sync to our chainwork, in seconds */
static constexpr int64_t CHAIN_SYNC_TIMEOUT = 20 * 60; // 20 minutes
/** How frequently to check for stale tips, in seconds */
static constexpr int64_t STALE_CHECK_INTERVAL = 10 * 60 * 60 * 5; // 10 minutes
/** How frequently to check for extra outbound peers and disconnect, in seconds */
static constexpr int64_t EXTRA_PEER_CHECK_INTERVAL = 45;
/** Minimum time an outbound-peer-eviction candidate must be connected for, in order to evict, in seconds */
static constexpr int64_t MINIMUM_CONNECT_TIME = 30;
/** SHA256("main address relay")[0:8] */
static constexpr uint64_t RANDOMIZER_ID_ADDRESS_RELAY = 0x3cac0035b5866b90ULL;
/// Age after which a stale block will no longer be served if requested as
/// protection against fingerprinting. Set to one month, denominated in seconds.
static constexpr int STALE_RELAY_AGE_LIMIT = 30 * 24 * 60 * 60;
/// Age after which a block is considered historical for purposes of rate
/// limiting block relay. Set to one week, denominated in seconds.
static constexpr int HISTORICAL_BLOCK_AGE = 7 * 24 * 60 * 60;
/** Time between pings automatically sent out for latency probing and keepalive */
static constexpr std::chrono::minutes PING_INTERVAL{2};
/** The maximum number of entries in a locator */
static const unsigned int MAX_LOCATOR_SZ = 101;
/** The maximum number of entries in an 'inv' protocol message */
static const unsigned int MAX_INV_SZ = 50000;
/** Maximum number of in-flight transaction requests from a peer. It is not a hard limit, but the threshold at which
 *  point the OVERLOADED_PEER_TX_DELAY kicks in. */
static constexpr int32_t MAX_PEER_TX_REQUEST_IN_FLIGHT = 100;
/** Maximum number of transactions to consider for requesting, per peer. It provides a reasonable DoS limit to
 *  per-peer memory usage spent on announcements, while covering peers continuously sending INVs at the maximum
 *  rate (by our own policy, see INVENTORY_BROADCAST_PER_SECOND) for several minutes, while not receiving
 *  the actual transaction (from any peer) in response to requests for them. */
static constexpr int32_t MAX_PEER_TX_ANNOUNCEMENTS = 5000;
/** How long to delay requesting transactions via txids, if we have wtxid-relaying peers */
static constexpr auto TXID_RELAY_DELAY = std::chrono::seconds{2};
/** How long to delay requesting transactions from non-preferred peers */
static constexpr auto NONPREF_PEER_TX_DELAY = std::chrono::seconds{2};
/** How long to delay requesting transactions from overloaded peers (see MAX_PEER_TX_REQUEST_IN_FLIGHT). */
static constexpr auto OVERLOADED_PEER_TX_DELAY = std::chrono::seconds{2};
/** How long to wait (in microseconds) before downloading a transaction from an additional peer */
static constexpr std::chrono::microseconds GETDATA_TX_INTERVAL{std::chrono::seconds{60}};
/** Limit to avoid sending big packets. Not used in processing incoming GETDATA for compatibility */
static const unsigned int MAX_GETDATA_SZ = 1000;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Time during which a peer must stall block download progress before being disconnected. */
static constexpr auto BLOCK_STALLING_TIMEOUT = 2s;
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  less than this number, we reached its tip. Changing this value is a protocol upgrade. */
static const unsigned int MAX_HEADERS_RESULTS = 2000;
/** Maximum depth of blocks we're willing to serve as compact blocks to peers
 *  when requested. For older blocks, a regular BLOCK response will be sent. */
static const int MAX_CMPCTBLOCK_DEPTH = 5;
/** Maximum depth of blocks we're willing to respond to GETBLOCKTXN requests for. */
static const int MAX_BLOCKTXN_DEPTH = 10;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and pruning harder). We'll probably
 *  want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
/** Block download timeout base, expressed in multiples of the block interval (i.e. 10 min) */
static constexpr double BLOCK_DOWNLOAD_TIMEOUT_BASE = 1;
/** Additional block download timeout per parallel downloading peer (i.e. 5 min) */
static constexpr double BLOCK_DOWNLOAD_TIMEOUT_PER_PEER = 0.5;
/** Maximum number of headers to announce when relaying blocks with headers message.*/
static const unsigned int MAX_BLOCKS_TO_ANNOUNCE = 8;
/** Maximum number of unconnecting headers announcements before DoS score */
static const int MAX_UNCONNECTING_HEADERS = 10;
/** Minimum blocks required to signal NODE_NETWORK_LIMITED */
static const unsigned int NODE_NETWORK_LIMITED_MIN_BLOCKS = 288;
/** Average delay between local address broadcasts */
static constexpr auto AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL = 24h;
/** Average delay between peer address broadcasts */
static constexpr auto AVG_ADDRESS_BROADCAST_INTERVAL = 30s;
/** Average delay between trickled inventory transmissions for inbound peers.
 *  Blocks and peers with noban permission bypass this. */
static constexpr auto INBOUND_INVENTORY_BROADCAST_INTERVAL = 5s;
/** Average delay between trickled inventory transmissions for outbound peers.
 *  Use a smaller delay as there is less privacy concern for them.
 *  Blocks and peers with noban permission bypass this. */
static constexpr auto OUTBOUND_INVENTORY_BROADCAST_INTERVAL = 2s;
/** Maximum rate of inventory items to send per second.
 *  Limits the impact of low-fee transaction floods. */
static constexpr unsigned int INVENTORY_BROADCAST_PER_SECOND = 7;
/** Maximum number of inventory items to send per transmission. */
static constexpr unsigned int INVENTORY_BROADCAST_MAX = INVENTORY_BROADCAST_PER_SECOND * count_seconds(INBOUND_INVENTORY_BROADCAST_INTERVAL);
/** The number of most recently announced transactions a peer can request. */
static constexpr unsigned int INVENTORY_MAX_RECENT_RELAY = 3500;
/** Verify that INVENTORY_MAX_RECENT_RELAY is enough to cache everything typically
 *  relayed before unconditional relay from the mempool kicks in. This is only a
 *  lower bound, and it should be larger to account for higher inv rate to outbound
 *  peers, and random variations in the broadcast mechanism. */
static_assert(INVENTORY_MAX_RECENT_RELAY >= INVENTORY_BROADCAST_PER_SECOND * UNCONDITIONAL_RELAY_DELAY / std::chrono::seconds{1}, "INVENTORY_RELAY_MAX too low");
/** Average delay between feefilter broadcasts in seconds. */
static constexpr auto AVG_FEEFILTER_BROADCAST_INTERVAL = 10min;
/** Maximum feefilter broadcast delay after significant change. */
static constexpr auto MAX_FEEFILTER_CHANGE_DELAY = 5min;
/** Maximum number of compact filters that may be requested with one getcfilters. See BIP 157. */
static constexpr uint32_t MAX_GETCFILTERS_SIZE = 1000;
/** Maximum number of cf hashes that may be requested with one getcfheaders. See BIP 157. */
static constexpr uint32_t MAX_GETCFHEADERS_SIZE = 2000;
/** the maximum percentage of addresses from our addrman to return in response to a getaddr message. */
static constexpr size_t MAX_PCT_ADDR_TO_SEND = 23;

// Internal stuff
namespace {
/** Blocks that are in flight, and that are in the queue to be downloaded. */
struct QueuedBlock {
    uint256 hash;
    const CBlockIndex* pindex;                               //!< Optional.
    bool fValidatedHeaders;                                  //!< Whether this block has validated headers at the time of request.
    std::unique_ptr<PartiallyDownloadedBlock> partialBlock;  //!< Optional, used for CMPCTBLOCK downloads
};

/**
 * Data structure for an individual peer. This struct is not protected by
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

    /** Protects misbehavior data members */
    Mutex m_misbehavior_mutex;
    /** Accumulated misbehavior score for this peer */
    int m_misbehavior_score GUARDED_BY(m_misbehavior_mutex){0};
    /** Whether this peer should be disconnected and marked as discouraged (unless it has the noban permission). */
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

    /** Set of txids to reconsider once their parent transactions have been accepted **/
    std::set<uint256> m_orphan_work_set GUARDED_BY(g_cs_orphans);

    /** Protects m_getdata_requests **/
    Mutex m_getdata_requests_mutex;
    /** Work queue of items requested by this peer **/
    std::deque<CInv> m_getdata_requests GUARDED_BY(m_getdata_requests_mutex);

    explicit Peer(NodeId id) : m_id(id) {}
};

using PeerRef = std::shared_ptr<Peer>;

class PeerManagerImpl final : public PeerManager
{
public:
    PeerManagerImpl(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman,
                    BanMan* banman, CScheduler& scheduler, ChainstateManager& chainman,
                    CTxMemPool& pool, bool ignore_incoming_txs);

    /** Overridden from CValidationInterface. */
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex* pindex) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void BlockChecked(const CBlock& block, const BlockValidationState& state) override;
    void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock) override;

    /** Implement NetEventsInterface */
    void InitializeNode(CNode* pnode) override;
    void FinalizeNode(const CNode& node) override;
    bool ProcessMessages(CNode* pfrom, std::atomic<bool>& interrupt) override;
    bool SendMessages(CNode* pto) override EXCLUSIVE_LOCKS_REQUIRED(pto->cs_sendProcessing);

    /** Implement PeerManager */
    void CheckForStaleTipAndEvictPeers() override;
    bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) override;
    bool IgnoresIncomingTxs() override { return m_ignore_incoming_txs; }
    void SendPings() override;
    void RelayTransaction(const uint256& txid, const uint256& wtxid) override;
    void SetBestHeight(int height) override { m_best_height = height; };
    void Misbehaving(const NodeId pnode, const int howmuch, const std::string& message) override;
    void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                        const std::chrono::microseconds time_received, const std::atomic<bool>& interruptMsgProc) override;

private:
    /** Consider evicting an outbound peer based on the amount of time they've been behind our tip */
    void ConsiderEviction(CNode& pto, int64_t time_in_seconds) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** If we have extra outbound peers, try to disconnect the one with the oldest block announcement */
    void EvictExtraOutboundPeers(int64_t time_in_seconds) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Retrieve unbroadcast transactions from the mempool and reattempt sending to peers */
    void ReattemptInitialBroadcast(CScheduler& scheduler);

    /** Get a shared pointer to the Peer object.
     *  May return an empty shared_ptr if the Peer object can't be found. */
    PeerRef GetPeerRef(NodeId id) const;

    /** Get a shared pointer to the Peer object and remove it from m_peer_map.
     *  May return an empty shared_ptr if the Peer object can't be found. */
    PeerRef RemovePeer(NodeId id);

    /**
     * Potentially mark a node discouraged based on the contents of a BlockValidationState object
     *
     * @param[in] via_compact_block this bool is passed in because net_processing should
     * punish peers differently depending on whether the data was provided in a compact
     * block message or not. If the compact block had a valid header, but contained invalid
     * txs, the peer should not be punished. See BIP 152.
     *
     * @return Returns true if the peer was punished (probably disconnected)
     */
    bool MaybePunishNodeForBlock(NodeId nodeid, const BlockValidationState& state,
                                 bool via_compact_block, const std::string& message = "");

    /**
     * Potentially disconnect and discourage a node based on the contents of a TxValidationState object
     *
     * @return Returns true if the peer was punished (probably disconnected)
     */
    bool MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message = "");

    /** Maybe disconnect a peer and discourage future connections from its address.
     *
     * @param[in]   pnode     The node to check.
     * @param[in]   peer      The peer object to check.
     * @return                True if the peer was marked for disconnection in this function
     */
    bool MaybeDiscourageAndDisconnect(CNode& pnode, Peer& peer);

    void ProcessOrphanTx(std::set<uint256>& orphan_work_set) EXCLUSIVE_LOCKS_REQUIRED(cs_main, g_cs_orphans);
    /** Process a single headers message from a peer. */
    void ProcessHeadersMessage(CNode& pfrom, const Peer& peer,
                               const std::vector<CBlockHeader>& headers,
                               bool via_compact_block);

    void SendBlockTransactions(CNode& pfrom, const CBlock& block, const BlockTransactionsRequest& req);

    /** Register with TxRequestTracker that an INV has been received from a
     *  peer. The announcement parameters are decided in PeerManager and then
     *  passed to TxRequestTracker. */
    void AddTxAnnouncement(const CNode& node, const GenTxid& gtxid, std::chrono::microseconds current_time)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Send a version message to a peer */
    void PushNodeVersion(CNode& pnode, int64_t nTime);

    /** Send a ping message every PING_INTERVAL or if requested via RPC. May
     *  mark the peer to be disconnected if a ping has timed out. */
    void MaybeSendPing(CNode& node_to, Peer& peer);

    const CChainParams& m_chainparams;
    CConnman& m_connman;
    CAddrMan& m_addrman;
    /** Pointer to this node's banman. May be nullptr - check existence before dereferencing. */
    BanMan* const m_banman;
    ChainstateManager& m_chainman;
    CTxMemPool& m_mempool;
    TxRequestTracker m_txrequest GUARDED_BY(::cs_main);

    /** The height of the best chain */
    std::atomic<int> m_best_height{-1};

    int64_t m_stale_tip_check_time; //!< Next time to check for stale tip

    /** Whether this node is running in blocks only mode */
    const bool m_ignore_incoming_txs;

    /** Whether we've completed initial sync yet, for determining when to turn
      * on extra block-relay-only peers. */
    bool m_initial_sync_finished{false};

    /** Protects m_peer_map. This mutex must not be locked while holding a lock
     *  on any of the mutexes inside a Peer object. */
    mutable Mutex m_peer_mutex;
    /**
     * Map of all Peer objects, keyed by peer id. This map is protected
     * by the m_peer_mutex. Once a shared pointer reference is
     * taken, the lock may be released. Individual fields are protected by
     * their own locks.
     */
    std::map<NodeId, PeerRef> m_peer_map GUARDED_BY(m_peer_mutex);

    /** Number of nodes with fSyncStarted. */
    int nSyncStarted GUARDED_BY(cs_main) = 0;

    /**
     * Sources of received blocks, saved to be able punish them when processing
     * happens afterwards.
     * Set mapBlockSource[hash].second to false if the node should not be
     * punished if the block is invalid.
     */
    std::map<uint256, std::pair<NodeId, bool>> mapBlockSource GUARDED_BY(cs_main);

    /** Number of peers with wtxid relay. */
    int m_wtxid_relay_peers GUARDED_BY(cs_main) = 0;

    /** Number of outbound peers with m_chain_sync.m_protect. */
    int m_outbound_peers_with_protect_from_disconnect GUARDED_BY(cs_main) = 0;

    bool AlreadyHaveTx(const GenTxid& gtxid) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * Filter for transactions that were recently rejected by
     * AcceptToMemoryPool. These are not rerequested until the chain tip
     * changes, at which point the entire filter is reset.
     *
     * Without this filter we'd be re-requesting txs from each of our peers,
     * increasing bandwidth consumption considerably. For instance, with 100
     * peers, half of which relay a tx we don't accept, that might be a 50x
     * bandwidth increase. A flooding attacker attempting to roll-over the
     * filter using minimum-sized, 60byte, transactions might manage to send
     * 1000/sec if we have fast peers, so we pick 120,000 to give our peers a
     * two minute window to send invs to us.
     *
     * Decreasing the false positive rate is fairly cheap, so we pick one in a
     * million to make it highly unlikely for users to have issues with this
     * filter.
     *
     * We typically only add wtxids to this filter. For non-segwit
     * transactions, the txid == wtxid, so this only prevents us from
     * re-downloading non-segwit transactions when communicating with
     * non-wtxidrelay peers -- which is important for avoiding malleation
     * attacks that could otherwise interfere with transaction relay from
     * non-wtxidrelay peers. For communicating with wtxidrelay peers, having
     * the reject filter store wtxids is exactly what we want to avoid
     * redownload of a rejected transaction.
     *
     * In cases where we can tell that a segwit transaction will fail
     * validation no matter the witness, we may add the txid of such
     * transaction to the filter as well. This can be helpful when
     * communicating with txid-relay peers or if we were to otherwise fetch a
     * transaction via txid (eg in our orphan handling).
     *
     * Memory used: 1.3 MB
     */
    std::unique_ptr<CRollingBloomFilter> recentRejects GUARDED_BY(cs_main);
    uint256 hashRecentRejectsChainTip GUARDED_BY(cs_main);

    /*
     * Filter for transactions that have been recently confirmed.
     * We use this to avoid requesting transactions that have already been
     * confirnmed.
     */
    Mutex m_recent_confirmed_transactions_mutex;
    std::unique_ptr<CRollingBloomFilter> m_recent_confirmed_transactions GUARDED_BY(m_recent_confirmed_transactions_mutex);

    /* Returns a bool indicating whether we requested this block.
     * Also used if a block was /not/ received and timed out or started with another peer
     */
    bool MarkBlockAsReceived(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /* Mark a block as in flight
     * Returns false, still setting pit, if the block was already in flight from the same peer
     * pit will only be valid as long as the same cs_main lock is being held
     */
    bool MarkBlockAsInFlight(NodeId nodeid, const uint256& hash, const CBlockIndex* pindex = nullptr, std::list<QueuedBlock>::iterator** pit = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    bool TipMayBeStale() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Update pindexLastCommonBlock and add not-in-flight missing successors to vBlocks, until it has
     *  at most count entries.
     */
    void FindNextBlocksToDownload(NodeId nodeid, unsigned int count, std::vector<const CBlockIndex*>& vBlocks, NodeId& nodeStaller) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> > mapBlocksInFlight GUARDED_BY(cs_main);

    /** When our tip was last updated. */
    std::atomic<int64_t> m_last_tip_update{0};

    /** Determine whether or not a peer can request a transaction, and return it (or nullptr if not found or not allowed). */
    CTransactionRef FindTxForGetData(const CNode& peer, const GenTxid& gtxid, const std::chrono::seconds mempool_req, const std::chrono::seconds now) LOCKS_EXCLUDED(cs_main);

    void ProcessGetData(CNode& pfrom, Peer& peer, const std::atomic<bool>& interruptMsgProc) EXCLUSIVE_LOCKS_REQUIRED(!cs_main, peer.m_getdata_requests_mutex);

    /** Relay map (txid or wtxid -> CTransactionRef) */
    typedef std::map<uint256, CTransactionRef> MapRelay;
    MapRelay mapRelay GUARDED_BY(cs_main);
    /** Expiration-time ordered list of (expire time, relay map entry) pairs. */
    std::deque<std::pair<std::chrono::microseconds, MapRelay::iterator>> g_relay_expiration GUARDED_BY(cs_main);

    /**
     * When a peer sends us a valid block, instruct it to announce blocks to us
     * using CMPCTBLOCK if possible by adding its nodeid to the end of
     * lNodesAnnouncingHeaderAndIDs, and keeping that list under a certain size by
     * removing the first element if necessary.
     */
    void MaybeSetPeerAsAnnouncingHeaderAndIDs(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Stack of nodes which we have set to announce using compact blocks */
    std::list<NodeId> lNodesAnnouncingHeaderAndIDs GUARDED_BY(cs_main);

    /** Number of peers from which we're downloading blocks. */
    int nPeersWithValidatedDownloads GUARDED_BY(cs_main) = 0;

    /** Storage for orphan information */
    TxOrphanage m_orphanage;

    void AddToCompactExtraTransactions(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Orphan/conflicted/etc transactions that are kept for compact block reconstruction.
     *  The last -blockreconstructionextratxn/DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN of
     *  these are kept in a ring buffer */
    std::vector<std::pair<uint256, CTransactionRef>> vExtraTxnForCompact GUARDED_BY(g_cs_orphans);
    /** Offset into vExtraTxnForCompact to insert the next tx */
    size_t vExtraTxnForCompactIt GUARDED_BY(g_cs_orphans) = 0;

    void ProcessBlockAvailability(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void UpdateBlockAvailability(NodeId nodeid, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool CanDirectFetch() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool BlockRequestAllowed(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool AlreadyHaveBlock(const uint256& block_hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ProcessGetBlockData(CNode& pfrom, Peer& peer, const CInv& inv);
    bool PrepareBlockFilterRequest(CNode& peer,
                                   BlockFilterType filter_type, uint32_t start_height,
                                   const uint256& stop_hash, uint32_t max_height_diff,
                                   const CBlockIndex*& stop_index,
                                   BlockFilterIndex*& filter_index);
    void ProcessGetCFilters(CNode& peer, CDataStream& vRecv);
    void ProcessGetCFHeaders(CNode& peer, CDataStream& vRecv);
    void ProcessGetCFCheckPt(CNode& peer, CDataStream& vRecv);
};
} // namespace

namespace {
    /** Number of preferable block download peers. */
    int nPreferredDownload GUARDED_BY(cs_main) = 0;
} // namespace

namespace {
/**
 * Maintain validation-specific state about nodes, protected by cs_main, instead
 * by CNode's own locks. This simplifies asynchronous operation, where
 * processing of incoming data is done after the ProcessMessage call returns,
 * and we're no longer holding the node's locks.
 */
struct CNodeState {
    //! The best known block we know this peer has announced.
    const CBlockIndex* pindexBestKnownBlock{nullptr};
    //! The hash of the last unknown block this peer has announced.
    uint256 hashLastUnknownBlock{};
    //! The last full block we both have.
    const CBlockIndex* pindexLastCommonBlock{nullptr};
    //! The best header we have sent our peer.
    const CBlockIndex* pindexBestHeaderSent{nullptr};
    //! Length of current-streak of unconnecting headers announcements
    int nUnconnectingHeaders{0};
    //! Whether we've started headers synchronization with this peer.
    bool fSyncStarted{false};
    //! When to potentially disconnect peer for stalling headers download
    std::chrono::microseconds m_headers_sync_timeout{0us};
    //! Since when we're stalling block download progress (in microseconds), or 0.
    std::chrono::microseconds m_stalling_since{0us};
    std::list<QueuedBlock> vBlocksInFlight;
    //! When the first entry in vBlocksInFlight started downloading. Don't care when vBlocksInFlight is empty.
    std::chrono::microseconds m_downloading_since{0us};
    int nBlocksInFlight{0};
    int nBlocksInFlightValidHeaders{0};
    //! Whether we consider this a preferred download peer.
    bool fPreferredDownload{false};
    //! Whether this peer wants invs or headers (when possible) for block announcements.
    bool fPreferHeaders{false};
    //! Whether this peer wants invs or cmpctblocks (when possible) for block announcements.
    bool fPreferHeaderAndIDs{false};
    /**
      * Whether this peer will send us cmpctblocks if we request them.
      * This is not used to gate request logic, as we really only care about fSupportsDesiredCmpctVersion,
      * but is used as a flag to "lock in" the version of compact blocks (fWantsCmpctWitness) we send.
      */
    bool fProvidesHeaderAndIDs{false};
    //! Whether this peer can give us witnesses
    bool fHaveWitness{false};
    //! Whether this peer wants witnesses in cmpctblocks/blocktxns
    bool fWantsCmpctWitness{false};
    /**
     * If we've announced NODE_WITNESS to this peer: whether the peer sends witnesses in cmpctblocks/blocktxns,
     * otherwise: whether this peer sends non-witnesses in cmpctblocks/blocktxns.
     */
    bool fSupportsDesiredCmpctVersion{false};

    /** State used to enforce CHAIN_SYNC_TIMEOUT and EXTRA_PEER_CHECK_INTERVAL logic.
      *
      * Both are only in effect for outbound, non-manual, non-protected connections.
      * Any peer protected (m_protect = true) is not chosen for eviction. A peer is
      * marked as protected if all of these are true:
      *   - its connection type is IsBlockOnlyConn() == false
      *   - it gave us a valid connecting header
      *   - we haven't reached MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT yet
      *   - its chain tip has at least as much work as ours
      *
      * CHAIN_SYNC_TIMEOUT: if a peer's best known block has less work than our tip,
      * set a timeout CHAIN_SYNC_TIMEOUT seconds in the future:
      *   - If at timeout their best known block now has more work than our tip
      *     when the timeout was set, then either reset the timeout or clear it
      *     (after comparing against our current tip's work)
      *   - If at timeout their best known block still has less work than our
      *     tip did when the timeout was set, then send a getheaders message,
      *     and set a shorter timeout, HEADERS_RESPONSE_TIME seconds in future.
      *     If their best known block is still behind when that new timeout is
      *     reached, disconnect.
      *
      * EXTRA_PEER_CHECK_INTERVAL: after each interval, if we have too many outbound peers,
      * drop the outbound one that least recently announced us a new block.
      */
    struct ChainSyncTimeoutState {
        //! A timeout used for checking whether our peer has sufficiently synced
        int64_t m_timeout{0};
        //! A header with the work we require on our peer's chain
        const CBlockIndex* m_work_header{nullptr};
        //! After timeout is reached, set to true after sending getheaders
        bool m_sent_getheaders{false};
        //! Whether this peer is protected from disconnection due to a bad/slow chain
        bool m_protect{false};
    };

    ChainSyncTimeoutState m_chain_sync;

    //! Time of last new block announcement
    int64_t m_last_block_announcement{0};

    //! Whether this peer is an inbound connection
    const bool m_is_inbound;

    //! A rolling bloom filter of all announced tx CInvs to this peer.
    CRollingBloomFilter m_recently_announced_invs = CRollingBloomFilter{INVENTORY_MAX_RECENT_RELAY, 0.000001};

    //! Whether this peer relays txs via wtxid
    bool m_wtxid_relay{false};

    CNodeState(bool is_inbound) : m_is_inbound(is_inbound) {}
};

/** Map maintaining per-node state. */
static std::map<NodeId, CNodeState> mapNodeState GUARDED_BY(cs_main);

static CNodeState *State(NodeId pnode) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    std::map<NodeId, CNodeState>::iterator it = mapNodeState.find(pnode);
    if (it == mapNodeState.end())
        return nullptr;
    return &it->second;
}

static void UpdatePreferredDownload(const CNode& node, CNodeState* state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    nPreferredDownload -= state->fPreferredDownload;

    // Whether this node should be marked as a preferred download node.
    state->fPreferredDownload = (!node.IsInboundConn() || node.HasPermission(PF_NOBAN)) && !node.IsAddrFetchConn() && !node.fClient;

    nPreferredDownload += state->fPreferredDownload;
}

bool PeerManagerImpl::MarkBlockAsReceived(const uint256& hash)
{
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator itInFlight = mapBlocksInFlight.find(hash);
    if (itInFlight != mapBlocksInFlight.end()) {
        CNodeState *state = State(itInFlight->second.first);
        assert(state != nullptr);
        state->nBlocksInFlightValidHeaders -= itInFlight->second.second->fValidatedHeaders;
        if (state->nBlocksInFlightValidHeaders == 0 && itInFlight->second.second->fValidatedHeaders) {
            // Last validated block on the queue was received.
            nPeersWithValidatedDownloads--;
        }
        if (state->vBlocksInFlight.begin() == itInFlight->second.second) {
            // First block on the queue was received, update the start download time for the next one
            state->m_downloading_since = std::max(state->m_downloading_since, GetTime<std::chrono::microseconds>());
        }
        state->vBlocksInFlight.erase(itInFlight->second.second);
        state->nBlocksInFlight--;
        state->m_stalling_since = 0us;
        mapBlocksInFlight.erase(itInFlight);
        return true;
    }
    return false;
}

bool PeerManagerImpl::MarkBlockAsInFlight(NodeId nodeid, const uint256& hash, const CBlockIndex* pindex, std::list<QueuedBlock>::iterator** pit)
{
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    // Short-circuit most stuff in case it is from the same node
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator itInFlight = mapBlocksInFlight.find(hash);
    if (itInFlight != mapBlocksInFlight.end() && itInFlight->second.first == nodeid) {
        if (pit) {
            *pit = &itInFlight->second.second;
        }
        return false;
    }

    // Make sure it's not listed somewhere already.
    MarkBlockAsReceived(hash);

    std::list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(),
            {hash, pindex, pindex != nullptr, std::unique_ptr<PartiallyDownloadedBlock>(pit ? new PartiallyDownloadedBlock(&m_mempool) : nullptr)});
    state->nBlocksInFlight++;
    state->nBlocksInFlightValidHeaders += it->fValidatedHeaders;
    if (state->nBlocksInFlight == 1) {
        // We're starting a block download (batch) from this peer.
        state->m_downloading_since = GetTime<std::chrono::microseconds>();
    }
    if (state->nBlocksInFlightValidHeaders == 1 && pindex != nullptr) {
        nPeersWithValidatedDownloads++;
    }
    itInFlight = mapBlocksInFlight.insert(std::make_pair(hash, std::make_pair(nodeid, it))).first;
    if (pit)
        *pit = &itInFlight->second.second;
    return true;
}

void PeerManagerImpl::MaybeSetPeerAsAnnouncingHeaderAndIDs(NodeId nodeid)
{
    AssertLockHeld(cs_main);
    CNodeState* nodestate = State(nodeid);
    if (!nodestate || !nodestate->fSupportsDesiredCmpctVersion) {
        // Never ask from peers who can't provide witnesses.
        return;
    }
    if (nodestate->fProvidesHeaderAndIDs) {
        for (std::list<NodeId>::iterator it = lNodesAnnouncingHeaderAndIDs.begin(); it != lNodesAnnouncingHeaderAndIDs.end(); it++) {
            if (*it == nodeid) {
                lNodesAnnouncingHeaderAndIDs.erase(it);
                lNodesAnnouncingHeaderAndIDs.push_back(nodeid);
                return;
            }
        }
        m_connman.ForNode(nodeid, [this](CNode* pfrom) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);
            uint64_t nCMPCTBLOCKVersion = (pfrom->GetLocalServices() & NODE_WITNESS) ? 2 : 1;
            if (lNodesAnnouncingHeaderAndIDs.size() >= 3) {
                // As per BIP152, we only get 3 of our peers to announce
                // blocks using compact encodings.
                m_connman.ForNode(lNodesAnnouncingHeaderAndIDs.front(), [this, nCMPCTBLOCKVersion](CNode* pnodeStop){
                    m_connman.PushMessage(pnodeStop, CNetMsgMaker(pnodeStop->GetCommonVersion()).Make(NetMsgType::SENDCMPCT, /*fAnnounceUsingCMPCTBLOCK=*/false, nCMPCTBLOCKVersion));
                    // save BIP152 bandwidth state: we select peer to be low-bandwidth
                    pnodeStop->m_bip152_highbandwidth_to = false;
                    return true;
                });
                lNodesAnnouncingHeaderAndIDs.pop_front();
            }
            m_connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetCommonVersion()).Make(NetMsgType::SENDCMPCT, /*fAnnounceUsingCMPCTBLOCK=*/true, nCMPCTBLOCKVersion));
            // save BIP152 bandwidth state: we select peer to be high-bandwidth
            pfrom->m_bip152_highbandwidth_to = true;
            lNodesAnnouncingHeaderAndIDs.push_back(pfrom->GetId());
            return true;
        });
    }
}

bool PeerManagerImpl::TipMayBeStale()
{
    AssertLockHeld(cs_main);
    const Consensus::Params& consensusParams = m_chainparams.GetConsensus();
    if (m_last_tip_update == 0) {
        m_last_tip_update = GetTime();
    }
    return m_last_tip_update < GetTime() - consensusParams.nPowTargetSpacing * 3 && mapBlocksInFlight.empty();
}

bool PeerManagerImpl::CanDirectFetch()
{
    return m_chainman.ActiveChain().Tip()->GetBlockTime() > GetAdjustedTime() - m_chainparams.GetConsensus().nPowTargetSpacing * 20;
}

static bool PeerHasHeader(CNodeState *state, const CBlockIndex *pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (state->pindexBestKnownBlock && pindex == state->pindexBestKnownBlock->GetAncestor(pindex->nHeight))
        return true;
    if (state->pindexBestHeaderSent && pindex == state->pindexBestHeaderSent->GetAncestor(pindex->nHeight))
        return true;
    return false;
}

/** Check whether the last unknown block a peer advertised is not yet known. */
void PeerManagerImpl::ProcessBlockAvailability(NodeId nodeid) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    if (!state->hashLastUnknownBlock.IsNull()) {
        const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(state->hashLastUnknownBlock);
        if (pindex && pindex->nChainWork > 0) {
            if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
                state->pindexBestKnownBlock = pindex;
            }
            state->hashLastUnknownBlock.SetNull();
        }
    }
}

/** Update tracking information about which blocks a peer is assumed to have. */
void PeerManagerImpl::UpdateBlockAvailability(NodeId nodeid, const uint256 &hash) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    ProcessBlockAvailability(nodeid);

    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(hash);
    if (pindex && pindex->nChainWork > 0) {
        // An actually better block was announced.
        if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
            state->pindexBestKnownBlock = pindex;
        }
    } else {
        // An unknown block was announced; just assume that the latest one is the best one.
        state->hashLastUnknownBlock = hash;
    }
}

void PeerManagerImpl::FindNextBlocksToDownload(NodeId nodeid, unsigned int count, std::vector<const CBlockIndex*>& vBlocks, NodeId& nodeStaller)
{
    if (count == 0)
        return;

    vBlocks.reserve(vBlocks.size() + count);
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    // Make sure pindexBestKnownBlock is up to date, we'll need it.
    ProcessBlockAvailability(nodeid);

    if (state->pindexBestKnownBlock == nullptr || state->pindexBestKnownBlock->nChainWork < m_chainman.ActiveChain().Tip()->nChainWork || state->pindexBestKnownBlock->nChainWork < nMinimumChainWork) {
        // This peer has nothing interesting.
        return;
    }

    if (state->pindexLastCommonBlock == nullptr) {
        // Bootstrap quickly by guessing a parent of our best tip is the forking point.
        // Guessing wrong in either direction is not a problem.
        state->pindexLastCommonBlock = m_chainman.ActiveChain()[std::min(state->pindexBestKnownBlock->nHeight, m_chainman.ActiveChain().Height())];
    }

    // If the peer reorganized, our previous pindexLastCommonBlock may not be an ancestor
    // of its current tip anymore. Go back enough to fix that.
    state->pindexLastCommonBlock = LastCommonAncestor(state->pindexLastCommonBlock, state->pindexBestKnownBlock);
    if (state->pindexLastCommonBlock == state->pindexBestKnownBlock)
        return;

    const Consensus::Params& consensusParams = m_chainparams.GetConsensus();
    std::vector<const CBlockIndex*> vToFetch;
    const CBlockIndex *pindexWalk = state->pindexLastCommonBlock;
    // Never fetch further than the best block we know the peer has, or more than BLOCK_DOWNLOAD_WINDOW + 1 beyond the last
    // linked block we have in common with this peer. The +1 is so we can detect stalling, namely if we would be able to
    // download that next block if the window were 1 larger.
    int nWindowEnd = state->pindexLastCommonBlock->nHeight + BLOCK_DOWNLOAD_WINDOW;
    int nMaxHeight = std::min<int>(state->pindexBestKnownBlock->nHeight, nWindowEnd + 1);
    NodeId waitingfor = -1;
    while (pindexWalk->nHeight < nMaxHeight) {
        // Read up to 128 (or more, if more blocks than that are needed) successors of pindexWalk (towards
        // pindexBestKnownBlock) into vToFetch. We fetch 128, because CBlockIndex::GetAncestor may be as expensive
        // as iterating over ~100 CBlockIndex* entries anyway.
        int nToFetch = std::min(nMaxHeight - pindexWalk->nHeight, std::max<int>(count - vBlocks.size(), 128));
        vToFetch.resize(nToFetch);
        pindexWalk = state->pindexBestKnownBlock->GetAncestor(pindexWalk->nHeight + nToFetch);
        vToFetch[nToFetch - 1] = pindexWalk;
        for (unsigned int i = nToFetch - 1; i > 0; i--) {
            vToFetch[i - 1] = vToFetch[i]->pprev;
        }

        // Iterate over those blocks in vToFetch (in forward direction), adding the ones that
        // are not yet downloaded and not in flight to vBlocks. In the meantime, update
        // pindexLastCommonBlock as long as all ancestors are already downloaded, or if it's
        // already part of our chain (and therefore don't need it even if pruned).
        for (const CBlockIndex* pindex : vToFetch) {
            if (!pindex->IsValid(BLOCK_VALID_TREE)) {
                // We consider the chain that this peer is on invalid.
                return;
            }
            if (!State(nodeid)->fHaveWitness && IsWitnessEnabled(pindex->pprev, consensusParams)) {
                // We wouldn't download this block or its descendants from this peer.
                return;
            }
            if (pindex->nStatus & BLOCK_HAVE_DATA || m_chainman.ActiveChain().Contains(pindex)) {
                if (pindex->HaveTxsDownloaded())
                    state->pindexLastCommonBlock = pindex;
            } else if (mapBlocksInFlight.count(pindex->GetBlockHash()) == 0) {
                // The block is not already downloaded, and not yet in flight.
                if (pindex->nHeight > nWindowEnd) {
                    // We reached the end of the window.
                    if (vBlocks.size() == 0 && waitingfor != nodeid) {
                        // We aren't able to fetch anything, but we would be if the download window was one larger.
                        nodeStaller = waitingfor;
                    }
                    return;
                }
                vBlocks.push_back(pindex);
                if (vBlocks.size() == count) {
                    return;
                }
            } else if (waitingfor == -1) {
                // This is the first already-in-flight block.
                waitingfor = mapBlocksInFlight[pindex->GetBlockHash()].first;
            }
        }
    }
}

} // namespace

void PeerManagerImpl::PushNodeVersion(CNode& pnode, int64_t nTime)
{
    // Note that pnode->GetLocalServices() is a reflection of the local
    // services we were offering when the CNode object was created for this
    // peer.
    ServiceFlags nLocalNodeServices = pnode.GetLocalServices();
    uint64_t nonce = pnode.GetLocalNonce();
    const int nNodeStartingHeight{m_best_height};
    NodeId nodeid = pnode.GetId();
    CAddress addr = pnode.addr;

    CAddress addrYou = addr.IsRoutable() && !IsProxy(addr) && addr.IsAddrV1Compatible() ?
                           addr :
                           CAddress(CService(), addr.nServices);
    CAddress addrMe = CAddress(CService(), nLocalNodeServices);

    const bool tx_relay = !m_ignore_incoming_txs && pnode.m_tx_relay != nullptr;
    m_connman.PushMessage(&pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::VERSION, PROTOCOL_VERSION, (uint64_t)nLocalNodeServices, nTime, addrYou, addrMe,
            nonce, strSubVersion, nNodeStartingHeight, tx_relay));

    if (fLogIPs) {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, them=%s, txrelay=%d, peer=%d\n", PROTOCOL_VERSION, nNodeStartingHeight, addrMe.ToString(), addrYou.ToString(), tx_relay, nodeid);
    } else {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, txrelay=%d, peer=%d\n", PROTOCOL_VERSION, nNodeStartingHeight, addrMe.ToString(), tx_relay, nodeid);
    }
}

void PeerManagerImpl::AddTxAnnouncement(const CNode& node, const GenTxid& gtxid, std::chrono::microseconds current_time)
{
    AssertLockHeld(::cs_main); // For m_txrequest
    NodeId nodeid = node.GetId();
    if (!node.HasPermission(PF_RELAY) && m_txrequest.Count(nodeid) >= MAX_PEER_TX_ANNOUNCEMENTS) {
        // Too many queued announcements from this peer
        return;
    }
    const CNodeState* state = State(nodeid);

    // Decide the TxRequestTracker parameters for this announcement:
    // - "preferred": if fPreferredDownload is set (= outbound, or PF_NOBAN permission)
    // - "reqtime": current time plus delays for:
    //   - NONPREF_PEER_TX_DELAY for announcements from non-preferred connections
    //   - TXID_RELAY_DELAY for txid announcements while wtxid peers are available
    //   - OVERLOADED_PEER_TX_DELAY for announcements from peers which have at least
    //     MAX_PEER_TX_REQUEST_IN_FLIGHT requests in flight (and don't have PF_RELAY).
    auto delay = std::chrono::microseconds{0};
    const bool preferred = state->fPreferredDownload;
    if (!preferred) delay += NONPREF_PEER_TX_DELAY;
    if (!gtxid.IsWtxid() && m_wtxid_relay_peers > 0) delay += TXID_RELAY_DELAY;
    const bool overloaded = !node.HasPermission(PF_RELAY) &&
        m_txrequest.CountInFlight(nodeid) >= MAX_PEER_TX_REQUEST_IN_FLIGHT;
    if (overloaded) delay += OVERLOADED_PEER_TX_DELAY;
    m_txrequest.ReceivedInv(nodeid, gtxid, preferred, current_time + delay);
}

// This function is used for testing the stale tip eviction logic, see
// denialofservice_tests.cpp
void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds)
{
    LOCK(cs_main);
    CNodeState *state = State(node);
    if (state) state->m_last_block_announcement = time_in_seconds;
}

void PeerManagerImpl::InitializeNode(CNode *pnode)
{
    NodeId nodeid = pnode->GetId();
    {
        LOCK(cs_main);
        mapNodeState.emplace_hint(mapNodeState.end(), std::piecewise_construct, std::forward_as_tuple(nodeid), std::forward_as_tuple(pnode->IsInboundConn()));
        assert(m_txrequest.Count(nodeid) == 0);
    }
    {
        PeerRef peer = std::make_shared<Peer>(nodeid);
        LOCK(m_peer_mutex);
        m_peer_map.emplace_hint(m_peer_map.end(), nodeid, std::move(peer));
    }
    if (!pnode->IsInboundConn()) {
        PushNodeVersion(*pnode, GetTime());
    }
}

void PeerManagerImpl::ReattemptInitialBroadcast(CScheduler& scheduler)
{
    std::set<uint256> unbroadcast_txids = m_mempool.GetUnbroadcastTxs();

    for (const auto& txid : unbroadcast_txids) {
        CTransactionRef tx = m_mempool.get(txid);

        if (tx != nullptr) {
            LOCK(cs_main);
            RelayTransaction(txid, tx->GetWitnessHash());
        } else {
            m_mempool.RemoveUnbroadcastTx(txid, true);
        }
    }

    // Schedule next run for 10-15 minutes in the future.
    // We add randomness on every cycle to avoid the possibility of P2P fingerprinting.
    const std::chrono::milliseconds delta = std::chrono::minutes{10} + GetRandMillis(std::chrono::minutes{5});
    scheduler.scheduleFromNow([&] { ReattemptInitialBroadcast(scheduler); }, delta);
}

void PeerManagerImpl::FinalizeNode(const CNode& node)
{
    NodeId nodeid = node.GetId();
    int misbehavior{0};
    {
    LOCK(cs_main);
    {
        // We remove the PeerRef from g_peer_map here, but we don't always
        // destruct the Peer. Sometimes another thread is still holding a
        // PeerRef, so the refcount is >= 1. Be careful not to do any
        // processing here that assumes Peer won't be changed before it's
        // destructed.
        PeerRef peer = RemovePeer(nodeid);
        assert(peer != nullptr);
        misbehavior = WITH_LOCK(peer->m_misbehavior_mutex, return peer->m_misbehavior_score);
    }
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    if (state->fSyncStarted)
        nSyncStarted--;

    for (const QueuedBlock& entry : state->vBlocksInFlight) {
        mapBlocksInFlight.erase(entry.hash);
    }
    WITH_LOCK(g_cs_orphans, m_orphanage.EraseForPeer(nodeid));
    m_txrequest.DisconnectedPeer(nodeid);
    nPreferredDownload -= state->fPreferredDownload;
    nPeersWithValidatedDownloads -= (state->nBlocksInFlightValidHeaders != 0);
    assert(nPeersWithValidatedDownloads >= 0);
    m_outbound_peers_with_protect_from_disconnect -= state->m_chain_sync.m_protect;
    assert(m_outbound_peers_with_protect_from_disconnect >= 0);
    m_wtxid_relay_peers -= state->m_wtxid_relay;
    assert(m_wtxid_relay_peers >= 0);

    mapNodeState.erase(nodeid);

    if (mapNodeState.empty()) {
        // Do a consistency check after the last peer is removed.
        assert(mapBlocksInFlight.empty());
        assert(nPreferredDownload == 0);
        assert(nPeersWithValidatedDownloads == 0);
        assert(m_outbound_peers_with_protect_from_disconnect == 0);
        assert(m_wtxid_relay_peers == 0);
        assert(m_txrequest.Size() == 0);
    }
    } // cs_main
    if (node.fSuccessfullyConnected && misbehavior == 0 &&
        !node.IsBlockOnlyConn() && !node.IsInboundConn()) {
        // Only change visible addrman state for full outbound peers.  We don't
        // call Connected() for feeler connections since they don't have
        // fSuccessfullyConnected set.
        m_addrman.Connected(node.addr);
    }
    LogPrint(BCLog::NET, "Cleared nodestate for peer=%d\n", nodeid);
}

PeerRef PeerManagerImpl::GetPeerRef(NodeId id) const
{
    LOCK(m_peer_mutex);
    auto it = m_peer_map.find(id);
    return it != m_peer_map.end() ? it->second : nullptr;
}

PeerRef PeerManagerImpl::RemovePeer(NodeId id)
{
    PeerRef ret;
    LOCK(m_peer_mutex);
    auto it = m_peer_map.find(id);
    if (it != m_peer_map.end()) {
        ret = std::move(it->second);
        m_peer_map.erase(it);
    }
    return ret;
}

bool PeerManagerImpl::GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats)
{
    {
        LOCK(cs_main);
        CNodeState* state = State(nodeid);
        if (state == nullptr)
            return false;
        stats.nSyncHeight = state->pindexBestKnownBlock ? state->pindexBestKnownBlock->nHeight : -1;
        stats.nCommonHeight = state->pindexLastCommonBlock ? state->pindexLastCommonBlock->nHeight : -1;
        for (const QueuedBlock& queue : state->vBlocksInFlight) {
            if (queue.pindex)
                stats.vHeightInFlight.push_back(queue.pindex->nHeight);
        }
    }

    PeerRef peer = GetPeerRef(nodeid);
    if (peer == nullptr) return false;
    stats.m_starting_height = peer->m_starting_height;
    // It is common for nodes with good ping times to suddenly become lagged,
    // due to a new block arriving or other large transfer.
    // Merely reporting pingtime might fool the caller into thinking the node was still responsive,
    // since pingtime does not update until the ping is complete, which might take a while.
    // So, if a ping is taking an unusually long time in flight,
    // the caller can immediately detect that this is happening.
    std::chrono::microseconds ping_wait{0};
    if ((0 != peer->m_ping_nonce_sent) && (0 != peer->m_ping_start.load().count())) {
        ping_wait = GetTime<std::chrono::microseconds>() - peer->m_ping_start.load();
    }

    stats.m_ping_wait = ping_wait;

    return true;
}

void PeerManagerImpl::AddToCompactExtraTransactions(const CTransactionRef& tx)
{
    size_t max_extra_txn = gArgs.GetArg("-blockreconstructionextratxn", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN);
    if (max_extra_txn <= 0)
        return;
    if (!vExtraTxnForCompact.size())
        vExtraTxnForCompact.resize(max_extra_txn);
    vExtraTxnForCompact[vExtraTxnForCompactIt] = std::make_pair(tx->GetWitnessHash(), tx);
    vExtraTxnForCompactIt = (vExtraTxnForCompactIt + 1) % max_extra_txn;
}

void PeerManagerImpl::Misbehaving(const NodeId pnode, const int howmuch, const std::string& message)
{
    assert(howmuch > 0);

    PeerRef peer = GetPeerRef(pnode);
    if (peer == nullptr) return;

    LOCK(peer->m_misbehavior_mutex);
    peer->m_misbehavior_score += howmuch;
    const std::string message_prefixed = message.empty() ? "" : (": " + message);
    if (peer->m_misbehavior_score >= DISCOURAGEMENT_THRESHOLD && peer->m_misbehavior_score - howmuch < DISCOURAGEMENT_THRESHOLD) {
        LogPrint(BCLog::NET, "Misbehaving: peer=%d (%d -> %d) DISCOURAGE THRESHOLD EXCEEDED%s\n", pnode, peer->m_misbehavior_score - howmuch, peer->m_misbehavior_score, message_prefixed);
        peer->m_should_discourage = true;
    } else {
        LogPrint(BCLog::NET, "Misbehaving: peer=%d (%d -> %d)%s\n", pnode, peer->m_misbehavior_score - howmuch, peer->m_misbehavior_score, message_prefixed);
    }
}

bool PeerManagerImpl::MaybePunishNodeForBlock(NodeId nodeid, const BlockValidationState& state,
                                              bool via_compact_block, const std::string& message)
{
    switch (state.GetResult()) {
    case BlockValidationResult::BLOCK_RESULT_UNSET:
        break;
    // The node is providing invalid data:
    case BlockValidationResult::BLOCK_CONSENSUS:
    case BlockValidationResult::BLOCK_MUTATED:
        if (!via_compact_block) {
            Misbehaving(nodeid, 100, message);
            return true;
        }
        break;
    case BlockValidationResult::BLOCK_CACHED_INVALID:
        {
            LOCK(cs_main);
            CNodeState *node_state = State(nodeid);
            if (node_state == nullptr) {
                break;
            }

            // Discourage outbound (but not inbound) peers if on an invalid chain.
            // Exempt HB compact block peers. Manual connections are always protected from discouragement.
            if (!via_compact_block && !node_state->m_is_inbound) {
                Misbehaving(nodeid, 100, message);
                return true;
            }
            break;
        }
    case BlockValidationResult::BLOCK_INVALID_HEADER:
    case BlockValidationResult::BLOCK_CHECKPOINT:
    case BlockValidationResult::BLOCK_INVALID_PREV:
        Misbehaving(nodeid, 100, message);
        return true;
    // Conflicting (but not necessarily invalid) data or different policy:
    case BlockValidationResult::BLOCK_MISSING_PREV:
        // TODO: Handle this much more gracefully (10 DoS points is super arbitrary)
        Misbehaving(nodeid, 10, message);
        return true;
    case BlockValidationResult::BLOCK_RECENT_CONSENSUS_CHANGE:
    case BlockValidationResult::BLOCK_TIME_FUTURE:
        break;
    }
    if (message != "") {
        LogPrint(BCLog::NET, "peer=%d: %s\n", nodeid, message);
    }
    return false;
}

bool PeerManagerImpl::MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message)
{
    switch (state.GetResult()) {
    case TxValidationResult::TX_RESULT_UNSET:
        break;
    // The node is providing invalid data:
    case TxValidationResult::TX_CONSENSUS:
        Misbehaving(nodeid, 100, message);
        return true;
    // Conflicting (but not necessarily invalid) data or different policy:
    case TxValidationResult::TX_RECENT_CONSENSUS_CHANGE:
    case TxValidationResult::TX_INPUTS_NOT_STANDARD:
    case TxValidationResult::TX_NOT_STANDARD:
    case TxValidationResult::TX_MISSING_INPUTS:
    case TxValidationResult::TX_PREMATURE_SPEND:
    case TxValidationResult::TX_WITNESS_MUTATED:
    case TxValidationResult::TX_WITNESS_STRIPPED:
    case TxValidationResult::TX_CONFLICT:
    case TxValidationResult::TX_MEMPOOL_POLICY:
        break;
    }
    if (message != "") {
        LogPrint(BCLog::NET, "peer=%d: %s\n", nodeid, message);
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////////
//
// blockchain -> download logic notification
//

// To prevent fingerprinting attacks, only send blocks/headers outside of the
// active chain if they are no more than a month older (both in time, and in
// best equivalent proof of work) than the best header chain we know about and
// we fully-validated them at some point.
bool PeerManagerImpl::BlockRequestAllowed(const CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);
    if (m_chainman.ActiveChain().Contains(pindex)) return true;
    return pindex->IsValid(BLOCK_VALID_SCRIPTS) && (pindexBestHeader != nullptr) &&
           (pindexBestHeader->GetBlockTime() - pindex->GetBlockTime() < STALE_RELAY_AGE_LIMIT) &&
           (GetBlockProofEquivalentTime(*pindexBestHeader, *pindex, *pindexBestHeader, m_chainparams.GetConsensus()) < STALE_RELAY_AGE_LIMIT);
}

std::unique_ptr<PeerManager> PeerManager::make(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman,
                                               BanMan* banman, CScheduler& scheduler, ChainstateManager& chainman,
                                               CTxMemPool& pool, bool ignore_incoming_txs)
{
    return std::make_unique<PeerManagerImpl>(chainparams, connman, addrman, banman, scheduler, chainman, pool, ignore_incoming_txs);
}

PeerManagerImpl::PeerManagerImpl(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman,
                                 BanMan* banman, CScheduler& scheduler, ChainstateManager& chainman,
                                 CTxMemPool& pool, bool ignore_incoming_txs)
    : m_chainparams(chainparams),
      m_connman(connman),
      m_addrman(addrman),
      m_banman(banman),
      m_chainman(chainman),
      m_mempool(pool),
      m_stale_tip_check_time(0),
      m_ignore_incoming_txs(ignore_incoming_txs)
{
    assert(std::addressof(g_chainman) == std::addressof(m_chainman));
    // Initialize global variables that cannot be constructed at startup.
    recentRejects.reset(new CRollingBloomFilter(120000, 0.000001));

    // Blocks don't typically have more than 4000 transactions, so this should
    // be at least six blocks (~1 hr) worth of transactions that we can store,
    // inserting both a txid and wtxid for every observed transaction.
    // If the number of transactions appearing in a block goes up, or if we are
    // seeing getdata requests more than an hour after initial announcement, we
    // can increase this number.
    // The false positive rate of 1/1M should come out to less than 1
    // transaction per day that would be inadvertently ignored (which is the
    // same probability that we have in the reject filter).
    m_recent_confirmed_transactions.reset(new CRollingBloomFilter(48000, 0.000001));

    // Stale tip checking and peer eviction are on two different timers, but we
    // don't want them to get out of sync due to drift in the scheduler, so we
    // combine them in one function and schedule at the quicker (peer-eviction)
    // timer.
    static_assert(EXTRA_PEER_CHECK_INTERVAL < STALE_CHECK_INTERVAL, "peer eviction timer should be less than stale tip check timer");
    scheduler.scheduleEvery([this] { this->CheckForStaleTipAndEvictPeers(); }, std::chrono::seconds{EXTRA_PEER_CHECK_INTERVAL});

    // schedule next run for 10-15 minutes in the future
    const std::chrono::milliseconds delta = std::chrono::minutes{10} + GetRandMillis(std::chrono::minutes{5});
    scheduler.scheduleFromNow([&] { ReattemptInitialBroadcast(scheduler); }, delta);
}

/**
 * Evict orphan txn pool entries based on a newly connected
 * block, remember the recently confirmed transactions, and delete tracked
 * announcements for them. Also save the time of the last tip update.
 */
void PeerManagerImpl::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    m_orphanage.EraseForBlock(*pblock);
    m_last_tip_update = GetTime();

    {
        LOCK(m_recent_confirmed_transactions_mutex);
        for (const auto& ptx : pblock->vtx) {
            m_recent_confirmed_transactions->insert(ptx->GetHash());
            if (ptx->GetHash() != ptx->GetWitnessHash()) {
                m_recent_confirmed_transactions->insert(ptx->GetWitnessHash());
            }
        }
    }
    {
        LOCK(cs_main);
        for (const auto& ptx : pblock->vtx) {
            m_txrequest.ForgetTxHash(ptx->GetHash());
            m_txrequest.ForgetTxHash(ptx->GetWitnessHash());
        }
    }
}

void PeerManagerImpl::BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex* pindex)
{
    // To avoid relay problems with transactions that were previously
    // confirmed, clear our filter of recently confirmed transactions whenever
    // there's a reorg.
    // This means that in a 1-block reorg (where 1 block is disconnected and
    // then another block reconnected), our filter will drop to having only one
    // block's worth of transactions in it, but that should be fine, since
    // presumably the most common case of relaying a confirmed transaction
    // should be just after a new block containing it is found.
    LOCK(m_recent_confirmed_transactions_mutex);
    m_recent_confirmed_transactions->reset();
}

// All of the following cache a recent block, and are protected by cs_most_recent_block
static RecursiveMutex cs_most_recent_block;
static std::shared_ptr<const CBlock> most_recent_block GUARDED_BY(cs_most_recent_block);
static std::shared_ptr<const CBlockHeaderAndShortTxIDs> most_recent_compact_block GUARDED_BY(cs_most_recent_block);
static uint256 most_recent_block_hash GUARDED_BY(cs_most_recent_block);
static bool fWitnessesPresentInMostRecentCompactBlock GUARDED_BY(cs_most_recent_block);

/**
 * Maintain state about the best-seen block and fast-announce a compact block
 * to compatible peers.
 */
void PeerManagerImpl::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock)
{
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> pcmpctblock = std::make_shared<const CBlockHeaderAndShortTxIDs> (*pblock, true);
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);

    LOCK(cs_main);

    static int nHighestFastAnnounce = 0;
    if (pindex->nHeight <= nHighestFastAnnounce)
        return;
    nHighestFastAnnounce = pindex->nHeight;

    bool fWitnessEnabled = IsWitnessEnabled(pindex->pprev, m_chainparams.GetConsensus());
    uint256 hashBlock(pblock->GetHash());

    {
        LOCK(cs_most_recent_block);
        most_recent_block_hash = hashBlock;
        most_recent_block = pblock;
        most_recent_compact_block = pcmpctblock;
        fWitnessesPresentInMostRecentCompactBlock = fWitnessEnabled;
    }

    m_connman.ForEachNode([this, &pcmpctblock, pindex, &msgMaker, fWitnessEnabled, &hashBlock](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        AssertLockHeld(::cs_main);

        // TODO: Avoid the repeated-serialization here
        if (pnode->GetCommonVersion() < INVALID_CB_NO_BAN_VERSION || pnode->fDisconnect)
            return;
        ProcessBlockAvailability(pnode->GetId());
        CNodeState &state = *State(pnode->GetId());
        // If the peer has, or we announced to them the previous block already,
        // but we don't think they have this one, go ahead and announce it
        if (state.fPreferHeaderAndIDs && (!fWitnessEnabled || state.fWantsCmpctWitness) &&
                !PeerHasHeader(&state, pindex) && PeerHasHeader(&state, pindex->pprev)) {

            LogPrint(BCLog::NET, "%s sending header-and-ids %s to peer=%d\n", "PeerManager::NewPoWValidBlock",
                    hashBlock.ToString(), pnode->GetId());
            m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::CMPCTBLOCK, *pcmpctblock));
            state.pindexBestHeaderSent = pindex;
        }
    });
}

/**
 * Update our best height and announce any block hashes which weren't previously
 * in m_chainman.ActiveChain() to our peers.
 */
void PeerManagerImpl::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    SetBestHeight(pindexNew->nHeight);
    SetServiceFlagsIBDCache(!fInitialDownload);

    // Don't relay inventory during initial block download.
    if (fInitialDownload) return;

    // Find the hashes of all blocks that weren't previously in the best chain.
    std::vector<uint256> vHashes;
    const CBlockIndex *pindexToAnnounce = pindexNew;
    while (pindexToAnnounce != pindexFork) {
        vHashes.push_back(pindexToAnnounce->GetBlockHash());
        pindexToAnnounce = pindexToAnnounce->pprev;
        if (vHashes.size() == MAX_BLOCKS_TO_ANNOUNCE) {
            // Limit announcements in case of a huge reorganization.
            // Rely on the peer's synchronization mechanism in that case.
            break;
        }
    }

    {
        LOCK(m_peer_mutex);
        for (auto& it : m_peer_map) {
            Peer& peer = *it.second;
            LOCK(peer.m_block_inv_mutex);
            for (const uint256& hash : reverse_iterate(vHashes)) {
                peer.m_blocks_for_headers_relay.push_back(hash);
            }
        }
    }

    m_connman.WakeMessageHandler();
}

/**
 * Handle invalid block rejection and consequent peer discouragement, maintain which
 * peers announce compact blocks.
 */
void PeerManagerImpl::BlockChecked(const CBlock& block, const BlockValidationState& state)
{
    LOCK(cs_main);

    const uint256 hash(block.GetHash());
    std::map<uint256, std::pair<NodeId, bool>>::iterator it = mapBlockSource.find(hash);

    // If the block failed validation, we know where it came from and we're still connected
    // to that peer, maybe punish.
    if (state.IsInvalid() &&
        it != mapBlockSource.end() &&
        State(it->second.first)) {
            MaybePunishNodeForBlock(/*nodeid=*/ it->second.first, state, /*via_compact_block=*/ !it->second.second);
    }
    // Check that:
    // 1. The block is valid
    // 2. We're not in initial block download
    // 3. This is currently the best block we're aware of. We haven't updated
    //    the tip yet so we have no way to check this directly here. Instead we
    //    just check that there are currently no other blocks in flight.
    else if (state.IsValid() &&
             !m_chainman.ActiveChainstate().IsInitialBlockDownload() &&
             mapBlocksInFlight.count(hash) == mapBlocksInFlight.size()) {
        if (it != mapBlockSource.end()) {
            MaybeSetPeerAsAnnouncingHeaderAndIDs(it->second.first);
        }
    }
    if (it != mapBlockSource.end())
        mapBlockSource.erase(it);
}

//////////////////////////////////////////////////////////////////////////////
//
// Messages
//


bool PeerManagerImpl::AlreadyHaveTx(const GenTxid& gtxid)
{
    assert(recentRejects);
    if (m_chainman.ActiveChain().Tip()->GetBlockHash() != hashRecentRejectsChainTip) {
        // If the chain tip has changed previously rejected transactions
        // might be now valid, e.g. due to a nLockTime'd tx becoming valid,
        // or a double-spend. Reset the rejects filter and give those
        // txs a second chance.
        hashRecentRejectsChainTip = m_chainman.ActiveChain().Tip()->GetBlockHash();
        recentRejects->reset();
    }

    const uint256& hash = gtxid.GetHash();

    if (m_orphanage.HaveTx(gtxid)) return true;

    {
        LOCK(m_recent_confirmed_transactions_mutex);
        if (m_recent_confirmed_transactions->contains(hash)) return true;
    }

    return recentRejects->contains(hash) || m_mempool.exists(gtxid);
}

bool PeerManagerImpl::AlreadyHaveBlock(const uint256& block_hash)
{
    return m_chainman.m_blockman.LookupBlockIndex(block_hash) != nullptr;
}

void PeerManagerImpl::SendPings()
{
    LOCK(m_peer_mutex);
    for(auto& it : m_peer_map) it.second->m_ping_queued = true;
}

void PeerManagerImpl::RelayTransaction(const uint256& txid, const uint256& wtxid)
{
    m_connman.ForEachNode([&txid, &wtxid](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        AssertLockHeld(::cs_main);

        CNodeState* state = State(pnode->GetId());
        if (state == nullptr) return;
        if (state->m_wtxid_relay) {
            pnode->PushTxInventory(wtxid);
        } else {
            pnode->PushTxInventory(txid);
        }
    });
}

/**
 * Relay (gossip) an address to a few randomly chosen nodes.
 * We choose the same nodes within a given 24h window (if the list of connected
 * nodes does not change) and we don't relay to nodes that already know an
 * address. So within 24h we will likely relay a given address once. This is to
 * prevent a peer from unjustly giving their address better propagation by sending
 * it to us repeatedly.
 * @param[in] originator The peer that sent us the address. We don't want to relay it back.
 * @param[in] addr Address to relay.
 * @param[in] fReachable Whether the address' network is reachable. We relay unreachable
 * addresses less.
 * @param[in] connman Connection manager to choose nodes to relay to.
 */
static void RelayAddress(const CNode& originator,
                         const CAddress& addr,
                         bool fReachable,
                         const CConnman& connman)
{
    if (!fReachable && !addr.IsRelayable()) return;

    // Relay to a limited number of other nodes
    // Use deterministic randomness to send to the same nodes for 24 hours
    // at a time so the m_addr_knowns of the chosen nodes prevent repeats
    uint64_t hashAddr = addr.GetHash();
    const CSipHasher hasher = connman.GetDeterministicRandomizer(RANDOMIZER_ID_ADDRESS_RELAY).Write(hashAddr << 32).Write((GetTime() + hashAddr) / (24 * 60 * 60));
    FastRandomContext insecure_rand;

    // Relay reachable addresses to 2 peers. Unreachable addresses are relayed randomly to 1 or 2 peers.
    unsigned int nRelayNodes = (fReachable || (hasher.Finalize() & 1)) ? 2 : 1;

    std::array<std::pair<uint64_t, CNode*>,2> best{{{0, nullptr}, {0, nullptr}}};
    assert(nRelayNodes <= best.size());

    auto sortfunc = [&best, &hasher, nRelayNodes, &originator, &addr](CNode* pnode) {
        if (pnode->RelayAddrsWithConn() && pnode != &originator && pnode->IsAddrCompatible(addr)) {
            uint64_t hashKey = CSipHasher(hasher).Write(pnode->GetId()).Finalize();
            for (unsigned int i = 0; i < nRelayNodes; i++) {
                 if (hashKey > best[i].first) {
                     std::copy(best.begin() + i, best.begin() + nRelayNodes - 1, best.begin() + i + 1);
                     best[i] = std::make_pair(hashKey, pnode);
                     break;
                 }
            }
        }
    };

    auto pushfunc = [&addr, &best, nRelayNodes, &insecure_rand] {
        for (unsigned int i = 0; i < nRelayNodes && best[i].first != 0; i++) {
            best[i].second->PushAddress(addr, insecure_rand);
        }
    };

    connman.ForEachNodeThen(std::move(sortfunc), std::move(pushfunc));
}

void PeerManagerImpl::ProcessGetBlockData(CNode& pfrom, Peer& peer, const CInv& inv)
{
    std::shared_ptr<const CBlock> a_recent_block;
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> a_recent_compact_block;
    bool fWitnessesPresentInARecentCompactBlock;
    {
        LOCK(cs_most_recent_block);
        a_recent_block = most_recent_block;
        a_recent_compact_block = most_recent_compact_block;
        fWitnessesPresentInARecentCompactBlock = fWitnessesPresentInMostRecentCompactBlock;
    }

    bool need_activate_chain = false;
    {
        LOCK(cs_main);
        const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(inv.hash);
        if (pindex) {
            if (pindex->HaveTxsDownloaded() && !pindex->IsValid(BLOCK_VALID_SCRIPTS) &&
                    pindex->IsValid(BLOCK_VALID_TREE)) {
                // If we have the block and all of its parents, but have not yet validated it,
                // we might be in the middle of connecting it (ie in the unlock of cs_main
                // before ActivateBestChain but after AcceptBlock).
                // In this case, we need to run ActivateBestChain prior to checking the relay
                // conditions below.
                need_activate_chain = true;
            }
        }
    } // release cs_main before calling ActivateBestChain
    if (need_activate_chain) {
        BlockValidationState state;
        if (!m_chainman.ActiveChainstate().ActivateBestChain(state, m_chainparams, a_recent_block)) {
            LogPrint(BCLog::NET, "failed to activate chain (%s)\n", state.ToString());
        }
    }

    LOCK(cs_main);
    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(inv.hash);
    if (!pindex) {
        return;
    }
    if (!BlockRequestAllowed(pindex)) {
        LogPrint(BCLog::NET, "%s: ignoring request from peer=%i for old block that isn't in the main chain\n", __func__, pfrom.GetId());
        return;
    }
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    // disconnect node in case we have reached the outbound limit for serving historical blocks
    if (m_connman.OutboundTargetReached(true) &&
        (((pindexBestHeader != nullptr) && (pindexBestHeader->GetBlockTime() - pindex->GetBlockTime() > HISTORICAL_BLOCK_AGE)) || inv.IsMsgFilteredBlk()) &&
        !pfrom.HasPermission(PF_DOWNLOAD) // nodes with the download permission may exceed target
    ) {
        LogPrint(BCLog::NET, "historical block serving limit reached, disconnect peer=%d\n", pfrom.GetId());
        pfrom.fDisconnect = true;
        return;
    }
    // Avoid leaking prune-height by never sending blocks below the NODE_NETWORK_LIMITED threshold
    if (!pfrom.HasPermission(PF_NOBAN) && (
            (((pfrom.GetLocalServices() & NODE_NETWORK_LIMITED) == NODE_NETWORK_LIMITED) && ((pfrom.GetLocalServices() & NODE_NETWORK) != NODE_NETWORK) && (m_chainman.ActiveChain().Tip()->nHeight - pindex->nHeight > (int)NODE_NETWORK_LIMITED_MIN_BLOCKS + 2 /* add two blocks buffer extension for possible races */) )
       )) {
        LogPrint(BCLog::NET, "Ignore block request below NODE_NETWORK_LIMITED threshold, disconnect peer=%d\n", pfrom.GetId());
        //disconnect node and prevent it from stalling (would otherwise wait for the missing block)
        pfrom.fDisconnect = true;
        return;
    }
    // Pruned nodes may have deleted the block, so check whether
    // it's available before trying to send.
    if (!(pindex->nStatus & BLOCK_HAVE_DATA)) {
        return;
    }
    std::shared_ptr<const CBlock> pblock;
    if (a_recent_block && a_recent_block->GetHash() == pindex->GetBlockHash()) {
        pblock = a_recent_block;
    } else if (inv.IsMsgWitnessBlk()) {
        // Fast-path: in this case it is possible to serve the block directly from disk,
        // as the network format matches the format on disk
        std::vector<uint8_t> block_data;
        if (!ReadRawBlockFromDisk(block_data, pindex, m_chainparams.MessageStart())) {
            assert(!"cannot load block from disk");
        }
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, MakeSpan(block_data)));
        // Don't set pblock as we've sent the block
    } else {
        // Send block from disk
        std::shared_ptr<CBlock> pblockRead = std::make_shared<CBlock>();
        if (!ReadBlockFromDisk(*pblockRead, pindex, m_chainparams.GetConsensus())) {
            assert(!"cannot load block from disk");
        }
        pblock = pblockRead;
    }
    if (pblock) {
        if (inv.IsMsgBlk()) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(SERIALIZE_TRANSACTION_NO_WITNESS, NetMsgType::BLOCK, *pblock));
        } else if (inv.IsMsgWitnessBlk()) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, *pblock));
        } else if (inv.IsMsgFilteredBlk()) {
            bool sendMerkleBlock = false;
            CMerkleBlock merkleBlock;
            if (pfrom.m_tx_relay != nullptr) {
                LOCK(pfrom.m_tx_relay->cs_filter);
                if (pfrom.m_tx_relay->pfilter) {
                    sendMerkleBlock = true;
                    merkleBlock = CMerkleBlock(*pblock, *pfrom.m_tx_relay->pfilter);
                }
            }
            if (sendMerkleBlock) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::MERKLEBLOCK, merkleBlock));
                // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                // This avoids hurting performance by pointlessly requiring a round-trip
                // Note that there is currently no way for a node to request any single transactions we didn't send here -
                // they must either disconnect and retry or request the full block.
                // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                // however we MUST always provide at least what the remote peer needs
                typedef std::pair<unsigned int, uint256> PairType;
                for (PairType& pair : merkleBlock.vMatchedTxn)
                    m_connman.PushMessage(&pfrom, msgMaker.Make(SERIALIZE_TRANSACTION_NO_WITNESS, NetMsgType::TX, *pblock->vtx[pair.first]));
            }
            // else
            // no response
        } else if (inv.IsMsgCmpctBlk()) {
            // If a peer is asking for old blocks, we're almost guaranteed
            // they won't have a useful mempool to match against a compact block,
            // and we don't feel like constructing the object for them, so
            // instead we respond with the full, non-compact block.
            bool fPeerWantsWitness = State(pfrom.GetId())->fWantsCmpctWitness;
            int nSendFlags = fPeerWantsWitness ? 0 : SERIALIZE_TRANSACTION_NO_WITNESS;
            if (CanDirectFetch() && pindex->nHeight >= m_chainman.ActiveChain().Height() - MAX_CMPCTBLOCK_DEPTH) {
                if ((fPeerWantsWitness || !fWitnessesPresentInARecentCompactBlock) && a_recent_compact_block && a_recent_compact_block->header.GetHash() == pindex->GetBlockHash()) {
                    m_connman.PushMessage(&pfrom, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, *a_recent_compact_block));
                } else {
                    CBlockHeaderAndShortTxIDs cmpctblock(*pblock, fPeerWantsWitness);
                    m_connman.PushMessage(&pfrom, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, cmpctblock));
                }
            } else {
                m_connman.PushMessage(&pfrom, msgMaker.Make(nSendFlags, NetMsgType::BLOCK, *pblock));
            }
        }
    }

    {
        LOCK(peer.m_block_inv_mutex);
        // Trigger the peer node to send a getblocks request for the next batch of inventory
        if (inv.hash == peer.m_continuation_block) {
            // Send immediately. This must send even if redundant,
            // and we want it right after the last block so they don't
            // wait for other stuff first.
            std::vector<CInv> vInv;
            vInv.push_back(CInv(MSG_BLOCK, m_chainman.ActiveChain().Tip()->GetBlockHash()));
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::INV, vInv));
            peer.m_continuation_block.SetNull();
        }
    }
}

CTransactionRef PeerManagerImpl::FindTxForGetData(const CNode& peer, const GenTxid& gtxid, const std::chrono::seconds mempool_req, const std::chrono::seconds now)
{
    auto txinfo = m_mempool.info(gtxid);
    if (txinfo.tx) {
        // If a TX could have been INVed in reply to a MEMPOOL request,
        // or is older than UNCONDITIONAL_RELAY_DELAY, permit the request
        // unconditionally.
        if ((mempool_req.count() && txinfo.m_time <= mempool_req) || txinfo.m_time <= now - UNCONDITIONAL_RELAY_DELAY) {
            return std::move(txinfo.tx);
        }
    }

    {
        LOCK(cs_main);
        // Otherwise, the transaction must have been announced recently.
        if (State(peer.GetId())->m_recently_announced_invs.contains(gtxid.GetHash())) {
            // If it was, it can be relayed from either the mempool...
            if (txinfo.tx) return std::move(txinfo.tx);
            // ... or the relay pool.
            auto mi = mapRelay.find(gtxid.GetHash());
            if (mi != mapRelay.end()) return mi->second;
        }
    }

    return {};
}

void PeerManagerImpl::ProcessGetData(CNode& pfrom, Peer& peer, const std::atomic<bool>& interruptMsgProc)
{
    AssertLockNotHeld(cs_main);

    std::deque<CInv>::iterator it = peer.m_getdata_requests.begin();
    std::vector<CInv> vNotFound;
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());

    const std::chrono::seconds now = GetTime<std::chrono::seconds>();
    // Get last mempool request time
    const std::chrono::seconds mempool_req = pfrom.m_tx_relay != nullptr ? pfrom.m_tx_relay->m_last_mempool_req.load()
                                                                          : std::chrono::seconds::min();

    // Process as many TX items from the front of the getdata queue as
    // possible, since they're common and it's efficient to batch process
    // them.
    while (it != peer.m_getdata_requests.end() && it->IsGenTxMsg()) {
        if (interruptMsgProc) return;
        // The send buffer provides backpressure. If there's no space in
        // the buffer, pause processing until the next call.
        if (pfrom.fPauseSend) break;

        const CInv &inv = *it++;

        if (pfrom.m_tx_relay == nullptr) {
            // Ignore GETDATA requests for transactions from blocks-only peers.
            continue;
        }

        CTransactionRef tx = FindTxForGetData(pfrom, ToGenTxid(inv), mempool_req, now);
        if (tx) {
            // WTX and WITNESS_TX imply we serialize with witness
            int nSendFlags = (inv.IsMsgTx() ? SERIALIZE_TRANSACTION_NO_WITNESS : 0);
            m_connman.PushMessage(&pfrom, msgMaker.Make(nSendFlags, NetMsgType::TX, *tx));
            m_mempool.RemoveUnbroadcastTx(tx->GetHash());
            // As we're going to send tx, make sure its unconfirmed parents are made requestable.
            std::vector<uint256> parent_ids_to_add;
            {
                LOCK(m_mempool.cs);
                auto txiter = m_mempool.GetIter(tx->GetHash());
                if (txiter) {
                    const CTxMemPoolEntry::Parents& parents = (*txiter)->GetMemPoolParentsConst();
                    parent_ids_to_add.reserve(parents.size());
                    for (const CTxMemPoolEntry& parent : parents) {
                        if (parent.GetTime() > now - UNCONDITIONAL_RELAY_DELAY) {
                            parent_ids_to_add.push_back(parent.GetTx().GetHash());
                        }
                    }
                }
            }
            for (const uint256& parent_txid : parent_ids_to_add) {
                // Relaying a transaction with a recent but unconfirmed parent.
                if (WITH_LOCK(pfrom.m_tx_relay->cs_tx_inventory, return !pfrom.m_tx_relay->filterInventoryKnown.contains(parent_txid))) {
                    LOCK(cs_main);
                    State(pfrom.GetId())->m_recently_announced_invs.insert(parent_txid);
                }
            }
        } else {
            vNotFound.push_back(inv);
        }
    }

    // Only process one BLOCK item per call, since they're uncommon and can be
    // expensive to process.
    if (it != peer.m_getdata_requests.end() && !pfrom.fPauseSend) {
        const CInv &inv = *it++;
        if (inv.IsGenBlkMsg()) {
            ProcessGetBlockData(pfrom, peer, inv);
        }
        // else: If the first item on the queue is an unknown type, we erase it
        // and continue processing the queue on the next call.
    }

    peer.m_getdata_requests.erase(peer.m_getdata_requests.begin(), it);

    if (!vNotFound.empty()) {
        // Let the peer know that we didn't find what it asked for, so it doesn't
        // have to wait around forever.
        // SPV clients care about this message: it's needed when they are
        // recursively walking the dependencies of relevant unconfirmed
        // transactions. SPV clients want to do that because they want to know
        // about (and store and rebroadcast and risk analyze) the dependencies
        // of transactions relevant to them, without having to download the
        // entire memory pool.
        // Also, other nodes can use these messages to automatically request a
        // transaction from some other peer that annnounced it, and stop
        // waiting for us to respond.
        // In normal operation, we often send NOTFOUND messages for parents of
        // transactions that we relay; if a peer is missing a parent, they may
        // assume we have them and request the parents from us.
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::NOTFOUND, vNotFound));
    }
}

static uint32_t GetFetchFlags(const CNode& pfrom) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    uint32_t nFetchFlags = 0;
    if ((pfrom.GetLocalServices() & NODE_WITNESS) && State(pfrom.GetId())->fHaveWitness) {
        nFetchFlags |= MSG_WITNESS_FLAG;
    }
    return nFetchFlags;
}

void PeerManagerImpl::SendBlockTransactions(CNode& pfrom, const CBlock& block, const BlockTransactionsRequest& req)
{
    BlockTransactions resp(req);
    for (size_t i = 0; i < req.indexes.size(); i++) {
        if (req.indexes[i] >= block.vtx.size()) {
            Misbehaving(pfrom.GetId(), 100, "getblocktxn with out-of-bounds tx indices");
            return;
        }
        resp.txn[i] = block.vtx[req.indexes[i]];
    }
    LOCK(cs_main);
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    int nSendFlags = State(pfrom.GetId())->fWantsCmpctWitness ? 0 : SERIALIZE_TRANSACTION_NO_WITNESS;
    m_connman.PushMessage(&pfrom, msgMaker.Make(nSendFlags, NetMsgType::BLOCKTXN, resp));
}

void PeerManagerImpl::ProcessHeadersMessage(CNode& pfrom, const Peer& peer,
                                            const std::vector<CBlockHeader>& headers,
                                            bool via_compact_block)
{
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    size_t nCount = headers.size();

    if (nCount == 0) {
        // Nothing interesting. Stop asking this peers for more headers.
        return;
    }

    bool received_new_header = false;
    const CBlockIndex *pindexLast = nullptr;
    {
        LOCK(cs_main);
        CNodeState *nodestate = State(pfrom.GetId());

        // If this looks like it could be a block announcement (nCount <
        // MAX_BLOCKS_TO_ANNOUNCE), use special logic for handling headers that
        // don't connect:
        // - Send a getheaders message in response to try to connect the chain.
        // - The peer can send up to MAX_UNCONNECTING_HEADERS in a row that
        //   don't connect before giving DoS points
        // - Once a headers message is received that is valid and does connect,
        //   nUnconnectingHeaders gets reset back to 0.
        if (!m_chainman.m_blockman.LookupBlockIndex(headers[0].hashPrevBlock) && nCount < MAX_BLOCKS_TO_ANNOUNCE) {
            nodestate->nUnconnectingHeaders++;
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(pindexBestHeader), uint256()));
            LogPrint(BCLog::NET, "received header %s: missing prev block %s, sending getheaders (%d) to end (peer=%d, nUnconnectingHeaders=%d)\n",
                    headers[0].GetHash().ToString(),
                    headers[0].hashPrevBlock.ToString(),
                    pindexBestHeader->nHeight,
                    pfrom.GetId(), nodestate->nUnconnectingHeaders);
            // Set hashLastUnknownBlock for this peer, so that if we
            // eventually get the headers - even from a different peer -
            // we can use this peer to download.
            UpdateBlockAvailability(pfrom.GetId(), headers.back().GetHash());

            if (nodestate->nUnconnectingHeaders % MAX_UNCONNECTING_HEADERS == 0) {
                Misbehaving(pfrom.GetId(), 20, strprintf("%d non-connecting headers", nodestate->nUnconnectingHeaders));
            }
            return;
        }

        uint256 hashLastBlock;
        for (const CBlockHeader& header : headers) {
            if (!hashLastBlock.IsNull() && header.hashPrevBlock != hashLastBlock) {
                Misbehaving(pfrom.GetId(), 20, "non-continuous headers sequence");
                return;
            }
            hashLastBlock = header.GetHash();
        }

        // If we don't have the last header, then they'll have given us
        // something new (if these headers are valid).
        if (!m_chainman.m_blockman.LookupBlockIndex(hashLastBlock)) {
            received_new_header = true;
        }
    }

    BlockValidationState state;
    if (!m_chainman.ProcessNewBlockHeaders(headers, state, m_chainparams, &pindexLast)) {
        if (state.IsInvalid()) {
            MaybePunishNodeForBlock(pfrom.GetId(), state, via_compact_block, "invalid header received");
            return;
        }
    }

    {
        LOCK(cs_main);
        CNodeState *nodestate = State(pfrom.GetId());
        if (nodestate->nUnconnectingHeaders > 0) {
            LogPrint(BCLog::NET, "peer=%d: resetting nUnconnectingHeaders (%d -> 0)\n", pfrom.GetId(), nodestate->nUnconnectingHeaders);
        }
        nodestate->nUnconnectingHeaders = 0;

        assert(pindexLast);
        UpdateBlockAvailability(pfrom.GetId(), pindexLast->GetBlockHash());

        // From here, pindexBestKnownBlock should be guaranteed to be non-null,
        // because it is set in UpdateBlockAvailability. Some nullptr checks
        // are still present, however, as belt-and-suspenders.

        if (received_new_header && pindexLast->nChainWork > m_chainman.ActiveChain().Tip()->nChainWork) {
            nodestate->m_last_block_announcement = GetTime();
        }

        if (nCount == MAX_HEADERS_RESULTS) {
            // Headers message had its maximum size; the peer may have more headers.
            // TODO: optimize: if pindexLast is an ancestor of m_chainman.ActiveChain().Tip or pindexBestHeader, continue
            // from there instead.
            LogPrint(BCLog::NET, "more getheaders (%d) to end to peer=%d (startheight:%d)\n",
                                 pindexLast->nHeight, pfrom.GetId(), peer.m_starting_height);
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(pindexLast), uint256()));
        }

        // If this set of headers is valid and ends in a block with at least as
        // much work as our tip, download as much as possible.
        if (CanDirectFetch() && pindexLast->IsValid(BLOCK_VALID_TREE) && m_chainman.ActiveChain().Tip()->nChainWork <= pindexLast->nChainWork) {
            std::vector<const CBlockIndex*> vToFetch;
            const CBlockIndex *pindexWalk = pindexLast;
            // Calculate all the blocks we'd need to switch to pindexLast, up to a limit.
            while (pindexWalk && !m_chainman.ActiveChain().Contains(pindexWalk) && vToFetch.size() <= MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
                if (!(pindexWalk->nStatus & BLOCK_HAVE_DATA) &&
                        !mapBlocksInFlight.count(pindexWalk->GetBlockHash()) &&
                        (!IsWitnessEnabled(pindexWalk->pprev, m_chainparams.GetConsensus()) || State(pfrom.GetId())->fHaveWitness)) {
                    // We don't have this block, and it's not yet in flight.
                    vToFetch.push_back(pindexWalk);
                }
                pindexWalk = pindexWalk->pprev;
            }
            // If pindexWalk still isn't on our main chain, we're looking at a
            // very large reorg at a time we think we're close to caught up to
            // the main chain -- this shouldn't really happen.  Bail out on the
            // direct fetch and rely on parallel download instead.
            if (!m_chainman.ActiveChain().Contains(pindexWalk)) {
                LogPrint(BCLog::NET, "Large reorg, won't direct fetch to %s (%d)\n",
                        pindexLast->GetBlockHash().ToString(),
                        pindexLast->nHeight);
            } else {
                std::vector<CInv> vGetData;
                // Download as much as possible, from earliest to latest.
                for (const CBlockIndex *pindex : reverse_iterate(vToFetch)) {
                    if (nodestate->nBlocksInFlight >= MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
                        // Can't download any more from this peer
                        break;
                    }
                    uint32_t nFetchFlags = GetFetchFlags(pfrom);
                    vGetData.push_back(CInv(MSG_BLOCK | nFetchFlags, pindex->GetBlockHash()));
                    MarkBlockAsInFlight(pfrom.GetId(), pindex->GetBlockHash(), pindex);
                    LogPrint(BCLog::NET, "Requesting block %s from  peer=%d\n",
                            pindex->GetBlockHash().ToString(), pfrom.GetId());
                }
                if (vGetData.size() > 1) {
                    LogPrint(BCLog::NET, "Downloading blocks toward %s (%d) via headers direct fetch\n",
                            pindexLast->GetBlockHash().ToString(), pindexLast->nHeight);
                }
                if (vGetData.size() > 0) {
                    if (nodestate->fSupportsDesiredCmpctVersion && vGetData.size() == 1 && mapBlocksInFlight.size() == 1 && pindexLast->pprev->IsValid(BLOCK_VALID_CHAIN)) {
                        // In any case, we want to download using a compact block, not a regular one
                        vGetData[0] = CInv(MSG_CMPCT_BLOCK, vGetData[0].hash);
                    }
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vGetData));
                }
            }
        }
        // If we're in IBD, we want outbound peers that will serve us a useful
        // chain. Disconnect peers that are on chains with insufficient work.
        if (m_chainman.ActiveChainstate().IsInitialBlockDownload() && nCount != MAX_HEADERS_RESULTS) {
            // When nCount < MAX_HEADERS_RESULTS, we know we have no more
            // headers to fetch from this peer.
            if (nodestate->pindexBestKnownBlock && nodestate->pindexBestKnownBlock->nChainWork < nMinimumChainWork) {
                // This peer has too little work on their headers chain to help
                // us sync -- disconnect if it is an outbound disconnection
                // candidate.
                // Note: We compare their tip to nMinimumChainWork (rather than
                // m_chainman.ActiveChain().Tip()) because we won't start block download
                // until we have a headers chain that has at least
                // nMinimumChainWork, even if a peer has a chain past our tip,
                // as an anti-DoS measure.
                if (pfrom.IsOutboundOrBlockRelayConn()) {
                    LogPrintf("Disconnecting outbound peer %d -- headers chain has insufficient work\n", pfrom.GetId());
                    pfrom.fDisconnect = true;
                }
            }
        }

        // If this is an outbound full-relay peer, check to see if we should protect
        // it from the bad/lagging chain logic.
        // Note that outbound block-relay peers are excluded from this protection, and
        // thus always subject to eviction under the bad/lagging chain logic.
        // See ChainSyncTimeoutState.
        if (!pfrom.fDisconnect && pfrom.IsFullOutboundConn() && nodestate->pindexBestKnownBlock != nullptr) {
            if (m_outbound_peers_with_protect_from_disconnect < MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT && nodestate->pindexBestKnownBlock->nChainWork >= m_chainman.ActiveChain().Tip()->nChainWork && !nodestate->m_chain_sync.m_protect) {
                LogPrint(BCLog::NET, "Protecting outbound peer=%d from eviction\n", pfrom.GetId());
                nodestate->m_chain_sync.m_protect = true;
                ++m_outbound_peers_with_protect_from_disconnect;
            }
        }
    }

    return;
}

/**
 * Reconsider orphan transactions after a parent has been accepted to the mempool.
 *
 * @param[in/out]  orphan_work_set  The set of orphan transactions to reconsider. Generally only one
 *                                  orphan will be reconsidered on each call of this function. This set
 *                                  may be added to if accepting an orphan causes its children to be
 *                                  reconsidered.
 */
void PeerManagerImpl::ProcessOrphanTx(std::set<uint256>& orphan_work_set)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(g_cs_orphans);

    while (!orphan_work_set.empty()) {
        const uint256 orphanHash = *orphan_work_set.begin();
        orphan_work_set.erase(orphan_work_set.begin());

        const auto [porphanTx, from_peer] = m_orphanage.GetTx(orphanHash);
        if (porphanTx == nullptr) continue;

        const MempoolAcceptResult result = AcceptToMemoryPool(m_chainman.ActiveChainstate(), m_mempool, porphanTx, false /* bypass_limits */);
        const TxValidationState& state = result.m_state;

        if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            LogPrint(BCLog::MEMPOOL, "   accepted orphan tx %s\n", orphanHash.ToString());
            RelayTransaction(orphanHash, porphanTx->GetWitnessHash());
            m_orphanage.AddChildrenToWorkSet(*porphanTx, orphan_work_set);
            m_orphanage.EraseTx(orphanHash);
            for (const CTransactionRef& removedTx : result.m_replaced_transactions.value()) {
                AddToCompactExtraTransactions(removedTx);
            }
            break;
        } else if (state.GetResult() != TxValidationResult::TX_MISSING_INPUTS) {
            if (state.IsInvalid()) {
                LogPrint(BCLog::MEMPOOL, "   invalid orphan tx %s from peer=%d. %s\n",
                    orphanHash.ToString(),
                    from_peer,
                    state.ToString());
                // Maybe punish peer that gave us an invalid orphan tx
                MaybePunishNodeForTx(from_peer, state);
            }
            // Has inputs but not accepted to mempool
            // Probably non-standard or insufficient fee
            LogPrint(BCLog::MEMPOOL, "   removed orphan tx %s\n", orphanHash.ToString());
            if (state.GetResult() != TxValidationResult::TX_WITNESS_STRIPPED) {
                // We can add the wtxid of this transaction to our reject filter.
                // Do not add txids of witness transactions or witness-stripped
                // transactions to the filter, as they can have been malleated;
                // adding such txids to the reject filter would potentially
                // interfere with relay of valid transactions from peers that
                // do not support wtxid-based relay. See
                // https://github.com/bitcoin/bitcoin/issues/8279 for details.
                // We can remove this restriction (and always add wtxids to
                // the filter even for witness stripped transactions) once
                // wtxid-based relay is broadly deployed.
                // See also comments in https://github.com/bitcoin/bitcoin/pull/18044#discussion_r443419034
                // for concerns around weakening security of unupgraded nodes
                // if we start doing this too early.
                assert(recentRejects);
                recentRejects->insert(porphanTx->GetWitnessHash());
                // If the transaction failed for TX_INPUTS_NOT_STANDARD,
                // then we know that the witness was irrelevant to the policy
                // failure, since this check depends only on the txid
                // (the scriptPubKey being spent is covered by the txid).
                // Add the txid to the reject filter to prevent repeated
                // processing of this transaction in the event that child
                // transactions are later received (resulting in
                // parent-fetching by txid via the orphan-handling logic).
                if (state.GetResult() == TxValidationResult::TX_INPUTS_NOT_STANDARD && porphanTx->GetWitnessHash() != porphanTx->GetHash()) {
                    // We only add the txid if it differs from the wtxid, to
                    // avoid wasting entries in the rolling bloom filter.
                    recentRejects->insert(porphanTx->GetHash());
                }
            }
            m_orphanage.EraseTx(orphanHash);
            break;
        }
    }
    m_mempool.check(m_chainman.ActiveChainstate());
}

/**
 * Validation logic for compact filters request handling.
 *
 * May disconnect from the peer in the case of a bad request.
 *
 * @param[in]   peer            The peer that we received the request from
 * @param[in]   filter_type     The filter type the request is for. Must be basic filters.
 * @param[in]   start_height    The start height for the request
 * @param[in]   stop_hash       The stop_hash for the request
 * @param[in]   max_height_diff The maximum number of items permitted to request, as specified in BIP 157
 * @param[out]  stop_index      The CBlockIndex for the stop_hash block, if the request can be serviced.
 * @param[out]  filter_index    The filter index, if the request can be serviced.
 * @return                      True if the request can be serviced.
 */
bool PeerManagerImpl::PrepareBlockFilterRequest(CNode& peer,
                                                BlockFilterType filter_type, uint32_t start_height,
                                                const uint256& stop_hash, uint32_t max_height_diff,
                                                const CBlockIndex*& stop_index,
                                                BlockFilterIndex*& filter_index)
{
    const bool supported_filter_type =
        (filter_type == BlockFilterType::BASIC &&
         (peer.GetLocalServices() & NODE_COMPACT_FILTERS));
    if (!supported_filter_type) {
        LogPrint(BCLog::NET, "peer %d requested unsupported block filter type: %d\n",
                 peer.GetId(), static_cast<uint8_t>(filter_type));
        peer.fDisconnect = true;
        return false;
    }

    {
        LOCK(cs_main);
        stop_index = m_chainman.m_blockman.LookupBlockIndex(stop_hash);

        // Check that the stop block exists and the peer would be allowed to fetch it.
        if (!stop_index || !BlockRequestAllowed(stop_index)) {
            LogPrint(BCLog::NET, "peer %d requested invalid block hash: %s\n",
                     peer.GetId(), stop_hash.ToString());
            peer.fDisconnect = true;
            return false;
        }
    }

    uint32_t stop_height = stop_index->nHeight;
    if (start_height > stop_height) {
        LogPrint(BCLog::NET, "peer %d sent invalid getcfilters/getcfheaders with " /* Continued */
                 "start height %d and stop height %d\n",
                 peer.GetId(), start_height, stop_height);
        peer.fDisconnect = true;
        return false;
    }
    if (stop_height - start_height >= max_height_diff) {
        LogPrint(BCLog::NET, "peer %d requested too many cfilters/cfheaders: %d / %d\n",
                 peer.GetId(), stop_height - start_height + 1, max_height_diff);
        peer.fDisconnect = true;
        return false;
    }

    filter_index = GetBlockFilterIndex(filter_type);
    if (!filter_index) {
        LogPrint(BCLog::NET, "Filter index for supported type %s not found\n", BlockFilterTypeName(filter_type));
        return false;
    }

    return true;
}

/**
 * Handle a cfilters request.
 *
 * May disconnect from the peer in the case of a bad request.
 *
 * @param[in]   peer            The peer that we received the request from
 * @param[in]   vRecv           The raw message received
 */
void PeerManagerImpl::ProcessGetCFilters(CNode& peer, CDataStream& vRecv)
{
    uint8_t filter_type_ser;
    uint32_t start_height;
    uint256 stop_hash;

    vRecv >> filter_type_ser >> start_height >> stop_hash;

    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);

    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(peer, filter_type, start_height, stop_hash,
                                   MAX_GETCFILTERS_SIZE, stop_index, filter_index)) {
        return;
    }

    std::vector<BlockFilter> filters;
    if (!filter_index->LookupFilterRange(start_height, stop_index, filters)) {
        LogPrint(BCLog::NET, "Failed to find block filter in index: filter_type=%s, start_height=%d, stop_hash=%s\n",
                     BlockFilterTypeName(filter_type), start_height, stop_hash.ToString());
        return;
    }

    for (const auto& filter : filters) {
        CSerializedNetMsg msg = CNetMsgMaker(peer.GetCommonVersion())
            .Make(NetMsgType::CFILTER, filter);
        m_connman.PushMessage(&peer, std::move(msg));
    }
}

/**
 * Handle a cfheaders request.
 *
 * May disconnect from the peer in the case of a bad request.
 *
 * @param[in]   peer            The peer that we received the request from
 * @param[in]   vRecv           The raw message received
 */
void PeerManagerImpl::ProcessGetCFHeaders(CNode& peer, CDataStream& vRecv)
{
    uint8_t filter_type_ser;
    uint32_t start_height;
    uint256 stop_hash;

    vRecv >> filter_type_ser >> start_height >> stop_hash;

    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);

    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(peer, filter_type, start_height, stop_hash,
                                   MAX_GETCFHEADERS_SIZE, stop_index, filter_index)) {
        return;
    }

    uint256 prev_header;
    if (start_height > 0) {
        const CBlockIndex* const prev_block =
            stop_index->GetAncestor(static_cast<int>(start_height - 1));
        if (!filter_index->LookupFilterHeader(prev_block, prev_header)) {
            LogPrint(BCLog::NET, "Failed to find block filter header in index: filter_type=%s, block_hash=%s\n",
                         BlockFilterTypeName(filter_type), prev_block->GetBlockHash().ToString());
            return;
        }
    }

    std::vector<uint256> filter_hashes;
    if (!filter_index->LookupFilterHashRange(start_height, stop_index, filter_hashes)) {
        LogPrint(BCLog::NET, "Failed to find block filter hashes in index: filter_type=%s, start_height=%d, stop_hash=%s\n",
                     BlockFilterTypeName(filter_type), start_height, stop_hash.ToString());
        return;
    }

    CSerializedNetMsg msg = CNetMsgMaker(peer.GetCommonVersion())
        .Make(NetMsgType::CFHEADERS,
              filter_type_ser,
              stop_index->GetBlockHash(),
              prev_header,
              filter_hashes);
    m_connman.PushMessage(&peer, std::move(msg));
}

/**
 * Handle a getcfcheckpt request.
 *
 * May disconnect from the peer in the case of a bad request.
 *
 * @param[in]   peer            The peer that we received the request from
 * @param[in]   vRecv           The raw message received
 */
void PeerManagerImpl::ProcessGetCFCheckPt(CNode& peer, CDataStream& vRecv)
{
    uint8_t filter_type_ser;
    uint256 stop_hash;

    vRecv >> filter_type_ser >> stop_hash;

    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);

    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(peer, filter_type, /*start_height=*/0, stop_hash,
                                   /*max_height_diff=*/std::numeric_limits<uint32_t>::max(),
                                   stop_index, filter_index)) {
        return;
    }

    std::vector<uint256> headers(stop_index->nHeight / CFCHECKPT_INTERVAL);

    // Populate headers.
    const CBlockIndex* block_index = stop_index;
    for (int i = headers.size() - 1; i >= 0; i--) {
        int height = (i + 1) * CFCHECKPT_INTERVAL;
        block_index = block_index->GetAncestor(height);

        if (!filter_index->LookupFilterHeader(block_index, headers[i])) {
            LogPrint(BCLog::NET, "Failed to find block filter header in index: filter_type=%s, block_hash=%s\n",
                         BlockFilterTypeName(filter_type), block_index->GetBlockHash().ToString());
            return;
        }
    }

    CSerializedNetMsg msg = CNetMsgMaker(peer.GetCommonVersion())
        .Make(NetMsgType::CFCHECKPT,
              filter_type_ser,
              stop_index->GetBlockHash(),
              headers);
    m_connman.PushMessage(&peer, std::move(msg));
}

void PeerManagerImpl::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                                     const std::chrono::microseconds time_received,
                                     const std::atomic<bool>& interruptMsgProc)
{
    LogPrint(BCLog::NET, "received: %s (%u bytes) peer=%d\n", SanitizeString(msg_type), vRecv.size(), pfrom.GetId());

    PeerRef peer = GetPeerRef(pfrom.GetId());
    if (peer == nullptr) return;

    if (msg_type == NetMsgType::VERSION) {
        if (pfrom.nVersion != 0) {
            LogPrint(BCLog::NET, "redundant version message from peer=%d\n", pfrom.GetId());
            return;
        }

        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        uint64_t nServiceInt;
        ServiceFlags nServices;
        int nVersion;
        std::string cleanSubVer;
        int starting_height = -1;
        bool fRelay = true;

        vRecv >> nVersion >> nServiceInt >> nTime >> addrMe;
        if (nTime < 0) {
            nTime = 0;
        }
        nServices = ServiceFlags(nServiceInt);
        if (!pfrom.IsInboundConn())
        {
            m_addrman.SetServices(pfrom.addr, nServices);
        }
        if (pfrom.ExpectServicesFromConn() && !HasAllDesirableServiceFlags(nServices))
        {
            LogPrint(BCLog::NET, "peer=%d does not offer the expected services (%08x offered, %08x expected); disconnecting\n", pfrom.GetId(), nServices, GetDesirableServiceFlags(nServices));
            pfrom.fDisconnect = true;
            return;
        }

        if (nVersion < MIN_PEER_PROTO_VERSION) {
            // disconnect from peers older than this proto version
            LogPrint(BCLog::NET, "peer=%d using obsolete version %i; disconnecting\n", pfrom.GetId(), nVersion);
            pfrom.fDisconnect = true;
            return;
        }

        if (!vRecv.empty())
            vRecv >> addrFrom >> nNonce;
        if (!vRecv.empty()) {
            std::string strSubVer;
            vRecv >> LIMITED_STRING(strSubVer, MAX_SUBVERSION_LENGTH);
            cleanSubVer = SanitizeString(strSubVer);
        }
        if (!vRecv.empty()) {
            vRecv >> starting_height;
        }
        if (!vRecv.empty())
            vRecv >> fRelay;
        // Disconnect if we connected to ourself
        if (pfrom.IsInboundConn() && !m_connman.CheckIncomingNonce(nNonce))
        {
            LogPrintf("connected to self at %s, disconnecting\n", pfrom.addr.ToString());
            pfrom.fDisconnect = true;
            return;
        }

        if (pfrom.IsInboundConn() && addrMe.IsRoutable())
        {
            SeenLocal(addrMe);
        }

        // Inbound peers send us their version message when they connect.
        // We send our version message in response.
        if (pfrom.IsInboundConn()) PushNodeVersion(pfrom, GetAdjustedTime());

        // Change version
        const int greatest_common_version = std::min(nVersion, PROTOCOL_VERSION);
        pfrom.SetCommonVersion(greatest_common_version);
        pfrom.nVersion = nVersion;

        const CNetMsgMaker msg_maker(greatest_common_version);

        if (greatest_common_version >= WTXID_RELAY_VERSION) {
            m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::WTXIDRELAY));
        }

        // Signal ADDRv2 support (BIP155).
        if (greatest_common_version >= 70016) {
            // BIP155 defines addrv2 and sendaddrv2 for all protocol versions, but some
            // implementations reject messages they don't know. As a courtesy, don't send
            // it to nodes with a version before 70016, as no software is known to support
            // BIP155 that doesn't announce at least that protocol version number.
            m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::SENDADDRV2));
        }

        m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::VERACK));

        pfrom.nServices = nServices;
        pfrom.SetAddrLocal(addrMe);
        {
            LOCK(pfrom.cs_SubVer);
            pfrom.cleanSubVer = cleanSubVer;
        }
        peer->m_starting_height = starting_height;

        // set nodes not relaying blocks and tx and not serving (parts) of the historical blockchain as "clients"
        pfrom.fClient = (!(nServices & NODE_NETWORK) && !(nServices & NODE_NETWORK_LIMITED));

        // set nodes not capable of serving the complete blockchain history as "limited nodes"
        pfrom.m_limited_node = (!(nServices & NODE_NETWORK) && (nServices & NODE_NETWORK_LIMITED));

        if (pfrom.m_tx_relay != nullptr) {
            LOCK(pfrom.m_tx_relay->cs_filter);
            pfrom.m_tx_relay->fRelayTxes = fRelay; // set to true after we get the first filter* message
        }

        if((nServices & NODE_WITNESS))
        {
            LOCK(cs_main);
            State(pfrom.GetId())->fHaveWitness = true;
        }

        // Potentially mark this peer as a preferred download peer.
        {
        LOCK(cs_main);
        UpdatePreferredDownload(pfrom, State(pfrom.GetId()));
        }

        if (!pfrom.IsInboundConn() && !pfrom.IsBlockOnlyConn()) {
            // For outbound peers, we try to relay our address (so that other
            // nodes can try to find us more quickly, as we have no guarantee
            // that an outbound peer is even aware of how to reach us) and do a
            // one-time address fetch (to help populate/update our addrman). If
            // we're starting up for the first time, our addrman may be pretty
            // empty and no one will know who we are, so these mechanisms are
            // important to help us connect to the network.
            //
            // We skip this for block-relay-only peers to avoid potentially leaking
            // information about our block-relay-only connections via address relay.
            if (fListen && !m_chainman.ActiveChainstate().IsInitialBlockDownload())
            {
                CAddress addr = GetLocalAddress(&pfrom.addr, pfrom.GetLocalServices());
                FastRandomContext insecure_rand;
                if (addr.IsRoutable())
                {
                    LogPrint(BCLog::NET, "ProcessMessages: advertising address %s\n", addr.ToString());
                    pfrom.PushAddress(addr, insecure_rand);
                } else if (IsPeerAddrLocalGood(&pfrom)) {
                    addr.SetIP(addrMe);
                    LogPrint(BCLog::NET, "ProcessMessages: advertising address %s\n", addr.ToString());
                    pfrom.PushAddress(addr, insecure_rand);
                }
            }

            // Get recent addresses
            m_connman.PushMessage(&pfrom, CNetMsgMaker(greatest_common_version).Make(NetMsgType::GETADDR));
            pfrom.fGetAddr = true;
        }

        if (!pfrom.IsInboundConn()) {
            // For non-inbound connections, we update the addrman to record
            // connection success so that addrman will have an up-to-date
            // notion of which peers are online and available.
            //
            // While we strive to not leak information about block-relay-only
            // connections via the addrman, not moving an address to the tried
            // table is also potentially detrimental because new-table entries
            // are subject to eviction in the event of addrman collisions.  We
            // mitigate the information-leak by never calling
            // CAddrMan::Connected() on block-relay-only peers; see
            // FinalizeNode().
            //
            // This moves an address from New to Tried table in Addrman,
            // resolves tried-table collisions, etc.
            m_addrman.Good(pfrom.addr);
        }

        std::string remoteAddr;
        if (fLogIPs)
            remoteAddr = ", peeraddr=" + pfrom.addr.ToString();

        LogPrint(BCLog::NET, "receive version message: %s: version %d, blocks=%d, us=%s, txrelay=%d, peer=%d%s\n",
                  cleanSubVer, pfrom.nVersion,
                  peer->m_starting_height, addrMe.ToString(), fRelay, pfrom.GetId(),
                  remoteAddr);

        int64_t nTimeOffset = nTime - GetTime();
        pfrom.nTimeOffset = nTimeOffset;
        AddTimeData(pfrom.addr, nTimeOffset);

        // If the peer is old enough to have the old alert system, send it the final alert.
        if (greatest_common_version <= 70012) {
            CDataStream finalAlert(ParseHex("60010000000000000000000000ffffff7f00000000ffffff7ffeffff7f01ffffff7f00000000ffffff7f00ffffff7f002f555247454e543a20416c657274206b657920636f6d70726f6d697365642c2075706772616465207265717569726564004630440220653febd6410f470f6bae11cad19c48413becb1ac2c17f908fd0fd53bdc3abd5202206d0e9c96fe88d4a0f01ed9dedae2b6f9e00da94cad0fecaae66ecf689bf71b50"), SER_NETWORK, PROTOCOL_VERSION);
            m_connman.PushMessage(&pfrom, CNetMsgMaker(greatest_common_version).Make("alert", finalAlert));
        }

        // Feeler connections exist only to verify if address is online.
        if (pfrom.IsFeelerConn()) {
            LogPrint(BCLog::NET, "feeler connection completed peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
        }
        return;
    }

    if (pfrom.nVersion == 0) {
        // Must have a version message before anything else
        LogPrint(BCLog::NET, "non-version message before version handshake. Message \"%s\" from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());
        return;
    }

    // At this point, the outgoing message serialization version can't change.
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());

    if (msg_type == NetMsgType::VERACK) {
        if (pfrom.fSuccessfullyConnected) {
            LogPrint(BCLog::NET, "ignoring redundant verack message from peer=%d\n", pfrom.GetId());
            return;
        }

        if (!pfrom.IsInboundConn()) {
            LogPrintf("New outbound peer connected: version: %d, blocks=%d, peer=%d%s (%s)\n",
                      pfrom.nVersion.load(), peer->m_starting_height,
                      pfrom.GetId(), (fLogIPs ? strprintf(", peeraddr=%s", pfrom.addr.ToString()) : ""),
                      pfrom.ConnectionTypeAsString());
        }

        if (pfrom.GetCommonVersion() >= SENDHEADERS_VERSION) {
            // Tell our peer we prefer to receive headers rather than inv's
            // We send this to non-NODE NETWORK peers as well, because even
            // non-NODE NETWORK peers can announce blocks (such as pruning
            // nodes)
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SENDHEADERS));
        }
        if (pfrom.GetCommonVersion() >= SHORT_IDS_BLOCKS_VERSION) {
            // Tell our peer we are willing to provide version 1 or 2 cmpctblocks
            // However, we do not request new block announcements using
            // cmpctblock messages.
            // We send this to non-NODE NETWORK peers as well, because
            // they may wish to request compact blocks from us
            bool fAnnounceUsingCMPCTBLOCK = false;
            uint64_t nCMPCTBLOCKVersion = 2;
            if (pfrom.GetLocalServices() & NODE_WITNESS)
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion));
            nCMPCTBLOCKVersion = 1;
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion));
        }
        pfrom.fSuccessfullyConnected = true;
        return;
    }

    if (msg_type == NetMsgType::SENDHEADERS) {
        LOCK(cs_main);
        State(pfrom.GetId())->fPreferHeaders = true;
        return;
    }

    if (msg_type == NetMsgType::SENDCMPCT) {
        bool fAnnounceUsingCMPCTBLOCK = false;
        uint64_t nCMPCTBLOCKVersion = 0;
        vRecv >> fAnnounceUsingCMPCTBLOCK >> nCMPCTBLOCKVersion;
        if (nCMPCTBLOCKVersion == 1 || ((pfrom.GetLocalServices() & NODE_WITNESS) && nCMPCTBLOCKVersion == 2)) {
            LOCK(cs_main);
            // fProvidesHeaderAndIDs is used to "lock in" version of compact blocks we send (fWantsCmpctWitness)
            if (!State(pfrom.GetId())->fProvidesHeaderAndIDs) {
                State(pfrom.GetId())->fProvidesHeaderAndIDs = true;
                State(pfrom.GetId())->fWantsCmpctWitness = nCMPCTBLOCKVersion == 2;
            }
            if (State(pfrom.GetId())->fWantsCmpctWitness == (nCMPCTBLOCKVersion == 2)) { // ignore later version announces
                State(pfrom.GetId())->fPreferHeaderAndIDs = fAnnounceUsingCMPCTBLOCK;
                // save whether peer selects us as BIP152 high-bandwidth peer
                // (receiving sendcmpct(1) signals high-bandwidth, sendcmpct(0) low-bandwidth)
                pfrom.m_bip152_highbandwidth_from = fAnnounceUsingCMPCTBLOCK;
            }
            if (!State(pfrom.GetId())->fSupportsDesiredCmpctVersion) {
                if (pfrom.GetLocalServices() & NODE_WITNESS)
                    State(pfrom.GetId())->fSupportsDesiredCmpctVersion = (nCMPCTBLOCKVersion == 2);
                else
                    State(pfrom.GetId())->fSupportsDesiredCmpctVersion = (nCMPCTBLOCKVersion == 1);
            }
        }
        return;
    }

    // BIP339 defines feature negotiation of wtxidrelay, which must happen between
    // VERSION and VERACK to avoid relay problems from switching after a connection is up.
    if (msg_type == NetMsgType::WTXIDRELAY) {
        if (pfrom.fSuccessfullyConnected) {
            // Disconnect peers that send a wtxidrelay message after VERACK.
            LogPrint(BCLog::NET, "wtxidrelay received after verack from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        if (pfrom.GetCommonVersion() >= WTXID_RELAY_VERSION) {
            LOCK(cs_main);
            if (!State(pfrom.GetId())->m_wtxid_relay) {
                State(pfrom.GetId())->m_wtxid_relay = true;
                m_wtxid_relay_peers++;
            } else {
                LogPrint(BCLog::NET, "ignoring duplicate wtxidrelay from peer=%d\n", pfrom.GetId());
            }
        } else {
            LogPrint(BCLog::NET, "ignoring wtxidrelay due to old common version=%d from peer=%d\n", pfrom.GetCommonVersion(), pfrom.GetId());
        }
        return;
    }

    // BIP155 defines feature negotiation of addrv2 and sendaddrv2, which must happen
    // between VERSION and VERACK.
    if (msg_type == NetMsgType::SENDADDRV2) {
        if (pfrom.fSuccessfullyConnected) {
            // Disconnect peers that send a SENDADDRV2 message after VERACK.
            LogPrint(BCLog::NET, "sendaddrv2 received after verack from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        pfrom.m_wants_addrv2 = true;
        return;
    }

    if (!pfrom.fSuccessfullyConnected) {
        LogPrint(BCLog::NET, "Unsupported message \"%s\" prior to verack from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());
        return;
    }

    if (msg_type == NetMsgType::ADDR || msg_type == NetMsgType::ADDRV2) {
        int stream_version = vRecv.GetVersion();
        if (msg_type == NetMsgType::ADDRV2) {
            // Add ADDRV2_FORMAT to the version so that the CNetAddr and CAddress
            // unserialize methods know that an address in v2 format is coming.
            stream_version |= ADDRV2_FORMAT;
        }

        OverrideStream<CDataStream> s(&vRecv, vRecv.GetType(), stream_version);
        std::vector<CAddress> vAddr;

        s >> vAddr;

        if (!pfrom.RelayAddrsWithConn()) {
            LogPrint(BCLog::NET, "ignoring %s message from %s peer=%d\n", msg_type, pfrom.ConnectionTypeAsString(), pfrom.GetId());
            return;
        }
        if (vAddr.size() > MAX_ADDR_TO_SEND)
        {
            Misbehaving(pfrom.GetId(), 20, strprintf("%s message size = %u", msg_type, vAddr.size()));
            return;
        }

        // Store the new addresses
        std::vector<CAddress> vAddrOk;
        int64_t nNow = GetAdjustedTime();
        int64_t nSince = nNow - 10 * 60;
        for (CAddress& addr : vAddr)
        {
            if (interruptMsgProc)
                return;

            // We only bother storing full nodes, though this may include
            // things which we would not make an outbound connection to, in
            // part because we may make feeler connections to them.
            if (!MayHaveUsefulAddressDB(addr.nServices) && !HasAllDesirableServiceFlags(addr.nServices))
                continue;

            if (addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
                addr.nTime = nNow - 5 * 24 * 60 * 60;
            pfrom.AddAddressKnown(addr);
            if (m_banman && (m_banman->IsDiscouraged(addr) || m_banman->IsBanned(addr))) {
                // Do not process banned/discouraged addresses beyond remembering we received them
                continue;
            }
            bool fReachable = IsReachable(addr);
            if (addr.nTime > nSince && !pfrom.fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
            {
                // Relay to a limited number of other nodes
                RelayAddress(pfrom, addr, fReachable, m_connman);
            }
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        m_addrman.Add(vAddrOk, pfrom.addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom.fGetAddr = false;
        if (pfrom.IsAddrFetchConn()) {
            LogPrint(BCLog::NET, "addrfetch connection completed peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
        }
        return;
    }

    if (msg_type == NetMsgType::INV) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(pfrom.GetId(), 20, strprintf("inv message size = %u", vInv.size()));
            return;
        }

        // We won't accept tx inv's if we're in blocks-only mode, or this is a
        // block-relay-only peer
        bool fBlocksOnly = m_ignore_incoming_txs || (pfrom.m_tx_relay == nullptr);

        // Allow peers with relay permission to send data other than blocks in blocks only mode
        if (pfrom.HasPermission(PF_RELAY)) {
            fBlocksOnly = false;
        }

        LOCK(cs_main);

        const auto current_time = GetTime<std::chrono::microseconds>();
        uint256* best_block{nullptr};

        for (CInv& inv : vInv) {
            if (interruptMsgProc) return;

            // Ignore INVs that don't match wtxidrelay setting.
            // Note that orphan parent fetching always uses MSG_TX GETDATAs regardless of the wtxidrelay setting.
            // This is fine as no INV messages are involved in that process.
            if (State(pfrom.GetId())->m_wtxid_relay) {
                if (inv.IsMsgTx()) continue;
            } else {
                if (inv.IsMsgWtx()) continue;
            }

            if (inv.IsMsgBlk()) {
                const bool fAlreadyHave = AlreadyHaveBlock(inv.hash);
                LogPrint(BCLog::NET, "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom.GetId());

                UpdateBlockAvailability(pfrom.GetId(), inv.hash);
                if (!fAlreadyHave && !fImporting && !fReindex && !mapBlocksInFlight.count(inv.hash)) {
                    // Headers-first is the primary method of announcement on
                    // the network. If a node fell back to sending blocks by inv,
                    // it's probably for a re-org. The final block hash
                    // provided should be the highest, so send a getheaders and
                    // then fetch the blocks we need to catch up.
                    best_block = &inv.hash;
                }
            } else if (inv.IsGenTxMsg()) {
                const GenTxid gtxid = ToGenTxid(inv);
                const bool fAlreadyHave = AlreadyHaveTx(gtxid);
                LogPrint(BCLog::NET, "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom.GetId());

                pfrom.AddKnownTx(inv.hash);
                if (fBlocksOnly) {
                    LogPrint(BCLog::NET, "transaction (%s) inv sent in violation of protocol, disconnecting peer=%d\n", inv.hash.ToString(), pfrom.GetId());
                    pfrom.fDisconnect = true;
                    return;
                } else if (!fAlreadyHave && !m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
                    AddTxAnnouncement(pfrom, gtxid, current_time);
                }
            } else {
                LogPrint(BCLog::NET, "Unknown inv type \"%s\" received from peer=%d\n", inv.ToString(), pfrom.GetId());
            }
        }

        if (best_block != nullptr) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(pindexBestHeader), *best_block));
            LogPrint(BCLog::NET, "getheaders (%d) %s to peer=%d\n", pindexBestHeader->nHeight, best_block->ToString(), pfrom.GetId());
        }

        return;
    }

    if (msg_type == NetMsgType::GETDATA) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(pfrom.GetId(), 20, strprintf("getdata message size = %u", vInv.size()));
            return;
        }

        LogPrint(BCLog::NET, "received getdata (%u invsz) peer=%d\n", vInv.size(), pfrom.GetId());

        if (vInv.size() > 0) {
            LogPrint(BCLog::NET, "received getdata for: %s peer=%d\n", vInv[0].ToString(), pfrom.GetId());
        }

        {
            LOCK(peer->m_getdata_requests_mutex);
            peer->m_getdata_requests.insert(peer->m_getdata_requests.end(), vInv.begin(), vInv.end());
            ProcessGetData(pfrom, *peer, interruptMsgProc);
        }

        return;
    }

    if (msg_type == NetMsgType::GETBLOCKS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "getblocks locator size %lld > %d, disconnect peer=%d\n", locator.vHave.size(), MAX_LOCATOR_SZ, pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }

        // We might have announced the currently-being-connected tip using a
        // compact block, which resulted in the peer sending a getblocks
        // request, which we would otherwise respond to without the new block.
        // To avoid this situation we simply verify that we are on our best
        // known chain now. This is super overkill, but we handle it better
        // for getheaders requests, and there are no known nodes which support
        // compact blocks but still use getblocks to request blocks.
        {
            std::shared_ptr<const CBlock> a_recent_block;
            {
                LOCK(cs_most_recent_block);
                a_recent_block = most_recent_block;
            }
            BlockValidationState state;
            if (!m_chainman.ActiveChainstate().ActivateBestChain(state, m_chainparams, a_recent_block)) {
                LogPrint(BCLog::NET, "failed to activate chain (%s)\n", state.ToString());
            }
        }

        LOCK(cs_main);

        // Find the last block the caller has in the main chain
        const CBlockIndex* pindex = m_chainman.m_blockman.FindForkInGlobalIndex(m_chainman.ActiveChain(), locator);

        // Send the rest of the chain
        if (pindex)
            pindex = m_chainman.ActiveChain().Next(pindex);
        int nLimit = 500;
        LogPrint(BCLog::NET, "getblocks %d to %s limit %d from peer=%d\n", (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), nLimit, pfrom.GetId());
        for (; pindex; pindex = m_chainman.ActiveChain().Next(pindex))
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrint(BCLog::NET, "  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            // If pruning, don't inv blocks unless we have on disk and are likely to still have
            // for some reasonable time window (1 hour) that block relay might require.
            const int nPrunedBlocksLikelyToHave = MIN_BLOCKS_TO_KEEP - 3600 / m_chainparams.GetConsensus().nPowTargetSpacing;
            if (fPruneMode && (!(pindex->nStatus & BLOCK_HAVE_DATA) || pindex->nHeight <= m_chainman.ActiveChain().Tip()->nHeight - nPrunedBlocksLikelyToHave))
            {
                LogPrint(BCLog::NET, " getblocks stopping, pruned or too old block at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            WITH_LOCK(peer->m_block_inv_mutex, peer->m_blocks_for_inv_relay.push_back(pindex->GetBlockHash()));
            if (--nLimit <= 0) {
                // When this block is requested, we'll send an inv that'll
                // trigger the peer to getblocks the next batch of inventory.
                LogPrint(BCLog::NET, "  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                WITH_LOCK(peer->m_block_inv_mutex, {peer->m_continuation_block = pindex->GetBlockHash();});
                break;
            }
        }
        return;
    }

    if (msg_type == NetMsgType::GETBLOCKTXN) {
        BlockTransactionsRequest req;
        vRecv >> req;

        std::shared_ptr<const CBlock> recent_block;
        {
            LOCK(cs_most_recent_block);
            if (most_recent_block_hash == req.blockhash)
                recent_block = most_recent_block;
            // Unlock cs_most_recent_block to avoid cs_main lock inversion
        }
        if (recent_block) {
            SendBlockTransactions(pfrom, *recent_block, req);
            return;
        }

        {
            LOCK(cs_main);

            const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(req.blockhash);
            if (!pindex || !(pindex->nStatus & BLOCK_HAVE_DATA)) {
                LogPrint(BCLog::NET, "Peer %d sent us a getblocktxn for a block we don't have\n", pfrom.GetId());
                return;
            }

            if (pindex->nHeight >= m_chainman.ActiveChain().Height() - MAX_BLOCKTXN_DEPTH) {
                CBlock block;
                bool ret = ReadBlockFromDisk(block, pindex, m_chainparams.GetConsensus());
                assert(ret);

                SendBlockTransactions(pfrom, block, req);
                return;
            }
        }

        // If an older block is requested (should never happen in practice,
        // but can happen in tests) send a block response instead of a
        // blocktxn response. Sending a full block response instead of a
        // small blocktxn response is preferable in the case where a peer
        // might maliciously send lots of getblocktxn requests to trigger
        // expensive disk reads, because it will require the peer to
        // actually receive all the data read from disk over the network.
        LogPrint(BCLog::NET, "Peer %d sent us a getblocktxn for a block > %i deep\n", pfrom.GetId(), MAX_BLOCKTXN_DEPTH);
        CInv inv;
        WITH_LOCK(cs_main, inv.type = State(pfrom.GetId())->fWantsCmpctWitness ? MSG_WITNESS_BLOCK : MSG_BLOCK);
        inv.hash = req.blockhash;
        WITH_LOCK(peer->m_getdata_requests_mutex, peer->m_getdata_requests.push_back(inv));
        // The message processing loop will go around again (without pausing) and we'll respond then
        return;
    }

    if (msg_type == NetMsgType::GETHEADERS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "getheaders locator size %lld > %d, disconnect peer=%d\n", locator.vHave.size(), MAX_LOCATOR_SZ, pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }

        LOCK(cs_main);
        if (m_chainman.ActiveChainstate().IsInitialBlockDownload() && !pfrom.HasPermission(PF_DOWNLOAD)) {
            LogPrint(BCLog::NET, "Ignoring getheaders from peer=%d because node is in initial block download\n", pfrom.GetId());
            return;
        }

        CNodeState *nodestate = State(pfrom.GetId());
        const CBlockIndex* pindex = nullptr;
        if (locator.IsNull())
        {
            // If locator is null, return the hashStop block
            pindex = m_chainman.m_blockman.LookupBlockIndex(hashStop);
            if (!pindex) {
                return;
            }

            if (!BlockRequestAllowed(pindex)) {
                LogPrint(BCLog::NET, "%s: ignoring request from peer=%i for old block header that isn't in the main chain\n", __func__, pfrom.GetId());
                return;
            }
        }
        else
        {
            // Find the last block the caller has in the main chain
            pindex = m_chainman.m_blockman.FindForkInGlobalIndex(m_chainman.ActiveChain(), locator);
            if (pindex)
                pindex = m_chainman.ActiveChain().Next(pindex);
        }

        // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
        std::vector<CBlock> vHeaders;
        int nLimit = MAX_HEADERS_RESULTS;
        LogPrint(BCLog::NET, "getheaders %d to %s from peer=%d\n", (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), pfrom.GetId());
        for (; pindex; pindex = m_chainman.ActiveChain().Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        }
        // pindex can be nullptr either if we sent m_chainman.ActiveChain().Tip() OR
        // if our peer has m_chainman.ActiveChain().Tip() (and thus we are sending an empty
        // headers message). In both cases it's safe to update
        // pindexBestHeaderSent to be our tip.
        //
        // It is important that we simply reset the BestHeaderSent value here,
        // and not max(BestHeaderSent, newHeaderSent). We might have announced
        // the currently-being-connected tip using a compact block, which
        // resulted in the peer sending a headers request, which we respond to
        // without the new block. By resetting the BestHeaderSent, we ensure we
        // will re-announce the new block via headers (or compact blocks again)
        // in the SendMessages logic.
        nodestate->pindexBestHeaderSent = pindex ? pindex : m_chainman.ActiveChain().Tip();
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
        return;
    }

    if (msg_type == NetMsgType::TX) {
        // Stop processing the transaction early if
        // 1) We are in blocks only mode and peer has no relay permission
        // 2) This peer is a block-relay-only peer
        if ((m_ignore_incoming_txs && !pfrom.HasPermission(PF_RELAY)) || (pfrom.m_tx_relay == nullptr))
        {
            LogPrint(BCLog::NET, "transaction sent in violation of protocol peer=%d\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }

        CTransactionRef ptx;
        vRecv >> ptx;
        const CTransaction& tx = *ptx;

        const uint256& txid = ptx->GetHash();
        const uint256& wtxid = ptx->GetWitnessHash();

        LOCK2(cs_main, g_cs_orphans);

        CNodeState* nodestate = State(pfrom.GetId());

        const uint256& hash = nodestate->m_wtxid_relay ? wtxid : txid;
        pfrom.AddKnownTx(hash);
        if (nodestate->m_wtxid_relay && txid != wtxid) {
            // Insert txid into filterInventoryKnown, even for
            // wtxidrelay peers. This prevents re-adding of
            // unconfirmed parents to the recently_announced
            // filter, when a child tx is requested. See
            // ProcessGetData().
            pfrom.AddKnownTx(txid);
        }

        m_txrequest.ReceivedResponse(pfrom.GetId(), txid);
        if (tx.HasWitness()) m_txrequest.ReceivedResponse(pfrom.GetId(), wtxid);

        // We do the AlreadyHaveTx() check using wtxid, rather than txid - in the
        // absence of witness malleation, this is strictly better, because the
        // recent rejects filter may contain the wtxid but rarely contains
        // the txid of a segwit transaction that has been rejected.
        // In the presence of witness malleation, it's possible that by only
        // doing the check with wtxid, we could overlook a transaction which
        // was confirmed with a different witness, or exists in our mempool
        // with a different witness, but this has limited downside:
        // mempool validation does its own lookup of whether we have the txid
        // already; and an adversary can already relay us old transactions
        // (older than our recency filter) if trying to DoS us, without any need
        // for witness malleation.
        if (AlreadyHaveTx(GenTxid(/* is_wtxid=*/true, wtxid))) {
            if (pfrom.HasPermission(PF_FORCERELAY)) {
                // Always relay transactions received from peers with forcerelay
                // permission, even if they were already in the mempool, allowing
                // the node to function as a gateway for nodes hidden behind it.
                if (!m_mempool.exists(tx.GetHash())) {
                    LogPrintf("Not relaying non-mempool transaction %s from forcerelay peer=%d\n", tx.GetHash().ToString(), pfrom.GetId());
                } else {
                    LogPrintf("Force relaying tx %s from peer=%d\n", tx.GetHash().ToString(), pfrom.GetId());
                    RelayTransaction(tx.GetHash(), tx.GetWitnessHash());
                }
            }
            return;
        }

        const MempoolAcceptResult result = AcceptToMemoryPool(m_chainman.ActiveChainstate(), m_mempool, ptx, false /* bypass_limits */);
        const TxValidationState& state = result.m_state;

        if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            m_mempool.check(m_chainman.ActiveChainstate());
            // As this version of the transaction was acceptable, we can forget about any
            // requests for it.
            m_txrequest.ForgetTxHash(tx.GetHash());
            m_txrequest.ForgetTxHash(tx.GetWitnessHash());
            RelayTransaction(tx.GetHash(), tx.GetWitnessHash());
            m_orphanage.AddChildrenToWorkSet(tx, peer->m_orphan_work_set);

            pfrom.nLastTXTime = GetTime();

            LogPrint(BCLog::MEMPOOL, "AcceptToMemoryPool: peer=%d: accepted %s (poolsz %u txn, %u kB)\n",
                pfrom.GetId(),
                tx.GetHash().ToString(),
                m_mempool.size(), m_mempool.DynamicMemoryUsage() / 1000);

            for (const CTransactionRef& removedTx : result.m_replaced_transactions.value()) {
                AddToCompactExtraTransactions(removedTx);
            }

            // Recursively process any orphan transactions that depended on this one
            ProcessOrphanTx(peer->m_orphan_work_set);
        }
        else if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS)
        {
            bool fRejectedParents = false; // It may be the case that the orphans parents have all been rejected

            // Deduplicate parent txids, so that we don't have to loop over
            // the same parent txid more than once down below.
            std::vector<uint256> unique_parents;
            unique_parents.reserve(tx.vin.size());
            for (const CTxIn& txin : tx.vin) {
                // We start with all parents, and then remove duplicates below.
                unique_parents.push_back(txin.prevout.hash);
            }
            std::sort(unique_parents.begin(), unique_parents.end());
            unique_parents.erase(std::unique(unique_parents.begin(), unique_parents.end()), unique_parents.end());
            for (const uint256& parent_txid : unique_parents) {
                if (recentRejects->contains(parent_txid)) {
                    fRejectedParents = true;
                    break;
                }
            }
            if (!fRejectedParents) {
                const auto current_time = GetTime<std::chrono::microseconds>();

                for (const uint256& parent_txid : unique_parents) {
                    // Here, we only have the txid (and not wtxid) of the
                    // inputs, so we only request in txid mode, even for
                    // wtxidrelay peers.
                    // Eventually we should replace this with an improved
                    // protocol for getting all unconfirmed parents.
                    const GenTxid gtxid{/* is_wtxid=*/false, parent_txid};
                    pfrom.AddKnownTx(parent_txid);
                    if (!AlreadyHaveTx(gtxid)) AddTxAnnouncement(pfrom, gtxid, current_time);
                }

                if (m_orphanage.AddTx(ptx, pfrom.GetId())) {
                    AddToCompactExtraTransactions(ptx);
                }

                // Once added to the orphan pool, a tx is considered AlreadyHave, and we shouldn't request it anymore.
                m_txrequest.ForgetTxHash(tx.GetHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());

                // DoS prevention: do not allow m_orphanage to grow unbounded (see CVE-2012-3789)
                unsigned int nMaxOrphanTx = (unsigned int)std::max((int64_t)0, gArgs.GetArg("-maxorphantx", DEFAULT_MAX_ORPHAN_TRANSACTIONS));
                unsigned int nEvicted = m_orphanage.LimitOrphans(nMaxOrphanTx);
                if (nEvicted > 0) {
                    LogPrint(BCLog::MEMPOOL, "orphanage overflow, removed %u tx\n", nEvicted);
                }
            } else {
                LogPrint(BCLog::MEMPOOL, "not keeping orphan with rejected parents %s\n",tx.GetHash().ToString());
                // We will continue to reject this tx since it has rejected
                // parents so avoid re-requesting it from other peers.
                // Here we add both the txid and the wtxid, as we know that
                // regardless of what witness is provided, we will not accept
                // this, so we don't need to allow for redownload of this txid
                // from any of our non-wtxidrelay peers.
                recentRejects->insert(tx.GetHash());
                recentRejects->insert(tx.GetWitnessHash());
                m_txrequest.ForgetTxHash(tx.GetHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());
            }
        } else {
            if (state.GetResult() != TxValidationResult::TX_WITNESS_STRIPPED) {
                // We can add the wtxid of this transaction to our reject filter.
                // Do not add txids of witness transactions or witness-stripped
                // transactions to the filter, as they can have been malleated;
                // adding such txids to the reject filter would potentially
                // interfere with relay of valid transactions from peers that
                // do not support wtxid-based relay. See
                // https://github.com/bitcoin/bitcoin/issues/8279 for details.
                // We can remove this restriction (and always add wtxids to
                // the filter even for witness stripped transactions) once
                // wtxid-based relay is broadly deployed.
                // See also comments in https://github.com/bitcoin/bitcoin/pull/18044#discussion_r443419034
                // for concerns around weakening security of unupgraded nodes
                // if we start doing this too early.
                assert(recentRejects);
                recentRejects->insert(tx.GetWitnessHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());
                // If the transaction failed for TX_INPUTS_NOT_STANDARD,
                // then we know that the witness was irrelevant to the policy
                // failure, since this check depends only on the txid
                // (the scriptPubKey being spent is covered by the txid).
                // Add the txid to the reject filter to prevent repeated
                // processing of this transaction in the event that child
                // transactions are later received (resulting in
                // parent-fetching by txid via the orphan-handling logic).
                if (state.GetResult() == TxValidationResult::TX_INPUTS_NOT_STANDARD && tx.GetWitnessHash() != tx.GetHash()) {
                    recentRejects->insert(tx.GetHash());
                    m_txrequest.ForgetTxHash(tx.GetHash());
                }
                if (RecursiveDynamicUsage(*ptx) < 100000) {
                    AddToCompactExtraTransactions(ptx);
                }
            }
        }

        // If a tx has been detected by recentRejects, we will have reached
        // this point and the tx will have been ignored. Because we haven't run
        // the tx through AcceptToMemoryPool, we won't have computed a DoS
        // score for it or determined exactly why we consider it invalid.
        //
        // This means we won't penalize any peer subsequently relaying a DoSy
        // tx (even if we penalized the first peer who gave it to us) because
        // we have to account for recentRejects showing false positives. In
        // other words, we shouldn't penalize a peer if we aren't *sure* they
        // submitted a DoSy tx.
        //
        // Note that recentRejects doesn't just record DoSy or invalid
        // transactions, but any tx not accepted by the mempool, which may be
        // due to node policy (vs. consensus). So we can't blanket penalize a
        // peer simply for relaying a tx that our recentRejects has caught,
        // regardless of false positives.

        if (state.IsInvalid()) {
            LogPrint(BCLog::MEMPOOLREJ, "%s from peer=%d was not accepted: %s\n", tx.GetHash().ToString(),
                pfrom.GetId(),
                state.ToString());
            MaybePunishNodeForTx(pfrom.GetId(), state);
        }
        return;
    }

    if (msg_type == NetMsgType::CMPCTBLOCK)
    {
        // Ignore cmpctblock received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected cmpctblock message received from peer %d\n", pfrom.GetId());
            return;
        }

        CBlockHeaderAndShortTxIDs cmpctblock;
        vRecv >> cmpctblock;

        bool received_new_header = false;

        {
        LOCK(cs_main);

        if (!m_chainman.m_blockman.LookupBlockIndex(cmpctblock.header.hashPrevBlock)) {
            // Doesn't connect (or is genesis), instead of DoSing in AcceptBlockHeader, request deeper headers
            if (!m_chainman.ActiveChainstate().IsInitialBlockDownload())
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(pindexBestHeader), uint256()));
            return;
        }

        if (!m_chainman.m_blockman.LookupBlockIndex(cmpctblock.header.GetHash())) {
            received_new_header = true;
        }
        }

        const CBlockIndex *pindex = nullptr;
        BlockValidationState state;
        if (!m_chainman.ProcessNewBlockHeaders({cmpctblock.header}, state, m_chainparams, &pindex)) {
            if (state.IsInvalid()) {
                MaybePunishNodeForBlock(pfrom.GetId(), state, /*via_compact_block*/ true, "invalid header via cmpctblock");
                return;
            }
        }

        // When we succeed in decoding a block's txids from a cmpctblock
        // message we typically jump to the BLOCKTXN handling code, with a
        // dummy (empty) BLOCKTXN message, to re-use the logic there in
        // completing processing of the putative block (without cs_main).
        bool fProcessBLOCKTXN = false;
        CDataStream blockTxnMsg(SER_NETWORK, PROTOCOL_VERSION);

        // If we end up treating this as a plain headers message, call that as well
        // without cs_main.
        bool fRevertToHeaderProcessing = false;

        // Keep a CBlock for "optimistic" compactblock reconstructions (see
        // below)
        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        bool fBlockReconstructed = false;

        {
        LOCK2(cs_main, g_cs_orphans);
        // If AcceptBlockHeader returned true, it set pindex
        assert(pindex);
        UpdateBlockAvailability(pfrom.GetId(), pindex->GetBlockHash());

        CNodeState *nodestate = State(pfrom.GetId());

        // If this was a new header with more work than our tip, update the
        // peer's last block announcement time
        if (received_new_header && pindex->nChainWork > m_chainman.ActiveChain().Tip()->nChainWork) {
            nodestate->m_last_block_announcement = GetTime();
        }

        std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator blockInFlightIt = mapBlocksInFlight.find(pindex->GetBlockHash());
        bool fAlreadyInFlight = blockInFlightIt != mapBlocksInFlight.end();

        if (pindex->nStatus & BLOCK_HAVE_DATA) // Nothing to do here
            return;

        if (pindex->nChainWork <= m_chainman.ActiveChain().Tip()->nChainWork || // We know something better
                pindex->nTx != 0) { // We had this block at some point, but pruned it
            if (fAlreadyInFlight) {
                // We requested this block for some reason, but our mempool will probably be useless
                // so we just grab the block via normal getdata
                std::vector<CInv> vInv(1);
                vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(pfrom), cmpctblock.header.GetHash());
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
            }
            return;
        }

        // If we're not close to tip yet, give up and let parallel block fetch work its magic
        if (!fAlreadyInFlight && !CanDirectFetch()) {
            return;
        }

        if (IsWitnessEnabled(pindex->pprev, m_chainparams.GetConsensus()) && !nodestate->fSupportsDesiredCmpctVersion) {
            // Don't bother trying to process compact blocks from v1 peers
            // after segwit activates.
            return;
        }

        // We want to be a bit conservative just to be extra careful about DoS
        // possibilities in compact block processing...
        if (pindex->nHeight <= m_chainman.ActiveChain().Height() + 2) {
            if ((!fAlreadyInFlight && nodestate->nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) ||
                 (fAlreadyInFlight && blockInFlightIt->second.first == pfrom.GetId())) {
                std::list<QueuedBlock>::iterator* queuedBlockIt = nullptr;
                if (!MarkBlockAsInFlight(pfrom.GetId(), pindex->GetBlockHash(), pindex, &queuedBlockIt)) {
                    if (!(*queuedBlockIt)->partialBlock)
                        (*queuedBlockIt)->partialBlock.reset(new PartiallyDownloadedBlock(&m_mempool));
                    else {
                        // The block was already in flight using compact blocks from the same peer
                        LogPrint(BCLog::NET, "Peer sent us compact block we were already syncing!\n");
                        return;
                    }
                }

                PartiallyDownloadedBlock& partialBlock = *(*queuedBlockIt)->partialBlock;
                ReadStatus status = partialBlock.InitData(cmpctblock, vExtraTxnForCompact);
                if (status == READ_STATUS_INVALID) {
                    MarkBlockAsReceived(pindex->GetBlockHash()); // Reset in-flight state in case Misbehaving does not result in a disconnect
                    Misbehaving(pfrom.GetId(), 100, "invalid compact block");
                    return;
                } else if (status == READ_STATUS_FAILED) {
                    // Duplicate txindexes, the block is now in-flight, so just request it
                    std::vector<CInv> vInv(1);
                    vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(pfrom), cmpctblock.header.GetHash());
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                    return;
                }

                BlockTransactionsRequest req;
                for (size_t i = 0; i < cmpctblock.BlockTxCount(); i++) {
                    if (!partialBlock.IsTxAvailable(i))
                        req.indexes.push_back(i);
                }
                if (req.indexes.empty()) {
                    // Dirty hack to jump to BLOCKTXN code (TODO: move message handling into their own functions)
                    BlockTransactions txn;
                    txn.blockhash = cmpctblock.header.GetHash();
                    blockTxnMsg << txn;
                    fProcessBLOCKTXN = true;
                } else {
                    req.blockhash = pindex->GetBlockHash();
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETBLOCKTXN, req));
                }
            } else {
                // This block is either already in flight from a different
                // peer, or this peer has too many blocks outstanding to
                // download from.
                // Optimistically try to reconstruct anyway since we might be
                // able to without any round trips.
                PartiallyDownloadedBlock tempBlock(&m_mempool);
                ReadStatus status = tempBlock.InitData(cmpctblock, vExtraTxnForCompact);
                if (status != READ_STATUS_OK) {
                    // TODO: don't ignore failures
                    return;
                }
                std::vector<CTransactionRef> dummy;
                status = tempBlock.FillBlock(*pblock, dummy);
                if (status == READ_STATUS_OK) {
                    fBlockReconstructed = true;
                }
            }
        } else {
            if (fAlreadyInFlight) {
                // We requested this block, but its far into the future, so our
                // mempool will probably be useless - request the block normally
                std::vector<CInv> vInv(1);
                vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(pfrom), cmpctblock.header.GetHash());
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                return;
            } else {
                // If this was an announce-cmpctblock, we want the same treatment as a header message
                fRevertToHeaderProcessing = true;
            }
        }
        } // cs_main

        if (fProcessBLOCKTXN) {
            return ProcessMessage(pfrom, NetMsgType::BLOCKTXN, blockTxnMsg, time_received, interruptMsgProc);
        }

        if (fRevertToHeaderProcessing) {
            // Headers received from HB compact block peers are permitted to be
            // relayed before full validation (see BIP 152), so we don't want to disconnect
            // the peer if the header turns out to be for an invalid block.
            // Note that if a peer tries to build on an invalid chain, that
            // will be detected and the peer will be disconnected/discouraged.
            return ProcessHeadersMessage(pfrom, *peer, {cmpctblock.header}, /*via_compact_block=*/true);
        }

        if (fBlockReconstructed) {
            // If we got here, we were able to optimistically reconstruct a
            // block that is in flight from some other peer.
            {
                LOCK(cs_main);
                mapBlockSource.emplace(pblock->GetHash(), std::make_pair(pfrom.GetId(), false));
            }
            bool fNewBlock = false;
            // Setting fForceProcessing to true means that we bypass some of
            // our anti-DoS protections in AcceptBlock, which filters
            // unrequested blocks that might be trying to waste our resources
            // (eg disk space). Because we only try to reconstruct blocks when
            // we're close to caught up (via the CanDirectFetch() requirement
            // above, combined with the behavior of not requesting blocks until
            // we have a chain with at least nMinimumChainWork), and we ignore
            // compact blocks with less work than our tip, it is safe to treat
            // reconstructed compact blocks as having been requested.
            m_chainman.ProcessNewBlock(m_chainparams, pblock, /*fForceProcessing=*/true, &fNewBlock);
            if (fNewBlock) {
                pfrom.nLastBlockTime = GetTime();
            } else {
                LOCK(cs_main);
                mapBlockSource.erase(pblock->GetHash());
            }
            LOCK(cs_main); // hold cs_main for CBlockIndex::IsValid()
            if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS)) {
                // Clear download state for this block, which is in
                // process from some other peer.  We do this after calling
                // ProcessNewBlock so that a malleated cmpctblock announcement
                // can't be used to interfere with block relay.
                MarkBlockAsReceived(pblock->GetHash());
            }
        }
        return;
    }

    if (msg_type == NetMsgType::BLOCKTXN)
    {
        // Ignore blocktxn received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected blocktxn message received from peer %d\n", pfrom.GetId());
            return;
        }

        BlockTransactions resp;
        vRecv >> resp;

        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        bool fBlockRead = false;
        {
            LOCK(cs_main);

            std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator it = mapBlocksInFlight.find(resp.blockhash);
            if (it == mapBlocksInFlight.end() || !it->second.second->partialBlock ||
                    it->second.first != pfrom.GetId()) {
                LogPrint(BCLog::NET, "Peer %d sent us block transactions for block we weren't expecting\n", pfrom.GetId());
                return;
            }

            PartiallyDownloadedBlock& partialBlock = *it->second.second->partialBlock;
            ReadStatus status = partialBlock.FillBlock(*pblock, resp.txn);
            if (status == READ_STATUS_INVALID) {
                MarkBlockAsReceived(resp.blockhash); // Reset in-flight state in case Misbehaving does not result in a disconnect
                Misbehaving(pfrom.GetId(), 100, "invalid compact block/non-matching block transactions");
                return;
            } else if (status == READ_STATUS_FAILED) {
                // Might have collided, fall back to getdata now :(
                std::vector<CInv> invs;
                invs.push_back(CInv(MSG_BLOCK | GetFetchFlags(pfrom), resp.blockhash));
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, invs));
            } else {
                // Block is either okay, or possibly we received
                // READ_STATUS_CHECKBLOCK_FAILED.
                // Note that CheckBlock can only fail for one of a few reasons:
                // 1. bad-proof-of-work (impossible here, because we've already
                //    accepted the header)
                // 2. merkleroot doesn't match the transactions given (already
                //    caught in FillBlock with READ_STATUS_FAILED, so
                //    impossible here)
                // 3. the block is otherwise invalid (eg invalid coinbase,
                //    block is too big, too many legacy sigops, etc).
                // So if CheckBlock failed, #3 is the only possibility.
                // Under BIP 152, we don't discourage the peer unless proof of work is
                // invalid (we don't require all the stateless checks to have
                // been run).  This is handled below, so just treat this as
                // though the block was successfully read, and rely on the
                // handling in ProcessNewBlock to ensure the block index is
                // updated, etc.
                MarkBlockAsReceived(resp.blockhash); // it is now an empty pointer
                fBlockRead = true;
                // mapBlockSource is used for potentially punishing peers and
                // updating which peers send us compact blocks, so the race
                // between here and cs_main in ProcessNewBlock is fine.
                // BIP 152 permits peers to relay compact blocks after validating
                // the header only; we should not punish peers if the block turns
                // out to be invalid.
                mapBlockSource.emplace(resp.blockhash, std::make_pair(pfrom.GetId(), false));
            }
        } // Don't hold cs_main when we call into ProcessNewBlock
        if (fBlockRead) {
            bool fNewBlock = false;
            // Since we requested this block (it was in mapBlocksInFlight), force it to be processed,
            // even if it would not be a candidate for new tip (missing previous block, chain not long enough, etc)
            // This bypasses some anti-DoS logic in AcceptBlock (eg to prevent
            // disk-space attacks), but this should be safe due to the
            // protections in the compact block handler -- see related comment
            // in compact block optimistic reconstruction handling.
            m_chainman.ProcessNewBlock(m_chainparams, pblock, /*fForceProcessing=*/true, &fNewBlock);
            if (fNewBlock) {
                pfrom.nLastBlockTime = GetTime();
            } else {
                LOCK(cs_main);
                mapBlockSource.erase(pblock->GetHash());
            }
        }
        return;
    }

    if (msg_type == NetMsgType::HEADERS)
    {
        // Ignore headers received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected headers message received from peer %d\n", pfrom.GetId());
            return;
        }

        std::vector<CBlockHeader> headers;

        // Bypass the normal CBlock deserialization, as we don't want to risk deserializing 2000 full blocks.
        unsigned int nCount = ReadCompactSize(vRecv);
        if (nCount > MAX_HEADERS_RESULTS) {
            Misbehaving(pfrom.GetId(), 20, strprintf("headers message size = %u", nCount));
            return;
        }
        headers.resize(nCount);
        for (unsigned int n = 0; n < nCount; n++) {
            vRecv >> headers[n];
            ReadCompactSize(vRecv); // ignore tx count; assume it is 0.
        }

        return ProcessHeadersMessage(pfrom, *peer, headers, /*via_compact_block=*/false);
    }

    if (msg_type == NetMsgType::BLOCK)
    {
        // Ignore block received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected block message received from peer %d\n", pfrom.GetId());
            return;
        }

        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        vRecv >> *pblock;

        LogPrint(BCLog::NET, "received block %s peer=%d\n", pblock->GetHash().ToString(), pfrom.GetId());

        bool forceProcessing = false;
        const uint256 hash(pblock->GetHash());
        {
            LOCK(cs_main);
            // Also always process if we requested the block explicitly, as we may
            // need it even though it is not a candidate for a new best tip.
            forceProcessing |= MarkBlockAsReceived(hash);
            // mapBlockSource is only used for punishing peers and setting
            // which peers send us compact blocks, so the race between here and
            // cs_main in ProcessNewBlock is fine.
            mapBlockSource.emplace(hash, std::make_pair(pfrom.GetId(), true));
        }
        bool fNewBlock = false;
        m_chainman.ProcessNewBlock(m_chainparams, pblock, forceProcessing, &fNewBlock);
        if (fNewBlock) {
            pfrom.nLastBlockTime = GetTime();
        } else {
            LOCK(cs_main);
            mapBlockSource.erase(pblock->GetHash());
        }
        return;
    }

    if (msg_type == NetMsgType::GETADDR) {
        // This asymmetric behavior for inbound and outbound connections was introduced
        // to prevent a fingerprinting attack: an attacker can send specific fake addresses
        // to users' AddrMan and later request them by sending getaddr messages.
        // Making nodes which are behind NAT and can only make outgoing connections ignore
        // the getaddr message mitigates the attack.
        if (!pfrom.IsInboundConn()) {
            LogPrint(BCLog::NET, "Ignoring \"getaddr\" from %s connection. peer=%d\n", pfrom.ConnectionTypeAsString(), pfrom.GetId());
            return;
        }

        // Only send one GetAddr response per connection to reduce resource waste
        //  and discourage addr stamping of INV announcements.
        if (pfrom.fSentAddr) {
            LogPrint(BCLog::NET, "Ignoring repeated \"getaddr\". peer=%d\n", pfrom.GetId());
            return;
        }
        pfrom.fSentAddr = true;

        pfrom.vAddrToSend.clear();
        std::vector<CAddress> vAddr;
        if (pfrom.HasPermission(PF_ADDR)) {
            vAddr = m_connman.GetAddresses(MAX_ADDR_TO_SEND, MAX_PCT_ADDR_TO_SEND);
        } else {
            vAddr = m_connman.GetAddresses(pfrom, MAX_ADDR_TO_SEND, MAX_PCT_ADDR_TO_SEND);
        }
        FastRandomContext insecure_rand;
        for (const CAddress &addr : vAddr) {
            pfrom.PushAddress(addr, insecure_rand);
        }
        return;
    }

    if (msg_type == NetMsgType::MEMPOOL) {
        if (!(pfrom.GetLocalServices() & NODE_BLOOM) && !pfrom.HasPermission(PF_MEMPOOL))
        {
            if (!pfrom.HasPermission(PF_NOBAN))
            {
                LogPrint(BCLog::NET, "mempool request with bloom filters disabled, disconnect peer=%d\n", pfrom.GetId());
                pfrom.fDisconnect = true;
            }
            return;
        }

        if (m_connman.OutboundTargetReached(false) && !pfrom.HasPermission(PF_MEMPOOL))
        {
            if (!pfrom.HasPermission(PF_NOBAN))
            {
                LogPrint(BCLog::NET, "mempool request with bandwidth limit reached, disconnect peer=%d\n", pfrom.GetId());
                pfrom.fDisconnect = true;
            }
            return;
        }

        if (pfrom.m_tx_relay != nullptr) {
            LOCK(pfrom.m_tx_relay->cs_tx_inventory);
            pfrom.m_tx_relay->fSendMempool = true;
        }
        return;
    }



    //someone is asking for an nftasset class - if we are storing the nft database, then check if we have it
    //if we are not storing the database, or if we dont have it in cache, relay request to our peers
    if (msg_type == NetMsgType::REQNFTASSETCLASS) {

        char assetClassHashHex[65];
        vRecv >> assetClassHashHex;

        bool haveThisOne = false;
        if (gArgs.GetArg("-nftnode", "") == "true") {
            if (g_nftMgr->assetClassInDatabase(assetClassHashHex)) {
                haveThisOne = true;
                CNFTAssetClass* assetClass = g_nftMgr->retrieveAssetClassFromDatabase(assetClassHashHex);
                assetClass->createSerialData();
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SNDNFTASSETCLASS, assetClass->strSerialData));
            }
        }

        else if (g_nftMgr->assetClassInCache(assetClassHashHex)) {
            haveThisOne = true;
            CNFTAssetClass* assetClass = g_nftMgr->retrieveAssetClassFromCache(assetClassHashHex);
            assetClass->createSerialData();            
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SNDNFTASSETCLASS, assetClass->strSerialData));
        }


        if (!haveThisOne)
            g_nftMgr->queueAssetClassRequest(assetClassHashHex);

        return;
    }


    //someone has sent us an NFT class, store it in the cache
    if (msg_type == NetMsgType::SNDNFTASSETCLASS) {
        std::vector<unsigned char> assetData;

        vRecv >> assetData;

        CNFTAssetClass* assetClass = new CNFTAssetClass();
        assetClass->loadFromSerialData(assetData);

        g_nftMgr->addAssetClassToCache(assetClass);

        return;
    }

    if (msg_type == NetMsgType::REQNFTASSET) {
        printf("********************REQNFTASSET**************************\n");
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SNDNFTASSET));
        return;
    }

    if (msg_type == NetMsgType::SNDNFTASSET) {
        printf("********************SNDNFTASSET**************************\n");
        return;
    }


    if (msg_type == NetMsgType::PING) {
        if (pfrom.GetCommonVersion() > BIP0031_VERSION) {
            uint64_t nonce = 0;
            vRecv >> nonce;
            // Echo the message back with the nonce. This allows for two useful features:
            //
            // 1) A remote node can quickly check if the connection is operational
            // 2) Remote nodes can measure the latency of the network thread. If this node
            //    is overloaded it won't respond to pings quickly and the remote node can
            //    avoid sending us more work, like chain download requests.
            //
            // The nonce stops the remote getting confused between different pings: without
            // it, if the remote node sends a ping once per second and this node takes 5
            // seconds to respond to each, the 5th ping the remote sends would appear to
            // return very quickly.
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::PONG, nonce));
        }
        return;
    }

    if (msg_type == NetMsgType::PONG) {
        const auto ping_end = time_received;
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;

        if (nAvail >= sizeof(nonce)) {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (peer->m_ping_nonce_sent != 0) {
                if (nonce == peer->m_ping_nonce_sent) {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished = true;
                    const auto ping_time = ping_end - peer->m_ping_start.load();
                    if (ping_time.count() >= 0) {
                        // Let connman know about this successful ping-pong
                        pfrom.PongReceived(ping_time);
                    } else {
                        // This should never happen
                        sProblem = "Timing mishap";
                    }
                } else {
                    // Nonce mismatches are normal when pings are overlapping
                    sProblem = "Nonce mismatch";
                    if (nonce == 0) {
                        // This is most likely a bug in another implementation somewhere; cancel this ping
                        bPingFinished = true;
                        sProblem = "Nonce zero";
                    }
                }
            } else {
                sProblem = "Unsolicited pong without ping";
            }
        } else {
            // This is most likely a bug in another implementation somewhere; cancel this ping
            bPingFinished = true;
            sProblem = "Short payload";
        }

        if (!(sProblem.empty())) {
            LogPrint(BCLog::NET, "pong peer=%d: %s, %x expected, %x received, %u bytes\n",
                pfrom.GetId(),
                sProblem,
                peer->m_ping_nonce_sent,
                nonce,
                nAvail);
        }
        if (bPingFinished) {
            peer->m_ping_nonce_sent = 0;
        }
        return;
    }

    if (msg_type == NetMsgType::FILTERLOAD) {
        if (!(pfrom.GetLocalServices() & NODE_BLOOM)) {
            LogPrint(BCLog::NET, "filterload received despite not offering bloom services from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints())
        {
            // There is no excuse for sending a too-large filter
            Misbehaving(pfrom.GetId(), 100, "too-large bloom filter");
        }
        else if (pfrom.m_tx_relay != nullptr)
        {
            LOCK(pfrom.m_tx_relay->cs_filter);
            pfrom.m_tx_relay->pfilter.reset(new CBloomFilter(filter));
            pfrom.m_tx_relay->fRelayTxes = true;
        }
        return;
    }

    if (msg_type == NetMsgType::FILTERADD) {
        if (!(pfrom.GetLocalServices() & NODE_BLOOM)) {
            LogPrint(BCLog::NET, "filteradd received despite not offering bloom services from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        std::vector<unsigned char> vData;
        vRecv >> vData;

        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        bool bad = false;
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            bad = true;
        } else if (pfrom.m_tx_relay != nullptr) {
            LOCK(pfrom.m_tx_relay->cs_filter);
            if (pfrom.m_tx_relay->pfilter) {
                pfrom.m_tx_relay->pfilter->insert(vData);
            } else {
                bad = true;
            }
        }
        if (bad) {
            Misbehaving(pfrom.GetId(), 100, "bad filteradd message");
        }
        return;
    }

    if (msg_type == NetMsgType::FILTERCLEAR) {
        if (!(pfrom.GetLocalServices() & NODE_BLOOM)) {
            LogPrint(BCLog::NET, "filterclear received despite not offering bloom services from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        if (pfrom.m_tx_relay == nullptr) {
            return;
        }
        LOCK(pfrom.m_tx_relay->cs_filter);
        pfrom.m_tx_relay->pfilter = nullptr;
        pfrom.m_tx_relay->fRelayTxes = true;
        return;
    }

    if (msg_type == NetMsgType::FEEFILTER) {
        CAmount newFeeFilter = 0;
        vRecv >> newFeeFilter;
        if (MoneyRange(newFeeFilter)) {
            if (pfrom.m_tx_relay != nullptr) {
                pfrom.m_tx_relay->minFeeFilter = newFeeFilter;
            }
            LogPrint(BCLog::NET, "received: feefilter of %s from peer=%d\n", CFeeRate(newFeeFilter).ToString(), pfrom.GetId());
        }
        return;
    }

    if (msg_type == NetMsgType::GETCFILTERS) {
        ProcessGetCFilters(pfrom, vRecv);
        return;
    }

    if (msg_type == NetMsgType::GETCFHEADERS) {
        ProcessGetCFHeaders(pfrom, vRecv);
        return;
    }

    if (msg_type == NetMsgType::GETCFCHECKPT) {
        ProcessGetCFCheckPt(pfrom, vRecv);
        return;
    }

    if (msg_type == NetMsgType::NOTFOUND) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() <= MAX_PEER_TX_ANNOUNCEMENTS + MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            LOCK(::cs_main);
            for (CInv &inv : vInv) {
                if (inv.IsGenTxMsg()) {
                    // If we receive a NOTFOUND message for a tx we requested, mark the announcement for it as
                    // completed in TxRequestTracker.
                    m_txrequest.ReceivedResponse(pfrom.GetId(), inv.hash);
                }
            }
        }
        return;
    }

    // Ignore unknown commands for extensibility
    LogPrint(BCLog::NET, "Unknown command \"%s\" from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());
    return;
}

bool PeerManagerImpl::MaybeDiscourageAndDisconnect(CNode& pnode, Peer& peer)
{
    {
        LOCK(peer.m_misbehavior_mutex);

        // There's nothing to do if the m_should_discourage flag isn't set
        if (!peer.m_should_discourage) return false;

        peer.m_should_discourage = false;
    } // peer.m_misbehavior_mutex

    if (pnode.HasPermission(PF_NOBAN)) {
        // We never disconnect or discourage peers for bad behavior if they have the NOBAN permission flag
        LogPrintf("Warning: not punishing noban peer %d!\n", peer.m_id);
        return false;
    }

    if (pnode.IsManualConn()) {
        // We never disconnect or discourage manual peers for bad behavior
        LogPrintf("Warning: not punishing manually connected peer %d!\n", peer.m_id);
        return false;
    }

    if (pnode.addr.IsLocal()) {
        // We disconnect local peers for bad behavior but don't discourage (since that would discourage
        // all peers on the same local address)
        LogPrint(BCLog::NET, "Warning: disconnecting but not discouraging %s peer %d!\n",
                 pnode.m_inbound_onion ? "inbound onion" : "local", peer.m_id);
        pnode.fDisconnect = true;
        return true;
    }

    // Normal case: Disconnect the peer and discourage all nodes sharing the address
    LogPrint(BCLog::NET, "Disconnecting and discouraging peer %d!\n", peer.m_id);
    if (m_banman) m_banman->Discourage(pnode.addr);
    m_connman.DisconnectNode(pnode.addr);
    return true;
}

bool PeerManagerImpl::ProcessMessages(CNode* pfrom, std::atomic<bool>& interruptMsgProc)
{
    bool fMoreWork = false;

    PeerRef peer = GetPeerRef(pfrom->GetId());
    if (peer == nullptr) return false;

    {
        LOCK(peer->m_getdata_requests_mutex);
        if (!peer->m_getdata_requests.empty()) {
            ProcessGetData(*pfrom, *peer, interruptMsgProc);
        }
    }

    {
        LOCK2(cs_main, g_cs_orphans);
        if (!peer->m_orphan_work_set.empty()) {
            ProcessOrphanTx(peer->m_orphan_work_set);
        }
    }

    if (pfrom->fDisconnect)
        return false;

    // this maintains the order of responses
    // and prevents m_getdata_requests to grow unbounded
    {
        LOCK(peer->m_getdata_requests_mutex);
        if (!peer->m_getdata_requests.empty()) return true;
    }

    {
        LOCK(g_cs_orphans);
        if (!peer->m_orphan_work_set.empty()) return true;
    }

    // Don't bother if send buffer is too full to respond anyway
    if (pfrom->fPauseSend) return false;

    std::list<CNetMessage> msgs;
    {
        LOCK(pfrom->cs_vProcessMsg);
        if (pfrom->vProcessMsg.empty()) return false;
        // Just take one message
        msgs.splice(msgs.begin(), pfrom->vProcessMsg, pfrom->vProcessMsg.begin());
        pfrom->nProcessQueueSize -= msgs.front().m_raw_message_size;
        pfrom->fPauseRecv = pfrom->nProcessQueueSize > m_connman.GetReceiveFloodSize();
        fMoreWork = !pfrom->vProcessMsg.empty();
    }
    CNetMessage& msg(msgs.front());

    if (gArgs.GetBoolArg("-capturemessages", false)) {
        CaptureMessage(pfrom->addr, msg.m_command, MakeUCharSpan(msg.m_recv), /* incoming */ true);
    }

    msg.SetVersion(pfrom->GetCommonVersion());
    const std::string& msg_type = msg.m_command;

    // Message size
    unsigned int nMessageSize = msg.m_message_size;

    try {
        ProcessMessage(*pfrom, msg_type, msg.m_recv, msg.m_time, interruptMsgProc);
        if (interruptMsgProc) return false;
        {
            LOCK(peer->m_getdata_requests_mutex);
            if (!peer->m_getdata_requests.empty()) fMoreWork = true;
        }
    } catch (const std::exception& e) {
        LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' (%s) caught\n", __func__, SanitizeString(msg_type), nMessageSize, e.what(), typeid(e).name());
    } catch (...) {
        LogPrint(BCLog::NET, "%s(%s, %u bytes): Unknown exception caught\n", __func__, SanitizeString(msg_type), nMessageSize);
    }

    return fMoreWork;
}

void PeerManagerImpl::ConsiderEviction(CNode& pto, int64_t time_in_seconds)
{
    AssertLockHeld(cs_main);

    CNodeState &state = *State(pto.GetId());
    const CNetMsgMaker msgMaker(pto.GetCommonVersion());

    if (!state.m_chain_sync.m_protect && pto.IsOutboundOrBlockRelayConn() && state.fSyncStarted) {
        // This is an outbound peer subject to disconnection if they don't
        // announce a block with as much work as the current tip within
        // CHAIN_SYNC_TIMEOUT + HEADERS_RESPONSE_TIME seconds (note: if
        // their chain has more work than ours, we should sync to it,
        // unless it's invalid, in which case we should find that out and
        // disconnect from them elsewhere).
        if (state.pindexBestKnownBlock != nullptr && state.pindexBestKnownBlock->nChainWork >= m_chainman.ActiveChain().Tip()->nChainWork) {
            if (state.m_chain_sync.m_timeout != 0) {
                state.m_chain_sync.m_timeout = 0;
                state.m_chain_sync.m_work_header = nullptr;
                state.m_chain_sync.m_sent_getheaders = false;
            }
        } else if (state.m_chain_sync.m_timeout == 0 || (state.m_chain_sync.m_work_header != nullptr && state.pindexBestKnownBlock != nullptr && state.pindexBestKnownBlock->nChainWork >= state.m_chain_sync.m_work_header->nChainWork)) {
            // Our best block known by this peer is behind our tip, and we're either noticing
            // that for the first time, OR this peer was able to catch up to some earlier point
            // where we checked against our tip.
            // Either way, set a new timeout based on current tip.
            state.m_chain_sync.m_timeout = time_in_seconds + CHAIN_SYNC_TIMEOUT;
            state.m_chain_sync.m_work_header = m_chainman.ActiveChain().Tip();
            state.m_chain_sync.m_sent_getheaders = false;
        } else if (state.m_chain_sync.m_timeout > 0 && time_in_seconds > state.m_chain_sync.m_timeout) {
            // No evidence yet that our peer has synced to a chain with work equal to that
            // of our tip, when we first detected it was behind. Send a single getheaders
            // message to give the peer a chance to update us.
            if (state.m_chain_sync.m_sent_getheaders) {
                // They've run out of time to catch up!
                LogPrintf("Disconnecting outbound peer %d for old chain, best known block = %s\n", pto.GetId(), state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>");
                pto.fDisconnect = true;
            } else {
                assert(state.m_chain_sync.m_work_header);
                LogPrint(BCLog::NET, "sending getheaders to outbound peer=%d to verify chain work (current best known block:%s, benchmark blockhash: %s)\n", pto.GetId(), state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>", state.m_chain_sync.m_work_header->GetBlockHash().ToString());
                m_connman.PushMessage(&pto, msgMaker.Make(NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(state.m_chain_sync.m_work_header->pprev), uint256()));
                state.m_chain_sync.m_sent_getheaders = true;
                constexpr int64_t HEADERS_RESPONSE_TIME = 120; // 2 minutes
                // Bump the timeout to allow a response, which could clear the timeout
                // (if the response shows the peer has synced), reset the timeout (if
                // the peer syncs to the required work but not to our tip), or result
                // in disconnect (if we advance to the timeout and pindexBestKnownBlock
                // has not sufficiently progressed)
                state.m_chain_sync.m_timeout = time_in_seconds + HEADERS_RESPONSE_TIME;
            }
        }
    }
}

void PeerManagerImpl::EvictExtraOutboundPeers(int64_t time_in_seconds)
{
    // If we have any extra block-relay-only peers, disconnect the youngest unless
    // it's given us a block -- in which case, compare with the second-youngest, and
    // out of those two, disconnect the peer who least recently gave us a block.
    // The youngest block-relay-only peer would be the extra peer we connected
    // to temporarily in order to sync our tip; see net.cpp.
    // Note that we use higher nodeid as a measure for most recent connection.
    if (m_connman.GetExtraBlockRelayCount() > 0) {
        std::pair<NodeId, int64_t> youngest_peer{-1, 0}, next_youngest_peer{-1, 0};

        m_connman.ForEachNode([&](CNode* pnode) {
            if (!pnode->IsBlockOnlyConn() || pnode->fDisconnect) return;
            if (pnode->GetId() > youngest_peer.first) {
                next_youngest_peer = youngest_peer;
                youngest_peer.first = pnode->GetId();
                youngest_peer.second = pnode->nLastBlockTime;
            }
        });
        NodeId to_disconnect = youngest_peer.first;
        if (youngest_peer.second > next_youngest_peer.second) {
            // Our newest block-relay-only peer gave us a block more recently;
            // disconnect our second youngest.
            to_disconnect = next_youngest_peer.first;
        }
        m_connman.ForNode(to_disconnect, [&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);
            // Make sure we're not getting a block right now, and that
            // we've been connected long enough for this eviction to happen
            // at all.
            // Note that we only request blocks from a peer if we learn of a
            // valid headers chain with at least as much work as our tip.
            CNodeState *node_state = State(pnode->GetId());
            if (node_state == nullptr ||
                (time_in_seconds - pnode->nTimeConnected >= MINIMUM_CONNECT_TIME && node_state->nBlocksInFlight == 0)) {
                pnode->fDisconnect = true;
                LogPrint(BCLog::NET, "disconnecting extra block-relay-only peer=%d (last block received at time %d)\n", pnode->GetId(), pnode->nLastBlockTime);
                return true;
            } else {
                LogPrint(BCLog::NET, "keeping block-relay-only peer=%d chosen for eviction (connect time: %d, blocks_in_flight: %d)\n",
                    pnode->GetId(), pnode->nTimeConnected, node_state->nBlocksInFlight);
            }
            return false;
        });
    }

    // Check whether we have too many outbound-full-relay peers
    if (m_connman.GetExtraFullOutboundCount() > 0) {
        // If we have more outbound-full-relay peers than we target, disconnect one.
        // Pick the outbound-full-relay peer that least recently announced
        // us a new block, with ties broken by choosing the more recent
        // connection (higher node id)
        NodeId worst_peer = -1;
        int64_t oldest_block_announcement = std::numeric_limits<int64_t>::max();

        m_connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);

            // Only consider outbound-full-relay peers that are not already
            // marked for disconnection
            if (!pnode->IsFullOutboundConn() || pnode->fDisconnect) return;
            CNodeState *state = State(pnode->GetId());
            if (state == nullptr) return; // shouldn't be possible, but just in case
            // Don't evict our protected peers
            if (state->m_chain_sync.m_protect) return;
            if (state->m_last_block_announcement < oldest_block_announcement || (state->m_last_block_announcement == oldest_block_announcement && pnode->GetId() > worst_peer)) {
                worst_peer = pnode->GetId();
                oldest_block_announcement = state->m_last_block_announcement;
            }
        });
        if (worst_peer != -1) {
            bool disconnected = m_connman.ForNode(worst_peer, [&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                AssertLockHeld(::cs_main);

                // Only disconnect a peer that has been connected to us for
                // some reasonable fraction of our check-frequency, to give
                // it time for new information to have arrived.
                // Also don't disconnect any peer we're trying to download a
                // block from.
                CNodeState &state = *State(pnode->GetId());
                if (time_in_seconds - pnode->nTimeConnected > MINIMUM_CONNECT_TIME && state.nBlocksInFlight == 0) {
                    LogPrint(BCLog::NET, "disconnecting extra outbound peer=%d (last block announcement received at time %d)\n", pnode->GetId(), oldest_block_announcement);
                    pnode->fDisconnect = true;
                    return true;
                } else {
                    LogPrint(BCLog::NET, "keeping outbound peer=%d chosen for eviction (connect time: %d, blocks_in_flight: %d)\n", pnode->GetId(), pnode->nTimeConnected, state.nBlocksInFlight);
                    return false;
                }
            });
            if (disconnected) {
                // If we disconnected an extra peer, that means we successfully
                // connected to at least one peer after the last time we
                // detected a stale tip. Don't try any more extra peers until
                // we next detect a stale tip, to limit the load we put on the
                // network from these extra connections.
                m_connman.SetTryNewOutboundPeer(false);
            }
        }
    }
}

void PeerManagerImpl::CheckForStaleTipAndEvictPeers()
{
    LOCK(cs_main);

    int64_t time_in_seconds = GetTime();

    EvictExtraOutboundPeers(time_in_seconds);

    if (time_in_seconds > m_stale_tip_check_time) {
        // Check whether our tip is stale, and if so, allow using an extra
        // outbound peer
        if (!fImporting && !fReindex && m_connman.GetNetworkActive() && m_connman.GetUseAddrmanOutgoing() && TipMayBeStale()) {
            LogPrintf("Potential stale tip detected, will try using extra outbound peer (last tip update: %d seconds ago)\n", time_in_seconds - m_last_tip_update);
            m_connman.SetTryNewOutboundPeer(true);
        } else if (m_connman.GetTryNewOutboundPeer()) {
            m_connman.SetTryNewOutboundPeer(false);
        }
        m_stale_tip_check_time = time_in_seconds + STALE_CHECK_INTERVAL;
    }

    if (!m_initial_sync_finished && CanDirectFetch()) {
        m_connman.StartExtraBlockRelayPeers();
        m_initial_sync_finished = true;
    }
}

void PeerManagerImpl::MaybeSendPing(CNode& node_to, Peer& peer)
{
    // Use mockable time for ping timeouts.
    // This means that setmocktime may cause pings to time out.
    auto now = GetTime<std::chrono::microseconds>();

    if (m_connman.RunInactivityChecks(node_to) && peer.m_ping_nonce_sent &&
        now > peer.m_ping_start.load() + std::chrono::seconds{TIMEOUT_INTERVAL}) {
        LogPrint(BCLog::NET, "ping timeout: %fs peer=%d\n", 0.000001 * count_microseconds(now - peer.m_ping_start.load()), peer.m_id);
        node_to.fDisconnect = true;
        return;
    }

    const CNetMsgMaker msgMaker(node_to.GetCommonVersion());
    bool pingSend = false;

    if (peer.m_ping_queued) {
        // RPC ping request by user
        pingSend = true;
    }

    if (peer.m_ping_nonce_sent == 0 && now > peer.m_ping_start.load() + PING_INTERVAL) {
        // Ping automatically sent as a latency probe & keepalive.
        pingSend = true;
    }

    if (pingSend) {
        uint64_t nonce = 0;
        while (nonce == 0) {
            GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
        }
        peer.m_ping_queued = false;
        peer.m_ping_start = now;
        if (node_to.GetCommonVersion() > BIP0031_VERSION) {
            peer.m_ping_nonce_sent = nonce;
            m_connman.PushMessage(&node_to, msgMaker.Make(NetMsgType::PING, nonce));
        } else {
            // Peer is too old to support ping command with nonce, pong will never arrive.
            peer.m_ping_nonce_sent = 0;
            m_connman.PushMessage(&node_to, msgMaker.Make(NetMsgType::PING));
        }
    }
}

namespace {
class CompareInvMempoolOrder
{
    CTxMemPool *mp;
    bool m_wtxid_relay;
public:
    explicit CompareInvMempoolOrder(CTxMemPool *_mempool, bool use_wtxid)
    {
        mp = _mempool;
        m_wtxid_relay = use_wtxid;
    }

    bool operator()(std::set<uint256>::iterator a, std::set<uint256>::iterator b)
    {
        /* As std::make_heap produces a max-heap, we want the entries with the
         * fewest ancestors/highest fee to sort later. */
        return mp->CompareDepthAndScore(*b, *a, m_wtxid_relay);
    }
};
}

bool PeerManagerImpl::SendMessages(CNode* pto)
{
    PeerRef peer = GetPeerRef(pto->GetId());
    if (!peer) return false;
    const Consensus::Params& consensusParams = m_chainparams.GetConsensus();

    // We must call MaybeDiscourageAndDisconnect first, to ensure that we'll
    // disconnect misbehaving peers even before the version handshake is complete.
    if (MaybeDiscourageAndDisconnect(*pto, *peer)) return true;

    // Don't send anything until the version handshake is complete
    if (!pto->fSuccessfullyConnected || pto->fDisconnect)
        return true;

    // If we get here, the outgoing message serialization version is set and can't change.
    const CNetMsgMaker msgMaker(pto->GetCommonVersion());

    MaybeSendPing(*pto, *peer);

    // MaybeSendPing may have marked peer for disconnection
    if (pto->fDisconnect) return true;

    {
        LOCK(cs_main);

        CNodeState& state = *State(pto->GetId());

        // Address refresh broadcast
        auto current_time = GetTime<std::chrono::microseconds>();

        if (fListen && pto->RelayAddrsWithConn() &&
            !m_chainman.ActiveChainstate().IsInitialBlockDownload() &&
            pto->m_next_local_addr_send < current_time) {
            // If we've sent before, clear the bloom filter for the peer, so that our
            // self-announcement will actually go out.
            // This might be unnecessary if the bloom filter has already rolled
            // over since our last self-announcement, but there is only a small
            // bandwidth cost that we can incur by doing this (which happens
            // once a day on average).
            if (pto->m_next_local_addr_send != 0us) {
                pto->m_addr_known->reset();
            }
            if (std::optional<CAddress> local_addr = GetLocalAddrForPeer(pto)) {
                FastRandomContext insecure_rand;
                pto->PushAddress(*local_addr, insecure_rand);
            }
            pto->m_next_local_addr_send = PoissonNextSend(current_time, AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL);
        }

        //
        // Message: addr
        //
        if (pto->RelayAddrsWithConn() && pto->m_next_addr_send < current_time) {
            pto->m_next_addr_send = PoissonNextSend(current_time, AVG_ADDRESS_BROADCAST_INTERVAL);
            std::vector<CAddress> vAddr;
            vAddr.reserve(pto->vAddrToSend.size());
            assert(pto->m_addr_known);

            const char* msg_type;
            int make_flags;
            if (pto->m_wants_addrv2) {
                msg_type = NetMsgType::ADDRV2;
                make_flags = ADDRV2_FORMAT;
            } else {
                msg_type = NetMsgType::ADDR;
                make_flags = 0;
            }

            for (const CAddress& addr : pto->vAddrToSend) {
                if (!pto->m_addr_known->contains(addr.GetKey())) {
                    pto->m_addr_known->insert(addr.GetKey());
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than MAX_ADDR_TO_SEND
                    if (vAddr.size() >= MAX_ADDR_TO_SEND) {
                        m_connman.PushMessage(pto, msgMaker.Make(make_flags, msg_type, vAddr));
                        vAddr.clear();
                    }
                }
            }
            pto->vAddrToSend.clear();
            if (!vAddr.empty())
                m_connman.PushMessage(pto, msgMaker.Make(make_flags, msg_type, vAddr));
            // we only send the big addr message once
            if (pto->vAddrToSend.capacity() > 40)
                pto->vAddrToSend.shrink_to_fit();
        }

        // Start block sync
        if (pindexBestHeader == nullptr)
            pindexBestHeader = m_chainman.ActiveChain().Tip();
        bool fFetch = state.fPreferredDownload || (nPreferredDownload == 0 && !pto->fClient && !pto->IsAddrFetchConn()); // Download if this is a nice peer, or we have no nice peers and this one might do.
        if (!state.fSyncStarted && !pto->fClient && !fImporting && !fReindex) {
            // Only actively request headers from a single peer, unless we're close to today.
            if ((nSyncStarted == 0 && fFetch) || pindexBestHeader->GetBlockTime() > GetAdjustedTime() - 24 * 60 * 60) {
                state.fSyncStarted = true;
                state.m_headers_sync_timeout = current_time + HEADERS_DOWNLOAD_TIMEOUT_BASE +
                                               (
                                                   // Convert HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER to microseconds before scaling
                                                   // to maintain precision
                                                   std::chrono::microseconds{HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER} *
                                                   (GetAdjustedTime() - pindexBestHeader->GetBlockTime()) / consensusParams.nPowTargetSpacing);
                nSyncStarted++;
                const CBlockIndex* pindexStart = pindexBestHeader;
                /* If possible, start at the block preceding the currently
                   best known header.  This ensures that we always get a
                   non-empty list of headers back as long as the peer
                   is up-to-date.  With a non-empty response, we can initialise
                   the peer's known best block.  This wouldn't be possible
                   if we requested starting at pindexBestHeader and
                   got back an empty response.  */
                if (pindexStart->pprev)
                    pindexStart = pindexStart->pprev;
                LogPrint(BCLog::NET, "initial getheaders (%d) to peer=%d (startheight:%d)\n", pindexStart->nHeight, pto->GetId(), peer->m_starting_height);
                m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(pindexStart), uint256()));
            }
        }

        //
        // Try sending block announcements via headers
        //
        {
            // If we have less than MAX_BLOCKS_TO_ANNOUNCE in our
            // list of block hashes we're relaying, and our peer wants
            // headers announcements, then find the first header
            // not yet known to our peer but would connect, and send.
            // If no header would connect, or if we have too many
            // blocks, or if the peer doesn't want headers, just
            // add all to the inv queue.
            LOCK(peer->m_block_inv_mutex);
            std::vector<CBlock> vHeaders;
            bool fRevertToInv = ((!state.fPreferHeaders &&
                                  (!state.fPreferHeaderAndIDs || peer->m_blocks_for_headers_relay.size() > 1)) ||
                                 peer->m_blocks_for_headers_relay.size() > MAX_BLOCKS_TO_ANNOUNCE);
            const CBlockIndex* pBestIndex = nullptr; // last header queued for delivery
            ProcessBlockAvailability(pto->GetId());  // ensure pindexBestKnownBlock is up-to-date

            if (!fRevertToInv) {
                bool fFoundStartingHeader = false;
                // Try to find first header that our peer doesn't have, and
                // then send all headers past that one.  If we come across any
                // headers that aren't on m_chainman.ActiveChain(), give up.
                for (const uint256& hash : peer->m_blocks_for_headers_relay) {
                    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(hash);
                    assert(pindex);
                    if (m_chainman.ActiveChain()[pindex->nHeight] != pindex) {
                        // Bail out if we reorged away from this block
                        fRevertToInv = true;
                        break;
                    }
                    if (pBestIndex != nullptr && pindex->pprev != pBestIndex) {
                        // This means that the list of blocks to announce don't
                        // connect to each other.
                        // This shouldn't really be possible to hit during
                        // regular operation (because reorgs should take us to
                        // a chain that has some block not on the prior chain,
                        // which should be caught by the prior check), but one
                        // way this could happen is by using invalidateblock /
                        // reconsiderblock repeatedly on the tip, causing it to
                        // be added multiple times to m_blocks_for_headers_relay.
                        // Robustly deal with this rare situation by reverting
                        // to an inv.
                        fRevertToInv = true;
                        break;
                    }
                    pBestIndex = pindex;
                    if (fFoundStartingHeader) {
                        // add this to the headers message
                        vHeaders.push_back(pindex->GetBlockHeader());
                    } else if (PeerHasHeader(&state, pindex)) {
                        continue; // keep looking for the first new block
                    } else if (pindex->pprev == nullptr || PeerHasHeader(&state, pindex->pprev)) {
                        // Peer doesn't have this header but they do have the prior one.
                        // Start sending headers.
                        fFoundStartingHeader = true;
                        vHeaders.push_back(pindex->GetBlockHeader());
                    } else {
                        // Peer doesn't have this header or the prior one -- nothing will
                        // connect, so bail out.
                        fRevertToInv = true;
                        break;
                    }
                }
            }
            if (!fRevertToInv && !vHeaders.empty()) {
                if (vHeaders.size() == 1 && state.fPreferHeaderAndIDs) {
                    // We only send up to 1 block as header-and-ids, as otherwise
                    // probably means we're doing an initial-ish-sync or they're slow
                    LogPrint(BCLog::NET, "%s sending header-and-ids %s to peer=%d\n", __func__,
                             vHeaders.front().GetHash().ToString(), pto->GetId());

                    int nSendFlags = state.fWantsCmpctWitness ? 0 : SERIALIZE_TRANSACTION_NO_WITNESS;

                    bool fGotBlockFromCache = false;
                    {
                        LOCK(cs_most_recent_block);
                        if (most_recent_block_hash == pBestIndex->GetBlockHash()) {
                            if (state.fWantsCmpctWitness || !fWitnessesPresentInMostRecentCompactBlock)
                                m_connman.PushMessage(pto, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, *most_recent_compact_block));
                            else {
                                CBlockHeaderAndShortTxIDs cmpctblock(*most_recent_block, state.fWantsCmpctWitness);
                                m_connman.PushMessage(pto, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, cmpctblock));
                            }
                            fGotBlockFromCache = true;
                        }
                    }
                    if (!fGotBlockFromCache) {
                        CBlock block;
                        bool ret = ReadBlockFromDisk(block, pBestIndex, consensusParams);
                        assert(ret);
                        CBlockHeaderAndShortTxIDs cmpctblock(block, state.fWantsCmpctWitness);
                        m_connman.PushMessage(pto, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, cmpctblock));
                    }
                    state.pindexBestHeaderSent = pBestIndex;
                } else if (state.fPreferHeaders) {
                    if (vHeaders.size() > 1) {
                        LogPrint(BCLog::NET, "%s: %u headers, range (%s, %s), to peer=%d\n", __func__,
                                 vHeaders.size(),
                                 vHeaders.front().GetHash().ToString(),
                                 vHeaders.back().GetHash().ToString(), pto->GetId());
                    } else {
                        LogPrint(BCLog::NET, "%s: sending header %s to peer=%d\n", __func__,
                                 vHeaders.front().GetHash().ToString(), pto->GetId());
                    }
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
                    state.pindexBestHeaderSent = pBestIndex;
                } else
                    fRevertToInv = true;
            }
            if (fRevertToInv) {
                // If falling back to using an inv, just try to inv the tip.
                // The last entry in m_blocks_for_headers_relay was our tip at some point
                // in the past.
                if (!peer->m_blocks_for_headers_relay.empty()) {
                    const uint256& hashToAnnounce = peer->m_blocks_for_headers_relay.back();
                    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(hashToAnnounce);
                    assert(pindex);

                    // Warn if we're announcing a block that is not on the main chain.
                    // This should be very rare and could be optimized out.
                    // Just log for now.
                    if (m_chainman.ActiveChain()[pindex->nHeight] != pindex) {
                        LogPrint(BCLog::NET, "Announcing block %s not on main chain (tip=%s)\n",
                                 hashToAnnounce.ToString(), m_chainman.ActiveChain().Tip()->GetBlockHash().ToString());
                    }

                    // If the peer's chain has this block, don't inv it back.
                    if (!PeerHasHeader(&state, pindex)) {
                        peer->m_blocks_for_inv_relay.push_back(hashToAnnounce);
                        LogPrint(BCLog::NET, "%s: sending inv peer=%d hash=%s\n", __func__,
                                 pto->GetId(), hashToAnnounce.ToString());
                    }
                }
            }
            peer->m_blocks_for_headers_relay.clear();
        }

        //
        // Message: inventory
        //
        std::vector<CInv> vInv;
        {
            LOCK(peer->m_block_inv_mutex);
            vInv.reserve(std::max<size_t>(peer->m_blocks_for_inv_relay.size(), INVENTORY_BROADCAST_MAX));

            // Add blocks
            for (const uint256& hash : peer->m_blocks_for_inv_relay) {
                vInv.push_back(CInv(MSG_BLOCK, hash));
                if (vInv.size() == MAX_INV_SZ) {
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                    vInv.clear();
                }
            }
            peer->m_blocks_for_inv_relay.clear();

            if (pto->m_tx_relay != nullptr) {
                LOCK(pto->m_tx_relay->cs_tx_inventory);
                // Check whether periodic sends should happen
                bool fSendTrickle = pto->HasPermission(PF_NOBAN);
                if (pto->m_tx_relay->nNextInvSend < current_time) {
                    fSendTrickle = true;
                    if (pto->IsInboundConn()) {
                        pto->m_tx_relay->nNextInvSend = m_connman.PoissonNextSendInbound(current_time, INBOUND_INVENTORY_BROADCAST_INTERVAL);
                    } else {
                        pto->m_tx_relay->nNextInvSend = PoissonNextSend(current_time, OUTBOUND_INVENTORY_BROADCAST_INTERVAL);
                    }
                }

                // Time to send but the peer has requested we not relay transactions.
                if (fSendTrickle) {
                    LOCK(pto->m_tx_relay->cs_filter);
                    if (!pto->m_tx_relay->fRelayTxes) pto->m_tx_relay->setInventoryTxToSend.clear();
                }

                // Respond to BIP35 mempool requests
                if (fSendTrickle && pto->m_tx_relay->fSendMempool) {
                    auto vtxinfo = m_mempool.infoAll();
                    pto->m_tx_relay->fSendMempool = false;
                    const CFeeRate filterrate{pto->m_tx_relay->minFeeFilter.load()};

                    LOCK(pto->m_tx_relay->cs_filter);

                    for (const auto& txinfo : vtxinfo) {
                        const uint256& hash = state.m_wtxid_relay ? txinfo.tx->GetWitnessHash() : txinfo.tx->GetHash();
                        CInv inv(state.m_wtxid_relay ? MSG_WTX : MSG_TX, hash);
                        pto->m_tx_relay->setInventoryTxToSend.erase(hash);
                        // Don't send transactions that peers will not put into their mempool
                        if (txinfo.fee < filterrate.GetFee(txinfo.vsize)) {
                            continue;
                        }
                        if (pto->m_tx_relay->pfilter) {
                            if (!pto->m_tx_relay->pfilter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        }
                        pto->m_tx_relay->filterInventoryKnown.insert(hash);
                        // Responses to MEMPOOL requests bypass the m_recently_announced_invs filter.
                        vInv.push_back(inv);
                        if (vInv.size() == MAX_INV_SZ) {
                            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                            vInv.clear();
                        }
                    }
                    pto->m_tx_relay->m_last_mempool_req = GetTime<std::chrono::seconds>();
                }

                // Determine transactions to relay
                if (fSendTrickle) {
                    // Produce a vector with all candidates for sending
                    std::vector<std::set<uint256>::iterator> vInvTx;
                    vInvTx.reserve(pto->m_tx_relay->setInventoryTxToSend.size());
                    for (std::set<uint256>::iterator it = pto->m_tx_relay->setInventoryTxToSend.begin(); it != pto->m_tx_relay->setInventoryTxToSend.end(); it++) {
                        vInvTx.push_back(it);
                    }
                    const CFeeRate filterrate{pto->m_tx_relay->minFeeFilter.load()};
                    // Topologically and fee-rate sort the inventory we send for privacy and priority reasons.
                    // A heap is used so that not all items need sorting if only a few are being sent.
                    CompareInvMempoolOrder compareInvMempoolOrder(&m_mempool, state.m_wtxid_relay);
                    std::make_heap(vInvTx.begin(), vInvTx.end(), compareInvMempoolOrder);
                    // No reason to drain out at many times the network's capacity,
                    // especially since we have many peers and some will draw much shorter delays.
                    unsigned int nRelayedTransactions = 0;
                    LOCK(pto->m_tx_relay->cs_filter);
                    while (!vInvTx.empty() && nRelayedTransactions < INVENTORY_BROADCAST_MAX) {
                        // Fetch the top element from the heap
                        std::pop_heap(vInvTx.begin(), vInvTx.end(), compareInvMempoolOrder);
                        std::set<uint256>::iterator it = vInvTx.back();
                        vInvTx.pop_back();
                        uint256 hash = *it;
                        CInv inv(state.m_wtxid_relay ? MSG_WTX : MSG_TX, hash);
                        // Remove it from the to-be-sent set
                        pto->m_tx_relay->setInventoryTxToSend.erase(it);
                        // Check if not in the filter already
                        if (pto->m_tx_relay->filterInventoryKnown.contains(hash)) {
                            continue;
                        }
                        // Not in the mempool anymore? don't bother sending it.
                        auto txinfo = m_mempool.info(ToGenTxid(inv));
                        if (!txinfo.tx) {
                            continue;
                        }
                        auto txid = txinfo.tx->GetHash();
                        auto wtxid = txinfo.tx->GetWitnessHash();
                        // Peer told you to not send transactions at that feerate? Don't bother sending it.
                        if (txinfo.fee < filterrate.GetFee(txinfo.vsize)) {
                            continue;
                        }
                        if (pto->m_tx_relay->pfilter && !pto->m_tx_relay->pfilter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        // Send
                        State(pto->GetId())->m_recently_announced_invs.insert(hash);
                        vInv.push_back(inv);
                        nRelayedTransactions++;
                        {
                            // Expire old relay messages
                            while (!g_relay_expiration.empty() && g_relay_expiration.front().first < current_time) {
                                mapRelay.erase(g_relay_expiration.front().second);
                                g_relay_expiration.pop_front();
                            }

                            auto ret = mapRelay.emplace(txid, std::move(txinfo.tx));
                            if (ret.second) {
                                g_relay_expiration.emplace_back(current_time + RELAY_TX_CACHE_TIME, ret.first);
                            }
                            // Add wtxid-based lookup into mapRelay as well, so that peers can request by wtxid
                            auto ret2 = mapRelay.emplace(wtxid, ret.first->second);
                            if (ret2.second) {
                                g_relay_expiration.emplace_back(current_time + RELAY_TX_CACHE_TIME, ret2.first);
                            }
                        }
                        if (vInv.size() == MAX_INV_SZ) {
                            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                            vInv.clear();
                        }
                        pto->m_tx_relay->filterInventoryKnown.insert(hash);
                        if (hash != txid) {
                            // Insert txid into filterInventoryKnown, even for
                            // wtxidrelay peers. This prevents re-adding of
                            // unconfirmed parents to the recently_announced
                            // filter, when a child tx is requested. See
                            // ProcessGetData().
                            pto->m_tx_relay->filterInventoryKnown.insert(txid);
                        }
                    }
                }
            }
        }
        if (!vInv.empty())
            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));

        // Detect whether we're stalling
        current_time = GetTime<std::chrono::microseconds>();
        if (state.m_stalling_since.count() && state.m_stalling_since < current_time - BLOCK_STALLING_TIMEOUT) {
            // Stalling only triggers when the block download window cannot move. During normal steady state,
            // the download window should be much larger than the to-be-downloaded set of blocks, so disconnection
            // should only happen during initial block download.
            LogPrintf("Peer=%d is stalling block download, disconnecting\n", pto->GetId());
            pto->fDisconnect = true;
            return true;
        }
        // In case there is a block that has been in flight from this peer for block_interval * (1 + 0.5 * N)
        // (with N the number of peers from which we're downloading validated blocks), disconnect due to timeout.
        // We compensate for other peers to prevent killing off peers due to our own downstream link
        // being saturated. We only count validated in-flight blocks so peers can't advertise non-existing block hashes
        // to unreasonably increase our timeout.
        if (state.vBlocksInFlight.size() > 0) {
            QueuedBlock& queuedBlock = state.vBlocksInFlight.front();
            int nOtherPeersWithValidatedDownloads = nPeersWithValidatedDownloads - (state.nBlocksInFlightValidHeaders > 0);
            if (current_time > state.m_downloading_since + std::chrono::seconds{consensusParams.nPowTargetSpacing} * (BLOCK_DOWNLOAD_TIMEOUT_BASE + BLOCK_DOWNLOAD_TIMEOUT_PER_PEER * nOtherPeersWithValidatedDownloads)) {
                LogPrintf("Timeout downloading block %s from peer=%d, disconnecting\n", queuedBlock.hash.ToString(), pto->GetId());
                pto->fDisconnect = true;
                return true;
            }
        }
        // Check for headers sync timeouts
        if (state.fSyncStarted && state.m_headers_sync_timeout < std::chrono::microseconds::max()) {
            // Detect whether this is a stalling initial-headers-sync peer
            if (pindexBestHeader->GetBlockTime() <= GetAdjustedTime() - 24 * 60 * 60) {
                if (current_time > state.m_headers_sync_timeout && nSyncStarted == 1 && (nPreferredDownload - state.fPreferredDownload >= 1)) {
                    // Disconnect a peer (without the noban permission) if it is our only sync peer,
                    // and we have others we could be using instead.
                    // Note: If all our peers are inbound, then we won't
                    // disconnect our sync peer for stalling; we have bigger
                    // problems if we can't get any outbound peers.
                    if (!pto->HasPermission(PF_NOBAN)) {
                        LogPrintf("Timeout downloading headers from peer=%d, disconnecting\n", pto->GetId());
                        pto->fDisconnect = true;
                        return true;
                    } else {
                        LogPrintf("Timeout downloading headers from noban peer=%d, not disconnecting\n", pto->GetId());
                        // Reset the headers sync state so that we have a
                        // chance to try downloading from a different peer.
                        // Note: this will also result in at least one more
                        // getheaders message to be sent to
                        // this peer (eventually).
                        state.fSyncStarted = false;
                        nSyncStarted--;
                        state.m_headers_sync_timeout = 0us;
                    }
                }
            } else {
                // After we've caught up once, reset the timeout so we can't trigger
                // disconnect later.
                state.m_headers_sync_timeout = std::chrono::microseconds::max();
            }
        }

        // Check that outbound peers have reasonable chains
        // GetTime() is used by this anti-DoS logic so we can test this using mocktime
        ConsiderEviction(*pto, GetTime());

        //
        // Message: getdata (blocks)
        //
        std::vector<CInv> vGetData;
        if (!pto->fClient && ((fFetch && !pto->m_limited_node) || !m_chainman.ActiveChainstate().IsInitialBlockDownload()) && state.nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            std::vector<const CBlockIndex*> vToDownload;
            NodeId staller = -1;
            FindNextBlocksToDownload(pto->GetId(), MAX_BLOCKS_IN_TRANSIT_PER_PEER - state.nBlocksInFlight, vToDownload, staller);
            for (const CBlockIndex* pindex : vToDownload) {
                uint32_t nFetchFlags = GetFetchFlags(*pto);
                vGetData.push_back(CInv(MSG_BLOCK | nFetchFlags, pindex->GetBlockHash()));
                MarkBlockAsInFlight(pto->GetId(), pindex->GetBlockHash(), pindex);
                LogPrint(BCLog::NET, "Requesting block %s (%d) peer=%d\n", pindex->GetBlockHash().ToString(),
                         pindex->nHeight, pto->GetId());
            }
            if (state.nBlocksInFlight == 0 && staller != -1) {
                if (State(staller)->m_stalling_since == 0us) {
                    State(staller)->m_stalling_since = current_time;
                    LogPrint(BCLog::NET, "Stall started peer=%d\n", staller);
                }
            }
        }

        //
        // Message: getdata (transactions)
        //
        std::vector<std::pair<NodeId, GenTxid>> expired;
        auto requestable = m_txrequest.GetRequestable(pto->GetId(), current_time, &expired);
        for (const auto& entry : expired) {
            LogPrint(BCLog::NET, "timeout of inflight %s %s from peer=%d\n", entry.second.IsWtxid() ? "wtx" : "tx",
                     entry.second.GetHash().ToString(), entry.first);
        }
        for (const GenTxid& gtxid : requestable) {
            if (!AlreadyHaveTx(gtxid)) {
                LogPrint(BCLog::NET, "Requesting %s %s peer=%d\n", gtxid.IsWtxid() ? "wtx" : "tx",
                         gtxid.GetHash().ToString(), pto->GetId());
                vGetData.emplace_back(gtxid.IsWtxid() ? MSG_WTX : (MSG_TX | GetFetchFlags(*pto)), gtxid.GetHash());
                if (vGetData.size() >= MAX_GETDATA_SZ) {
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));
                    vGetData.clear();
                }
                m_txrequest.RequestedTx(pto->GetId(), gtxid.GetHash(), current_time + GETDATA_TX_INTERVAL);
            } else {
                // We have already seen this transaction, no need to download. This is just a belt-and-suspenders, as
                // this should already be called whenever a transaction becomes AlreadyHaveTx().
                m_txrequest.ForgetTxHash(gtxid.GetHash());
            }
        }


        if (!vGetData.empty())
            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));

        //
        // Message: feefilter
        //
        if (pto->m_tx_relay != nullptr &&
            !m_ignore_incoming_txs &&
            pto->GetCommonVersion() >= FEEFILTER_VERSION &&
            gArgs.GetBoolArg("-feefilter", DEFAULT_FEEFILTER) &&
            !pto->HasPermission(PF_FORCERELAY) // peers with the forcerelay permission should not filter txs to us
        ) {
            CAmount currentFilter = m_mempool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFeePerK();
            static FeeFilterRounder g_filter_rounder{CFeeRate{DEFAULT_MIN_RELAY_TX_FEE}};
            if (m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
                // Received tx-inv messages are discarded when the active
                // chainstate is in IBD, so tell the peer to not send them.
                currentFilter = MAX_MONEY;
            } else {
                static const CAmount MAX_FILTER{g_filter_rounder.round(MAX_MONEY)};
                if (pto->m_tx_relay->lastSentFeeFilter == MAX_FILTER) {
                    // Send the current filter if we sent MAX_FILTER previously
                    // and made it out of IBD.
                    pto->m_tx_relay->m_next_send_feefilter = 0us;
                }
            }
            if (current_time > pto->m_tx_relay->m_next_send_feefilter) {
                CAmount filterToSend = g_filter_rounder.round(currentFilter);
                // We always have a fee filter of at least minRelayTxFee
                filterToSend = std::max(filterToSend, ::minRelayTxFee.GetFeePerK());
                if (filterToSend != pto->m_tx_relay->lastSentFeeFilter) {
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::FEEFILTER, filterToSend));
                    pto->m_tx_relay->lastSentFeeFilter = filterToSend;
                }
                pto->m_tx_relay->m_next_send_feefilter = PoissonNextSend(current_time, AVG_FEEFILTER_BROADCAST_INTERVAL);
            }
            // If the fee filter has changed substantially and it's still more than MAX_FEEFILTER_CHANGE_DELAY
            // until scheduled broadcast, then move the broadcast to within MAX_FEEFILTER_CHANGE_DELAY.
            else if (current_time + MAX_FEEFILTER_CHANGE_DELAY < pto->m_tx_relay->m_next_send_feefilter &&
                     (currentFilter < 3 * pto->m_tx_relay->lastSentFeeFilter / 4 || currentFilter > 4 * pto->m_tx_relay->lastSentFeeFilter / 3)) {
                pto->m_tx_relay->m_next_send_feefilter = current_time + GetRandomDuration<std::chrono::microseconds>(MAX_FEEFILTER_CHANGE_DELAY);
            }
        }


        //Request any NFT asset classes that we need
        if (g_nftMgr->requestAssetClass.size() > 0) {
            m_connman.ForEachNode([this, &msgMaker](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                    AssertLockHeld(::cs_main);
                    {
                        LOCK(g_nftMgr->requestLock);
                        time_t now;
                        time(&now);
                        std::map<std::string, time_t>::iterator i = g_nftMgr->requestAssetClass.begin();
                        while (i != g_nftMgr->requestAssetClass.end()) {
                            char hexHashData[65];
                            memcpy(hexHashData, i->first.c_str(), 65);
                            hexHashData[64] = 0;
                            if (now - i->second > 10) { //TODO - hysterisis - could cause a DOS attack by loading lots of NFT hashes and not loading the assets
                                if (!g_nftMgr->assetClassInDatabase(hexHashData)) {
                                    if (pnode->GetCommonVersion() >= NFT_RELAY_VERSION) {
                                        LogPrint(BCLog::NET, "requesting NFT asset class %s to peer=%d\n", i->first.c_str(), pnode->GetId());
                                        m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::REQNFTASSETCLASS, hexHashData));
                                        i->second = now;
                                    }
                                }
                            }
                            i++;
                        }
                    }

            });
        }

        //request any NFT assets that we need
        if (g_nftMgr->requestAsset.size() > 0) {
            m_connman.ForEachNode([this, &msgMaker](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                AssertLockHeld(::cs_main);
                {
                    LOCK(g_nftMgr->requestLock);
                    time_t now;
                    time(&now);
                    std::map<std::string, time_t>::iterator i = g_nftMgr->requestAsset.begin();
                    while (i != g_nftMgr->requestAsset.end()) {
                        if (now - i->second > 10) {
                            char hexHashData[65];
                            std::string strHexHash = HexStr(i->first);
                            memcpy(hexHashData, strHexHash.c_str(), 65);
                            if (pnode->GetCommonVersion() >= NFT_RELAY_VERSION) {
                                LogPrint(BCLog::NET, "requesting NFT asset  %s to peer=%d\n", hexHashData, pnode->GetId());
                                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::REQNFTASSET, hexHashData));
                                i->second = now;
                            }
                        }
                        i++;
                    }
                }
            });
        }

    } // release cs_main
    return true;
}
