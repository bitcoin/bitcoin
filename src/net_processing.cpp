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
#include <validation.h>
#include <merkleblock.h>
#include <netmessagemaker.h>
#include <netbase.h>
#include <node/blockstorage.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <reverse_iterator.h>
#include <scheduler.h>
#include <streams.h>
#include <tinyformat.h>
#include <index/txindex.h>
#include <txmempool.h>
#include <util/check.h> // For NDEBUG compile time check
#include <util/system.h>
#include <util/strencodings.h>

#include <list>
#include <memory>
#include <optional>
#include <typeinfo>

#include <spork.h>
#include <governance/governance.h>
#include <masternode/sync.h>
#include <masternode/meta.h>
#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET
#include <coinjoin/context.h>
#include <coinjoin/server.h>

#include <evo/deterministicmns.h>
#include <evo/mnauth.h>
#include <evo/simplifiedmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/snapshot.h>

#include <statsd_client.h>

/** Maximum number of in-flight objects from a peer */
static constexpr int32_t MAX_PEER_OBJECT_IN_FLIGHT = 100;
/** Maximum number of announced objects from a peer */
static constexpr int32_t MAX_PEER_OBJECT_ANNOUNCEMENTS = 2 * MAX_INV_SZ;
/** How many microseconds to delay requesting transactions from inbound peers */
static constexpr std::chrono::microseconds INBOUND_PEER_TX_DELAY{std::chrono::seconds{2}};
/** How long to wait (in microseconds) before downloading a transaction from an additional peer */
static constexpr std::chrono::microseconds GETDATA_TX_INTERVAL{std::chrono::seconds{60}};
/** Maximum delay (in microseconds) for transaction requests to avoid biasing some peers over others. */
static constexpr std::chrono::microseconds MAX_GETDATA_RANDOM_DELAY{std::chrono::seconds{2}};
/** How long to wait (expiry * factor microseconds) before expiring an in-flight getdata request to a peer */
static constexpr int64_t TX_EXPIRY_INTERVAL_FACTOR = 10;
static_assert(INBOUND_PEER_TX_DELAY >= MAX_GETDATA_RANDOM_DELAY,
"To preserve security, MAX_GETDATA_RANDOM_DELAY should not exceed INBOUND_PEER_DELAY");
/** Limit to avoid sending big packets. Not used in processing incoming GETDATA for compatibility */
static const unsigned int MAX_GETDATA_SZ = 1000;

/** Expiration time for orphan transactions in seconds */
static constexpr int64_t ORPHAN_TX_EXPIRE_TIME = 20 * 60;
/** Minimum time between orphan transactions expire time checks in seconds */
static constexpr int64_t ORPHAN_TX_EXPIRE_INTERVAL = 5 * 60;
/** How long to cache transactions in mapRelay for normal relay */
static constexpr std::chrono::seconds RELAY_TX_CACHE_TIME{15 * 60};
/** Headers download timeout expressed in microseconds
 *  Timeout = base + per_header * (expected number of headers) */
static constexpr int64_t HEADERS_DOWNLOAD_TIMEOUT_BASE = 15 * 60 * 1000000; // 15 minutes
static constexpr int64_t HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER = 1000; // 1ms/header
/** Protect at least this many outbound peers from disconnection due to slow/
 * behind headers chain.
 */
static constexpr int32_t MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT = 4;
/** Timeout for (unprotected) outbound peers to sync to our chainwork, in seconds */
static constexpr int64_t CHAIN_SYNC_TIMEOUT = 20 * 60; // 20 minutes
/** How frequently to check for stale tips, in seconds */
static constexpr int64_t STALE_CHECK_INTERVAL = 2.5 * 60; // 2.5 minutes (~block interval)
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
/** Time between pings automatically sent out for latency probing and keepalive (in seconds). */
static const int PING_INTERVAL = 2 * 60;
/** The maximum number of entries in a locator */
static const unsigned int MAX_LOCATOR_SZ = 101;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
static const unsigned int BLOCK_STALLING_TIMEOUT = 2;
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
/** Block download timeout base, expressed in millionths of the block interval (i.e. 10 min) */
static const int64_t BLOCK_DOWNLOAD_TIMEOUT_BASE = 1000000;
/** Additional block download timeout per parallel downloading peer (i.e. 5 min) */
static const int64_t BLOCK_DOWNLOAD_TIMEOUT_PER_PEER = 500000;
/** Maximum number of headers to announce when relaying blocks with headers message.*/
static const unsigned int MAX_BLOCKS_TO_ANNOUNCE = 8;
/** Maximum number of unconnecting headers announcements before DoS score */
static const int MAX_UNCONNECTING_HEADERS = 10;
/** Minimum blocks required to signal NODE_NETWORK_LIMITED */
static const unsigned int NODE_NETWORK_LIMITED_MIN_BLOCKS = 288;

/** Average delay between local address broadcasts in seconds. */
static constexpr std::chrono::hours AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL{24};
/** Average delay between peer address broadcasts in seconds. */
static constexpr std::chrono::seconds AVG_ADDRESS_BROADCAST_INTERVAL{30};
/** Average delay between trickled inventory transmissions in seconds.
 *  Blocks and peers with noban permission bypass this, regular outbound peers get half this delay,
 *  Masternode outbound peers get quarter this delay. */
static const unsigned int INVENTORY_BROADCAST_INTERVAL = 5;
/** Maximum number of inventory items to send per transmission.
 *  Limits the impact of low-fee transaction floods.
 *  We have 4 times smaller block times in Dash, so we need to push 4 times more invs per 1MB. */
static constexpr unsigned int INVENTORY_BROADCAST_MAX_PER_1MB_BLOCK = 4 * 7 * INVENTORY_BROADCAST_INTERVAL;
/** Maximum number of compact filters that may be requested with one getcfilters. See BIP 157. */
static constexpr uint32_t MAX_GETCFILTERS_SIZE = 1000;
/** Maximum number of cf hashes that may be requested with one getcfheaders. See BIP 157. */
static constexpr uint32_t MAX_GETCFHEADERS_SIZE = 2000;
/** the maximum percentage of addresses from our addrman to return in response to a getaddr message. */
static constexpr size_t MAX_PCT_ADDR_TO_SEND = 23;

struct COrphanTx {
    // When modifying, adapt the copy of this definition in tests/DoS_tests.
    CTransactionRef tx;
    NodeId fromPeer;
    int64_t nTimeExpire;
    size_t list_pos;
    size_t nTxSize;
};
RecursiveMutex g_cs_orphans;
std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(g_cs_orphans);

size_t nMapOrphanTransactionsSize = 0;
void EraseOrphansFor(NodeId peer);

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
                    BanMan* banman, CScheduler &scheduler, ChainstateManager& chainman,
                    CTxMemPool& pool, CGovernanceManager& govman, const std::unique_ptr<CJContext>& cj_ctx,
                    const std::unique_ptr<LLMQContext>& llmq_ctx, bool ignore_incoming_txs);

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
    void CheckForStaleTipAndEvictPeers(const Consensus::Params &consensusParams) override;
    bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) override;
    bool IgnoresIncomingTxs() override { return m_ignore_incoming_txs; }
    void RelayTransaction(const uint256& txid) override;
    void SetBestHeight(int height) override { m_best_height = height; };
    void Misbehaving(const NodeId pnode, const int howmuch, const std::string& message = "") override;
    void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                        int64_t nTimeReceived, const std::atomic<bool>& interruptMsgProc) override;
    bool IsBanned(NodeId pnode) override EXCLUSIVE_LOCKS_REQUIRED(cs_main);

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
     * Potentially ban a node based on the contents of a TxValidationState object
     *
     * @return Returns true if the peer was punished (probably disconnected)
     *
     * Changes here may need to be reflected in TxRelayMayResultInDisconnect().
     */
    bool MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message = "");

    /** Maybe disconnect a peer and discourage future connections from its address.
     *
     * @param[in]   pnode     The node to check.
     * @return                True if the peer was marked for disconnection in this function
     */
    bool MaybeDiscourageAndDisconnect(CNode& pnode);

    void ProcessOrphanTx(std::set<uint256>& orphan_work_set)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main, g_cs_orphans);
    /** Process a single headers message from a peer. */
    void ProcessHeadersMessage(CNode& pfrom, const std::vector<CBlockHeader>& headers, bool via_compact_block);

    void SendBlockTransactions(CNode& pfrom, const CBlock& block, const BlockTransactionsRequest& req);

    /** Send a version message to a peer */
    void PushNodeVersion(CNode& pnode, int64_t nTime);

    const CChainParams& m_chainparams;
    CConnman& m_connman;
    CAddrMan& m_addrman;
    /** Pointer to this node's banman. May be nullptr - check existence before dereferencing. */
    BanMan* const m_banman;
    ChainstateManager& m_chainman;
    CTxMemPool& m_mempool;
    const std::unique_ptr<CJContext>& m_cj_ctx;
    const std::unique_ptr<LLMQContext>& m_llmq_ctx;
    CGovernanceManager& m_govman;

    /** The height of the best chain */
    std::atomic<int> m_best_height{-1};

    /** Next time to check for stale tip */
    int64_t m_stale_tip_check_time{0};

    /** Whether this node is running in blocks only mode */
    const bool m_ignore_incoming_txs;

    /** Protects m_peer_map */
    mutable Mutex m_peer_mutex;
    /**
     * Map of all Peer objects, keyed by peer id. This map is protected
     * by the m_peer_mutex. Once a shared pointer reference is
     * taken, the lock may be released. Individual fields are protected by
     * their own locks.
     */
    std::map<NodeId, PeerRef> m_peer_map GUARDED_BY(m_peer_mutex);

    /** Check whether the last unknown block a peer advertised is not yet known. */
    void ProcessBlockAvailability(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Update tracking information about which blocks a peer is assumed to have. */
    void UpdateBlockAvailability(NodeId nodeid, const uint256 &hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool CanDirectFetch(const Consensus::Params &consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * To prevent fingerprinting attacks, only send blocks/headers outside of the
     * active chain if they are no more than a month older (both in time, and in
     * best equivalent proof of work) than the best header chain we know about and
     * we fully-validated them at some point.
     */
    bool BlockRequestAllowed(const CBlockIndex* pindex, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ProcessGetBlockData(CNode& pfrom, const CChainParams& chainparams, const CInv& inv, CConnman& connman, llmq::CInstantSendManager& isman);

    /**
     * Validation logic for compact filters request handling.
     *
     * May disconnect from the peer in the case of a bad request.
     *
     * @param[in]   peer            The peer that we received the request from
     * @param[in]   chain_params    Chain parameters
     * @param[in]   filter_type     The filter type the request is for. Must be basic filters.
     * @param[in]   start_height    The start height for the request
     * @param[in]   stop_hash       The stop_hash for the request
     * @param[in]   max_height_diff The maximum number of items permitted to request, as specified in BIP 157
     * @param[out]  stop_index      The CBlockIndex for the stop_hash block, if the request can be serviced.
     * @param[out]  filter_index    The filter index, if the request can be serviced.
     * @return                      True if the request can be serviced.
     */
    bool PrepareBlockFilterRequest(CNode& peer, const CChainParams& chain_params,
                                   BlockFilterType filter_type, uint32_t start_height,
                                   const uint256& stop_hash, uint32_t max_height_diff,
                                   const CBlockIndex*& stop_index,
                                   BlockFilterIndex*& filter_index);

    /**
     * Handle a cfilters request.
     *
     * May disconnect from the peer in the case of a bad request.
     *
     * @param[in]   peer            The peer that we received the request from
     * @param[in]   vRecv           The raw message received
     * @param[in]   chain_params    Chain parameters
     * @param[in]   connman         Pointer to the connection manager
     */
    void ProcessGetCFilters(CNode& peer, CDataStream& vRecv, const CChainParams& chain_params,
                                   CConnman& connman);

    /**
     * Handle a cfheaders request.
     *
     * May disconnect from the peer in the case of a bad request.
     *
     * @param[in]   peer            The peer that we received the request from
     * @param[in]   vRecv           The raw message received
     * @param[in]   chain_params    Chain parameters
     * @param[in]   connman         Pointer to the connection manager
     */
    void ProcessGetCFHeaders(CNode& peer, CDataStream& vRecv, const CChainParams& chain_params,
                                    CConnman& connman);

    /**
     * Handle a getcfcheckpt request.
     *
     * May disconnect from the peer in the case of a bad request.
     *
     * @param[in]   peer            The peer that we received the request from
     * @param[in]   vRecv           The raw message received
     * @param[in]   chain_params    Chain parameters
     * @param[in]   connman         Pointer to the connection manager
     */
    void ProcessGetCFCheckPt(CNode& peer, CDataStream& vRecv, const CChainParams& chain_params,
                                    CConnman& connman);

    /** Number of nodes with fSyncStarted. */
    int nSyncStarted GUARDED_BY(cs_main) = 0;

    /**
     * Sources of received blocks, saved to be able punish them when processing
     * happens afterwards.
     * Set mapBlockSource[hash].second to false if the node should not be
     * punished if the block is invalid.
     */
    std::map<uint256, std::pair<NodeId, bool>> mapBlockSource GUARDED_BY(cs_main);

    /** Number of outbound peers with m_chain_sync.m_protect. */
    int m_outbound_peers_with_protect_from_disconnect GUARDED_BY(cs_main) = 0;

    bool AlreadyHave(const CInv& inv) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

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
     * Memory used: 1.3MB
     */
    CRollingBloomFilter m_recent_rejects GUARDED_BY(::cs_main){120'000, 0.000'001};
    uint256 hashRecentRejectsChainTip GUARDED_BY(cs_main);

    /*
     * Filter for transactions that have been recently confirmed.
     * We use this to avoid requesting transactions that have already been
     * confirnmed.
     *
     * Blocks don't typically have more than 4000 transactions, so this should
     * be at least six blocks (~1 hr) worth of transactions that we can store,
     * inserting both a txid and wtxid for every observed transaction.
     * If the number of transactions appearing in a block goes up, or if we are
     * seeing getdata requests more than an hour after initial announcement, we
     * can increase this number.
     * The false positive rate of 1/1M should come out to less than 1
     * transaction per day that would be inadvertently ignored (which is the
     * same probability that we have in the reject filter).
     */
    Mutex m_recent_confirmed_transactions_mutex;
    CRollingBloomFilter m_recent_confirmed_transactions GUARDED_BY(m_recent_confirmed_transactions_mutex){48'000, 0.000'001};

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
    CTransactionRef FindTxForGetData(CNode* peer, const uint256& txid, const std::chrono::seconds mempool_req, const std::chrono::seconds longlived_mempool_time) LOCKS_EXCLUDED(cs_main);

    void ProcessGetData(CNode& pfrom, Peer& peer, const std::atomic<bool>& interruptMsgProc) LOCKS_EXCLUDED(cs_main) EXCLUSIVE_LOCKS_REQUIRED(peer.m_getdata_requests_mutex);

    void ProcessBlock(CNode& pfrom, const std::shared_ptr<const CBlock>& pblock, bool fForceProcessing);

    /** Relay map */
    typedef std::map<uint256, CTransactionRef> MapRelay;
    MapRelay mapRelay GUARDED_BY(cs_main);
    /** Expiration-time ordered list of (expire time, relay map entry) pairs. */
    std::deque<std::pair<int64_t, MapRelay::iterator>> vRelayExpiration GUARDED_BY(cs_main);

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
};
} // namespace

namespace {

    /** Number of preferable block download peers. */
    int nPreferredDownload GUARDED_BY(cs_main) = 0;

    struct IteratorComparator
    {
        template<typename I>
        bool operator()(const I& a, const I& b) const
        {
            return &(*a) < &(*b);
        }
    };
    std::map<COutPoint, std::set<std::map<uint256, COrphanTx>::iterator, IteratorComparator>> mapOrphanTransactionsByPrev GUARDED_BY(g_cs_orphans);

    std::vector<std::map<uint256, COrphanTx>::iterator> g_orphan_list GUARDED_BY(g_cs_orphans); //! For random eviction

    static size_t vExtraTxnForCompactIt GUARDED_BY(g_cs_orphans) = 0;
    static std::vector<std::pair<uint256, CTransactionRef>> vExtraTxnForCompact GUARDED_BY(g_cs_orphans);
} // namespace

namespace {
/**
 * Maintain validation-specific state about nodes, protected by cs_main, instead
 * by CNode's own locks. This simplifies asynchronous operation, where
 * processing of incoming data is done after the ProcessMessage call returns,
 * and we're no longer holding the node's locks.
 */
struct CNodeState {
    //! The peer's address
    const CService address;
    //! The best known block we know this peer has announced.
    const CBlockIndex *pindexBestKnownBlock;
    //! The hash of the last unknown block this peer has announced.
    uint256 hashLastUnknownBlock;
    //! The last full block we both have.
    const CBlockIndex *pindexLastCommonBlock;
    //! The best header we have sent our peer.
    const CBlockIndex *pindexBestHeaderSent;
    //! Length of current-streak of unconnecting headers announcements
    int nUnconnectingHeaders;
    //! Whether we've started headers synchronization with this peer.
    bool fSyncStarted;
    //! When to potentially disconnect peer for stalling headers download
    int64_t nHeadersSyncTimeout;
    //! Since when we're stalling block download progress (in microseconds), or 0.
    int64_t nStallingSince;
    std::list<QueuedBlock> vBlocksInFlight;
    //! When the first entry in vBlocksInFlight started downloading. Don't care when vBlocksInFlight is empty.
    int64_t nDownloadingSince;
    int nBlocksInFlight;
    int nBlocksInFlightValidHeaders;
    //! Whether we consider this a preferred download peer.
    bool fPreferredDownload;
    //! Whether this peer wants invs or headers (when possible) for block announcements.
    bool fPreferHeaders;
    //! Whether this peer wants invs or compressed headers (when possible) for block announcements.
    bool fPreferHeadersCompressed;
    //! Whether this peer wants invs or cmpctblocks (when possible) for block announcements.
    bool fPreferHeaderAndIDs;
    //! Whether this peer will send us cmpctblocks if we request them
    bool fProvidesHeaderAndIDs;
    /**
     * If we've announced last version to this peer: whether the peer sends last version in cmpctblocks/blocktxns,
     * otherwise: whether this peer sends non-last version in cmpctblocks/blocktxns.
     */
    bool fSupportsDesiredCmpctVersion;

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
    int64_t m_last_block_announcement;

    /*
     * State associated with objects download.
     *
     * Tx download algorithm:
     *
     *   When inv comes in, queue up (process_time, inv) inside the peer's
     *   CNodeState (m_object_process_time) as long as m_object_announced for the peer
     *   isn't too big (MAX_PEER_OBJECT_ANNOUNCEMENTS).
     *
     *   The process_time for a objects is set to nNow for outbound peers,
     *   nNow + 2 seconds for inbound peers. This is the time at which we'll
     *   consider trying to request the objects from the peer in
     *   SendMessages(). The delay for inbound peers is to allow outbound peers
     *   a chance to announce before we request from inbound peers, to prevent
     *   an adversary from using inbound connections to blind us to a
     *   objects (InvBlock).
     *
     *   When we call SendMessages() for a given peer,
     *   we will loop over the objects in m_object_process_time, looking
     *   at the objects whose process_time <= nNow. We'll request each
     *   such objects that we don't have already and that hasn't been
     *   requested from another peer recently, up until we hit the
     *   MAX_PEER_OBJECT_IN_FLIGHT limit for the peer. Then we'll update
     *   g_already_asked_for for each requested inv, storing the time of the
     *   GETDATA request. We use g_already_asked_for to coordinate objects
     *   requests amongst our peers.
     *
     *   For objects that we still need but we have already recently
     *   requested from some other peer, we'll reinsert (process_time, inv)
     *   back into the peer's m_object_process_time at the point in the future at
     *   which the most recent GETDATA request would time out (ie
     *   GetObjectInterval + the request time stored in g_already_asked_for).
     *   We add an additional delay for inbound peers, again to prefer
     *   attempting download from outbound peers first.
     *   We also add an extra small random delay up to 2 seconds
     *   to avoid biasing some peers over others. (e.g., due to fixed ordering
     *   of peer processing in ThreadMessageHandler).
     *
     *   When we receive a objects from a peer, we remove the inv from the
     *   peer's m_object_in_flight set and from their recently announced set
     *   (m_object_announced).  We also clear g_already_asked_for for that entry, so
     *   that if somehow the objects is not accepted but also not added to
     *   the reject filter, then we will eventually redownload from other
     *   peers.
     */
    struct ObjectDownloadState {
        /* Track when to attempt download of announced objects (process
         * time in micros -> inv)
         */
        std::multimap<std::chrono::microseconds, CInv> m_object_process_time;

        //! Store all the objects a peer has recently announced
        std::set<CInv> m_object_announced;

        //! Store objects which were requested by us, with timestamp
        std::map<CInv, std::chrono::microseconds> m_object_in_flight;

        //! Periodically check for stuck getdata requests
        std::chrono::microseconds m_check_expiry_timer{0};
    };

    ObjectDownloadState m_object_download;

    //! Whether this peer is an inbound connection
    bool m_is_inbound;

    CNodeState(CAddress addrIn, bool is_inbound) :
        address(addrIn), m_is_inbound(is_inbound)
    {
        pindexBestKnownBlock = nullptr;
        hashLastUnknownBlock.SetNull();
        pindexLastCommonBlock = nullptr;
        pindexBestHeaderSent = nullptr;
        nUnconnectingHeaders = 0;
        fSyncStarted = false;
        nHeadersSyncTimeout = 0;
        nStallingSince = 0;
        nDownloadingSince = 0;
        nBlocksInFlight = 0;
        nBlocksInFlightValidHeaders = 0;
        fPreferredDownload = false;
        fPreferHeaders = false;
        fPreferHeadersCompressed = false;
        fPreferHeaderAndIDs = false;
        fProvidesHeaderAndIDs = false;
        fSupportsDesiredCmpctVersion = false;
        m_chain_sync = { 0, nullptr, false, false };
        m_last_block_announcement = 0;
    }
};

// Keeps track of the time (in microseconds) when transactions were requested last time
unordered_limitedmap<uint256, std::chrono::microseconds, StaticSaltedHasher> g_already_asked_for(MAX_INV_SZ, MAX_INV_SZ * 2);
unordered_limitedmap<uint256, std::chrono::microseconds, StaticSaltedHasher> g_erased_object_requests(MAX_INV_SZ, MAX_INV_SZ * 2);

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
            state->nDownloadingSince = std::max(state->nDownloadingSince, GetTimeMicros());
        }
        state->vBlocksInFlight.erase(itInFlight->second.second);
        state->nBlocksInFlight--;
        state->nStallingSince = 0;
        mapBlocksInFlight.erase(itInFlight);
        return true;
    }
    return false;
}

bool PeerManagerImpl::MarkBlockAsInFlight(NodeId nodeid, const uint256& hash, const CBlockIndex *pindex, std::list<QueuedBlock>::iterator **pit)
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
        state->nDownloadingSince = GetTimeMicros();
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
        // Never ask from peers who can't provide desired version.
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
        m_connman.ForNode(nodeid, [this](CNode* pfrom){
            AssertLockHeld(cs_main);
            uint64_t nCMPCTBLOCKVersion = 1;
            if (lNodesAnnouncingHeaderAndIDs.size() >= 3) {
                // As per BIP152, we only get 3 of our peers to announce
                // blocks using compact encodings.
                m_connman.ForNode(lNodesAnnouncingHeaderAndIDs.front(), [this, nCMPCTBLOCKVersion](CNode* pnodeStop){
                    AssertLockHeld(cs_main);
                    m_connman.PushMessage(pnodeStop, CNetMsgMaker(pnodeStop->GetSendVersion()).Make(NetMsgType::SENDCMPCT, /*fAnnounceUsingCMPCTBLOCK=*/false, nCMPCTBLOCKVersion));
                    return true;
                });
                lNodesAnnouncingHeaderAndIDs.pop_front();
            }
            m_connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::SENDCMPCT, /*fAnnounceUsingCMPCTBLOCK=*/true, nCMPCTBLOCKVersion));
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

bool PeerManagerImpl::CanDirectFetch(const Consensus::Params &consensusParams)
{
    return m_chainman.ActiveChain().Tip()->GetBlockTime() > GetAdjustedTime() - consensusParams.nPowTargetSpacing * 20;
}

static bool PeerHasHeader(CNodeState *state, const CBlockIndex *pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (state->pindexBestKnownBlock && pindex == state->pindexBestKnownBlock->GetAncestor(pindex->nHeight))
        return true;
    if (state->pindexBestHeaderSent && pindex == state->pindexBestHeaderSent->GetAncestor(pindex->nHeight))
        return true;
    return false;
}

void PeerManagerImpl::ProcessBlockAvailability(NodeId nodeid)
{
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

void PeerManagerImpl::UpdateBlockAvailability(NodeId nodeid, const uint256 &hash)
{
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
    const auto& params = Params();

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

    uint256 mnauthChallenge;
    GetRandBytes(mnauthChallenge.begin(), mnauthChallenge.size());
    pnode.SetSentMNAuthChallenge(mnauthChallenge);

    int nProtocolVersion = PROTOCOL_VERSION;
    if (params.NetworkIDString() != CBaseChainParams::MAIN && gArgs.IsArgSet("-pushversion")) {
        nProtocolVersion = gArgs.GetArg("-pushversion", PROTOCOL_VERSION);
    }

    m_connman.PushMessage(&pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::VERSION, nProtocolVersion, (uint64_t)nLocalNodeServices, nTime, addrYou, addrMe,
            nonce, strSubVersion, nNodeStartingHeight, !m_ignore_incoming_txs && pnode.IsAddrRelayPeer(), mnauthChallenge, pnode.m_masternode_connection.load()));

    if (fLogIPs) {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, them=%s, peer=%d\n", nProtocolVersion, nNodeStartingHeight, addrMe.ToString(), addrYou.ToString(), nodeid);
    } else {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, peer=%d\n", nProtocolVersion, nNodeStartingHeight, addrMe.ToString(), nodeid);
    }
}

void EraseObjectRequest(CNodeState* nodestate, const CInv& inv) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "%s -- inv=(%s)\n", __func__, inv.ToString());
    g_already_asked_for.erase(inv.hash);
    g_erased_object_requests.insert(std::make_pair(inv.hash, GetTime<std::chrono::microseconds>()));

    if (nodestate) {
        nodestate->m_object_download.m_object_announced.erase(inv);
        nodestate->m_object_download.m_object_in_flight.erase(inv);
    }
}

void EraseObjectRequest(NodeId nodeId, const CInv& inv) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto* state = State(nodeId);
    if (!state) {
        return;
    }
    EraseObjectRequest(state, inv);
}

std::chrono::microseconds GetObjectRequestTime(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto it = g_already_asked_for.find(hash);
    if (it != g_already_asked_for.end()) {
        return it->second;
    }
    return {};
}

void UpdateObjectRequestTime(const uint256& hash, std::chrono::microseconds request_time) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto it = g_already_asked_for.find(hash);
    if (it == g_already_asked_for.end()) {
        g_already_asked_for.insert(std::make_pair(hash, request_time));
    } else {
        g_already_asked_for.update(it, request_time);
    }
}

std::chrono::microseconds GetObjectInterval(int invType)
{
    // some messages need to be re-requested faster when the first announcing peer did not answer to GETDATA
    switch(invType)
    {
        case MSG_QUORUM_RECOVERED_SIG:
            return std::chrono::seconds{15};
        case MSG_CLSIG:
            return std::chrono::seconds{5};
        case MSG_ISDLOCK:
            return std::chrono::seconds{10};
        default:
            return GETDATA_TX_INTERVAL;
    }
}

std::chrono::microseconds GetObjectExpiryInterval(int invType)
{
    return GetObjectInterval(invType) * TX_EXPIRY_INTERVAL_FACTOR;
}

std::chrono::microseconds GetObjectRandomDelay(int invType)
{
    if (invType == MSG_TX) {
        return GetRandMicros(MAX_GETDATA_RANDOM_DELAY);
    }
    return {};
}

std::chrono::microseconds CalculateObjectGetDataTime(const CInv& inv, std::chrono::microseconds current_time, bool use_inbound_delay) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    std::chrono::microseconds process_time;
    const auto last_request_time = GetObjectRequestTime(inv.hash);
    // First time requesting this tx
    if (last_request_time.count() == 0) {
        process_time = current_time;
    } else {
        // Randomize the delay to avoid biasing some peers over others (such as due to
        // fixed ordering of peer processing in ThreadMessageHandler)
        process_time = last_request_time + GetObjectInterval(inv.type) + GetObjectRandomDelay(inv.type);
    }

    // We delay processing announcements from inbound peers
    if (inv.type == MSG_TX && !fMasternodeMode && use_inbound_delay) process_time += INBOUND_PEER_TX_DELAY;

    return process_time;
}

void RequestObject(CNodeState* state, const CInv& inv, std::chrono::microseconds current_time, bool fForce = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    CNodeState::ObjectDownloadState& peer_download_state = state->m_object_download;
    if (peer_download_state.m_object_announced.size() >= MAX_PEER_OBJECT_ANNOUNCEMENTS ||
            peer_download_state.m_object_process_time.size() >= MAX_PEER_OBJECT_ANNOUNCEMENTS ||
            peer_download_state.m_object_announced.count(inv)) {
        // Too many queued announcements from this peer, or we already have
        // this announcement
        return;
    }
    peer_download_state.m_object_announced.insert(inv);

    // Calculate the time to try requesting this transaction. Use
    // fPreferredDownload as a proxy for outbound peers.
    std::chrono::microseconds process_time = CalculateObjectGetDataTime(inv, current_time, !state->fPreferredDownload);

    peer_download_state.m_object_process_time.emplace(process_time, inv);

    if (fForce) {
        // make sure this object is actually requested ASAP
        g_erased_object_requests.erase(inv.hash);
        g_already_asked_for.erase(inv.hash);
    }

    LogPrint(BCLog::NET, "%s -- inv=(%s), current_time=%d, process_time=%d, delta=%d\n", __func__, inv.ToString(), current_time.count(), process_time.count(), (process_time - current_time).count());
}

void RequestObject(NodeId nodeId, const CInv& inv, std::chrono::microseconds current_time, bool fForce) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto* state = State(nodeId);
    if (!state) {
        return;
    }
    RequestObject(state, inv, current_time, fForce);
}

size_t GetRequestedObjectCount(NodeId nodeId)
{
    AssertLockHeld(cs_main);
    auto* state = State(nodeId);
    if (!state) {
        return 0;
    }
    return state->m_object_download.m_object_process_time.size();
}

// This function is used for testing the stale tip eviction logic, see
// denialofservice_tests.cpp
void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds)
{
    LOCK(cs_main);
    CNodeState *state = State(node);
    if (state) state->m_last_block_announcement = time_in_seconds;
}

void PeerManagerImpl::InitializeNode(CNode *pnode) {
    CAddress addr = pnode->addr;
    NodeId nodeid = pnode->GetId();
    {
        LOCK(cs_main);
        mapNodeState.emplace_hint(mapNodeState.end(), std::piecewise_construct, std::forward_as_tuple(nodeid), std::forward_as_tuple(addr, pnode->IsInboundConn()));
    }
    {
        PeerRef peer = std::make_shared<Peer>(nodeid);
        LOCK(m_peer_mutex);
        m_peer_map.emplace_hint(m_peer_map.end(), nodeid, std::move(peer));
    }
    if (!pnode->IsInboundConn())
        PushNodeVersion(*pnode, GetTime());
}

void PeerManagerImpl::ReattemptInitialBroadcast(CScheduler& scheduler)
{
    std::set<uint256> unbroadcast_txids = m_mempool.GetUnbroadcastTxs();

    for (const uint256& txid : unbroadcast_txids) {
        // Sanity check: all unbroadcast txns should exist in the mempool
        if (m_mempool.exists(txid)) {
            RelayTransaction(txid);
        } else {
            m_mempool.RemoveUnbroadcastTx(txid, true);
        }
    }

    // Schedule next run for 10-15 minutes in the future.
    // We add randomness on every cycle to avoid the possibility of P2P fingerprinting.
    const std::chrono::milliseconds delta = std::chrono::minutes{10} + GetRandMillis(std::chrono::minutes{5});
    scheduler.scheduleFromNow([&] { ReattemptInitialBroadcast(scheduler); }, delta);
}

void PeerManagerImpl::FinalizeNode(const CNode& node) {
    NodeId nodeid = node.GetId();
    int misbehavior{0};
    LOCK(cs_main);
    {
    {
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
    EraseOrphansFor(nodeid);
    nPreferredDownload -= state->fPreferredDownload;
    nPeersWithValidatedDownloads -= (state->nBlocksInFlightValidHeaders != 0);
    assert(nPeersWithValidatedDownloads >= 0);
    m_outbound_peers_with_protect_from_disconnect -= state->m_chain_sync.m_protect;
    assert(m_outbound_peers_with_protect_from_disconnect >= 0);

    mapNodeState.erase(nodeid);

    if (mapNodeState.empty()) {
        // Do a consistency check after the last peer is removed.
        assert(mapBlocksInFlight.empty());
        assert(nPreferredDownload == 0);
        assert(nPeersWithValidatedDownloads == 0);
        assert(m_outbound_peers_with_protect_from_disconnect == 0);
    }
    } // cs_main

    if (node.fSuccessfullyConnected && misbehavior == 0 && node.IsAddrRelayPeer() && !node.IsInboundConn()) {
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
    stats.m_misbehavior_score = WITH_LOCK(peer->m_misbehavior_mutex, return peer->m_misbehavior_score);

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//

static void AddToCompactExtraTransactions(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans)
{
    size_t max_extra_txn = gArgs.GetArg("-blockreconstructionextratxn", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN);
    if (max_extra_txn <= 0)
        return;
    if (!vExtraTxnForCompact.size())
        vExtraTxnForCompact.resize(max_extra_txn);
    vExtraTxnForCompact[vExtraTxnForCompactIt] = std::make_pair(tx->GetHash(), tx);
    vExtraTxnForCompactIt = (vExtraTxnForCompactIt + 1) % max_extra_txn;
}

bool AddOrphanTx(const CTransactionRef& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans)
{
    const uint256& hash = tx->GetHash();
    if (mapOrphanTransactions.count(hash))
        return false;

    // Ignore big transactions, to avoid a
    // send-big-orphans memory exhaustion attack. If a peer has a legitimate
    // large transaction with a missing parent then we assume
    // it will rebroadcast it later, after the parent transaction(s)
    // have been mined or received.
    // 100 orphans, each of which is at most 99,999 bytes big is
    // at most 10 megabytes of orphans and somewhat more byprev index (in the worst case):
    unsigned int sz = GetSerializeSize(*tx, CTransaction::CURRENT_VERSION);
    if (sz > MAX_STANDARD_TX_SIZE)
    {
        LogPrint(BCLog::MEMPOOL, "ignoring large orphan tx (size: %u, hash: %s)\n", sz, hash.ToString());
        return false;
    }

    auto ret = mapOrphanTransactions.emplace(hash, COrphanTx{tx, peer, GetTime() + ORPHAN_TX_EXPIRE_TIME, g_orphan_list.size(), sz});
    assert(ret.second);
    g_orphan_list.push_back(ret.first);
    for (const CTxIn& txin : tx->vin) {
        mapOrphanTransactionsByPrev[txin.prevout].insert(ret.first);
    }

    AddToCompactExtraTransactions(tx);

    nMapOrphanTransactionsSize += sz;

    LogPrint(BCLog::MEMPOOL, "stored orphan tx %s (mapsz %u outsz %u)\n", hash.ToString(),
             mapOrphanTransactions.size(), mapOrphanTransactionsByPrev.size());
    statsClient.inc("transactions.orphans.add", 1.0f);
    statsClient.gauge("transactions.orphans", mapOrphanTransactions.size());
    return true;
}

int static EraseOrphanTx(uint256 hash) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans)
{
    std::map<uint256, COrphanTx>::iterator it = mapOrphanTransactions.find(hash);
    if (it == mapOrphanTransactions.end())
        return 0;
    for (const CTxIn& txin : it->second.tx->vin)
    {
        auto itPrev = mapOrphanTransactionsByPrev.find(txin.prevout);
        if (itPrev == mapOrphanTransactionsByPrev.end())
            continue;
        itPrev->second.erase(it);
        if (itPrev->second.empty())
            mapOrphanTransactionsByPrev.erase(itPrev);
    }

    size_t old_pos = it->second.list_pos;
    assert(g_orphan_list[old_pos] == it);
    if (old_pos + 1 != g_orphan_list.size()) {
        // Unless we're deleting the last entry in g_orphan_list, move the last
        // entry to the position we're deleting.
        auto it_last = g_orphan_list.back();
        g_orphan_list[old_pos] = it_last;
        it_last->second.list_pos = old_pos;
    }
    g_orphan_list.pop_back();

    assert(nMapOrphanTransactionsSize >= it->second.nTxSize);
    nMapOrphanTransactionsSize -= it->second.nTxSize;
    mapOrphanTransactions.erase(it);
    statsClient.inc("transactions.orphans.remove", 1.0f);
    statsClient.gauge("transactions.orphans", mapOrphanTransactions.size());
    return 1;
}

void EraseOrphansFor(NodeId peer)
{
    LOCK(g_cs_orphans);
    int nErased = 0;
    std::map<uint256, COrphanTx>::iterator iter = mapOrphanTransactions.begin();
    while (iter != mapOrphanTransactions.end())
    {
        std::map<uint256, COrphanTx>::iterator maybeErase = iter++; // increment to avoid iterator becoming invalid
        if (maybeErase->second.fromPeer == peer)
        {
            nErased += EraseOrphanTx(maybeErase->second.tx->GetHash());
        }
    }
    if (nErased > 0) LogPrint(BCLog::MEMPOOL, "Erased %d orphan tx from peer=%d\n", nErased, peer);
}


unsigned int LimitOrphanTxSize(unsigned int nMaxOrphansSize)
{
    LOCK(g_cs_orphans);

    unsigned int nEvicted = 0;
    static int64_t nNextSweep;
    int64_t nNow = GetTime();
    if (nNextSweep <= nNow) {
        // Sweep out expired orphan pool entries:
        int nErased = 0;
        int64_t nMinExpTime = nNow + ORPHAN_TX_EXPIRE_TIME - ORPHAN_TX_EXPIRE_INTERVAL;
        std::map<uint256, COrphanTx>::iterator iter = mapOrphanTransactions.begin();
        while (iter != mapOrphanTransactions.end())
        {
            std::map<uint256, COrphanTx>::iterator maybeErase = iter++;
            if (maybeErase->second.nTimeExpire <= nNow) {
                nErased += EraseOrphanTx(maybeErase->second.tx->GetHash());
            } else {
                nMinExpTime = std::min(maybeErase->second.nTimeExpire, nMinExpTime);
            }
        }
        // Sweep again 5 minutes after the next entry that expires in order to batch the linear scan.
        nNextSweep = nMinExpTime + ORPHAN_TX_EXPIRE_INTERVAL;
        if (nErased > 0) LogPrint(BCLog::MEMPOOL, "Erased %d orphan tx due to expiration\n", nErased);
    }
    FastRandomContext rng;
    while (!mapOrphanTransactions.empty() && nMapOrphanTransactionsSize > nMaxOrphansSize)
    {
        // Evict a random orphan:
        size_t randompos = rng.randrange(g_orphan_list.size());
        EraseOrphanTx(g_orphan_list[randompos]->first);
        ++nEvicted;
    }
    return nEvicted;
}

void PeerManagerImpl::Misbehaving(const NodeId pnode, const int howmuch, const std::string& message)
{
    assert(howmuch > 0);

    PeerRef peer = GetPeerRef(pnode);
    if (peer == nullptr) return;

    LOCK(peer->m_misbehavior_mutex);
    peer->m_misbehavior_score += howmuch;
    const int banscore = gArgs.GetArg("-banscore", DEFAULT_BANSCORE_THRESHOLD);
    const std::string message_prefixed = message.empty() ? "" : (": " + message);
    if (peer->m_misbehavior_score >= banscore && peer->m_misbehavior_score - howmuch < banscore)
    {
        LogPrint(BCLog::NET, "Misbehaving: peer=%d (%d -> %d) DISCOURAGE THRESHOLD EXCEEDED%s\n", pnode, peer->m_misbehavior_score - howmuch, peer->m_misbehavior_score, message_prefixed);
        peer->m_should_discourage = true;
        statsClient.inc("misbehavior.banned", 1.0f);
    } else {
        LogPrint(BCLog::NET, "Misbehaving: peer=%d (%d -> %d)%s\n", pnode, peer->m_misbehavior_score - howmuch, peer->m_misbehavior_score, message_prefixed);
        statsClient.count("misbehavior.amount", howmuch, 1.0);
    }
}

bool PeerManagerImpl::IsBanned(NodeId pnode)
{
    PeerRef peer = GetPeerRef(pnode);
    if (peer == nullptr)
        return false;
    LOCK(peer->m_misbehavior_mutex);
    if (peer->m_should_discourage) {
        return true;
    }
    return false;
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
    case BlockValidationResult::BLOCK_CHAINLOCK:
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

bool PeerManagerImpl::MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message) {
    switch (state.GetResult()) {
    case TxValidationResult::TX_RESULT_UNSET:
        break;
    // The node is providing invalid data:
    case TxValidationResult::TX_CONSENSUS:
        {
            LOCK(cs_main);
            Misbehaving(nodeid, 100, message);
            return true;
        }
    // Conflicting (but not necessarily invalid) data or different policy:
    case TxValidationResult::TX_RECENT_CONSENSUS_CHANGE:
    case TxValidationResult::TX_NOT_STANDARD:
    case TxValidationResult::TX_MISSING_INPUTS:
    case TxValidationResult::TX_PREMATURE_SPEND:
    case TxValidationResult::TX_CONFLICT:
    case TxValidationResult::TX_MEMPOOL_POLICY:
    // moved from BLOCK
    case TxValidationResult::TX_BAD_SPECIAL:
    case TxValidationResult::TX_CONFLICT_LOCK:
        break;
    }
    if (message != "") {
        LogPrint(BCLog::NET, "peer=%d: %s\n", nodeid, message);
    }
    return false;
}

bool PeerManagerImpl::BlockRequestAllowed(const CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    AssertLockHeld(cs_main);
    if (m_chainman.ActiveChain().Contains(pindex)) return true;
    return pindex->IsValid(BLOCK_VALID_SCRIPTS) && (pindexBestHeader != nullptr) &&
        (pindexBestHeader->GetBlockTime() - pindex->GetBlockTime() < STALE_RELAY_AGE_LIMIT) &&
        (GetBlockProofEquivalentTime(*pindexBestHeader, *pindex, *pindexBestHeader, consensusParams) < STALE_RELAY_AGE_LIMIT);
}

std::unique_ptr<PeerManager> PeerManager::make(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman, BanMan* banman,
                                               CScheduler &scheduler, ChainstateManager& chainman, CTxMemPool& pool,
                                               CGovernanceManager& govman, const std::unique_ptr<CJContext>& cj_ctx,
                                               const std::unique_ptr<LLMQContext>& llmq_ctx, bool ignore_incoming_txs)
{
    return std::make_unique<PeerManagerImpl>(chainparams, connman, addrman, banman, scheduler, chainman, pool, govman, cj_ctx, llmq_ctx, ignore_incoming_txs);
}

PeerManagerImpl::PeerManagerImpl(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman, BanMan* banman,
                                 CScheduler &scheduler, ChainstateManager& chainman, CTxMemPool& pool,
                                 CGovernanceManager& govman, const std::unique_ptr<CJContext>& cj_ctx,
                                 const std::unique_ptr<LLMQContext>& llmq_ctx, bool ignore_incoming_txs)
    : m_chainparams(chainparams),
      m_connman(connman),
      m_addrman(addrman),
      m_banman(banman),
      m_chainman(chainman),
      m_mempool(pool),
      m_cj_ctx(cj_ctx),
      m_llmq_ctx(llmq_ctx),
      m_govman(govman),
      m_ignore_incoming_txs(ignore_incoming_txs)
{
    assert(std::addressof(g_chainman) == std::addressof(m_chainman));
    const Consensus::Params& consensusParams = Params().GetConsensus();
    // Stale tip checking and peer eviction are on two different timers, but we
    // don't want them to get out of sync due to drift in the scheduler, so we
    // combine them in one function and schedule at the quicker (peer-eviction)
    // timer.
    static_assert(EXTRA_PEER_CHECK_INTERVAL < STALE_CHECK_INTERVAL, "peer eviction timer should be less than stale tip check timer");
    scheduler.scheduleEvery([this, consensusParams] { this->CheckForStaleTipAndEvictPeers(consensusParams); }, std::chrono::seconds{EXTRA_PEER_CHECK_INTERVAL});

    // schedule next run for 10-15 minutes in the future
    const std::chrono::milliseconds delta = std::chrono::minutes{10} + GetRandMillis(std::chrono::minutes{5});
    scheduler.scheduleFromNow([&] { ReattemptInitialBroadcast(scheduler); }, delta);
}

/**
 * Evict orphan txn pool entries (EraseOrphanTx) based on a newly connected
 * block. Also save the time of the last tip update.
 */
void PeerManagerImpl::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    {
        LOCK2(cs_main, g_cs_orphans);

        std::vector<uint256> vOrphanErase;
        std::set<uint256> orphanWorkSet;

        for (const CTransactionRef& ptx : pblock->vtx) {
            const CTransaction& tx = *ptx;

            // Which orphan pool entries we should reprocess and potentially try to accept into mempool again?
            for (size_t i = 0; i < tx.vin.size(); i++) {
                auto itByPrev = mapOrphanTransactionsByPrev.find(COutPoint(tx.GetHash(), (uint32_t)i));
                if (itByPrev == mapOrphanTransactionsByPrev.end()) continue;
                for (const auto& elem : itByPrev->second) {
                    orphanWorkSet.insert(elem->first);
                }
            }

            // Which orphan pool entries must we evict?
            for (const auto& txin : tx.vin) {
                auto itByPrev = mapOrphanTransactionsByPrev.find(txin.prevout);
                if (itByPrev == mapOrphanTransactionsByPrev.end()) continue;
                for (auto mi = itByPrev->second.begin(); mi != itByPrev->second.end(); ++mi) {
                    const CTransaction& orphanTx = *(*mi)->second.tx;
                    const uint256& orphanHash = orphanTx.GetHash();
                    vOrphanErase.push_back(orphanHash);
                }
            }
        }

        // Erase orphan transactions included or precluded by this block
        if (vOrphanErase.size()) {
            int nErased = 0;
            for (const uint256& orphanHash : vOrphanErase) {
                nErased += EraseOrphanTx(orphanHash);
            }
            LogPrint(BCLog::MEMPOOL, "Erased %d orphan tx included or conflicted by block\n", nErased);
        }

        while (!orphanWorkSet.empty()) {
            LogPrint(BCLog::MEMPOOL, "Trying to process %d orphans\n", orphanWorkSet.size());
            ProcessOrphanTx(orphanWorkSet);
        }

        m_last_tip_update = GetTime();
    }
    {
        LOCK(m_recent_confirmed_transactions_mutex);
        for (const auto& ptx : pblock->vtx) {
            m_recent_confirmed_transactions.insert(ptx->GetHash());
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
    m_recent_confirmed_transactions.reset();
}

// All of the following cache a recent block, and are protected by cs_most_recent_block
static RecursiveMutex cs_most_recent_block;
static std::shared_ptr<const CBlock> most_recent_block GUARDED_BY(cs_most_recent_block);
static std::shared_ptr<const CBlockHeaderAndShortTxIDs> most_recent_compact_block GUARDED_BY(cs_most_recent_block);
static uint256 most_recent_block_hash GUARDED_BY(cs_most_recent_block);

/**
 * Maintain state about the best-seen block and fast-announce a compact block
 * to compatible peers.
 */
void PeerManagerImpl::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock) {
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> pcmpctblock = std::make_shared<const CBlockHeaderAndShortTxIDs> (*pblock);
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);

    LOCK(cs_main);

    static int nHighestFastAnnounce = 0;
    if (pindex->nHeight <= nHighestFastAnnounce)
        return;
    nHighestFastAnnounce = pindex->nHeight;

    uint256 hashBlock(pblock->GetHash());

    {
        LOCK(cs_most_recent_block);
        most_recent_block_hash = hashBlock;
        most_recent_block = pblock;
        most_recent_compact_block = pcmpctblock;
    }

    m_connman.ForEachNode([this, &pcmpctblock, pindex, &msgMaker, &hashBlock](CNode* pnode) {
        AssertLockHeld(cs_main);
        // TODO: Avoid the repeated-serialization here
        if (pnode->fDisconnect)
            return;
        ProcessBlockAvailability(pnode->GetId());
        CNodeState &state = *State(pnode->GetId());
        // If the peer has, or we announced to them the previous block already,
        // but we don't think they have this one, go ahead and announce it
        if (state.fPreferHeaderAndIDs &&
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
void PeerManagerImpl::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload){
    m_best_height = pindexNew->nHeight;

    SetServiceFlagsIBDCache(!fInitialDownload);
    if (!fInitialDownload) {
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
        // Relay inventory, but don't relay old inventory during initial block download.
        m_connman.ForEachNode([this, &vHashes](CNode* pnode) {
            if (!pnode->CanRelay()) {
                return;
            }
            if (m_best_height > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : 0)) {
                for (const uint256& hash : reverse_iterate(vHashes)) {
                    pnode->PushBlockHash(hash);
                }
            }
        });
        m_connman.WakeMessageHandler();
    }
}

/**
 * Handle invalid block rejection and consequent peer discouragement, maintain which
 * peers announce compact blocks.
 */
void PeerManagerImpl::BlockChecked(const CBlock& block, const BlockValidationState& state)
{
    LOCK(cs_main);

    const uint256 hash(block.GetHash());
    std::map<uint256, std::pair<NodeId, bool> >::iterator it = mapBlockSource.find(hash);

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


bool PeerManagerImpl::AlreadyHave(const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:
    case MSG_DSTX:
        {
            if (m_chainman.ActiveChain().Tip()->GetBlockHash() != hashRecentRejectsChainTip)
            {
                // If the chain tip has changed previously rejected transactions
                // might be now valid, e.g. due to a nLockTime'd tx becoming valid,
                // or a double-spend. Reset the rejects filter and give those
                // txs a second chance.
                hashRecentRejectsChainTip = m_chainman.ActiveChain().Tip()->GetBlockHash();
                m_recent_rejects.reset();
            }

            {
                LOCK(g_cs_orphans);
                if (mapOrphanTransactions.count(inv.hash)) return true;
            }

            {
                LOCK(m_recent_confirmed_transactions_mutex);
                if (m_recent_confirmed_transactions.contains(inv.hash)) return true;
            }

            // When we receive an islock for a previously rejected transaction, we have to
            // drop the first-seen tx (which such a locked transaction was conflicting with)
            // and re-request the locked transaction (which did not make it into the mempool
            // previously due to txn-mempool-conflict rule). This means that we must ignore
            // m_recent_rejects filter for such locked txes here.
            // We also ignore m_recent_rejects filter for DSTX-es because a malicious peer  might
            // relay a valid DSTX as a regular TX first which would skip all the specific checks
            // but would cause such tx to be rejected by ATMP due to 0 fee. Ignoring it here
            // should let DSTX to be propagated by honest peer later. Note, that a malicious
            // masternode would not be able to exploit this to spam the network with specially
            // crafted invalid DSTX-es and potentially cause high load cheaply, because
            // corresponding checks in ProcessMessage won't let it to send DSTX-es too often.
            bool fIgnoreRecentRejects = inv.type == MSG_DSTX ||
                                        m_llmq_ctx->isman->IsWaitingForTx(inv.hash) ||
                                        m_llmq_ctx->isman->IsLocked(inv.hash);

            return (!fIgnoreRecentRejects && m_recent_rejects.contains(inv.hash)) ||
                   (inv.type == MSG_DSTX && static_cast<bool>(CCoinJoin::GetDSTX(inv.hash))) ||
                   m_mempool.exists(inv.hash) ||
                   (g_txindex != nullptr && g_txindex->HasTx(inv.hash));
        }

    case MSG_BLOCK:
        return m_chainman.m_blockman.LookupBlockIndex(inv.hash) != nullptr;

    /*
        Dash Related Inventory Messages

        --

        We shouldn't update the sync times for each of the messages when we already have it.
        We're going to be asking many nodes upfront for the full inventory list, so we'll get duplicates of these.
        We want to only update the time on new hits, so that we can time out appropriately if needed.
    */

    case MSG_SPORK:
        {
            return sporkManager->GetSporkByHash(inv.hash).has_value();
        }

    case MSG_GOVERNANCE_OBJECT:
    case MSG_GOVERNANCE_OBJECT_VOTE:
        return !m_govman.ConfirmInventoryRequest(inv);

    case MSG_QUORUM_FINAL_COMMITMENT:
        return m_llmq_ctx->quorum_block_processor->HasMineableCommitment(inv.hash);
    case MSG_QUORUM_CONTRIB:
    case MSG_QUORUM_COMPLAINT:
    case MSG_QUORUM_JUSTIFICATION:
    case MSG_QUORUM_PREMATURE_COMMITMENT:
        return m_llmq_ctx->qdkgsman->AlreadyHave(inv);
    case MSG_QUORUM_RECOVERED_SIG:
        return m_llmq_ctx->sigman->AlreadyHave(inv);
    case MSG_CLSIG:
        return m_llmq_ctx->clhandler->AlreadyHave(inv);
    case MSG_ISDLOCK:
        return m_llmq_ctx->isman->AlreadyHave(inv);
    }

    // Don't know what it is, just say we already got one
    return true;
}

void PeerManagerImpl::RelayTransaction(const uint256& txid)
{
    CInv inv(CCoinJoin::GetDSTX(txid) ? MSG_DSTX : MSG_TX, txid);
    m_connman.ForEachNode([&inv](CNode* pnode)
    {
        pnode->PushInventory(inv);
    });
}

static void RelayAddress(const CAddress& addr, bool fReachable, const CConnman& connman)
{
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

    auto sortfunc = [&best, &hasher, nRelayNodes, addr](CNode* pnode) {
        if (pnode->IsAddrRelayPeer() && pnode->IsAddrCompatible(addr)) {
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

void PeerManagerImpl::ProcessGetBlockData(CNode& pfrom, const CChainParams& chainparams, const CInv& inv, CConnman& connman, llmq::CInstantSendManager& isman)
{
    bool send = false;
    std::shared_ptr<const CBlock> a_recent_block;
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> a_recent_compact_block;
    const Consensus::Params& consensusParams = chainparams.GetConsensus();
    {
        LOCK(cs_most_recent_block);
        a_recent_block = most_recent_block;
        a_recent_compact_block = most_recent_compact_block;
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
        if (!m_chainman.ActiveChainstate().ActivateBestChain(state, a_recent_block)) {
            LogPrint(BCLog::NET, "failed to activate chain (%s)\n", state.ToString());
        }
    }

    LOCK(cs_main);
    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(inv.hash);
    if (pindex) {
        send = BlockRequestAllowed(pindex, consensusParams);
        if (!send) {
            LogPrint(BCLog::NET,"%s: ignoring request from peer=%i for old block that isn't in the main chain\n", __func__, pfrom.GetId());
        }
    }
    const CNetMsgMaker msgMaker(pfrom.GetSendVersion());
    // disconnect node in case we have reached the outbound limit for serving historical blocks
    if (send &&
        connman.OutboundTargetReached(true) &&
        (((pindexBestHeader != nullptr) && (pindexBestHeader->GetBlockTime() - pindex->GetBlockTime() > HISTORICAL_BLOCK_AGE)) || inv.type == MSG_FILTERED_BLOCK) &&
        !pfrom.HasPermission(PF_DOWNLOAD) // nodes with the download permission may exceed target
    ) {
        LogPrint(BCLog::NET, "historical block serving limit reached, disconnect peer=%d\n", pfrom.GetId());

        //disconnect node
        pfrom.fDisconnect = true;
        send = false;
    }
    // Avoid leaking prune-height by never sending blocks below the NODE_NETWORK_LIMITED threshold
    if (send && !pfrom.HasPermission(PF_NOBAN) && (
            (((pfrom.GetLocalServices() & NODE_NETWORK_LIMITED) == NODE_NETWORK_LIMITED) && ((pfrom.GetLocalServices() & NODE_NETWORK) != NODE_NETWORK) && (m_chainman.ActiveChain().Tip()->nHeight - pindex->nHeight > (int)NODE_NETWORK_LIMITED_MIN_BLOCKS + 2 /* add two blocks buffer extension for possible races */) )
       )) {
        LogPrint(BCLog::NET, "Ignore block request below NODE_NETWORK_LIMITED threshold from peer=%d\n", pfrom.GetId());

        //disconnect node and prevent it from stalling (would otherwise wait for the missing block)
        pfrom.fDisconnect = true;
        send = false;
    }
    // Pruned nodes may have deleted the block, so check whether
    // it's available before trying to send.
    if (send && (pindex->nStatus & BLOCK_HAVE_DATA))
    {
        std::shared_ptr<const CBlock> pblock;
        if (a_recent_block && a_recent_block->GetHash() == pindex->GetBlockHash()) {
            pblock = a_recent_block;
        } else {
            // Send block from disk
            std::shared_ptr<CBlock> pblockRead = std::make_shared<CBlock>();
            if (!ReadBlockFromDisk(*pblockRead, pindex, consensusParams))
                assert(!"cannot load block from disk");
            pblock = pblockRead;
        }
        if (pblock) {
            if (inv.type == MSG_BLOCK)
                connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, *pblock));
            else if (inv.type == MSG_FILTERED_BLOCK) {
                bool sendMerkleBlock = false;
                CMerkleBlock merkleBlock;
                if (pfrom.IsAddrRelayPeer()) {
                    LOCK(pfrom.m_tx_relay->cs_filter);
                    if (pfrom.m_tx_relay->pfilter) {
                        sendMerkleBlock = true;
                        merkleBlock = CMerkleBlock(*pblock, *pfrom.m_tx_relay->pfilter);
                    }
                }
                if (sendMerkleBlock) {
                    connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::MERKLEBLOCK, merkleBlock));
                    // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                    // This avoids hurting performance by pointlessly requiring a round-trip
                    // Note that there is currently no way for a node to request any single transactions we didn't send here -
                    // they must either disconnect and retry or request the full block.
                    // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                    // however we MUST always provide at least what the remote peer needs
                    typedef std::pair<unsigned int, uint256> PairType;
                    for (PairType &pair : merkleBlock.vMatchedTxn) {
                        connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::TX, *pblock->vtx[pair.first]));
                    }
                    for (PairType &pair : merkleBlock.vMatchedTxn) {
                        auto islock = isman.GetInstantSendLockByTxid(pair.second);
                        if (islock != nullptr) {
                            connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::ISDLOCK, *islock));
                        }
                    }
                }
                // else
                // no response
            } else if (inv.type == MSG_CMPCT_BLOCK) {
                // If a peer is asking for old blocks, we're almost guaranteed
                // they won't have a useful mempool to match against a compact block,
                // and we don't feel like constructing the object for them, so
                // instead we respond with the full, non-compact block.
                if (CanDirectFetch(consensusParams) &&
                    pindex->nHeight >= m_chainman.ActiveChain().Height() - MAX_CMPCTBLOCK_DEPTH) {
                    if (a_recent_compact_block &&
                        a_recent_compact_block->header.GetHash() == pindex->GetBlockHash()) {
                        connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::CMPCTBLOCK, *a_recent_compact_block));
                    } else {
                        CBlockHeaderAndShortTxIDs cmpctblock(*pblock);
                        connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::CMPCTBLOCK, cmpctblock));
                    }
                } else {
                    connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, *pblock));
                }
            }
        }
        // Trigger the peer node to send a getblocks request for the next batch of inventory
        if (inv.hash == pfrom.hashContinue)
        {
            // Bypass PushInventory, this must send even if redundant,
            // and we want it right after the last block so they don't
            // wait for other stuff first.
            std::vector<CInv> vInv;
            vInv.push_back(CInv(MSG_BLOCK, m_chainman.ActiveChain().Tip()->GetBlockHash()));
            connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::INV, vInv));
            pfrom.hashContinue.SetNull();
        }
    }
}

//! Determine whether or not a peer can request a transaction, and return it (or nullptr if not found or not allowed).
CTransactionRef PeerManagerImpl::FindTxForGetData(CNode* peer, const uint256& txid, const std::chrono::seconds mempool_req, const std::chrono::seconds longlived_mempool_time) LOCKS_EXCLUDED(cs_main)
{
    // Check if the requested transaction is so recent that we're just
    // about to announce it to the peer; if so, they certainly shouldn't
    // know we already have it.
    {
        LOCK(peer->m_tx_relay->cs_tx_inventory);
        if (peer->m_tx_relay->setInventoryTxToSend.count(txid)) return {};
    }

    {
        LOCK(cs_main);
        // Look up transaction in relay pool
        auto mi = mapRelay.find(txid);
        if (mi != mapRelay.end()) return mi->second;
    }

    auto txinfo = m_mempool.info(txid);
    if (txinfo.tx) {
        // To protect privacy, do not answer getdata using the mempool when
        // that TX couldn't have been INVed in reply to a MEMPOOL request,
        // or when it's too recent to have expired from mapRelay.
        if ((mempool_req.count() && txinfo.m_time <= mempool_req) || txinfo.m_time <= longlived_mempool_time) {
            return txinfo.tx;
        }
    }

    return {};
}

void PeerManagerImpl::ProcessGetData(CNode& pfrom, Peer& peer, const std::atomic<bool>& interruptMsgProc)
{
    AssertLockNotHeld(cs_main);

    std::deque<CInv>::iterator it = peer.m_getdata_requests.begin();
    std::vector<CInv> vNotFound;
    const CNetMsgMaker msgMaker(pfrom.GetSendVersion());

    // mempool entries added before this time have likely expired from mapRelay
    const std::chrono::seconds longlived_mempool_time = GetTime<std::chrono::seconds>() - RELAY_TX_CACHE_TIME;
    // Get last mempool request time
    const std::chrono::seconds mempool_req = !pfrom.IsAddrRelayPeer() ? pfrom.m_tx_relay->m_last_mempool_req.load()
                                                                          : std::chrono::seconds::min();

    // Process as many TX items from the front of the getdata queue as
    // possible, since they're common and it's efficient to batch process
    // them.
    while (it != peer.m_getdata_requests.end() && it->IsKnownType()) {
        if (interruptMsgProc)
            return;
        // The send buffer provides backpressure. If there's no space in
        // the buffer, pause processing until the next call.
        if (pfrom.fPauseSend)
            break;

        const CInv &inv = *it;

        if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK || inv.type == MSG_CMPCT_BLOCK) {
            break;
        }
        ++it;

        if (!pfrom.IsAddrRelayPeer() && NetMessageViolatesBlocksOnly(inv.GetCommand())) {
            // Note that if we receive a getdata for non-block messages
            // from a block-relay-only outbound peer that violate the policy,
            // we skip such getdata messages from this peer
            continue;
        }

        bool push = false;
        if (inv.type == MSG_TX || inv.type == MSG_DSTX) {
            CTransactionRef tx = FindTxForGetData(&pfrom, inv.hash, mempool_req, longlived_mempool_time);
            if (tx) {
                CCoinJoinBroadcastTx dstx;
                if (inv.type == MSG_DSTX) {
                    dstx = CCoinJoin::GetDSTX(inv.hash);
                }
                if (dstx) {
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::DSTX, dstx));
                } else {
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::TX, *tx));
                }
                m_mempool.RemoveUnbroadcastTx(inv.hash);
                push = true;
            }
        }

        if (!push && inv.type == MSG_SPORK) {
            if (auto opt_spork = sporkManager->GetSporkByHash(inv.hash)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SPORK, *opt_spork));
                push = true;
            }
        }

        if (!push && inv.type == MSG_GOVERNANCE_OBJECT) {
            CDataStream ss(SER_NETWORK, pfrom.GetSendVersion());
            bool topush = false;
            if (m_govman.HaveObjectForHash(inv.hash)) {
                ss.reserve(1000);
                if (m_govman.SerializeObjectForHash(inv.hash, ss)) {
                    topush = true;
                }
            }
            if (topush) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::MNGOVERNANCEOBJECT, ss));
                push = true;
            }
        }

        if (!push && inv.type == MSG_GOVERNANCE_OBJECT_VOTE) {
            CDataStream ss(SER_NETWORK, pfrom.GetSendVersion());
            bool topush = false;
            if (m_govman.HaveVoteForHash(inv.hash)) {
                ss.reserve(1000);
                if (m_govman.SerializeVoteForHash(inv.hash, ss)) {
                    topush = true;
                }
            }
            if (topush) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::MNGOVERNANCEOBJECTVOTE, ss));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_QUORUM_FINAL_COMMITMENT)) {
            llmq::CFinalCommitment o;
            if (m_llmq_ctx->quorum_block_processor->GetMineableCommitmentByHash(
                    inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QFCOMMITMENT, o));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_QUORUM_CONTRIB)) {
            llmq::CDKGContribution o;
            if (m_llmq_ctx->qdkgsman->GetContribution(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QCONTRIB, o));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_QUORUM_COMPLAINT)) {
            llmq::CDKGComplaint o;
            if (m_llmq_ctx->qdkgsman->GetComplaint(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QCOMPLAINT, o));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_QUORUM_JUSTIFICATION)) {
            llmq::CDKGJustification o;
            if (m_llmq_ctx->qdkgsman->GetJustification(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QJUSTIFICATION, o));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_QUORUM_PREMATURE_COMMITMENT)) {
            llmq::CDKGPrematureCommitment o;
            if (m_llmq_ctx->qdkgsman->GetPrematureCommitment(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QPCOMMITMENT, o));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_QUORUM_RECOVERED_SIG)) {
            llmq::CRecoveredSig o;
            if (m_llmq_ctx->sigman->GetRecoveredSigForGetData(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QSIGREC, o));
                push = true;
            }
        }

        if (!push && (inv.type == MSG_CLSIG)) {
            llmq::CChainLockSig o;
            if (m_llmq_ctx->clhandler->GetChainLockByHash(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::CLSIG, o));
                push = true;
            }
        }

        if (!push && inv.type == MSG_ISDLOCK) {
            llmq::CInstantSendLock o;
            if (m_llmq_ctx->isman->GetInstantSendLockByHash(inv.hash, o)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::ISDLOCK, o));
                push = true;
            }
        }

        if (!push) {
            vNotFound.push_back(inv);
        }
    }

    // Only process one BLOCK item per call, since they're uncommon and can be
    // expensive to process.
    if (it != peer.m_getdata_requests.end() && !pfrom.fPauseSend) {
        const CInv &inv = *it++;
        if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK || inv.type == MSG_CMPCT_BLOCK) {
            ProcessGetBlockData(pfrom, m_chainparams, inv, m_connman, *m_llmq_ctx->isman);
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

void PeerManagerImpl::SendBlockTransactions(CNode& pfrom, const CBlock& block, const BlockTransactionsRequest& req) {
    BlockTransactions resp(req);
    for (size_t i = 0; i < req.indexes.size(); i++) {
        if (req.indexes[i] >= block.vtx.size()) {
            Misbehaving(pfrom.GetId(), 100, "getblocktxn with out-of-bounds tx indices");
            return;
        }
        resp.txn[i] = block.vtx[req.indexes[i]];
    }
    LOCK(cs_main);
    CNetMsgMaker msgMaker(pfrom.GetSendVersion());
    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCKTXN, resp));
}

void PeerManagerImpl::ProcessHeadersMessage(CNode& pfrom, const std::vector<CBlockHeader>& headers, bool via_compact_block)
{
    const CNetMsgMaker msgMaker(pfrom.GetSendVersion());
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
            std::string msg_type = (pfrom.nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::GETHEADERS2 : NetMsgType::GETHEADERS;
            m_connman.PushMessage(&pfrom, msgMaker.Make(msg_type, m_chainman.ActiveChain().GetLocator(pindexBestHeader), uint256()));
            LogPrint(BCLog::NET, "received header %s: missing prev block %s, sending %s (%d) to end (peer=%d, nUnconnectingHeaders=%d)\n",
                    headers[0].GetHash().ToString(),
                    headers[0].hashPrevBlock.ToString(),
                    msg_type,
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
            std::string msg_type = (pfrom.nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::GETHEADERS2 : NetMsgType::GETHEADERS;
            LogPrint(BCLog::NET, "more %s (%d) to end to peer=%d (startheight:%d)\n", msg_type, pindexLast->nHeight, pfrom.GetId(), pfrom.nStartingHeight);
            m_connman.PushMessage(&pfrom, msgMaker.Make(msg_type, m_chainman.ActiveChain().GetLocator(pindexLast), uint256()));
        }

        bool fCanDirectFetch = CanDirectFetch(m_chainparams.GetConsensus());
        // If this set of headers is valid and ends in a block with at least as
        // much work as our tip, download as much as possible.
        if (fCanDirectFetch && pindexLast->IsValid(BLOCK_VALID_TREE) && m_chainman.ActiveChain().Tip()->nChainWork <= pindexLast->nChainWork) {
            std::vector<const CBlockIndex*> vToFetch;
            const CBlockIndex *pindexWalk = pindexLast;
            // Calculate all the blocks we'd need to switch to pindexLast, up to a limit.
            while (pindexWalk && !m_chainman.ActiveChain().Contains(pindexWalk) && vToFetch.size() <= MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
                if (!(pindexWalk->nStatus & BLOCK_HAVE_DATA) &&
                        !mapBlocksInFlight.count(pindexWalk->GetBlockHash())) {
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
                    vGetData.push_back(CInv(MSG_BLOCK, pindex->GetBlockHash()));
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
        if (!pfrom.fDisconnect && pfrom.IsOutboundOrBlockRelayConn() && nodestate->pindexBestKnownBlock != nullptr && pfrom.IsAddrRelayPeer()) {
            if (m_outbound_peers_with_protect_from_disconnect < MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT && nodestate->pindexBestKnownBlock->nChainWork >= m_chainman.ActiveChain().Tip()->nChainWork && !nodestate->m_chain_sync.m_protect) {
                LogPrint(BCLog::NET, "Protecting outbound peer=%d from eviction\n", pfrom.GetId());
                nodestate->m_chain_sync.m_protect = true;
                ++m_outbound_peers_with_protect_from_disconnect;
            }
        }
    }

    return;
}

void PeerManagerImpl::ProcessOrphanTx(std::set<uint256>& orphan_work_set)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(g_cs_orphans);
    std::set<NodeId> setMisbehaving;
    bool done = false;
    while (!done && !orphan_work_set.empty()) {
        const uint256 orphanHash = *orphan_work_set.begin();
        orphan_work_set.erase(orphan_work_set.begin());

        auto orphan_it = mapOrphanTransactions.find(orphanHash);
        if (orphan_it == mapOrphanTransactions.end()) continue;

        const CTransactionRef porphanTx = orphan_it->second.tx;
        const CTransaction& orphanTx = *porphanTx;
        NodeId fromPeer = orphan_it->second.fromPeer;
        // Use a new TxValidationState because orphans come from different peers (and we call
        // MaybePunishNodeForTx based on the source peer from the orphan map, not based on the peer
        // that relayed the previous transaction).
        TxValidationState orphan_state;

        if (setMisbehaving.count(fromPeer)) continue;
        if (AcceptToMemoryPool(m_chainman.ActiveChainstate(), m_mempool, orphan_state, porphanTx,
                false /* bypass_limits */, 0 /* nAbsurdFee */)) {
            LogPrint(BCLog::MEMPOOL, "   accepted orphan tx %s\n", orphanHash.ToString());
            RelayTransaction(orphanTx.GetHash());
            for (unsigned int i = 0; i < orphanTx.vout.size(); i++) {
                auto it_by_prev = mapOrphanTransactionsByPrev.find(COutPoint(orphanHash, i));
                if (it_by_prev != mapOrphanTransactionsByPrev.end()) {
                    for (const auto& elem : it_by_prev->second) {
                        orphan_work_set.insert(elem->first);
                    }
                }
            }
            EraseOrphanTx(orphanHash);
            done = true;
        } else if (orphan_state.GetResult() != TxValidationResult::TX_MISSING_INPUTS) {
            if (orphan_state.IsInvalid()) {
                // Punish peer that gave us an invalid orphan tx
                if (MaybePunishNodeForTx(fromPeer, orphan_state)) {
                    setMisbehaving.insert(fromPeer);
                }
                LogPrint(BCLog::MEMPOOL, "   invalid orphan tx %s\n", orphanHash.ToString());
            }
            // Has inputs but not accepted to mempool
            // Probably non-standard or insufficient fee
            LogPrint(BCLog::MEMPOOL, "   removed orphan tx %s\n", orphanHash.ToString());
            m_recent_rejects.insert(orphanHash);
            EraseOrphanTx(orphanHash);
            done = true;
        }
        m_mempool.check(m_chainman.ActiveChainstate());
    }
}

bool PeerManagerImpl::PrepareBlockFilterRequest(CNode& peer, const CChainParams& chain_params,
                                                BlockFilterType filter_type, uint32_t start_height,
                                                const uint256& stop_hash, uint32_t max_height_diff,
                                                const CBlockIndex*& stop_index,
                                                BlockFilterIndex*& filter_index)
{
    const bool supported_filter_type =
        (filter_type == BlockFilterType::BASIC_FILTER &&
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
        if (!stop_index || !BlockRequestAllowed(stop_index, chain_params.GetConsensus())) {
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

void PeerManagerImpl::ProcessGetCFilters(CNode& peer, CDataStream& vRecv, const CChainParams& chain_params,
                                         CConnman& connman)
{
    uint8_t filter_type_ser;
    uint32_t start_height;
    uint256 stop_hash;

    vRecv >> filter_type_ser >> start_height >> stop_hash;

    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);

    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(peer, chain_params, filter_type, start_height, stop_hash,
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
        CSerializedNetMsg msg = CNetMsgMaker(peer.GetSendVersion())
            .Make(NetMsgType::CFILTER, filter);
        connman.PushMessage(&peer, std::move(msg));
    }
}

void PeerManagerImpl::ProcessGetCFHeaders(CNode& peer, CDataStream& vRecv, const CChainParams& chain_params,
                                          CConnman& connman)
{
    uint8_t filter_type_ser;
    uint32_t start_height;
    uint256 stop_hash;

    vRecv >> filter_type_ser >> start_height >> stop_hash;

    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);

    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(peer, chain_params, filter_type, start_height, stop_hash,
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

    CSerializedNetMsg msg = CNetMsgMaker(peer.GetSendVersion())
        .Make(NetMsgType::CFHEADERS,
              filter_type_ser,
              stop_index->GetBlockHash(),
              prev_header,
              filter_hashes);
    connman.PushMessage(&peer, std::move(msg));
}

void PeerManagerImpl::ProcessGetCFCheckPt(CNode& peer, CDataStream& vRecv, const CChainParams& chain_params,
                                          CConnman& connman)
{
    uint8_t filter_type_ser;
    uint256 stop_hash;

    vRecv >> filter_type_ser >> stop_hash;

    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);

    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(peer, chain_params, filter_type, /*start_height=*/0, stop_hash,
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

    CSerializedNetMsg msg = CNetMsgMaker(peer.GetSendVersion())
        .Make(NetMsgType::CFCHECKPT,
              filter_type_ser,
              stop_index->GetBlockHash(),
              headers);
    connman.PushMessage(&peer, std::move(msg));
}

std::pair<bool /*ret*/, bool /*do_return*/> static ValidateDSTX(CTxMemPool& mempool, ChainstateManager& chainman, CCoinJoinBroadcastTx& dstx, uint256 hashTx)
{
    assert(::mmetaman != nullptr);

    if (!dstx.IsValidStructure()) {
        LogPrint(BCLog::COINJOIN, "DSTX -- Invalid DSTX structure: %s\n", hashTx.ToString());
        return {false, true};
    }
    if (CCoinJoin::GetDSTX(hashTx)) {
        LogPrint(BCLog::COINJOIN, "DSTX -- Already have %s, skipping...\n", hashTx.ToString());
        return {true, true}; // not an error
    }

    const CBlockIndex* pindex{nullptr};
    CDeterministicMNCPtr dmn{nullptr};
    {
        LOCK(cs_main);
        pindex = chainman.ActiveChain().Tip();
    }
    // It could be that a MN is no longer in the list but its DSTX is not yet mined.
    // Try to find a MN up to 24 blocks deep to make sure such dstx-es are relayed and processed correctly.
    if (dstx.masternodeOutpoint.IsNull()) {
        for (int i = 0; i < 24 && pindex; ++i) {
            dmn = deterministicMNManager->GetListForBlock(pindex).GetMN(dstx.m_protxHash);
            if (dmn) {
                dstx.masternodeOutpoint = dmn->collateralOutpoint;
                break;
            }
            pindex = pindex->pprev;
        }
    } else {
        for (int i = 0; i < 24 && pindex; ++i) {
            dmn = deterministicMNManager->GetListForBlock(pindex).GetMNByCollateral(dstx.masternodeOutpoint);
            if (dmn) {
                dstx.m_protxHash = dmn->proTxHash;
                break;
            }
            pindex = pindex->pprev;
        }
    }

    if (!dmn) {
        LogPrint(BCLog::COINJOIN, "DSTX -- Can't find masternode %s to verify %s\n", dstx.masternodeOutpoint.ToStringShort(), hashTx.ToString());
        return {false, true};
    }

    if (!mmetaman->GetMetaInfo(dmn->proTxHash)->IsValidForMixingTxes()) {
        LogPrint(BCLog::COINJOIN, "DSTX -- Masternode %s is sending too many transactions %s\n", dstx.masternodeOutpoint.ToStringShort(), hashTx.ToString());
        return {true, true};
        // TODO: Not an error? Could it be that someone is relaying old DSTXes
        // we have no idea about (e.g we were offline)? How to handle them?
    }

    if (!dstx.CheckSignature(dmn->pdmnState->pubKeyOperator.Get())) {
        LogPrint(BCLog::COINJOIN, "DSTX -- CheckSignature() failed for %s\n", hashTx.ToString());
        return {false, true};
    }

    LogPrint(BCLog::COINJOIN, "DSTX -- Got Masternode transaction %s\n", hashTx.ToString());
    mempool.PrioritiseTransaction(hashTx, 0.1*COIN);
    mmetaman->DisallowMixing(dmn->proTxHash);

    return {true, false};
}

void PeerManagerImpl::ProcessBlock(CNode& pfrom, const std::shared_ptr<const CBlock>& pblock, bool fForceProcessing)
{
    bool fNewBlock = false;
    m_chainman.ProcessNewBlock(m_chainparams, pblock, fForceProcessing, &fNewBlock);
    if (fNewBlock) {
        pfrom.nLastBlockTime = GetTime();
    } else {
        LOCK(cs_main);
        mapBlockSource.erase(pblock->GetHash());
    }
}

void PeerManagerImpl::ProcessMessage(
    CNode& pfrom,
    const std::string& msg_type,
    CDataStream& vRecv,
    int64_t nTimeReceived,
    const std::atomic<bool>& interruptMsgProc)
{
    LogPrint(BCLog::NET, "received: %s (%u bytes) peer=%d\n", SanitizeString(msg_type), vRecv.size(), pfrom.GetId());
    statsClient.inc("message.received." + SanitizeString(msg_type), 1.0f);


    PeerRef peer = GetPeerRef(pfrom.GetId());
    if (peer == nullptr) return;

    if (!(pfrom.GetLocalServices() & NODE_BLOOM) &&
              (msg_type == NetMsgType::FILTERLOAD ||
               msg_type == NetMsgType::FILTERADD))
    {
        Misbehaving(pfrom.GetId(), 100);
        return;
    }

    if (msg_type == NetMsgType::VERSION) {
        // Each connection can only send one version message
        if (pfrom.nVersion != 0)
        {
            Misbehaving(pfrom.GetId(), 1, "redundant version message");
            return;
        }

        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        uint64_t nServiceInt;
        ServiceFlags nServices;
        int nVersion;
        int nSendVersion;
        std::string cleanSubVer;
        int nStartingHeight = -1;
        bool fRelay = true;

        vRecv >> nVersion >> nServiceInt >> nTime >> addrMe;
        nSendVersion = std::min(nVersion, PROTOCOL_VERSION);
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
            vRecv >> nStartingHeight;
        }
        if (!vRecv.empty())
            vRecv >> fRelay;
        if (!vRecv.empty()) {
            uint256 receivedMNAuthChallenge;
            vRecv >> receivedMNAuthChallenge;
            pfrom.SetReceivedMNAuthChallenge(receivedMNAuthChallenge);
        }
        if (!vRecv.empty()) {
            bool fOtherMasternode = false;
            vRecv >> fOtherMasternode;
            if (pfrom.IsInboundConn()) {
                pfrom.m_masternode_connection = fOtherMasternode;
                if (fOtherMasternode) {
                    LogPrint(BCLog::NET_NETCONN, "peer=%d is an inbound masternode connection, not relaying anything to it\n", pfrom.GetId());
                    if (!fMasternodeMode) {
                        LogPrint(BCLog::NET_NETCONN, "but we're not a masternode, disconnecting\n");
                        pfrom.fDisconnect = true;
                        return;
                    }
                }
            }
        }
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

        // Be shy and don't send version until we hear
        if (pfrom.IsInboundConn())
            PushNodeVersion(pfrom, GetAdjustedTime());

        if (Params().NetworkIDString() == CBaseChainParams::DEVNET) {
            if (cleanSubVer.find(strprintf("devnet.%s", gArgs.GetDevNetName())) == std::string::npos) {
                LogPrintf("connected to wrong devnet. Reported version is %s, expected devnet name is %s\n", cleanSubVer, gArgs.GetDevNetName());
                if (!pfrom.IsInboundConn())
                    Misbehaving(pfrom.GetId(), 100); // don't try to connect again
                else
                    Misbehaving(pfrom.GetId(), 1); // whover connected, might just have made a mistake, don't ban him immediately
                pfrom.fDisconnect = true;
                return;
            }
        }

        const CNetMsgMaker msg_maker(INIT_PROTO_VERSION);
        // Signal ADDRv2 support (BIP155).
        if (nSendVersion >= ADDRV2_PROTO_VERSION) {
            // BIP155 defines addrv2 and sendaddrv2 for all protocol versions, but some
            // implementations reject messages they don't know. As a courtesy, don't send
            // it to nodes with a version before ADDRV2_PROTO_VERSION.
            m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::SENDADDRV2));
        }

        m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::VERACK));

        pfrom.nServices = nServices;
        pfrom.SetAddrLocal(addrMe);
        {
            LOCK(pfrom.cs_SubVer);
            pfrom.cleanSubVer = cleanSubVer;
        }
        pfrom.nStartingHeight = nStartingHeight;

        // set nodes not relaying blocks and tx and not serving (parts) of the historical blockchain as "clients"
        pfrom.fClient = (!(nServices & NODE_NETWORK) && !(nServices & NODE_NETWORK_LIMITED));

        // set nodes not capable of serving the complete blockchain history as "limited nodes"
        pfrom.m_limited_node = (!(nServices & NODE_NETWORK) && (nServices & NODE_NETWORK_LIMITED));

        if (pfrom.IsAddrRelayPeer()) {
            LOCK(pfrom.m_tx_relay->cs_filter);
            pfrom.m_tx_relay->fRelayTxes = fRelay; // set to true after we get the first filter* message
        }

        // Change version
        pfrom.SetSendVersion(nSendVersion);
        pfrom.nVersion = nVersion;

        // Potentially mark this peer as a preferred download peer.
        {
        LOCK(cs_main);
        UpdatePreferredDownload(pfrom, State(pfrom.GetId()));
        }

        if (!pfrom.IsInboundConn() && pfrom.IsAddrRelayPeer())
        {
            // Advertise our address
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
            m_connman.PushMessage(&pfrom, CNetMsgMaker(nSendVersion).Make(NetMsgType::GETADDR));
            pfrom.fGetAddr = true;
            m_addrman.Good(pfrom.addr);
        }

        std::string remoteAddr;
        if (fLogIPs)
            remoteAddr = ", peeraddr=" + pfrom.addr.ToString();

        LogPrint(BCLog::NET, "receive version message: %s: version %d, blocks=%d, us=%s, peer=%d%s\n",
                  cleanSubVer, pfrom.nVersion,
                  pfrom.nStartingHeight, addrMe.ToString(), pfrom.GetId(),
                  remoteAddr);

        int64_t nTimeOffset = nTime - GetTime();
        pfrom.nTimeOffset = nTimeOffset;
        AddTimeData(pfrom.addr, nTimeOffset);

        // Feeler connections exist only to verify if address is online.
        if (pfrom.IsFeelerConn()) {
            pfrom.fDisconnect = true;
        }
        return;
    }

    if (pfrom.nVersion == 0) {
        // Must have a version message before anything else
        Misbehaving(pfrom.GetId(), 1, "non-version message before version handshake");
        return;
    }

    // At this point, the outgoing message serialization version can't change.
    const CNetMsgMaker msgMaker(pfrom.GetSendVersion());

    bool fBlocksOnly = pfrom.IsBlockRelayOnly();

    if (msg_type == NetMsgType::VERACK)
    {
        pfrom.SetRecvVersion(std::min(pfrom.nVersion.load(), PROTOCOL_VERSION));

        if (!pfrom.IsInboundConn()) {
            LogPrintf("New outbound peer connected: version: %d, blocks=%d, peer=%d%s (%s)\n",
                      pfrom.nVersion.load(), pfrom.nStartingHeight,
                      pfrom.GetId(), (fLogIPs ? strprintf(", peeraddr=%s", pfrom.addr.ToString()) : ""),
                      pfrom.IsAddrRelayPeer()?  "full-relay" : "block-relay");
        }

        if (!pfrom.m_masternode_probe_connection) {
            CMNAuth::PushMNAUTH(pfrom, m_connman, m_chainman.ActiveChain().Tip());
        }

        // Tell our peer we prefer to receive headers rather than inv's
        // We send this to non-NODE NETWORK peers as well, because even
        // non-NODE NETWORK peers can announce blocks (such as pruning
        // nodes)
        m_connman.PushMessage(&pfrom, msgMaker.Make((pfrom.nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::SENDHEADERS2 : NetMsgType::SENDHEADERS));

        if (pfrom.CanRelay()) {
            // Tell our peer we are willing to provide version-1 cmpctblocks
            // However, we do not request new block announcements using
            // cmpctblock messages.
            // We send this to non-NODE NETWORK peers as well, because
            // they may wish to request compact blocks from us
            bool fAnnounceUsingCMPCTBLOCK = false;
            uint64_t nCMPCTBLOCKVersion = 1;
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion));
        }

        if (!fBlocksOnly) {
            // Tell our peer that he should send us CoinJoin queue messages
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SENDDSQUEUE, true));
            // Tell our peer that he should send us intra-quorum messages
            if (llmq::utils::IsWatchQuorumsEnabled() && m_connman.IsMasternodeQuorumNode(&pfrom)) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QWATCH));
            }
        }

        pfrom.fSuccessfullyConnected = true;
        return;
    }

    if (msg_type == NetMsgType::SENDADDRV2) {
        if (pfrom.GetSendVersion() < ADDRV2_PROTO_VERSION) {
            // Ignore previous implementations
            return;
        }
        if (pfrom.fSuccessfullyConnected) {
            // Disconnect peers that send SENDADDRV2 message after VERACK; this
            // must be negotiated between VERSION and VERACK.
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

    if (pfrom.nTimeFirstMessageReceived == 0) {
        // First message after VERSION/VERACK
        pfrom.nTimeFirstMessageReceived = GetSystemTimeInSeconds();
        pfrom.fFirstMessageIsMNAUTH = msg_type == NetMsgType::MNAUTH;
        // Note: do not break the flow here

        if (pfrom.m_masternode_probe_connection && !pfrom.fFirstMessageIsMNAUTH) {
            LogPrint(BCLog::NET, "connection is a masternode probe but first received message is not MNAUTH, peer=%d\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
    }

    // Stop processing non-block data early in blocks only mode and for block-relay-only peers
    if (fBlocksOnly && NetMessageViolatesBlocksOnly(msg_type)) {
        LogPrint(BCLog::NET, "%s sent in violation of protocol peer=%d\n", msg_type, pfrom.GetId());
        pfrom.fDisconnect = true;
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

        if (!pfrom.IsAddrRelayPeer()) {
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
            if (m_banman->IsDiscouraged(addr)) continue; // Do not process banned/discouraged addresses beyond remembering we received them
            if (m_banman->IsBanned(addr)) continue;
            bool fReachable = IsReachable(addr);
            if (addr.nTime > nSince && !pfrom.fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
            {
                RelayAddress(addr, fReachable, m_connman);
            }
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        m_addrman.Add(vAddrOk, pfrom.addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom.fGetAddr = false;
        if (pfrom.IsAddrFetchConn())
            pfrom.fDisconnect = true;
        return;
    }

    if (msg_type == NetMsgType::SENDHEADERS) {
        LOCK(cs_main);
        State(pfrom.GetId())->fPreferHeaders = true;
        return;
    }

    if (msg_type == NetMsgType::SENDHEADERS2) {
        LOCK(cs_main);
        State(pfrom.GetId())->fPreferHeadersCompressed = true;
        return;
    }

    if (msg_type == NetMsgType::SENDCMPCT) {
        bool fAnnounceUsingCMPCTBLOCK = false;
        uint64_t nCMPCTBLOCKVersion = 1;
        vRecv >> fAnnounceUsingCMPCTBLOCK >> nCMPCTBLOCKVersion;
        if (nCMPCTBLOCKVersion == 1) {
            LOCK(cs_main);
            State(pfrom.GetId())->fProvidesHeaderAndIDs = true;
            State(pfrom.GetId())->fPreferHeaderAndIDs = fAnnounceUsingCMPCTBLOCK;
            State(pfrom.GetId())->fSupportsDesiredCmpctVersion = true;
        }
        return;
    }


    if (msg_type == NetMsgType::SENDDSQUEUE)
    {
        bool b;
        vRecv >> b;
        pfrom.fSendDSQueue = b;
        return;
    }


    if (msg_type == NetMsgType::QSENDRECSIGS) {
        bool b;
        vRecv >> b;
        pfrom.fSendRecSigs = b;
        return;
    }

    if (msg_type == NetMsgType::INV) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(pfrom.GetId(), 20, strprintf("message inv size() = %u", vInv.size()));
            return;
        }

        LOCK(cs_main);

        const auto current_time = GetTime<std::chrono::microseconds>();
        uint256* best_block{nullptr};

        for (CInv &inv : vInv)
        {
            if(!inv.IsKnownType()) {
                LogPrint(BCLog::NET, "got inv of unknown type %d: %s peer=%d\n", inv.type, inv.hash.ToString(), pfrom.GetId());
                continue;
            }

            if (interruptMsgProc)
                return;

            bool fAlreadyHave = AlreadyHave(inv);
            LogPrint(BCLog::NET, "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom.GetId());
            statsClient.inc(strprintf("message.received.inv_%s", inv.GetCommand()), 1.0f);

            if (inv.type == MSG_BLOCK) {
                UpdateBlockAvailability(pfrom.GetId(), inv.hash);

                if (fAlreadyHave || fImporting || fReindex || mapBlocksInFlight.count(inv.hash)) {
                    continue;
                }

                CNodeState *state = State(pfrom.GetId());
                if (!state) {
                    continue;
                }

                // Download if this is a nice peer, or we have no nice peers and this one might do.
                bool fFetch = state->fPreferredDownload || (nPreferredDownload == 0 && !pfrom.IsAddrFetchConn());
                // Only actively request headers from a single peer, unless we're close to end of initial download.
                if ((nSyncStarted == 0 && fFetch) || pindexBestHeader->GetBlockTime() > GetAdjustedTime() - nMaxTipAge) {
                    // Make sure to mark this peer as the one we are currently syncing with etc.
                    state->fSyncStarted = true;
                    state->nHeadersSyncTimeout = GetTimeMicros() + HEADERS_DOWNLOAD_TIMEOUT_BASE + HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER * (GetAdjustedTime() - pindexBestHeader->GetBlockTime())/(m_chainparams.GetConsensus().nPowTargetSpacing);
                    nSyncStarted++;
                    // Headers-first is the primary method of announcement on
                    // the network. If a node fell back to sending blocks by inv,
                    // it's probably for a re-org. The final block hash
                    // provided should be the highest, so send a getheaders and
                    // then fetch the blocks we need to catch up.
                    best_block = &inv.hash;
                }
            } else {
                static std::set<int> allowWhileInIBDObjs = {
                        MSG_SPORK
                };

                pfrom.AddInventoryKnown(inv);
                if (fBlocksOnly && NetMessageViolatesBlocksOnly(inv.GetCommand())) {
                    LogPrint(BCLog::NET, "%s (%s) inv sent in violation of protocol, disconnecting peer=%d\n", inv.GetCommand(), inv.hash.ToString(), pfrom.GetId());
                    pfrom.fDisconnect = true;
                    return;
                } else if (!fAlreadyHave) {
                    if (fBlocksOnly && inv.type == MSG_ISDLOCK) {
                        if (pfrom.GetRecvVersion() <= ADDRV2_PROTO_VERSION) {
                            // It's ok to receive these invs, we just ignore them
                            // and do not request corresponding objects.
                            continue;
                        }
                        // Peers with newer versions should never send us these invs when we are in blocks-relay-only mode
                        LogPrint(BCLog::NET, "%s (%s) inv sent in violation of protocol, disconnecting peer=%d\n", inv.GetCommand(), inv.hash.ToString(), pfrom.GetId());
                        pfrom.fDisconnect = true;
                        return;
                    }
                    bool allowWhileInIBD = allowWhileInIBDObjs.count(inv.type);
                    if (allowWhileInIBD || !m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
                        RequestObject(State(pfrom.GetId()), inv, current_time);
                    }
                }
            }
        }
        if (best_block != nullptr) {
            std::string msg_type = (pfrom.nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::GETHEADERS2 : NetMsgType::GETHEADERS;
            m_connman.PushMessage(&pfrom, msgMaker.Make(msg_type, m_chainman.ActiveChain().GetLocator(pindexBestHeader), *best_block));
            LogPrint(BCLog::NET, "%s (%d) %s to peer=%d\n", msg_type, pindexBestHeader->nHeight, best_block->ToString(), pfrom.GetId());
        }

        return;
    }

    if (msg_type == NetMsgType::GETDATA) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(pfrom.GetId(), 20, strprintf("message getdata size() = %u", vInv.size()));
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
            if (!m_chainman.ActiveChainstate().ActivateBestChain(state, a_recent_block)) {
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
            if (pfrom.CanRelay()) {
                pfrom.PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            }
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll
                // trigger the peer to getblocks the next batch of inventory.
                LogPrint(BCLog::NET, "  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                pfrom.hashContinue = pindex->GetBlockHash();
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
        WITH_LOCK(cs_main, inv.type = MSG_BLOCK);
        inv.hash = req.blockhash;
        WITH_LOCK(peer->m_getdata_requests_mutex, peer->m_getdata_requests.push_back(inv));
        // The message processing loop will go around again (without pausing) and we'll respond then (without cs_main)
        return;
    }

    if (msg_type == NetMsgType::GETHEADERS || msg_type == NetMsgType::GETHEADERS2) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "%s locator size %lld > %d, disconnect peer=%d\n", msg_type, locator.vHave.size(), MAX_LOCATOR_SZ, pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }

        LOCK(cs_main);
        if (m_chainman.ActiveChainstate().IsInitialBlockDownload() && !pfrom.HasPermission(PF_DOWNLOAD)) {
            LogPrint(BCLog::NET, "Ignoring %s from peer=%d because node is in initial block download\n", msg_type, pfrom.GetId());
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

            if (!BlockRequestAllowed(pindex, m_chainparams.GetConsensus())) {
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

        const auto send_headers = [this /* for m_connman */, &hashStop, &pindex, &nodestate, &pfrom, &msgMaker](auto msg_type, auto& v_headers, auto callback) {
            int nLimit = MAX_HEADERS_RESULTS;
            for (; pindex; pindex = m_chainman.ActiveChain().Next(pindex)) {
                v_headers.push_back(callback(pindex));

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
            m_connman.PushMessage(&pfrom, msgMaker.Make(msg_type, v_headers));
        };

        LogPrint(BCLog::NET, "%s %d to %s from peer=%d\n", msg_type, (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), pfrom.GetId());
        if (msg_type == NetMsgType::GETHEADERS) {
            // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
            std::vector<CBlock> v_headers;
            send_headers(NetMsgType::HEADERS, v_headers, [](const auto block_pindex) { return block_pindex->GetBlockHeader(); });
        } else if (msg_type == NetMsgType::GETHEADERS2) {
            // Keeps track of the last 7 unique version blocks
            std::list<int32_t> last_unique_versions;
            std::vector<CompressibleBlockHeader> v_headers;

            send_headers(NetMsgType::HEADERS2, v_headers, [&v_headers, &last_unique_versions](const auto block_pindex) {
                CompressibleBlockHeader compressible_header{block_pindex->GetBlockHeader()};
                if (!v_headers.empty()) compressible_header.Compress(v_headers, last_unique_versions); // first block is always uncompressed
                return compressible_header;
            });
        }
        return;
    }

    if (msg_type == NetMsgType::TX || msg_type == NetMsgType::DSTX) {
        CTransactionRef ptx;
        CCoinJoinBroadcastTx dstx;
        int nInvType = MSG_TX;

        // Read data and assign inv type
        if(msg_type == NetMsgType::TX) {
            vRecv >> ptx;
        } else if (msg_type == NetMsgType::DSTX) {
            vRecv >> dstx;
            ptx = dstx.tx;
            nInvType = MSG_DSTX;
        }
        const CTransaction& tx = *ptx;

        CInv inv(nInvType, tx.GetHash());
        pfrom.AddInventoryKnown(inv);

        {
            LOCK(cs_main);
            EraseObjectRequest(pfrom.GetId(), inv);
        }

        // Process custom logic, no matter if tx will be accepted to mempool later or not
        if (nInvType == MSG_DSTX) {
           // Validate DSTX and return bRet if we need to return from here
           uint256 hashTx = tx.GetHash();
           const auto& [bRet, bDoReturn] = ValidateDSTX(m_mempool, m_chainman, dstx, hashTx);
           if (bDoReturn) {
               return;
           }
        }

        LOCK2(cs_main, g_cs_orphans);

        TxValidationState state;

        if (!AlreadyHave(inv) && AcceptToMemoryPool(m_chainman.ActiveChainstate(), m_mempool, state, ptx,
                false /* bypass_limits */, 0 /* nAbsurdFee */)) {
            // Process custom txes, this changes AlreadyHave to "true"
            if (nInvType == MSG_DSTX) {
                LogPrint(BCLog::COINJOIN, "DSTX -- Masternode transaction accepted, txid=%s, peer=%d\n",
                         tx.GetHash().ToString(), pfrom.GetId());
                CCoinJoin::AddDSTX(dstx);
            }

            m_mempool.check(m_chainman.ActiveChainstate());
            RelayTransaction(tx.GetHash());

            for (unsigned int i = 0; i < tx.vout.size(); i++) {
                auto it_by_prev = mapOrphanTransactionsByPrev.find(COutPoint(inv.hash, i));
                if (it_by_prev != mapOrphanTransactionsByPrev.end()) {
                    for (const auto& elem : it_by_prev->second) {
                        peer->m_orphan_work_set.insert(elem->first);
                    }
                }
            }

            pfrom.nLastTXTime = GetTime();

            LogPrint(BCLog::MEMPOOL, "AcceptToMemoryPool: peer=%d: accepted %s (poolsz %u txn, %u kB)\n",
                     pfrom.GetId(),
                     tx.GetHash().ToString(),
                     m_mempool.size(), m_mempool.DynamicMemoryUsage() / 1000);

            // Recursively process any orphan transactions that depended on this one
            ProcessOrphanTx(peer->m_orphan_work_set);
        }
        else if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS)
        {
            bool fRejectedParents = false; // It may be the case that the orphans parents have all been rejected
            for (const CTxIn& txin : tx.vin) {
                if (m_recent_rejects.contains(txin.prevout.hash)) {
                    fRejectedParents = true;
                    break;
                }
            }
            if (!fRejectedParents) {
                const auto current_time = GetTime<std::chrono::microseconds>();

                for (const CTxIn& txin : tx.vin) {
                    CInv _inv(MSG_TX, txin.prevout.hash);
                    pfrom.AddInventoryKnown(_inv);
                    if (!AlreadyHave(_inv)) RequestObject(State(pfrom.GetId()), _inv, current_time);
                    // We don't know if the previous tx was a regular or a mixing one, try both
                    CInv _inv2(MSG_DSTX, txin.prevout.hash);
                    pfrom.AddInventoryKnown(_inv2);
                    if (!AlreadyHave(_inv2)) RequestObject(State(pfrom.GetId()), _inv2, current_time);
                }
                AddOrphanTx(ptx, pfrom.GetId());

                // DoS prevention: do not allow mapOrphanTransactions to grow unbounded (see CVE-2012-3789)
                unsigned int nMaxOrphanTxSize = (unsigned int)std::max((int64_t)0, gArgs.GetArg("-maxorphantxsize", DEFAULT_MAX_ORPHAN_TRANSACTIONS_SIZE)) * 1000000;
                unsigned int nEvicted = LimitOrphanTxSize(nMaxOrphanTxSize);
                if (nEvicted > 0) {
                    LogPrint(BCLog::MEMPOOL, "mapOrphan overflow, removed %u tx\n", nEvicted);
                }
            } else {
                LogPrint(BCLog::MEMPOOL, "not keeping orphan with rejected parents %s\n",tx.GetHash().ToString());
                // We will continue to reject this tx since it has rejected
                // parents so avoid re-requesting it from other peers.
                m_recent_rejects.insert(tx.GetHash());
                m_llmq_ctx->isman->TransactionRemovedFromMempool(ptx);
            }
        } else {
            m_recent_rejects.insert(tx.GetHash());
            if (RecursiveDynamicUsage(*ptx) < 100000) {
                AddToCompactExtraTransactions(ptx);
            }

            if (pfrom.HasPermission(PF_FORCERELAY)) {
                // Always relay transactions received from peers with forcerelay permission, even
                // if they were already in the mempool,
                // allowing the node to function as a gateway for
                // nodes hidden behind it.
                if (!m_mempool.exists(tx.GetHash())) {
                    LogPrintf("Not relaying non-mempool transaction %s from forcerelay peer=%d\n", tx.GetHash().ToString(), pfrom.GetId());
                } else {
                    LogPrintf("Force relaying tx %s from peer=%d\n", tx.GetHash().ToString(), pfrom.GetId());
                    RelayTransaction(tx.GetHash());
                }
            }
        }

        // If a tx has been detected by m_recent_rejects, we will have reached
        // this point and the tx will have been ignored. Because we haven't run
        // the tx through AcceptToMemoryPool, we won't have computed a DoS
        // score for it or determined exactly why we consider it invalid.
        //
        // This means we won't penalize any peer subsequently relaying a DoSy
        // tx (even if we penalized the first peer who gave it to us) because
        // we have to account for m_recent_rejects showing false positives. In
        // other words, we shouldn't penalize a peer if we aren't *sure* they
        // submitted a DoSy tx.
        //
        // Note that m_recent_rejects doesn't just record DoSy or invalid
        // transactions, but any tx not accepted by the m_mempool, which may be
        // due to node policy (vs. consensus). So we can't blanket penalize a
        // peer simply for relaying a tx that our m_recent_rejects has caught,
        // regardless of false positives.

        if (state.IsInvalid())
        {
            LogPrint(BCLog::MEMPOOLREJ, "%s from peer=%d was not accepted: %s\n", tx.GetHash().ToString(),
                pfrom.GetId(),
                state.ToString());
            MaybePunishNodeForTx(pfrom.GetId(), state);
            m_llmq_ctx->isman->TransactionRemovedFromMempool(ptx);
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
                m_connman.PushMessage(&pfrom, msgMaker.Make((pfrom.nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::GETHEADERS2 : NetMsgType::GETHEADERS, m_chainman.ActiveChain().GetLocator(pindexBestHeader), uint256()));
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
                vInv[0] = CInv(MSG_BLOCK, cmpctblock.header.GetHash());
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
            }
            return;
        }

        // If we're not close to tip yet, give up and let parallel block fetch work its magic
        if (!fAlreadyInFlight && !CanDirectFetch(m_chainparams.GetConsensus()))
            return;

        // We want to be a bit conservative just to be extra careful about DoS
        // possibilities in compact block processing...
        if (pindex->nHeight <= m_chainman.ActiveChain().Height() + 2) {
            if ((!fAlreadyInFlight && nodestate->nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) ||
                 (fAlreadyInFlight && blockInFlightIt->second.first == pfrom.GetId())) {
                std::list<QueuedBlock>::iterator *queuedBlockIt = nullptr;
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
                    vInv[0] = CInv(MSG_BLOCK, cmpctblock.header.GetHash());
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
                vInv[0] = CInv(MSG_BLOCK, cmpctblock.header.GetHash());
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                return;
            } else {
                // If this was an announce-cmpctblock, we want the same treatment as a header message
                fRevertToHeaderProcessing = true;
            }
        }
        } // cs_main

        if (fProcessBLOCKTXN)
            return ProcessMessage(pfrom, NetMsgType::BLOCKTXN, blockTxnMsg, nTimeReceived, interruptMsgProc);

        if (fRevertToHeaderProcessing) {
            // Headers received from HB compact block peers are permitted to be
            // relayed before full validation (see BIP 152), so we don't want to disconnect
            // the peer if the header turns out to be for an invalid block.
            // Note that if a peer tries to build on an invalid chain, that
            // will be detected and the peer will be disconnected/discouraged.
            return ProcessHeadersMessage(pfrom, {cmpctblock.header}, /*punish_duplicate_invalid=*/true);
        }

        if (fBlockReconstructed) {
            // If we got here, we were able to optimistically reconstruct a
            // block that is in flight from some other peer.
            {
                LOCK(cs_main);
                mapBlockSource.emplace(pblock->GetHash(), std::make_pair(pfrom.GetId(), false));
            }
            // Setting fForceProcessing to true means that we bypass some of
            // our anti-DoS protections in AcceptBlock, which filters
            // unrequested blocks that might be trying to waste our resources
            // (eg disk space). Because we only try to reconstruct blocks when
            // we're close to caught up (via the CanDirectFetch() requirement
            // above, combined with the behavior of not requesting blocks until
            // we have a chain with at least nMinimumChainWork), and we ignore
            // compact blocks with less work than our tip, it is safe to treat
            // reconstructed compact blocks as having been requested.
            ProcessBlock(pfrom, pblock, /*fForceProcessing=*/true);
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
                invs.push_back(CInv(MSG_BLOCK, resp.blockhash));
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
            // Since we requested this block (it was in mapBlocksInFlight), force it to be processed,
            // even if it would not be a candidate for new tip (missing previous block, chain not long enough, etc)
            // This bypasses some anti-DoS logic in AcceptBlock (eg to prevent
            // disk-space attacks), but this should be safe due to the
            // protections in the compact block handler -- see related comment
            // in compact block optimistic reconstruction handling.
            ProcessBlock(pfrom, pblock, /*fForceProcessing=*/true);
        }
        return;
    }

    if (msg_type == NetMsgType::HEADERS || msg_type == NetMsgType::HEADERS2) {
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

        if (msg_type == NetMsgType::HEADERS) {
            headers.resize(nCount);
            for (unsigned int n = 0; n < nCount; n++) {
                vRecv >> headers[n];
                ReadCompactSize(vRecv); // ignore tx count; assume it is 0.
            }
        } else if (msg_type == NetMsgType::HEADERS2) {
            std::list<int32_t> last_unique_versions;
            for (unsigned int n = 0; n < nCount; n++) {
                CompressibleBlockHeader block_header_compressed;
                vRecv >> block_header_compressed;
                block_header_compressed.Uncompress(headers, last_unique_versions);
                headers.push_back(block_header_compressed);
            }
        }

        return ProcessHeadersMessage(pfrom, headers, /*via_compact_block=*/false);
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
        ProcessBlock(pfrom, pblock, forceProcessing);
        return;
    }

    if (msg_type == NetMsgType::GETADDR) {
        // This asymmetric behavior for inbound and outbound connections was introduced
        // to prevent a fingerprinting attack: an attacker can send specific fake addresses
        // to users' AddrMan and later request them by sending getaddr messages.
        // Making nodes which are behind NAT and can only make outgoing connections ignore
        // the getaddr message mitigates the attack.
        if (!pfrom.IsInboundConn()) {
            LogPrint(BCLog::NET, "Ignoring \"getaddr\" from outbound connection. peer=%d\n", pfrom.GetId());
            return;
        }
        if (!pfrom.IsAddrRelayPeer()) {
            LogPrint(BCLog::NET, "Ignoring \"getaddr\" from block-relay-only connection. peer=%d\n", pfrom.GetId());
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
            vAddr = m_connman.GetAddresses(MAX_ADDR_TO_SEND, MAX_PCT_ADDR_TO_SEND, /* network */ std::nullopt);
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

        if (pfrom.IsAddrRelayPeer()) {
            LOCK(pfrom.m_tx_relay->cs_tx_inventory);
            pfrom.m_tx_relay->fSendMempool = true;
        }
        return;
    }

    if (msg_type == NetMsgType::PING) {
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
        return;
    }

    if (msg_type == NetMsgType::PONG) {
        int64_t pingUsecEnd = nTimeReceived;
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;

        if (nAvail >= sizeof(nonce)) {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (pfrom.nPingNonceSent != 0) {
                if (nonce == pfrom.nPingNonceSent) {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished = true;
                    int64_t pingUsecTime = pingUsecEnd - pfrom.nPingUsecStart;
                    if (pingUsecTime > 0) {
                        // Successful ping time measurement, replace previous
                        pfrom.nPingUsecTime = pingUsecTime;
                        pfrom.nMinPingUsecTime = std::min(pfrom.nMinPingUsecTime.load(), pingUsecTime);
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
                pfrom.nPingNonceSent,
                nonce,
                nAvail);
        }
        if (bPingFinished) {
            pfrom.nPingNonceSent = 0;
        }
        return;
    }

    if (msg_type == NetMsgType::FILTERLOAD) {
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints())
        {
            // There is no excuse for sending a too-large filter
            Misbehaving(pfrom.GetId(), 100, "too-large bloom filter");
        }
        else if (pfrom.IsAddrRelayPeer())
        {
            LOCK(pfrom.m_tx_relay->cs_filter);
            pfrom.m_tx_relay->pfilter.reset(new CBloomFilter(filter));
            pfrom.m_tx_relay->fRelayTxes = true;
        }
        return;
    }

    if (msg_type == NetMsgType::FILTERADD) {
        std::vector<unsigned char> vData;
        vRecv >> vData;

        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        bool bad = false;
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            bad = true;
        } else if (pfrom.IsAddrRelayPeer()) {
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
        if (!pfrom.IsAddrRelayPeer()) {
            return;
        }
        LOCK(pfrom.m_tx_relay->cs_filter);
        if (pfrom.GetLocalServices() & NODE_BLOOM) {
            pfrom.m_tx_relay->pfilter = nullptr;
        }
        pfrom.m_tx_relay->fRelayTxes = true;
        return;
    }

    if (msg_type == NetMsgType::GETMNLISTDIFF) {
        CGetSimplifiedMNListDiff cmd;
        vRecv >> cmd;

        LOCK(cs_main);

        CSimplifiedMNListDiff mnListDiff;
        std::string strError;
        if (BuildSimplifiedMNListDiff(cmd.baseBlockHash, cmd.blockHash, mnListDiff, *m_llmq_ctx->quorum_block_processor, strError)) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::MNLISTDIFF, mnListDiff));
        } else {
            strError = strprintf("getmnlistdiff failed for baseBlockHash=%s, blockHash=%s. error=%s", cmd.baseBlockHash.ToString(), cmd.blockHash.ToString(), strError);
            Misbehaving(pfrom.GetId(), 1, strError);
        }
        return;
    }

    if (msg_type == NetMsgType::GETCFILTERS) {
        ProcessGetCFilters(pfrom, vRecv, m_chainparams, m_connman);
        return;
    }

    if (msg_type == NetMsgType::GETCFHEADERS) {
        ProcessGetCFHeaders(pfrom, vRecv, m_chainparams, m_connman);
        return;
    }

    if (msg_type == NetMsgType::GETCFCHECKPT) {
        ProcessGetCFCheckPt(pfrom, vRecv, m_chainparams, m_connman);
        return;
    }


    if (msg_type == NetMsgType::MNLISTDIFF) {
        // we have never requested this
        Misbehaving(pfrom.GetId(), 100, strprintf("received not-requested mnlistdiff. peer=%d", pfrom.GetId()));
        return;
    }

    if (msg_type == NetMsgType::GETQUORUMROTATIONINFO) {
        llmq::CGetQuorumRotationInfo cmd;
        vRecv >> cmd;

        LOCK(cs_main);

        llmq::CQuorumRotationInfo quorumRotationInfoRet;
        std::string strError;
        if (BuildQuorumRotationInfo(cmd, quorumRotationInfoRet, *m_llmq_ctx->qman, *m_llmq_ctx->quorum_block_processor, strError)) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QUORUMROTATIONINFO, quorumRotationInfoRet));
        } else {
            strError = strprintf("getquorumrotationinfo failed for size(baseBlockHashes)=%d, blockRequestHash=%s. error=%s", cmd.baseBlockHashes.size(), cmd.blockRequestHash.ToString(), strError);
            Misbehaving(pfrom.GetId(), 1, strError);
        }
        return;
    }

    if (msg_type == NetMsgType::QUORUMROTATIONINFO) {
        // we have never requested this
        Misbehaving(pfrom.GetId(), 100, strprintf("received not-requested quorumrotationinfo. peer=%d", pfrom.GetId()));
        return;
    }

    if (msg_type == NetMsgType::NOTFOUND) {
        // Remove the NOTFOUND transactions from the peer
        LOCK(cs_main);
        CNodeState *state = State(pfrom.GetId());
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() <= MAX_PEER_OBJECT_IN_FLIGHT + MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            for (CInv &inv : vInv) {
                if (inv.IsKnownType()) {
                    // If we receive a NOTFOUND message for a txid we requested, erase
                    // it from our data structures for this peer.
                    auto in_flight_it = state->m_object_download.m_object_in_flight.find(inv);
                    if (in_flight_it == state->m_object_download.m_object_in_flight.end()) {
                        // Skip any further work if this is a spurious NOTFOUND
                        // message.
                        continue;
                    }
                    state->m_object_download.m_object_in_flight.erase(in_flight_it);
                    state->m_object_download.m_object_announced.erase(inv);
                }
            }
        }
        return;
    }

    bool found = false;
    const std::vector<std::string> &allMessages = getAllNetMessageTypes();
    for (const std::string& msg : allMessages) {
        if(msg == msg_type) {
            found = true;
            break;
        }
    }

    if (found)
    {
        //probably one the extensions
#ifdef ENABLE_WALLET
        m_cj_ctx->queueman->ProcessMessage(pfrom, *this, msg_type, vRecv);
        for (auto& pair : m_cj_ctx->clientman->raw()) {
            pair.second->ProcessMessage(pfrom, *this, m_connman, m_mempool, msg_type, vRecv);
        }
#endif // ENABLE_WALLET
        m_cj_ctx->server->ProcessMessage(pfrom, *this, msg_type, vRecv);
        sporkManager->ProcessMessage(pfrom, *this, m_connman, msg_type, vRecv);
        ::masternodeSync->ProcessMessage(pfrom, msg_type, vRecv);
        m_govman.ProcessMessage(pfrom, *this, m_connman, msg_type, vRecv);
        CMNAuth::ProcessMessage(pfrom, *this, m_connman, msg_type, vRecv);
        m_llmq_ctx->quorum_block_processor->ProcessMessage(pfrom, msg_type, vRecv);
        m_llmq_ctx->qdkgsman->ProcessMessage(pfrom, *m_llmq_ctx->qman, msg_type, vRecv);
        m_llmq_ctx->qman->ProcessMessage(pfrom, msg_type, vRecv);
        m_llmq_ctx->shareman->ProcessMessage(pfrom, *sporkManager, msg_type, vRecv);
        m_llmq_ctx->sigman->ProcessMessage(pfrom, msg_type, vRecv);
        m_llmq_ctx->clhandler->ProcessMessage(pfrom, msg_type, vRecv);
        m_llmq_ctx->isman->ProcessMessage(pfrom, msg_type, vRecv);
        return;
    }

    // Ignore unknown commands for extensibility
    LogPrint(BCLog::NET, "Unknown command \"%s\" from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());

    return;
}

bool PeerManagerImpl::MaybeDiscourageAndDisconnect(CNode& pnode)
{
    const NodeId peer_id{pnode.GetId()};
    PeerRef peer = GetPeerRef(peer_id);
    if (peer == nullptr) return false;

    {
        LOCK(peer->m_misbehavior_mutex);

        // There's nothing to do if the m_should_discourage flag isn't set
        if (!peer->m_should_discourage) return false;

        peer->m_should_discourage = false;
    } // peer.m_misbehavior_mutex

    if (pnode.HasPermission(PF_NOBAN)) {
        // We never disconnect or discourage peers for bad behavior if they have the NOBAN permission flag
        LogPrintf("Warning: not punishing noban peer %d!\n", peer_id);
        return false;
    }

    if (pnode.IsManualConn()) {
        // We never disconnect or discourage manual peers for bad behavior
        LogPrintf("Warning: not punishing manually connected peer %d!\n", peer_id);
        return false;
    }

    if (pnode.addr.IsLocal()) {
        // We disconnect local peers for bad behavior but don't discourage (since that would discourage
        // all peers on the same local address)
        LogPrintf("Warning: disconnecting but not discouraging local peer %d!\n", peer_id);
        pnode.fDisconnect = true;
        return true;
    }

    // Disconnect and discourage all nodes sharing the address
    LogPrint(BCLog::NET, "Disconnecting and discouraging peer %d!\n", peer_id);
    if (m_banman) {
        m_banman->Discourage(pnode.addr);
    }
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
    if (pfrom->fPauseSend)
        return false;

    std::list<CNetMessage> msgs;
    {
        LOCK(pfrom->cs_vProcessMsg);
        if (pfrom->vProcessMsg.empty())
            return false;
        // Just take one message
        msgs.splice(msgs.begin(), pfrom->vProcessMsg, pfrom->vProcessMsg.begin());
        pfrom->nProcessQueueSize -= msgs.front().m_raw_message_size;
        pfrom->fPauseRecv = pfrom->nProcessQueueSize > m_connman.GetReceiveFloodSize();
        fMoreWork = !pfrom->vProcessMsg.empty();
    }
    CNetMessage& msg(msgs.front());

    msg.SetVersion(pfrom->GetRecvVersion());
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
    const CNetMsgMaker msgMaker(pto.GetSendVersion());

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
                std::string msg_type = (pto.nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::GETHEADERS2 : NetMsgType::GETHEADERS;
                LogPrint(BCLog::NET, "sending %s to outbound peer=%d to verify chain work (current best known block:%s, benchmark blockhash: %s)\n",
                        msg_type,
                        pto.GetId(),
                        state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>",
                        state.m_chain_sync.m_work_header->GetBlockHash().ToString());
                m_connman.PushMessage(&pto, msgMaker.Make(msg_type, m_chainman.ActiveChain().GetLocator(state.m_chain_sync.m_work_header->pprev), uint256()));
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
    // Check whether we have too many outbound peers
    int extra_peers = m_connman.GetExtraOutboundCount();
    if (extra_peers > 0) {
        // If we have more outbound peers than we target, disconnect one.
        // Pick the outbound peer that least recently announced
        // us a new block, with ties broken by choosing the more recent
        // connection (higher node id)
        NodeId worst_peer = -1;
        int64_t oldest_block_announcement = std::numeric_limits<int64_t>::max();

        m_connman.ForEachNode([&](CNode* pnode) {
            AssertLockHeld(cs_main);

            // Don't disconnect masternodes just because they were slow in block announcement
            if (pnode->m_masternode_connection) return;
            // Ignore non-outbound peers, or nodes marked for disconnect already
            if (!pnode->IsOutboundOrBlockRelayConn() || pnode->fDisconnect) return;
            CNodeState *state = State(pnode->GetId());
            if (state == nullptr) return; // shouldn't be possible, but just in case
            // Don't evict our protected peers
            if (state->m_chain_sync.m_protect) return;
            // Don't evict our block-relay-only peers.
            if (!pnode->IsAddrRelayPeer()) return;
            if (state->m_last_block_announcement < oldest_block_announcement || (state->m_last_block_announcement == oldest_block_announcement && pnode->GetId() > worst_peer)) {
                worst_peer = pnode->GetId();
                oldest_block_announcement = state->m_last_block_announcement;
            }
        });
        if (worst_peer != -1) {
            bool disconnected = m_connman.ForNode(worst_peer, [&](CNode *pnode) {
                AssertLockHeld(cs_main);

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

void PeerManagerImpl::CheckForStaleTipAndEvictPeers(const Consensus::Params &consensusParams)
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
}

namespace {
class CompareInvMempoolOrder
{
    CTxMemPool *mp;
public:
    explicit CompareInvMempoolOrder(CTxMemPool *_mempool)
    {
        mp = _mempool;
    }

    bool operator()(std::set<uint256>::iterator a, std::set<uint256>::iterator b)
    {
        /* As std::make_heap produces a max-heap, we want the entries with the
         * fewest ancestors/highest fee to sort later. */
        return mp->CompareDepthAndScore(*b, *a);
    }
};
}

bool PeerManagerImpl::SendMessages(CNode* pto)
{
    assert(m_llmq_ctx);

    const Consensus::Params& consensusParams = Params().GetConsensus();

    // We must call MaybeDiscourageAndDisconnect first, to ensure that we'll
    // disconnect misbehaving peers even before the version handshake is complete.
    if (MaybeDiscourageAndDisconnect(*pto)) return true;

    // Don't send anything until the version handshake is complete
    if (!pto->fSuccessfullyConnected || pto->fDisconnect)
        return true;

    // If we get here, the outgoing message serialization version is set and can't change.
    const CNetMsgMaker msgMaker(pto->GetSendVersion());

    //
    // Message: ping
    //
    bool pingSend = false;
    if (pto->fPingQueued) {
        // RPC ping request by user
        pingSend = true;
    }
    if (pto->nPingNonceSent == 0 && pto->nPingUsecStart + PING_INTERVAL * 1000000 < GetTimeMicros()) {
        // Ping automatically sent as a latency probe & keepalive.
        pingSend = true;
    }
    if (pingSend) {
        uint64_t nonce = 0;
        while (nonce == 0) {
            GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
        }
        pto->fPingQueued = false;
        pto->nPingUsecStart = GetTimeMicros();
        pto->nPingNonceSent = nonce;
        m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::PING, nonce));
    }

    {
        LOCK(cs_main);

        CNodeState &state = *State(pto->GetId());

        // Address refresh broadcast
        int64_t nNow = GetTimeMicros();
        auto current_time = GetTime<std::chrono::microseconds>();

        if (fListen && pto->IsAddrRelayPeer() &&
            !m_chainman.ActiveChainstate().IsInitialBlockDownload() &&
            pto->m_next_local_addr_send < current_time) {
            if (std::optional<CAddress> local_addr = GetLocalAddrForPeer(pto)) {
                FastRandomContext insecure_rand;
                pto->PushAddress(*local_addr, insecure_rand);
            }
            pto->m_next_local_addr_send = PoissonNextSend(current_time, AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL);
        }

        //
        // Message: addr
        //
        if (pto->IsAddrRelayPeer() && pto->m_next_addr_send < current_time) {
            pto->m_next_addr_send = PoissonNextSend(current_time, AVG_ADDRESS_BROADCAST_INTERVAL);
            std::vector<CAddress> vAddr;
            vAddr.reserve(pto->vAddrToSend.size());

            const char* msg_type;
            int make_flags;
            if (pto->m_wants_addrv2) {
                msg_type = NetMsgType::ADDRV2;
                make_flags = ADDRV2_FORMAT;
            } else {
                msg_type = NetMsgType::ADDR;
                make_flags = 0;
            }

            assert(pto->m_addr_known);
            for (const CAddress& addr : pto->vAddrToSend)
            {
                if (!pto->m_addr_known->contains(addr.GetKey()))
                {
                    pto->m_addr_known->insert(addr.GetKey());
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than MAX_ADDR_TO_SEND
                    if (vAddr.size() >= MAX_ADDR_TO_SEND)
                    {
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
        if (!state.fSyncStarted && !pto->fClient && !fImporting && !fReindex && pto->CanRelay()) {
            // Only actively request headers from a single peer, unless we're close to end of initial download.
            if ((nSyncStarted == 0 && fFetch) || pindexBestHeader->GetBlockTime() > GetAdjustedTime() - nMaxTipAge) {
                state.fSyncStarted = true;
                state.nHeadersSyncTimeout = GetTimeMicros() + HEADERS_DOWNLOAD_TIMEOUT_BASE + HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER * (GetAdjustedTime() - pindexBestHeader->GetBlockTime())/(consensusParams.nPowTargetSpacing);
                nSyncStarted++;
                const CBlockIndex *pindexStart = pindexBestHeader;
                /* If possible, start at the block preceding the currently
                   best known header.  This ensures that we always get a
                   non-empty list of headers back as long as the peer
                   is up-to-date.  With a non-empty response, we can initialise
                   the peer's known best block.  This wouldn't be possible
                   if we requested starting at pindexBestHeader and
                   got back an empty response.  */
                if (pindexStart->pprev)
                    pindexStart = pindexStart->pprev;
                std::string msg_type = (pto->nServices & NODE_HEADERS_COMPRESSED) ? NetMsgType::GETHEADERS2 : NetMsgType::GETHEADERS;
                LogPrint(BCLog::NET, "initial %s (%d) to peer=%d (startheight:%d)\n", msg_type, pindexStart->nHeight, pto->GetId(), pto->nStartingHeight);
                m_connman.PushMessage(pto, msgMaker.Make(msg_type, m_chainman.ActiveChain().GetLocator(pindexStart), uint256()));
            }
        }

        //
        // Try sending block announcements via headers
        //
        if (pto->CanRelay()) {
            // If we have less than MAX_BLOCKS_TO_ANNOUNCE in our
            // list of block hashes we're relaying, and our peer wants
            // headers announcements, then find the first header
            // not yet known to our peer but would connect, and send.
            // If no header would connect, or if we have too many
            // blocks, or if the peer doesn't want headers, just
            // add all to the inv queue.
            LOCK(pto->cs_inventory);
            std::vector<CBlock> vHeaders;
            bool fRevertToInv = ((!state.fPreferHeaders && !state.fPreferHeadersCompressed &&
                                 (!state.fPreferHeaderAndIDs || pto->vBlockHashesToAnnounce.size() > 1)) ||
                                 pto->vBlockHashesToAnnounce.size() > MAX_BLOCKS_TO_ANNOUNCE);
            const CBlockIndex *pBestIndex = nullptr; // last header queued for delivery
            ProcessBlockAvailability(pto->GetId()); // ensure pindexBestKnownBlock is up-to-date

            if (!fRevertToInv) {
                bool fFoundStartingHeader = false;
                // Try to find first header that our peer doesn't have, and
                // then send all headers past that one.  If we come across any
                // headers that aren't on m_chainman.ActiveChain(), give up.
                for (const uint256 &hash : pto->vBlockHashesToAnnounce) {
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
                        // be added multiple times to vBlockHashesToAnnounce.
                        // Robustly deal with this rare situation by reverting
                        // to an inv.
                        fRevertToInv = true;
                        break;
                    }
                    pBestIndex = pindex;
                    bool isPrevDevnetGenesisBlock = false;
                    if (!consensusParams.hashDevnetGenesisBlock.IsNull() &&
                        pindex->pprev != nullptr &&
                        pindex->pprev->GetBlockHash() == consensusParams.hashDevnetGenesisBlock) {
                        // even though the devnet genesis block was never transferred through the wire and thus not
                        // appear anywhere in the node state where we track what other nodes have or not have, we can
                        // assume that the other node already knows the devnet genesis block
                        isPrevDevnetGenesisBlock = true;
                    }
                    if (fFoundStartingHeader) {
                        // add this to the headers message
                        vHeaders.push_back(pindex->GetBlockHeader());
                    } else if (PeerHasHeader(&state, pindex)) {
                        continue; // keep looking for the first new block
                    } else if (pindex->pprev == nullptr || PeerHasHeader(&state, pindex->pprev) || isPrevDevnetGenesisBlock) {
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

                    bool fGotBlockFromCache = false;
                    {
                        LOCK(cs_most_recent_block);
                        if (most_recent_block_hash == pBestIndex->GetBlockHash()) {
                            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::CMPCTBLOCK, *most_recent_compact_block));
                            fGotBlockFromCache = true;
                        }
                    }
                    if (!fGotBlockFromCache) {
                        CBlock block;
                        bool ret = ReadBlockFromDisk(block, pBestIndex, consensusParams);
                        assert(ret);
                        CBlockHeaderAndShortTxIDs cmpctblock(block);
                        m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::CMPCTBLOCK, cmpctblock));
                    }
                    state.pindexBestHeaderSent = pBestIndex;
                } else if (state.fPreferHeadersCompressed) {
                    std::vector<CompressibleBlockHeader> vHeadersCompressed;
                    std::list<int32_t> last_unique_versions;

                    // Save other headers compressed
                    std::for_each(vHeaders.cbegin(), vHeaders.cend(), [&vHeadersCompressed, &last_unique_versions](const auto& block) {
                        CompressibleBlockHeader compressible_header{block.GetBlockHeader()};
                        compressible_header.Compress(vHeadersCompressed, last_unique_versions);
                        vHeadersCompressed.push_back(compressible_header);
                    });

                    // Push message to peer
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::HEADERS2, vHeadersCompressed));
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
                // The last entry in vBlockHashesToAnnounce was our tip at some point
                // in the past.
                if (!pto->vBlockHashesToAnnounce.empty()) {
                    const uint256 &hashToAnnounce = pto->vBlockHashesToAnnounce.back();
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
                        pto->PushInventory(CInv(MSG_BLOCK, hashToAnnounce));
                        LogPrint(BCLog::NET, "%s: sending inv peer=%d hash=%s\n", __func__,
                            pto->GetId(), hashToAnnounce.ToString());
                    }
                }
            }
            pto->vBlockHashesToAnnounce.clear();
        }

        //
        // Message: inventory
        //
        std::vector<CInv> vInv;
        {
            LOCK2(m_mempool.cs, pto->cs_inventory);

            size_t reserve = INVENTORY_BROADCAST_MAX_PER_1MB_BLOCK * MaxBlockSize() / 1000000;
            if (pto->IsAddrRelayPeer()) {
                LOCK(pto->m_tx_relay->cs_tx_inventory);
                reserve = std::min<size_t>(pto->m_tx_relay->setInventoryTxToSend.size(), reserve);
            }
            reserve = std::max<size_t>(reserve, pto->vInventoryBlockToSend.size());
            reserve = std::min<size_t>(reserve, MAX_INV_SZ);
            vInv.reserve(reserve);

            // Add blocks
            for (const uint256& hash : pto->vInventoryBlockToSend) {
                vInv.push_back(CInv(MSG_BLOCK, hash));
                if (vInv.size() == MAX_INV_SZ) {
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                    vInv.clear();
                }
            }
            pto->vInventoryBlockToSend.clear();

            auto queueAndMaybePushInv = [this, pto, &vInv, &msgMaker](const CInv& invIn) {
                AssertLockHeld(pto->m_tx_relay->cs_tx_inventory);
                pto->m_tx_relay->filterInventoryKnown.insert(invIn.hash);
                LogPrint(BCLog::NET, "SendMessages -- queued inv: %s  index=%d peer=%d\n", invIn.ToString(), vInv.size(), pto->GetId());
                vInv.push_back(invIn);
                if (vInv.size() == MAX_INV_SZ) {
                    LogPrint(BCLog::NET, "SendMessages -- pushing invs: count=%d peer=%d\n", vInv.size(), pto->GetId());
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                    vInv.clear();
                }
            };

            if (pto->IsAddrRelayPeer()) {
                LOCK(pto->m_tx_relay->cs_tx_inventory);
                // Check whether periodic sends should happen
                // Note: If this node is running in a Masternode mode, it makes no sense to delay outgoing txes
                // because we never produce any txes ourselves i.e. no privacy is lost in this case.
                bool fSendTrickle = pto->HasPermission(PF_NOBAN) || fMasternodeMode;
                if (pto->m_tx_relay->nNextInvSend < current_time) {
                    fSendTrickle = true;
                    if (pto->IsInboundConn()) {
                        pto->m_tx_relay->nNextInvSend = std::chrono::microseconds{m_connman.PoissonNextSendInbound(current_time.count(), INVENTORY_BROADCAST_INTERVAL)};
                    } else {
                        // Use half the delay for regular outbound peers, as there is less privacy concern for them.
                        // and quarter the delay for Masternode outbound peers, as there is even less privacy concern in this case.
                        pto->m_tx_relay->nNextInvSend = PoissonNextSend(current_time, std::chrono::seconds{INVENTORY_BROADCAST_INTERVAL >> 1 >> !pto->GetVerifiedProRegTxHash().IsNull()});
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

                    LOCK(pto->m_tx_relay->cs_filter);

                    // Send invs for txes and corresponding IS-locks
                    for (const auto& txinfo : vtxinfo) {
                        const uint256& hash = txinfo.tx->GetHash();
                        pto->m_tx_relay->setInventoryTxToSend.erase(hash);
                        if (pto->m_tx_relay->pfilter && !pto->m_tx_relay->pfilter->IsRelevantAndUpdate(*txinfo.tx)) continue;

                        int nInvType = CCoinJoin::GetDSTX(hash) ? MSG_DSTX : MSG_TX;
                        queueAndMaybePushInv(CInv(nInvType, hash));

                        const auto islock = m_llmq_ctx->isman->GetInstantSendLockByTxid(hash);
                        if (islock == nullptr) continue;
                        if (pto->nVersion < ISDLOCK_PROTO_VERSION) continue;
                        queueAndMaybePushInv(CInv(MSG_ISDLOCK, ::SerializeHash(*islock)));
                    }

                    // Send an inv for the best ChainLock we have
                    const auto& clsig = m_llmq_ctx->clhandler->GetBestChainLock();
                    if (!clsig.IsNull()) {
                        uint256 chainlockHash = ::SerializeHash(clsig);
                        queueAndMaybePushInv(CInv(MSG_CLSIG, chainlockHash));
                    }

                    pto->m_tx_relay->m_last_mempool_req = GetTime<std::chrono::seconds>();
                }

                // Determine transactions to relay
                if (fSendTrickle) {
                    LOCK(pto->m_tx_relay->cs_filter);

                    // Produce a vector with all candidates for sending
                    std::vector<std::set<uint256>::iterator> vInvTx;
                    vInvTx.reserve(pto->m_tx_relay->setInventoryTxToSend.size());
                    for (std::set<uint256>::iterator it = pto->m_tx_relay->setInventoryTxToSend.begin(); it != pto->m_tx_relay->setInventoryTxToSend.end(); it++) {
                        vInvTx.push_back(it);
                    }
                    // Topologically and fee-rate sort the inventory we send for privacy and priority reasons.
                    // A heap is used so that not all items need sorting if only a few are being sent.
                    CompareInvMempoolOrder compareInvMempoolOrder(&m_mempool);
                    std::make_heap(vInvTx.begin(), vInvTx.end(), compareInvMempoolOrder);
                    // No reason to drain out at many times the network's capacity,
                    // especially since we have many peers and some will draw much shorter delays.
                    unsigned int nRelayedTransactions = 0;
                    while (!vInvTx.empty() && nRelayedTransactions < INVENTORY_BROADCAST_MAX_PER_1MB_BLOCK * MaxBlockSize() / 1000000) {
                        // Fetch the top element from the heap
                        std::pop_heap(vInvTx.begin(), vInvTx.end(), compareInvMempoolOrder);
                        std::set<uint256>::iterator it = vInvTx.back();
                        vInvTx.pop_back();
                        uint256 hash = *it;
                        // Remove it from the to-be-sent set
                        pto->m_tx_relay->setInventoryTxToSend.erase(it);
                        // Check if not in the filter already
                        if (pto->m_tx_relay->filterInventoryKnown.contains(hash)) {
                            continue;
                        }
                        // Not in the mempool anymore? don't bother sending it.
                        auto txinfo = m_mempool.info(hash);
                        if (!txinfo.tx) {
                            continue;
                        }
                        if (pto->m_tx_relay->pfilter && !pto->m_tx_relay->pfilter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        // Send
                        nRelayedTransactions++;
                        {
                            // Expire old relay messages
                            while (!vRelayExpiration.empty() && vRelayExpiration.front().first < nNow)
                            {
                                mapRelay.erase(vRelayExpiration.front().second);
                                vRelayExpiration.pop_front();
                            }

                            auto ret = mapRelay.insert(std::make_pair(hash, std::move(txinfo.tx)));
                            if (ret.second) {
                                vRelayExpiration.push_back(std::make_pair(nNow + std::chrono::microseconds{RELAY_TX_CACHE_TIME}.count(), ret.first));
                            }
                        }
                        int nInvType = CCoinJoin::GetDSTX(hash) ? MSG_DSTX : MSG_TX;
                        queueAndMaybePushInv(CInv(nInvType, hash));
                    }
                }
            }

            {
                // Send non-tx/non-block inventory items
                LOCK2(pto->m_tx_relay->cs_tx_inventory, pto->m_tx_relay->cs_filter);

                bool fSendIS = pto->m_tx_relay->fRelayTxes && !pto->IsBlockRelayOnly();

                for (const auto& inv : pto->m_tx_relay->vInventoryOtherToSend) {
                    if (!pto->m_tx_relay->fRelayTxes && NetMessageViolatesBlocksOnly(inv.GetCommand())) {
                        continue;
                    }
                    if (pto->m_tx_relay->filterInventoryKnown.contains(inv.hash)) {
                        continue;
                    }
                    if (!fSendIS && inv.type == MSG_ISDLOCK) {
                        continue;
                    }
                    queueAndMaybePushInv(inv);
                }
                pto->m_tx_relay->vInventoryOtherToSend.clear();
            }
        }
        if (!vInv.empty())
            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));

        // Detect whether we're stalling
        current_time = GetTime<std::chrono::microseconds>();
        // nNow is the current system time (GetTimeMicros is not mockable) and
        // should be replaced by the mockable current_time eventually
        nNow = GetTimeMicros();
        if (state.nStallingSince && state.nStallingSince < nNow - 1000000 * BLOCK_STALLING_TIMEOUT) {
            // Stalling only triggers when the block download window cannot move. During normal steady state,
            // the download window should be much larger than the to-be-downloaded set of blocks, so disconnection
            // should only happen during initial block download.
            LogPrintf("Peer=%d is stalling block download, disconnecting\n", pto->GetId());
            pto->fDisconnect = true;
            return true;
        }
        // In case there is a block that has been in flight from this peer for 2 + 0.5 * N times the block interval
        // (with N the number of peers from which we're downloading validated blocks), disconnect due to timeout.
        // We compensate for other peers to prevent killing off peers due to our own downstream link
        // being saturated. We only count validated in-flight blocks so peers can't advertise non-existing block hashes
        // to unreasonably increase our timeout.
        if (state.vBlocksInFlight.size() > 0) {
            QueuedBlock &queuedBlock = state.vBlocksInFlight.front();
            int nOtherPeersWithValidatedDownloads = nPeersWithValidatedDownloads - (state.nBlocksInFlightValidHeaders > 0);
            if (nNow > state.nDownloadingSince + consensusParams.nPowTargetSpacing * (BLOCK_DOWNLOAD_TIMEOUT_BASE + BLOCK_DOWNLOAD_TIMEOUT_PER_PEER * nOtherPeersWithValidatedDownloads)) {
                LogPrintf("Timeout downloading block %s from peer=%d, disconnecting\n", queuedBlock.hash.ToString(), pto->GetId());
                pto->fDisconnect = true;
                return true;
            }
        }
        // Check for headers sync timeouts
        if (state.fSyncStarted && state.nHeadersSyncTimeout < std::numeric_limits<int64_t>::max()) {
            // Detect whether this is a stalling initial-headers-sync peer
            if (pindexBestHeader->GetBlockTime() <= GetAdjustedTime() - nMaxTipAge) {
                if (nNow > state.nHeadersSyncTimeout && nSyncStarted == 1 && (nPreferredDownload - state.fPreferredDownload >= 1)) {
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
                        state.nHeadersSyncTimeout = 0;
                    }
                }
            } else {
                // After we've caught up once, reset the timeout so we can't trigger
                // disconnect later.
                state.nHeadersSyncTimeout = std::numeric_limits<int64_t>::max();
            }
        }

        // Check that outbound peers have reasonable chains
        // GetTime() is used by this anti-DoS logic so we can test this using mocktime
        ConsiderEviction(*pto, GetTime());

        //
        // Message: getdata (blocks)
        //
        std::vector<CInv> vGetData;
        if (!pto->fClient && pto->CanRelay() && ((fFetch && !pto->m_limited_node) || !m_chainman.ActiveChainstate().IsInitialBlockDownload()) && state.nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            std::vector<const CBlockIndex*> vToDownload;
            NodeId staller = -1;
            FindNextBlocksToDownload(pto->GetId(), MAX_BLOCKS_IN_TRANSIT_PER_PEER - state.nBlocksInFlight, vToDownload, staller);
            for (const CBlockIndex *pindex : vToDownload) {
                vGetData.push_back(CInv(MSG_BLOCK, pindex->GetBlockHash()));
                MarkBlockAsInFlight(pto->GetId(), pindex->GetBlockHash(), pindex);
                LogPrint(BCLog::NET, "Requesting block %s (%d) peer=%d\n", pindex->GetBlockHash().ToString(),
                    pindex->nHeight, pto->GetId());
            }
            if (state.nBlocksInFlight == 0 && staller != -1) {
                if (State(staller)->nStallingSince == 0) {
                    State(staller)->nStallingSince = nNow;
                    LogPrint(BCLog::NET, "Stall started peer=%d\n", staller);
                }
            }
        }

        //
        // Message: getdata (non-blocks)
        //

        // For robustness, expire old requests after a long timeout, so that
        // we can resume downloading objects from a peer even if they
        // were unresponsive in the past.
        // Eventually we should consider disconnecting peers, but this is
        // conservative.
        if (state.m_object_download.m_check_expiry_timer <= current_time) {
            for (auto it=state.m_object_download.m_object_in_flight.begin(); it != state.m_object_download.m_object_in_flight.end();) {
                if (it->second <= current_time - GetObjectExpiryInterval(it->first.type)) {
                    LogPrint(BCLog::NET, "timeout of inflight object %s from peer=%d\n", it->first.ToString(), pto->GetId());
                    state.m_object_download.m_object_announced.erase(it->first);
                    state.m_object_download.m_object_in_flight.erase(it++);
                } else {
                    ++it;
                }
            }
            // On average, we do this check every GetObjectExpiryInterval. Randomize
            // so that we're not doing this for all peers at the same time.
            state.m_object_download.m_check_expiry_timer = current_time + GetObjectExpiryInterval(MSG_TX)/2 + GetRandMicros(GetObjectExpiryInterval(MSG_TX));
        }

        // DASH this code also handles non-TXs (Dash specific messages)
        auto& object_process_time = state.m_object_download.m_object_process_time;
        while (!object_process_time.empty() && object_process_time.begin()->first <= current_time && state.m_object_download.m_object_in_flight.size() < MAX_PEER_OBJECT_IN_FLIGHT) {
            const CInv inv = object_process_time.begin()->second;
            // Erase this entry from object_process_time (it may be added back for
            // processing at a later time, see below)
            object_process_time.erase(object_process_time.begin());
            if (g_erased_object_requests.count(inv.hash)) {
                LogPrint(BCLog::NET, "%s -- GETDATA skipping inv=(%s), peer=%d\n", __func__, inv.ToString(), pto->GetId());
                state.m_object_download.m_object_announced.erase(inv);
                state.m_object_download.m_object_in_flight.erase(inv);
                continue;
            }
            if (!AlreadyHave(inv)) {
                // If this object was last requested more than GetObjectInterval ago,
                // then request.
                const auto last_request_time = GetObjectRequestTime(inv.hash);
                if (last_request_time <= current_time - GetObjectInterval(inv.type)) {
                    LogPrint(BCLog::NET, "Requesting %s peer=%d\n", inv.ToString(), pto->GetId());
                    vGetData.push_back(inv);
                    if (vGetData.size() >= MAX_GETDATA_SZ) {
                        m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));
                        vGetData.clear();
                    }
                    UpdateObjectRequestTime(inv.hash, current_time);
                    state.m_object_download.m_object_in_flight.emplace(inv, current_time);
                } else {
                    // This object is in flight from someone else; queue
                    // up processing to happen after the download times out
                    // (with a slight delay for inbound peers, to prefer
                    // requests to outbound peers).
                    const auto next_process_time = CalculateObjectGetDataTime(inv, current_time, !state.fPreferredDownload);
                    object_process_time.emplace(next_process_time, inv);
                    LogPrint(BCLog::NET, "%s -- GETDATA re-queue inv=(%s), next_process_time=%d, delta=%d, peer=%d\n", __func__, inv.ToString(), next_process_time.count(), (next_process_time - current_time).count(), pto->GetId());
                }
            } else {
                // We have already seen this object, no need to download.
                state.m_object_download.m_object_announced.erase(inv);
                state.m_object_download.m_object_in_flight.erase(inv);
                LogPrint(BCLog::NET, "%s -- GETDATA already seen inv=(%s), peer=%d\n", __func__, inv.ToString(), pto->GetId());
            }
        }


        if (!vGetData.empty()) {
            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));
            LogPrint(BCLog::NET, "SendMessages -- GETDATA -- pushed size = %lu peer=%d\n", vGetData.size(), pto->GetId());
        }
    } // release cs_main
    return true;
}

class CNetProcessingCleanup
{
public:
    CNetProcessingCleanup() {}
    ~CNetProcessingCleanup() {
        // orphan transactions
        mapOrphanTransactions.clear();
        mapOrphanTransactionsByPrev.clear();
        nMapOrphanTransactionsSize = 0;
    }
};
static CNetProcessingCleanup instance_of_cnetprocessingcleanup;
