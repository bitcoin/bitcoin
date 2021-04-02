// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_processing.h>

#include <addrman.h>
#include <banman.h>
#include <blockencodings.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <validation.h>
#include <merkleblock.h>
#include <netmessagemaker.h>
#include <netbase.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <reverse_iterator.h>
#include <scheduler.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/validation.h>
#include <vbk/p2p_sync.hpp>
#include <vbk/pop_service.hpp>

#include <memory>

#if defined(NDEBUG)
# error "Bitcoin cannot be compiled without assertions."
#endif

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
static constexpr int64_t STALE_CHECK_INTERVAL = 10 * 60; // 10 minutes
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
/** Maximum number of in-flight transactions from a peer */
static constexpr int32_t MAX_PEER_TX_IN_FLIGHT = 100;
/** Maximum number of announced transactions from a peer */
static constexpr int32_t MAX_PEER_TX_ANNOUNCEMENTS = 2 * MAX_INV_SZ;
/** How many microseconds to delay requesting transactions from inbound peers */
static constexpr std::chrono::microseconds INBOUND_PEER_TX_DELAY{std::chrono::seconds{2}};
/** How long to wait (in microseconds) before downloading a transaction from an additional peer */
static constexpr std::chrono::microseconds GETDATA_TX_INTERVAL{std::chrono::seconds{60}};
/** Maximum delay (in microseconds) for transaction requests to avoid biasing some peers over others. */
static constexpr std::chrono::microseconds MAX_GETDATA_RANDOM_DELAY{std::chrono::seconds{2}};
/** How long to wait (in microseconds) before expiring an in-flight getdata request to a peer */
static constexpr std::chrono::microseconds TX_EXPIRY_INTERVAL{GETDATA_TX_INTERVAL * 10};
static_assert(INBOUND_PEER_TX_DELAY >= MAX_GETDATA_RANDOM_DELAY,
"To preserve security, MAX_GETDATA_RANDOM_DELAY should not exceed INBOUND_PEER_DELAY");
/** Limit to avoid sending big packets. Not used in processing incoming GETDATA for compatibility */
static const unsigned int MAX_GETDATA_SZ = 1000;


struct COrphanTx {
    // When modifying, adapt the copy of this definition in tests/DoS_tests.
    CTransactionRef tx;
    NodeId fromPeer;
    int64_t nTimeExpire;
    size_t list_pos;
};
CCriticalSection g_cs_orphans;
std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(g_cs_orphans);

void EraseOrphansFor(NodeId peer);

/** Average delay between local address broadcasts in seconds. */
static constexpr unsigned int AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL = 24 * 60 * 60;
/** Average delay between peer address broadcasts in seconds. */
static const unsigned int AVG_ADDRESS_BROADCAST_INTERVAL = 30;
/** Average delay between trickled inventory transmissions in seconds.
 *  Blocks and whitelisted receivers bypass this, outbound peers get half this delay. */
static const unsigned int INVENTORY_BROADCAST_INTERVAL = 5;
/** Maximum number of inventory items to send per transmission.
 *  Limits the impact of low-fee transaction floods. */
static constexpr unsigned int INVENTORY_BROADCAST_MAX = 7 * INVENTORY_BROADCAST_INTERVAL;
/** Average delay between feefilter broadcasts in seconds. */
static constexpr unsigned int AVG_FEEFILTER_BROADCAST_INTERVAL = 10 * 60;
/** Maximum feefilter broadcast delay after significant change. */
static constexpr unsigned int MAX_FEEFILTER_CHANGE_DELAY = 5 * 60;

// Internal stuff
namespace {
    /** Number of nodes with fSyncStarted. */
    int nSyncStarted GUARDED_BY(cs_main) = 0;

    /**
     * Sources of received blocks, saved to be able punish them when processing
     * happens afterwards.
     * Set mapBlockSource[hash].second to false if the node should not be
     * punished if the block is invalid.
     */
    std::map<uint256, std::pair<NodeId, bool>> mapBlockSource GUARDED_BY(cs_main);

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
     * Memory used: 1.3 MB
     */
    std::unique_ptr<CRollingBloomFilter> recentRejects GUARDED_BY(cs_main);
    uint256 hashRecentRejectsChainTip GUARDED_BY(cs_main);

    /** Blocks that are in flight, and that are in the queue to be downloaded. */
    struct QueuedBlock {
        uint256 hash;
        const CBlockIndex* pindex;                               //!< Optional.
        bool fValidatedHeaders;                                  //!< Whether this block has validated headers at the time of request.
        std::unique_ptr<PartiallyDownloadedBlock> partialBlock;  //!< Optional, used for CMPCTBLOCK downloads
    };
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> > mapBlocksInFlight GUARDED_BY(cs_main);

    /** Stack of nodes which we have set to announce using compact blocks */
    std::list<NodeId> lNodesAnnouncingHeaderAndIDs GUARDED_BY(cs_main);

    /** Number of preferable block download peers. */
    int nPreferredDownload GUARDED_BY(cs_main) = 0;

    /** Number of peers from which we're downloading blocks. */
    int nPeersWithValidatedDownloads GUARDED_BY(cs_main) = 0;

    /** Number of outbound peers with m_chain_sync.m_protect. */
    int g_outbound_peers_with_protect_from_disconnect GUARDED_BY(cs_main) = 0;

    /** When our tip was last updated. */
    std::atomic<int64_t> g_last_tip_update(0);

    /** Relay map */
    typedef std::map<uint256, CTransactionRef> MapRelay;
    MapRelay mapRelay GUARDED_BY(cs_main);
    /** Expiration-time ordered list of (expire time, relay map entry) pairs. */
    std::deque<std::pair<int64_t, MapRelay::iterator>> vRelayExpiration GUARDED_BY(cs_main);

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
    //! Whether we have a fully established connection.
    bool fCurrentlyConnected;
    //! Accumulated misbehaviour score for this peer.
    int nMisbehavior;
    //! Whether this peer should be disconnected and banned (unless whitelisted).
    bool fShouldBan;
    //! String name of this peer (debugging/logging purposes).
    const std::string name;
    //! The best known block we know this peer has announced.
    const CBlockIndex *pindexBestKnownBlock = nullptr;
    //! The block this peer thinks is current tip.
    const CBlockIndex *pindexLastAnnouncedBlock = nullptr;
    //! The hash of the last unknown block this peer has announced.
    uint256 hashLastUnknownBlock;
    //! The last full block we both have.
    const CBlockIndex *pindexLastCommonBlock = nullptr;
    //! The last full block we both have from announced chain.
    const CBlockIndex *pindexLastCommonAnnouncedBlock = nullptr;
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
    //! Whether this peer wants invs or cmpctblocks (when possible) for block announcements.
    bool fPreferHeaderAndIDs;
    /**
      * Whether this peer will send us cmpctblocks if we request them.
      * This is not used to gate request logic, as we really only care about fSupportsDesiredCmpctVersion,
      * but is used as a flag to "lock in" the version of compact blocks (fWantsCmpctWitness) we send.
      */
    bool fProvidesHeaderAndIDs;
    //! Whether this peer can give us witnesses
    bool fHaveWitness;
    //! Whether this peer wants witnesses in cmpctblocks/blocktxns
    bool fWantsCmpctWitness;
    /**
     * If we've announced NODE_WITNESS to this peer: whether the peer sends witnesses in cmpctblocks/blocktxns,
     * otherwise: whether this peer sends non-witnesses in cmpctblocks/blocktxns.
     */
    bool fSupportsDesiredCmpctVersion;

    /** State used to enforce CHAIN_SYNC_TIMEOUT
      * Only in effect for outbound, non-manual, full-relay connections, with
      * m_protect == false
      * Algorithm: if a peer's best known block has less work than our tip,
      * set a timeout CHAIN_SYNC_TIMEOUT seconds in the future:
      *   - If at timeout their best known block now has more work than our tip
      *     when the timeout was set, then either reset the timeout or clear it
      *     (after comparing against our current tip's work)
      *   - If at timeout their best known block still has less work than our
      *     tip did when the timeout was set, then send a getheaders message,
      *     and set a shorter timeout, HEADERS_RESPONSE_TIME seconds in future.
      *     If their best known block is still behind when that new timeout is
      *     reached, disconnect.
      */
    struct ChainSyncTimeoutState {
        //! A timeout used for checking whether our peer has sufficiently synced
        int64_t m_timeout;
        //! A header with the work we require on our peer's chain
        const CBlockIndex * m_work_header;
        //! After timeout is reached, set to true after sending getheaders
        bool m_sent_getheaders;
        //! Whether this peer is protected from disconnection due to a bad/slow chain
        bool m_protect;
    };

    ChainSyncTimeoutState m_chain_sync;

    //! Time of last new block announcement
    int64_t m_last_block_announcement;

    /*
     * State associated with transaction download.
     *
     * Tx download algorithm:
     *
     *   When inv comes in, queue up (process_time, txid) inside the peer's
     *   CNodeState (m_tx_process_time) as long as m_tx_announced for the peer
     *   isn't too big (MAX_PEER_TX_ANNOUNCEMENTS).
     *
     *   The process_time for a transaction is set to nNow for outbound peers,
     *   nNow + 2 seconds for inbound peers. This is the time at which we'll
     *   consider trying to request the transaction from the peer in
     *   SendMessages(). The delay for inbound peers is to allow outbound peers
     *   a chance to announce before we request from inbound peers, to prevent
     *   an adversary from using inbound connections to blind us to a
     *   transaction (InvBlock).
     *
     *   When we call SendMessages() for a given peer,
     *   we will loop over the transactions in m_tx_process_time, looking
     *   at the transactions whose process_time <= nNow. We'll request each
     *   such transaction that we don't have already and that hasn't been
     *   requested from another peer recently, up until we hit the
     *   MAX_PEER_TX_IN_FLIGHT limit for the peer. Then we'll update
     *   g_already_asked_for for each requested txid, storing the time of the
     *   GETDATA request. We use g_already_asked_for to coordinate transaction
     *   requests amongst our peers.
     *
     *   For transactions that we still need but we have already recently
     *   requested from some other peer, we'll reinsert (process_time, txid)
     *   back into the peer's m_tx_process_time at the point in the future at
     *   which the most recent GETDATA request would time out (ie
     *   GETDATA_TX_INTERVAL + the request time stored in g_already_asked_for).
     *   We add an additional delay for inbound peers, again to prefer
     *   attempting download from outbound peers first.
     *   We also add an extra small random delay up to 2 seconds
     *   to avoid biasing some peers over others. (e.g., due to fixed ordering
     *   of peer processing in ThreadMessageHandler).
     *
     *   When we receive a transaction from a peer, we remove the txid from the
     *   peer's m_tx_in_flight set and from their recently announced set
     *   (m_tx_announced).  We also clear g_already_asked_for for that entry, so
     *   that if somehow the transaction is not accepted but also not added to
     *   the reject filter, then we will eventually redownload from other
     *   peers.
     */
    struct TxDownloadState {
        /* Track when to attempt download of announced transactions (process
         * time in micros -> txid)
         */
        std::multimap<std::chrono::microseconds, uint256> m_tx_process_time;

        //! Store all the transactions a peer has recently announced
        std::set<uint256> m_tx_announced;

        //! Store transactions which were requested by us, with timestamp
        std::map<uint256, std::chrono::microseconds> m_tx_in_flight;

        //! Periodically check for stuck getdata requests
        std::chrono::microseconds m_check_expiry_timer{0};
    };

    TxDownloadState m_tx_download;

    //! Whether this peer is an inbound connection
    bool m_is_inbound;

    //! Whether this peer is a manual connection
    bool m_is_manual_connection;

    CNodeState(CAddress addrIn, std::string addrNameIn, bool is_inbound, bool is_manual) :
        address(addrIn), name(std::move(addrNameIn)), m_is_inbound(is_inbound),
        m_is_manual_connection (is_manual)
    {
        fCurrentlyConnected = false;
        nMisbehavior = 0;
        fShouldBan = false;
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
        fPreferHeaderAndIDs = false;
        fProvidesHeaderAndIDs = false;
        fHaveWitness = false;
        fWantsCmpctWitness = false;
        fSupportsDesiredCmpctVersion = false;
        m_chain_sync = { 0, nullptr, false, false };
        m_last_block_announcement = 0;
    }
};

// Keeps track of the time (in microseconds) when transactions were requested last time
limitedmap<uint256, std::chrono::microseconds> g_already_asked_for GUARDED_BY(cs_main)(MAX_INV_SZ);

/** Map maintaining per-node state. */
static std::map<NodeId, CNodeState> mapNodeState GUARDED_BY(cs_main);

static CNodeState *State(NodeId pnode) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    std::map<NodeId, CNodeState>::iterator it = mapNodeState.find(pnode);
    if (it == mapNodeState.end())
        return nullptr;
    return &it->second;
}

static void UpdatePreferredDownload(CNode* node, CNodeState* state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    nPreferredDownload -= state->fPreferredDownload;

    // Whether this node should be marked as a preferred download node.
    state->fPreferredDownload = (!node->fInbound || node->HasPermission(PF_NOBAN)) && !node->fOneShot && !node->fClient;

    nPreferredDownload += state->fPreferredDownload;
}

static void PushNodeVersion(CNode *pnode, CConnman* connman, int64_t nTime)
{
    // Note that pnode->GetLocalServices() is a reflection of the local
    // services we were offering when the CNode object was created for this
    // peer.
    ServiceFlags nLocalNodeServices = pnode->GetLocalServices();
    uint64_t nonce = pnode->GetLocalNonce();
    int nNodeStartingHeight = pnode->GetMyStartingHeight();
    NodeId nodeid = pnode->GetId();
    CAddress addr = pnode->addr;

    CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService(), addr.nServices));
    CAddress addrMe = CAddress(CService(), nLocalNodeServices);

    connman->PushMessage(pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::VERSION, PROTOCOL_VERSION, (uint64_t)nLocalNodeServices, nTime, addrYou, addrMe,
            nonce, strSubVersion, nNodeStartingHeight, ::g_relay_txes && pnode->m_tx_relay != nullptr));

    if (fLogIPs) {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, them=%s, peer=%d\n", PROTOCOL_VERSION, nNodeStartingHeight, addrMe.ToString(), addrYou.ToString(), nodeid);
    } else {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, peer=%d\n", PROTOCOL_VERSION, nNodeStartingHeight, addrMe.ToString(), nodeid);
    }
}

// Returns a bool indicating whether we requested this block.
// Also used if a block was /not/ received and timed out or started with another peer
static bool MarkBlockAsReceived(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
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

// returns false, still setting pit, if the block was already in flight from the same peer
// pit will only be valid as long as the same cs_main lock is being held
static bool MarkBlockAsInFlight(NodeId nodeid, const uint256& hash, const CBlockIndex* pindex = nullptr, std::list<QueuedBlock>::iterator** pit = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
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
            {hash, pindex, pindex != nullptr, std::unique_ptr<PartiallyDownloadedBlock>(pit ? new PartiallyDownloadedBlock(&mempool) : nullptr)});
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

/** Check whether the last unknown block a peer advertised is not yet known. */
static void ProcessBlockAvailability(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    if (!state->hashLastUnknownBlock.IsNull()) {
        const CBlockIndex* pindex = LookupBlockIndex(state->hashLastUnknownBlock);
        if (pindex && pindex->nChainWork > 0) {
            if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
                state->pindexBestKnownBlock = pindex;
            }
            state->hashLastUnknownBlock.SetNull();
        }
    }
}

/** Update tracking information about which blocks a peer is assumed to have. */
static void UpdateBlockAvailability(NodeId nodeid, const uint256 &hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    ProcessBlockAvailability(nodeid);

    const CBlockIndex* pindex = LookupBlockIndex(hash);
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

/** Update tracking information about which blocks a peer is assumed to have. */
static void UpdateBestChainTip(NodeId nodeid, const uint256 &tip) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    const CBlockIndex* pindex = LookupBlockIndex(tip);
    if (pindex && pindex->nChainWork > 0) {
        state->pindexLastAnnouncedBlock = pindex;
        LogPrint(BCLog::NET, "peer=%s: announced best chain %s\n", nodeid, tip.GetHex());

        // announced block is better by chainwork. update pindexBestKnownBlock
        if(state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
            state->pindexBestKnownBlock = pindex;
        }
    }

    ProcessBlockAvailability(nodeid);
}

/**
 * When a peer sends us a valid block, instruct it to announce blocks to us
 * using CMPCTBLOCK if possible by adding its nodeid to the end of
 * lNodesAnnouncingHeaderAndIDs, and keeping that list under a certain size by
 * removing the first element if necessary.
 */
static void MaybeSetPeerAsAnnouncingHeaderAndIDs(NodeId nodeid, CConnman* connman) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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
        connman->ForNode(nodeid, [connman](CNode* pfrom){
            AssertLockHeld(cs_main);
            uint64_t nCMPCTBLOCKVersion = (pfrom->GetLocalServices() & NODE_WITNESS) ? 2 : 1;
            if (lNodesAnnouncingHeaderAndIDs.size() >= 3) {
                // As per BIP152, we only get 3 of our peers to announce
                // blocks using compact encodings.
                connman->ForNode(lNodesAnnouncingHeaderAndIDs.front(), [connman, nCMPCTBLOCKVersion](CNode* pnodeStop){
                    AssertLockHeld(cs_main);
                    connman->PushMessage(pnodeStop, CNetMsgMaker(pnodeStop->GetSendVersion()).Make(NetMsgType::SENDCMPCT, /*fAnnounceUsingCMPCTBLOCK=*/false, nCMPCTBLOCKVersion));
                    return true;
                });
                lNodesAnnouncingHeaderAndIDs.pop_front();
            }
            connman->PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::SENDCMPCT, /*fAnnounceUsingCMPCTBLOCK=*/true, nCMPCTBLOCKVersion));
            lNodesAnnouncingHeaderAndIDs.push_back(pfrom->GetId());
            return true;
        });
    }
}

static bool TipMayBeStale(const Consensus::Params &consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    if (g_last_tip_update == 0) {
        g_last_tip_update = GetTime();
    }
    return g_last_tip_update < GetTime() - consensusParams.nPowTargetSpacing * 3 && mapBlocksInFlight.empty();
}

static bool CanDirectFetch(const Consensus::Params &consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    return ::ChainActive().Tip()->GetBlockTime() > GetAdjustedTime() - consensusParams.nPowTargetSpacing * 20;
}

static bool PeerHasHeader(CNodeState *state, const CBlockIndex *pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (state->pindexBestKnownBlock && pindex == state->pindexBestKnownBlock->GetAncestor(pindex->nHeight))
        return true;
    if (state->pindexBestHeaderSent && pindex == state->pindexBestHeaderSent->GetAncestor(pindex->nHeight))
        return true;
    return false;
}

/** Update pindexLastCommonBlock and add not-in-flight missing successors to vBlocks, until it has
 *  at most count entries. */
static void FindNextBlocksToDownload(NodeId nodeid, unsigned int count, std::vector<const CBlockIndex*>& vBlocks, NodeId& nodeStaller, const Consensus::Params& consensusParams,
    // either pindexBestBlock or pindexLastAnouncedBlock
    const CBlockIndex* bestBlock,
    // out parameter: sets last common block
    const CBlockIndex** lastCommonBlockOut
    ) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (count == 0)
        return;

    vBlocks.reserve(vBlocks.size() + count);

    if (bestBlock == nullptr || bestBlock->nChainWork < nMinimumChainWork) {
        // This peer has nothing interesting.
        return;
    }

    assert(lastCommonBlockOut);

    if (*lastCommonBlockOut == nullptr) {
        // Bootstrap quickly by guessing a parent of our best tip is the forking point.
        // Guessing wrong in either direction is not a problem.
        *lastCommonBlockOut = ::ChainActive()[std::min(bestBlock->nHeight, ::ChainActive().Height())];
    }

    // If the peer reorganized, our previous pindexLastCommonBlock may not be an ancestor
    // of its current tip anymore. Go back enough to fix that.
    *lastCommonBlockOut = LastCommonAncestor(*lastCommonBlockOut, bestBlock);
    if (*lastCommonBlockOut == bestBlock)
        return;

    std::vector<const CBlockIndex*> vToFetch;
    const CBlockIndex *pindexWalk = *lastCommonBlockOut;
    // Never fetch further than the best block we know the peer has, or more than BLOCK_DOWNLOAD_WINDOW + 1 beyond the last
    // linked block we have in common with this peer. The +1 is so we can detect stalling, namely if we would be able to
    // download that next block if the window were 1 larger.
    int nWindowEnd = (*lastCommonBlockOut)->nHeight + BLOCK_DOWNLOAD_WINDOW;
    int nMaxHeight = std::min<int>(bestBlock->nHeight, nWindowEnd + 1);
    NodeId waitingfor = -1;
    while (pindexWalk->nHeight < nMaxHeight) {
        // Read up to 128 (or more, if more blocks than that are needed) successors of pindexWalk (towards
        // pindexBestKnownBlock) into vToFetch. We fetch 128, because CBlockIndex::GetAncestor may be as expensive
        // as iterating over ~100 CBlockIndex* entries anyway.
        int nToFetch = std::min(nMaxHeight - pindexWalk->nHeight, std::max<int>(count - vBlocks.size(), 128));
        vToFetch.resize(nToFetch);
        pindexWalk = bestBlock->GetAncestor(pindexWalk->nHeight + nToFetch);
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
            if (pindex->nStatus & BLOCK_HAVE_DATA || ::ChainActive().Contains(pindex)) {
                if (pindex->HaveTxsDownloaded())
                    *lastCommonBlockOut = pindex;
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

void EraseTxRequest(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    g_already_asked_for.erase(txid);
}

std::chrono::microseconds GetTxRequestTime(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto it = g_already_asked_for.find(txid);
    if (it != g_already_asked_for.end()) {
        return it->second;
    }
    return {};
}

void UpdateTxRequestTime(const uint256& txid, std::chrono::microseconds request_time) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto it = g_already_asked_for.find(txid);
    if (it == g_already_asked_for.end()) {
        g_already_asked_for.insert(std::make_pair(txid, request_time));
    } else {
        g_already_asked_for.update(it, request_time);
    }
}

std::chrono::microseconds CalculateTxGetDataTime(const uint256& txid, std::chrono::microseconds current_time, bool use_inbound_delay) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    std::chrono::microseconds process_time;
    const auto last_request_time = GetTxRequestTime(txid);
    // First time requesting this tx
    if (last_request_time.count() == 0) {
        process_time = current_time;
    } else {
        // Randomize the delay to avoid biasing some peers over others (such as due to
        // fixed ordering of peer processing in ThreadMessageHandler)
        process_time = last_request_time + GETDATA_TX_INTERVAL + GetRandMicros(MAX_GETDATA_RANDOM_DELAY);
    }

    // We delay processing announcements from inbound peers
    if (use_inbound_delay) process_time += INBOUND_PEER_TX_DELAY;

    return process_time;
}

void RequestTx(CNodeState* state, const uint256& txid, std::chrono::microseconds current_time) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    CNodeState::TxDownloadState& peer_download_state = state->m_tx_download;
    if (peer_download_state.m_tx_announced.size() >= MAX_PEER_TX_ANNOUNCEMENTS ||
            peer_download_state.m_tx_process_time.size() >= MAX_PEER_TX_ANNOUNCEMENTS ||
            peer_download_state.m_tx_announced.count(txid)) {
        // Too many queued announcements from this peer, or we already have
        // this announcement
        return;
    }
    peer_download_state.m_tx_announced.insert(txid);

    // Calculate the time to try requesting this transaction. Use
    // fPreferredDownload as a proxy for outbound peers.
    const auto process_time = CalculateTxGetDataTime(txid, current_time, !state->fPreferredDownload);

    peer_download_state.m_tx_process_time.emplace(process_time, txid);
}

} // namespace

// This function is used for testing the stale tip eviction logic, see
// denialofservice_tests.cpp
void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds)
{
    LOCK(cs_main);
    CNodeState *state = State(node);
    if (state) state->m_last_block_announcement = time_in_seconds;
}

// Returns true for outbound peers, excluding manual connections, feelers, and
// one-shots.
static bool IsOutboundDisconnectionCandidate(const CNode *node)
{
    return !(node->fInbound || node->m_manual_connection || node->fFeeler || node->fOneShot);
}

void PeerLogicValidation::InitializeNode(CNode *pnode) {
    CAddress addr = pnode->addr;
    std::string addrName = pnode->GetAddrName();
    NodeId nodeid = pnode->GetId();
    {
        LOCK(cs_main);
        mapNodeState.emplace_hint(mapNodeState.end(), std::piecewise_construct, std::forward_as_tuple(nodeid), std::forward_as_tuple(addr, std::move(addrName), pnode->fInbound, pnode->m_manual_connection));
    }
    if(!pnode->fInbound)
        PushNodeVersion(pnode, connman, GetTime());
}

void PeerLogicValidation::FinalizeNode(NodeId nodeid, bool& fUpdateConnectionTime) {
    fUpdateConnectionTime = false;
    LOCK(cs_main);
    CNodeState *state = State(nodeid);
    assert(state != nullptr);

    if (state->fSyncStarted)
        nSyncStarted--;

    if (state->nMisbehavior == 0 && state->fCurrentlyConnected) {
        fUpdateConnectionTime = true;
    }

    for (const QueuedBlock& entry : state->vBlocksInFlight) {
        mapBlocksInFlight.erase(entry.hash);
    }
    EraseOrphansFor(nodeid);
    nPreferredDownload -= state->fPreferredDownload;
    nPeersWithValidatedDownloads -= (state->nBlocksInFlightValidHeaders != 0);
    assert(nPeersWithValidatedDownloads >= 0);
    g_outbound_peers_with_protect_from_disconnect -= state->m_chain_sync.m_protect;
    assert(g_outbound_peers_with_protect_from_disconnect >= 0);

    mapNodeState.erase(nodeid);
    // VeriBlock
    VeriBlock::p2p::erasePopDataNodeState(nodeid);

    if (mapNodeState.empty()) {
        // Do a consistency check after the last peer is removed.
        assert(mapBlocksInFlight.empty());
        assert(nPreferredDownload == 0);
        assert(nPeersWithValidatedDownloads == 0);
        assert(g_outbound_peers_with_protect_from_disconnect == 0);
    }
    LogPrint(BCLog::NET, "Cleared nodestate for peer=%d\n", nodeid);
}

bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats) {
    LOCK(cs_main);
    CNodeState *state = State(nodeid);
    if (state == nullptr)
        return false;
    stats.nMisbehavior = state->nMisbehavior;
    stats.nSyncHeight = state->pindexBestKnownBlock ? state->pindexBestKnownBlock->nHeight : -1;
    stats.nCommonHeight = state->pindexLastCommonBlock ? state->pindexLastCommonBlock->nHeight : -1;
    for (const QueuedBlock& queue : state->vBlocksInFlight) {
        if (queue.pindex)
            stats.vHeightInFlight.push_back(queue.pindex->nHeight);
    }
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
    vExtraTxnForCompact[vExtraTxnForCompactIt] = std::make_pair(tx->GetWitnessHash(), tx);
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
    // 100 orphans, each of which is at most 100,000 bytes big is
    // at most 10 megabytes of orphans and somewhat more byprev index (in the worst case):
    unsigned int sz = GetTransactionWeight(*tx);
    if (sz > MAX_STANDARD_TX_WEIGHT)
    {
        LogPrint(BCLog::MEMPOOL, "ignoring large orphan tx (size: %u, hash: %s)\n", sz, hash.ToString());
        return false;
    }

    auto ret = mapOrphanTransactions.emplace(hash, COrphanTx{tx, peer, GetTime() + ORPHAN_TX_EXPIRE_TIME, g_orphan_list.size()});
    assert(ret.second);
    g_orphan_list.push_back(ret.first);
    for (const CTxIn& txin : tx->vin) {
        mapOrphanTransactionsByPrev[txin.prevout].insert(ret.first);
    }

    AddToCompactExtraTransactions(tx);

    LogPrint(BCLog::MEMPOOL, "stored orphan tx %s (mapsz %u outsz %u)\n", hash.ToString(),
             mapOrphanTransactions.size(), mapOrphanTransactionsByPrev.size());
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

    mapOrphanTransactions.erase(it);
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


unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans)
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
    while (mapOrphanTransactions.size() > nMaxOrphans)
    {
        // Evict a random orphan:
        size_t randompos = rng.randrange(g_orphan_list.size());
        EraseOrphanTx(g_orphan_list[randompos]->first);
        ++nEvicted;
    }
    return nEvicted;
}

/**
 * Mark a misbehaving peer to be banned depending upon the value of `-banscore`.
 */
void Misbehaving(NodeId pnode, int howmuch, const std::string& message) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (howmuch == 0)
        return;

    CNodeState *state = State(pnode);
    if (state == nullptr)
        return;

    state->nMisbehavior += howmuch;
    int banscore = gArgs.GetArg("-banscore", DEFAULT_BANSCORE_THRESHOLD);
    std::string message_prefixed = message.empty() ? "" : (": " + message);
    if (state->nMisbehavior >= banscore && state->nMisbehavior - howmuch < banscore)
    {
        LogPrint(BCLog::NET, "%s: %s peer=%d (%d -> %d) BAN THRESHOLD EXCEEDED%s\n", __func__, state->name, pnode, state->nMisbehavior-howmuch, state->nMisbehavior, message_prefixed);
        state->fShouldBan = true;
    } else
        LogPrint(BCLog::NET, "%s: %s peer=%d (%d -> %d)%s\n", __func__, state->name, pnode, state->nMisbehavior-howmuch, state->nMisbehavior, message_prefixed);
}

/**
 * Returns true if the given validation state result may result in a peer
 * banning/disconnecting us. We use this to determine which unaccepted
 * transactions from a whitelisted peer that we can safely relay.
 */
static bool TxRelayMayResultInDisconnect(const TxValidationState& state) {
    return state.GetResult() == TxValidationResult::TX_CONSENSUS;
}

/**
 * Potentially ban a node based on the contents of a BlockValidationState object
 *
 * @param[in] via_compact_block: this bool is passed in because net_processing should
 * punish peers differently depending on whether the data was provided in a compact
 * block message or not. If the compact block had a valid header, but contained invalid
 * txs, the peer should not be punished. See BIP 152.
 *
 * @return Returns true if the peer was punished (probably disconnected)
 */
static bool MaybePunishNodeForBlock(NodeId nodeid, const BlockValidationState& state, bool via_compact_block, const std::string& message = "") {
    switch (state.GetResult()) {
    case BlockValidationResult::BLOCK_RESULT_UNSET:
        break;
    // The node is providing invalid data:
    case BlockValidationResult::BLOCK_CONSENSUS:
    case BlockValidationResult::BLOCK_MUTATED:
        if (!via_compact_block) {
            LOCK(cs_main);
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

            // Ban outbound (but not inbound) peers if on an invalid chain.
            // Exempt HB compact block peers and manual connections.
            if (!via_compact_block && !node_state->m_is_inbound && !node_state->m_is_manual_connection) {
                Misbehaving(nodeid, 100, message);
                return true;
            }
            break;
        }
    case BlockValidationResult::BLOCK_INVALID_HEADER:
    case BlockValidationResult::BLOCK_CHECKPOINT:
    case BlockValidationResult::BLOCK_INVALID_PREV:
        {
            LOCK(cs_main);
            Misbehaving(nodeid, 100, message);
        }
        return true;
    // Conflicting (but not necessarily invalid) data or different policy:
    case BlockValidationResult::BLOCK_MISSING_PREV:
        {
            // TODO: Handle this much more gracefully (10 DoS points is super arbitrary)
            LOCK(cs_main);
            Misbehaving(nodeid, 10, message);
        }
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

/**
 * Potentially ban a node based on the contents of a TxValidationState object
 *
 * @return Returns true if the peer was punished (probably disconnected)
 *
 * Changes here may need to be reflected in TxRelayMayResultInDisconnect().
 */
static bool MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message = "") {
    switch (state.GetResult()) {
    case TxValidationResult::TX_BAD_POP_DATA:
        LogPrint(BCLog::NET, "pop tx is invalid: %s\n", message);
        break;
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
    case TxValidationResult::TX_WITNESS_MUTATED:
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
static bool BlockRequestAllowed(const CBlockIndex* pindex, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    if (::ChainActive().Contains(pindex)) return true;
    return pindex->IsValid(BLOCK_VALID_SCRIPTS) && (pindexBestHeader != nullptr) &&
        (pindexBestHeader->GetBlockTime() - pindex->GetBlockTime() < STALE_RELAY_AGE_LIMIT) &&
        (GetBlockProofEquivalentTime(*pindexBestHeader, *pindex, *pindexBestHeader, consensusParams) < STALE_RELAY_AGE_LIMIT);
}

PeerLogicValidation::PeerLogicValidation(CConnman* connmanIn, BanMan* banman, CScheduler& scheduler)
    : connman(connmanIn), m_banman(banman), m_stale_tip_check_time(0)
{
    // Initialize global variables that cannot be constructed at startup.
    recentRejects.reset(new CRollingBloomFilter(120000, 0.000001));

    const Consensus::Params& consensusParams = Params().GetConsensus();
    // Stale tip checking and peer eviction are on two different timers, but we
    // don't want them to get out of sync due to drift in the scheduler, so we
    // combine them in one function and schedule at the quicker (peer-eviction)
    // timer.
    static_assert(EXTRA_PEER_CHECK_INTERVAL < STALE_CHECK_INTERVAL, "peer eviction timer should be less than stale tip check timer");
    scheduler.scheduleEvery(std::bind(&PeerLogicValidation::CheckForStaleTipAndEvictPeers, this, consensusParams), EXTRA_PEER_CHECK_INTERVAL * 1000);
}

/**
 * Evict orphan txn pool entries (EraseOrphanTx) based on a newly connected
 * block. Also save the time of the last tip update.
 */
void PeerLogicValidation::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted) {
    LOCK(g_cs_orphans);

    std::vector<uint256> vOrphanErase;

    for (const CTransactionRef& ptx : pblock->vtx) {
        const CTransaction& tx = *ptx;

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

    g_last_tip_update = GetTime();
}

// All of the following cache a recent block, and are protected by cs_most_recent_block
static CCriticalSection cs_most_recent_block;
static std::shared_ptr<const CBlock> most_recent_block GUARDED_BY(cs_most_recent_block);
static std::shared_ptr<const CBlockHeaderAndShortTxIDs> most_recent_compact_block GUARDED_BY(cs_most_recent_block);
static uint256 most_recent_block_hash GUARDED_BY(cs_most_recent_block);
static bool fWitnessesPresentInMostRecentCompactBlock GUARDED_BY(cs_most_recent_block);

/**
 * Maintain state about the best-seen block and fast-announce a compact block
 * to compatible peers.
 */
void PeerLogicValidation::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock) {
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> pcmpctblock = std::make_shared<const CBlockHeaderAndShortTxIDs> (*pblock, true);
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);

    LOCK(cs_main);

    static int nHighestFastAnnounce = 0;
    if (pindex->nHeight <= nHighestFastAnnounce)
        return;
    nHighestFastAnnounce = pindex->nHeight;

    bool fWitnessEnabled = IsWitnessEnabled(pindex->pprev, Params().GetConsensus());
    uint256 hashBlock(pblock->GetHash());

    {
        LOCK(cs_most_recent_block);
        most_recent_block_hash = hashBlock;
        most_recent_block = pblock;
        most_recent_compact_block = pcmpctblock;
        fWitnessesPresentInMostRecentCompactBlock = fWitnessEnabled;
    }

    connman->ForEachNode([this, &pcmpctblock, pindex, &msgMaker, fWitnessEnabled, &hashBlock](CNode* pnode) {
        AssertLockHeld(cs_main);

        // TODO: Avoid the repeated-serialization here
        if (pnode->nVersion < INVALID_CB_NO_BAN_VERSION || pnode->fDisconnect)
            return;
        ProcessBlockAvailability(pnode->GetId());
        CNodeState &state = *State(pnode->GetId());
        // If the peer has, or we announced to them the previous block already,
        // but we don't think they have this one, go ahead and announce it
        if (state.fPreferHeaderAndIDs && (!fWitnessEnabled || state.fWantsCmpctWitness) &&
                !PeerHasHeader(&state, pindex) && PeerHasHeader(&state, pindex->pprev)) {

            LogPrint(BCLog::NET, "%s sending header-and-ids %s to peer=%d\n", "PeerLogicValidation::NewPoWValidBlock",
                    hashBlock.ToString(), pnode->GetId());
            connman->PushMessage(pnode, msgMaker.Make(NetMsgType::CMPCTBLOCK, *pcmpctblock));
            state.pindexBestHeaderSent = pindex;
        }
    });
}

/**
 * Update our best height and announce any block hashes which weren't previously
 * in ::ChainActive() to our peers.
 */
void PeerLogicValidation::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    const int nNewHeight = pindexNew->nHeight;
    connman->SetBestHeight(nNewHeight);

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
        connman->ForEachNode([nNewHeight, &vHashes](CNode* pnode) {
            if (nNewHeight > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : 0)) {
                for (const uint256& hash : reverse_iterate(vHashes)) {
                    pnode->PushBlockHash(hash);
                }
            }
        });
        connman->WakeMessageHandler();
    }
}

/**
 * Handle invalid block rejection and consequent peer banning, maintain which
 * peers announce compact blocks.
 */
void PeerLogicValidation::BlockChecked(const CBlock& block, const BlockValidationState& state) {
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
             !::ChainstateActive().IsInitialBlockDownload() &&
             mapBlocksInFlight.count(hash) == mapBlocksInFlight.size()) {
        if (it != mapBlockSource.end()) {
            MaybeSetPeerAsAnnouncingHeaderAndIDs(it->second.first, connman);
        }
    }
    if (it != mapBlockSource.end())
        mapBlockSource.erase(it);
}

//////////////////////////////////////////////////////////////////////////////
//
// Messages
//


bool static AlreadyHave(const CInv& inv) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    switch (inv.type)
    {
    case MSG_TX:
    case MSG_WITNESS_TX:
        {
            assert(recentRejects);
            if (::ChainActive().Tip()->GetBlockHash() != hashRecentRejectsChainTip)
            {
                // If the chain tip has changed previously rejected transactions
                // might be now valid, e.g. due to a nLockTime'd tx becoming valid,
                // or a double-spend. Reset the rejects filter and give those
                // txs a second chance.
                hashRecentRejectsChainTip = ::ChainActive().Tip()->GetBlockHash();
                recentRejects->reset();
            }

            {
                LOCK(g_cs_orphans);
                if (mapOrphanTransactions.count(inv.hash)) return true;
            }
            const CCoinsViewCache& coins_cache = ::ChainstateActive().CoinsTip();

            return recentRejects->contains(inv.hash) ||
                   mempool.exists(inv.hash) ||
                   coins_cache.HaveCoinInCache(COutPoint(inv.hash, 0)) || // Best effort: only try output 0 and 1
                   coins_cache.HaveCoinInCache(COutPoint(inv.hash, 1));
        }
    case MSG_BLOCK:
    case MSG_WITNESS_BLOCK:
        return LookupBlockIndex(inv.hash) != nullptr;
    }
    // Don't know what it is, just say we already got one
    return true;
}

void RelayTransaction(const uint256& txid, const CConnman& connman)
{
    CInv inv(MSG_TX, txid);
    connman.ForEachNode([&inv](CNode* pnode)
    {
        pnode->PushInventory(inv);
    });
}

static void RelayAddress(const CAddress& addr, bool fReachable, CConnman* connman)
{
    unsigned int nRelayNodes = fReachable ? 2 : 1; // limited relaying of addresses outside our network(s)

    // Relay to a limited number of other nodes
    // Use deterministic randomness to send to the same nodes for 24 hours
    // at a time so the m_addr_knowns of the chosen nodes prevent repeats
    uint64_t hashAddr = addr.GetHash();
    const CSipHasher hasher = connman->GetDeterministicRandomizer(RANDOMIZER_ID_ADDRESS_RELAY).Write(hashAddr << 32).Write((GetTime() + hashAddr) / (24*60*60));
    FastRandomContext insecure_rand;

    std::array<std::pair<uint64_t, CNode*>,2> best{{{0, nullptr}, {0, nullptr}}};
    assert(nRelayNodes <= best.size());

    auto sortfunc = [&best, &hasher, nRelayNodes](CNode* pnode) {
        if (pnode->nVersion >= CADDR_TIME_VERSION && pnode->IsAddrRelayPeer()) {
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

    connman->ForEachNodeThen(std::move(sortfunc), std::move(pushfunc));
}

void static ProcessGetBlockData(CNode* pfrom, const CChainParams& chainparams, const CInv& inv, CConnman* connman)
{
    bool send = false;
    std::shared_ptr<const CBlock> a_recent_block;
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> a_recent_compact_block;
    bool fWitnessesPresentInARecentCompactBlock;
    const Consensus::Params& consensusParams = chainparams.GetConsensus();
    {
        LOCK(cs_most_recent_block);
        a_recent_block = most_recent_block;
        a_recent_compact_block = most_recent_compact_block;
        fWitnessesPresentInARecentCompactBlock = fWitnessesPresentInMostRecentCompactBlock;
    }

    bool need_activate_chain = false;
    {
        LOCK(cs_main);
        const CBlockIndex* pindex = LookupBlockIndex(inv.hash);
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
        if (!ActivateBestChain(state, Params(), a_recent_block)) {
            LogPrint(BCLog::NET, "failed to activate chain (%s)\n", FormatStateMessage(state));
        }
    }

    LOCK(cs_main);
    const CBlockIndex* pindex = LookupBlockIndex(inv.hash);
    if (pindex) {
        send = BlockRequestAllowed(pindex, consensusParams);
        if (!send) {
            LogPrint(BCLog::NET, "%s: ignoring request from peer=%i for old block that isn't in the main chain\n", __func__, pfrom->GetId());
        }
    }
    const CNetMsgMaker msgMaker(pfrom->GetSendVersion());
    // disconnect node in case we have reached the outbound limit for serving historical blocks
    // never disconnect whitelisted nodes
    if (send && connman->OutboundTargetReached(true) && ( ((pindexBestHeader != nullptr) && (pindexBestHeader->GetBlockTime() - pindex->GetBlockTime() > HISTORICAL_BLOCK_AGE)) || inv.type == MSG_FILTERED_BLOCK) && !pfrom->HasPermission(PF_NOBAN))
    {
        LogPrint(BCLog::NET, "historical block serving limit reached, disconnect peer=%d\n", pfrom->GetId());

        //disconnect node
        pfrom->fDisconnect = true;
        send = false;
    }
    // Avoid leaking prune-height by never sending blocks below the NODE_NETWORK_LIMITED threshold
    if (send && !pfrom->HasPermission(PF_NOBAN) && (
            (((pfrom->GetLocalServices() & NODE_NETWORK_LIMITED) == NODE_NETWORK_LIMITED) && ((pfrom->GetLocalServices() & NODE_NETWORK) != NODE_NETWORK) && (::ChainActive().Tip()->nHeight - pindex->nHeight > (int)NODE_NETWORK_LIMITED_MIN_BLOCKS + 2 /* add two blocks buffer extension for possible races */) )
       )) {
        LogPrint(BCLog::NET, "Ignore block request below NODE_NETWORK_LIMITED threshold from peer=%d\n", pfrom->GetId());

        //disconnect node and prevent it from stalling (would otherwise wait for the missing block)
        pfrom->fDisconnect = true;
        send = false;
    }
    // Pruned nodes may have deleted the block, so check whether
    // it's available before trying to send.
    if (send && (pindex->nStatus & BLOCK_HAVE_DATA))
    {
        std::shared_ptr<const CBlock> pblock;
        if (a_recent_block && a_recent_block->GetHash() == pindex->GetBlockHash()) {
            pblock = a_recent_block;
        } else if (inv.type == MSG_WITNESS_BLOCK) {
            // Fast-path: in this case it is possible to serve the block directly from disk,
            // as the network format matches the format on disk
            std::vector<uint8_t> block_data;
            if (!ReadRawBlockFromDisk(block_data, pindex, chainparams.MessageStart())) {
                assert(!"cannot load block from disk");
            }
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::BLOCK, MakeSpan(block_data)));
            // Don't set pblock as we've sent the block
        } else {
            // Send block from disk
            std::shared_ptr<CBlock> pblockRead = std::make_shared<CBlock>();
            if (!ReadBlockFromDisk(*pblockRead, pindex, consensusParams))
                assert(!"cannot load block from disk");
            pblock = pblockRead;
        }
        if (pblock) {
            if (inv.type == MSG_BLOCK)
                connman->PushMessage(pfrom, msgMaker.Make(SERIALIZE_TRANSACTION_NO_WITNESS, NetMsgType::BLOCK, *pblock));
            else if (inv.type == MSG_WITNESS_BLOCK)
                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::BLOCK, *pblock));
            else if (inv.type == MSG_FILTERED_BLOCK)
            {
                bool sendMerkleBlock = false;
                CMerkleBlock merkleBlock;
                if (pfrom->m_tx_relay != nullptr) {
                    LOCK(pfrom->m_tx_relay->cs_filter);
                    if (pfrom->m_tx_relay->pfilter) {
                        sendMerkleBlock = true;
                        merkleBlock = CMerkleBlock(*pblock, *pfrom->m_tx_relay->pfilter);
                    }
                }
                if (sendMerkleBlock) {
                    connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::MERKLEBLOCK, merkleBlock));
                    // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                    // This avoids hurting performance by pointlessly requiring a round-trip
                    // Note that there is currently no way for a node to request any single transactions we didn't send here -
                    // they must either disconnect and retry or request the full block.
                    // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                    // however we MUST always provide at least what the remote peer needs
                    typedef std::pair<unsigned int, uint256> PairType;
                    for (PairType& pair : merkleBlock.vMatchedTxn)
                        connman->PushMessage(pfrom, msgMaker.Make(SERIALIZE_TRANSACTION_NO_WITNESS, NetMsgType::TX, *pblock->vtx[pair.first]));
                }
                // else
                    // no response
            }
            else if (inv.type == MSG_CMPCT_BLOCK)
            {
                // If a peer is asking for old blocks, we're almost guaranteed
                // they won't have a useful mempool to match against a compact block,
                // and we don't feel like constructing the object for them, so
                // instead we respond with the full, non-compact block.
                bool fPeerWantsWitness = State(pfrom->GetId())->fWantsCmpctWitness;
                int nSendFlags = fPeerWantsWitness ? 0 : SERIALIZE_TRANSACTION_NO_WITNESS;
                if (CanDirectFetch(consensusParams) && pindex->nHeight >= ::ChainActive().Height() - MAX_CMPCTBLOCK_DEPTH) {
                    if ((fPeerWantsWitness || !fWitnessesPresentInARecentCompactBlock) && a_recent_compact_block && a_recent_compact_block->header.GetHash() == pindex->GetBlockHash()) {
                        connman->PushMessage(pfrom, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, *a_recent_compact_block));
                    } else {
                        CBlockHeaderAndShortTxIDs cmpctblock(*pblock, fPeerWantsWitness);
                        connman->PushMessage(pfrom, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, cmpctblock));
                    }
                } else {
                    connman->PushMessage(pfrom, msgMaker.Make(nSendFlags, NetMsgType::BLOCK, *pblock));
                }
            }
        }

        // Trigger the peer node to send a getblocks request for the next batch of inventory
        if (inv.hash == pfrom->hashContinue)
        {
            // Bypass PushInventory, this must send even if redundant,
            // and we want it right after the last block so they don't
            // wait for other stuff first.
            std::vector<CInv> vInv;
            vInv.push_back(CInv(MSG_BLOCK, ::ChainActive().Tip()->GetBlockHash()));
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::INV, vInv));
            pfrom->hashContinue.SetNull();
        }
    }
}

void static ProcessGetData(CNode* pfrom, const CChainParams& chainparams, CConnman* connman, const std::atomic<bool>& interruptMsgProc) LOCKS_EXCLUDED(cs_main)
{
    AssertLockNotHeld(cs_main);

    std::deque<CInv>::iterator it = pfrom->vRecvGetData.begin();
    std::vector<CInv> vNotFound;
    const CNetMsgMaker msgMaker(pfrom->GetSendVersion());

    // Note that if we receive a getdata for a MSG_TX or MSG_WITNESS_TX from a
    // block-relay-only outbound peer, we will stop processing further getdata
    // messages from this peer (likely resulting in our peer eventually
    // disconnecting us).
    if (pfrom->m_tx_relay != nullptr) {
        // mempool entries added before this time have likely expired from mapRelay
        const std::chrono::seconds longlived_mempool_time = GetTime<std::chrono::seconds>() - RELAY_TX_CACHE_TIME;
        const std::chrono::seconds mempool_req = pfrom->m_tx_relay->m_last_mempool_req.load();

        LOCK(cs_main);

        while (it != pfrom->vRecvGetData.end() && (it->type == MSG_TX || it->type == MSG_WITNESS_TX)) {
            if (interruptMsgProc)
                return;
            // Don't bother if send buffer is too full to respond anyway
            if (pfrom->fPauseSend)
                break;

            const CInv &inv = *it;
            it++;

            // Send stream from relay memory
            bool push = false;
            auto mi = mapRelay.find(inv.hash);
            int nSendFlags = (inv.type == MSG_TX ? SERIALIZE_TRANSACTION_NO_WITNESS : 0);
            if (mi != mapRelay.end()) {
                connman->PushMessage(pfrom, msgMaker.Make(nSendFlags, NetMsgType::TX, *mi->second));
                push = true;
            } else {
                auto txinfo = mempool.info(inv.hash);
                // To protect privacy, do not answer getdata using the mempool when
                // that TX couldn't have been INVed in reply to a MEMPOOL request,
                // or when it's too recent to have expired from mapRelay.
                if (txinfo.tx && (
                     (mempool_req.count() && txinfo.m_time <= mempool_req)
                      || (txinfo.m_time <= longlived_mempool_time)))
                {
                    connman->PushMessage(pfrom, msgMaker.Make(nSendFlags, NetMsgType::TX, *txinfo.tx));
                    push = true;
                }
            }
            if (!push) {
                vNotFound.push_back(inv);
            }
        }
    } // release cs_main

    if (it != pfrom->vRecvGetData.end() && !pfrom->fPauseSend) {
        const CInv &inv = *it;
        if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK || inv.type == MSG_CMPCT_BLOCK || inv.type == MSG_WITNESS_BLOCK) {
            it++;
            ProcessGetBlockData(pfrom, chainparams, inv, connman);
        }
    }

    // Unknown types in the GetData stay in vRecvGetData and block any future
    // message from this peer, see vRecvGetData check in ProcessMessages().
    // Depending on future p2p changes, we might either drop unknown getdata on
    // the floor or disconnect the peer.

    pfrom->vRecvGetData.erase(pfrom->vRecvGetData.begin(), it);

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
        connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::NOTFOUND, vNotFound));
    }
}

static uint32_t GetFetchFlags(CNode* pfrom) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    uint32_t nFetchFlags = 0;
    if ((pfrom->GetLocalServices() & NODE_WITNESS) && State(pfrom->GetId())->fHaveWitness) {
        nFetchFlags |= MSG_WITNESS_FLAG;
    }
    return nFetchFlags;
}

inline void static SendBlockTransactions(const CBlock& block, const BlockTransactionsRequest& req, CNode* pfrom, CConnman* connman) {
    BlockTransactions resp(req);
    for (size_t i = 0; i < req.indexes.size(); i++) {
        if (req.indexes[i] >= block.vtx.size()) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 100, strprintf("Peer %d sent us a getblocktxn with out-of-bounds tx indices", pfrom->GetId()));
            return;
        }
        resp.txn[i] = block.vtx[req.indexes[i]];
    }
    LOCK(cs_main);
    const CNetMsgMaker msgMaker(pfrom->GetSendVersion());
    int nSendFlags = State(pfrom->GetId())->fWantsCmpctWitness ? 0 : SERIALIZE_TRANSACTION_NO_WITNESS;

    //VeriBlock add popData
    resp.popData = block.popData;

    connman->PushMessage(pfrom, msgMaker.Make(nSendFlags, NetMsgType::BLOCKTXN, resp));
}

bool static ProcessHeadersMessage(CNode *pfrom, CConnman *connman, const std::vector<CBlockHeader>& headers, const CChainParams& chainparams, bool via_compact_block)
{
    const CNetMsgMaker msgMaker(pfrom->GetSendVersion());
    size_t nCount = headers.size();

    if (nCount == 0) {
        // Nothing interesting. Stop asking this peers for more headers.
        return true;
    }

    bool received_new_header = false;
    const CBlockIndex *pindexLast = nullptr;
    {
        LOCK(cs_main);
        CNodeState *nodestate = State(pfrom->GetId());

        // If this looks like it could be a block announcement (nCount <
        // MAX_BLOCKS_TO_ANNOUNCE), use special logic for handling headers that
        // don't connect:
        // - Send a getheaders message in response to try to connect the chain.
        // - The peer can send up to MAX_UNCONNECTING_HEADERS in a row that
        //   don't connect before giving DoS points
        // - Once a headers message is received that is valid and does connect,
        //   nUnconnectingHeaders gets reset back to 0.
        if (!LookupBlockIndex(headers[0].hashPrevBlock) && nCount < MAX_BLOCKS_TO_ANNOUNCE) {
            nodestate->nUnconnectingHeaders++;
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(pindexBestHeader), uint256()));
            LogPrint(BCLog::NET, "received header %s: missing prev block %s, sending getheaders (%d) to end (peer=%d, nUnconnectingHeaders=%d)\n",
                    headers[0].GetHash().ToString(),
                    headers[0].hashPrevBlock.ToString(),
                    pindexBestHeader->nHeight,
                    pfrom->GetId(), nodestate->nUnconnectingHeaders);
            // Set hashLastUnknownBlock for this peer, so that if we
            // eventually get the headers - even from a different peer -
            // we can use this peer to download.
            UpdateBlockAvailability(pfrom->GetId(), headers.back().GetHash());

            if (nodestate->nUnconnectingHeaders % MAX_UNCONNECTING_HEADERS == 0) {
                Misbehaving(pfrom->GetId(), 20);
            }
            return true;
        }

        uint256 hashLastBlock;
        for (const CBlockHeader& header : headers) {
            if (!hashLastBlock.IsNull() && header.hashPrevBlock != hashLastBlock) {
                Misbehaving(pfrom->GetId(), 20, "non-continuous headers sequence");
                return false;
            }
            hashLastBlock = header.GetHash();
        }

        // If we don't have the last header, then they'll have given us
        // something new (if these headers are valid).
        if (!LookupBlockIndex(hashLastBlock)) {
            received_new_header = true;
        }
    }

    BlockValidationState state;
    if (!ProcessNewBlockHeaders(headers, state, chainparams, &pindexLast)) {
        if (state.IsInvalid()) {
            MaybePunishNodeForBlock(pfrom->GetId(), state, via_compact_block, "invalid header received");
            return false;
        }
    }

    {
        LOCK(cs_main);
        CNodeState *nodestate = State(pfrom->GetId());
        if (nodestate->nUnconnectingHeaders > 0) {
            LogPrint(BCLog::NET, "peer=%d: resetting nUnconnectingHeaders (%d -> 0)\n", pfrom->GetId(), nodestate->nUnconnectingHeaders);
        }
        nodestate->nUnconnectingHeaders = 0;

        assert(pindexLast);
        UpdateBlockAvailability(pfrom->GetId(), pindexLast->GetBlockHash());

        // From here, pindexBestKnownBlock should be guaranteed to be non-null,
        // because it is set in UpdateBlockAvailability. Some nullptr checks
        // are still present, however, as belt-and-suspenders.

        if (received_new_header && pindexLast->nChainWork > ::ChainActive().Tip()->nChainWork) {
            nodestate->m_last_block_announcement = GetTime();
        }

        if (nCount == MAX_HEADERS_RESULTS) {
            // Headers message had its maximum size; the peer may have more headers.
            // TODO: optimize: if pindexLast is an ancestor of ::ChainActive().Tip or pindexBestHeader, continue
            // from there instead.
            LogPrint(BCLog::NET, "more getheaders (%d) to end to peer=%d (startheight:%d)\n", pindexLast->nHeight, pfrom->GetId(), pfrom->nStartingHeight);
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(pindexLast), uint256()));
        }

        bool fCanDirectFetch = CanDirectFetch(chainparams.GetConsensus());
        // If this set of headers is valid and ends in a block with at least as
        // much work as our tip, download as much as possible.
        if (fCanDirectFetch && pindexLast->IsValid(BLOCK_VALID_TREE)
            // VeriBlock: download the chain suggested by the peer
            /* && ::ChainActive().Tip()->nChainWork <= pindexLast->nChainWork */) {
            std::vector<const CBlockIndex*> vToFetch;
            const CBlockIndex *pindexWalk = pindexLast;
            // Calculate all the blocks we'd need to switch to pindexLast, up to a limit.
            while (pindexWalk && !::ChainActive().Contains(pindexWalk) && vToFetch.size() <= MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
                if (!(pindexWalk->nStatus & BLOCK_HAVE_DATA) &&
                        !mapBlocksInFlight.count(pindexWalk->GetBlockHash()) &&
                        (!IsWitnessEnabled(pindexWalk->pprev, chainparams.GetConsensus()) || State(pfrom->GetId())->fHaveWitness)) {
                    // We don't have this block, and it's not yet in flight.
                    vToFetch.push_back(pindexWalk);
                }
                pindexWalk = pindexWalk->pprev;
            }
            // If pindexWalk still isn't on our main chain, we're looking at a
            // very large reorg at a time we think we're close to caught up to
            // the main chain -- this shouldn't really happen.  Bail out on the
            // direct fetch and rely on parallel download instead.
            if (!::ChainActive().Contains(pindexWalk)) {
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
                    MarkBlockAsInFlight(pfrom->GetId(), pindex->GetBlockHash(), pindex);
                    LogPrint(BCLog::NET, "Requesting block %s from  peer=%d\n",
                            pindex->GetBlockHash().ToString(), pfrom->GetId());
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
                    connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETDATA, vGetData));
                }
            }
        }
        // If we're in IBD, we want outbound peers that will serve us a useful
        // chain. Disconnect peers that are on chains with insufficient work.
        if (::ChainstateActive().IsInitialBlockDownload() && nCount != MAX_HEADERS_RESULTS) {
            // When nCount < MAX_HEADERS_RESULTS, we know we have no more
            // headers to fetch from this peer.
            if (nodestate->pindexBestKnownBlock && nodestate->pindexBestKnownBlock->nChainWork < nMinimumChainWork) {
                // This peer has too little work on their headers chain to help
                // us sync -- disconnect if using an outbound slot (unless
                // whitelisted or addnode).
                // Note: We compare their tip to nMinimumChainWork (rather than
                // ::ChainActive().Tip()) because we won't start block download
                // until we have a headers chain that has at least
                // nMinimumChainWork, even if a peer has a chain past our tip,
                // as an anti-DoS measure.
                if (IsOutboundDisconnectionCandidate(pfrom)) {
                    LogPrintf("Disconnecting outbound peer %d -- headers chain has insufficient work\n", pfrom->GetId());
                    pfrom->fDisconnect = true;
                }
            }
        }

        if (!pfrom->fDisconnect && IsOutboundDisconnectionCandidate(pfrom) && nodestate->pindexBestKnownBlock != nullptr && pfrom->m_tx_relay != nullptr) {
            // If this is an outbound full-relay peer, check to see if we should protect
            // it from the bad/lagging chain logic.
            // Note that block-relay-only peers are already implicitly protected, so we
            // only consider setting m_protect for the full-relay peers.
            if (g_outbound_peers_with_protect_from_disconnect < MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT && nodestate->pindexBestKnownBlock->nChainWork >= ::ChainActive().Tip()->nChainWork && !nodestate->m_chain_sync.m_protect) {
                LogPrint(BCLog::NET, "Protecting outbound peer=%d from eviction\n", pfrom->GetId());
                nodestate->m_chain_sync.m_protect = true;
                ++g_outbound_peers_with_protect_from_disconnect;
            }
        }
    }

    return true;
}

void static ProcessOrphanTx(CConnman* connman, std::set<uint256>& orphan_work_set, std::list<CTransactionRef>& removed_txn) EXCLUSIVE_LOCKS_REQUIRED(cs_main, g_cs_orphans)
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
        if (AcceptToMemoryPool(mempool, orphan_state, porphanTx, &removed_txn, false /* bypass_limits */, 0 /* nAbsurdFee */)) {
            LogPrint(BCLog::MEMPOOL, "   accepted orphan tx %s\n", orphanHash.ToString());
            RelayTransaction(orphanHash, *connman);
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
            if (!orphanTx.HasWitness() && orphan_state.GetResult() != TxValidationResult::TX_WITNESS_MUTATED) {
                // Do not use rejection cache for witness transactions or
                // witness-stripped transactions, as they can have been malleated.
                // See https://github.com/bitcoin/bitcoin/issues/8279 for details.
                assert(recentRejects);
                recentRejects->insert(orphanHash);
            }
            EraseOrphanTx(orphanHash);
            done = true;
        }
        mempool.check(&::ChainstateActive().CoinsTip());
    }
}

bool static ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, int64_t nTimeReceived, const CChainParams& chainparams, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc)
{
    LogPrint(BCLog::NET, "received: %s (%u bytes) peer=%d\n", SanitizeString(strCommand), vRecv.size(), pfrom->GetId());
    if (gArgs.IsArgSet("-dropmessagestest") && GetRand(gArgs.GetArg("-dropmessagestest", 0)) == 0)
    {
        LogPrintf("dropmessagestest DROPPING RECV MESSAGE\n");
        return true;
    }

    // VeriBlock: if POP is not enabled, ignore POP-related P2P calls
    if (VeriBlock::isPopEnabled()) {
        int tipHeight = ChainActive().Height();
        if (Params().isPopActive(tipHeight)) {
            int pop_res = VeriBlock::p2p::processPopData(pfrom, strCommand, vRecv, connman);
            if (pop_res >= 0) {
                return pop_res;
            }
        }
    }

    if (!(pfrom->GetLocalServices() & NODE_BLOOM) &&
              (strCommand == NetMsgType::FILTERLOAD ||
               strCommand == NetMsgType::FILTERADD))
    {
        if (pfrom->nVersion >= NO_BLOOM_VERSION) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 100);
            return false;
        } else {
            pfrom->fDisconnect = true;
            return false;
        }
    }

    if (strCommand == NetMsgType::VERSION) {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0)
        {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 1);
            return false;
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
        nServices = ServiceFlags(nServiceInt);
        if (!pfrom->fInbound)
        {
            connman->SetServices(pfrom->addr, nServices);
        }
        if (!pfrom->fInbound && !pfrom->fFeeler && !pfrom->m_manual_connection && !HasAllDesirableServiceFlags(nServices))
        {
            LogPrint(BCLog::NET, "peer=%d does not offer the expected services (%08x offered, %08x expected); disconnecting\n", pfrom->GetId(), nServices, GetDesirableServiceFlags(nServices));
            pfrom->fDisconnect = true;
            return false;
        }

        if (nVersion < MIN_PEER_PROTO_VERSION) {
            // disconnect from peers older than this proto version
            LogPrint(BCLog::NET, "peer=%d using obsolete version %i; disconnecting\n", pfrom->GetId(), nVersion);
            pfrom->fDisconnect = true;
            return false;
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
        // Disconnect if we connected to ourself
        if (pfrom->fInbound && !connman->CheckIncomingNonce(nNonce))
        {
            LogPrintf("connected to self at %s, disconnecting\n", pfrom->addr.ToString());
            pfrom->fDisconnect = true;
            return true;
        }

        if (pfrom->fInbound && addrMe.IsRoutable())
        {
            SeenLocal(addrMe);
        }

        // Be shy and don't send version until we hear
        if (pfrom->fInbound)
            PushNodeVersion(pfrom, connman, GetAdjustedTime());

        connman->PushMessage(pfrom, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::VERACK));

        pfrom->nServices = nServices;
        pfrom->SetAddrLocal(addrMe);
        {
            LOCK(pfrom->cs_SubVer);
            pfrom->cleanSubVer = cleanSubVer;
        }
        pfrom->nStartingHeight = nStartingHeight;

        // set nodes not relaying blocks and tx and not serving (parts) of the historical blockchain as "clients"
        pfrom->fClient = (!(nServices & NODE_NETWORK) && !(nServices & NODE_NETWORK_LIMITED));

        // set nodes not capable of serving the complete blockchain history as "limited nodes"
        pfrom->m_limited_node = (!(nServices & NODE_NETWORK) && (nServices & NODE_NETWORK_LIMITED));

        if (pfrom->m_tx_relay != nullptr) {
            LOCK(pfrom->m_tx_relay->cs_filter);
            pfrom->m_tx_relay->fRelayTxes = fRelay; // set to true after we get the first filter* message
        }

        // Change version
        pfrom->SetSendVersion(nSendVersion);
        pfrom->nVersion = nVersion;

        if((nServices & NODE_WITNESS))
        {
            LOCK(cs_main);
            State(pfrom->GetId())->fHaveWitness = true;
        }

        // Potentially mark this peer as a preferred download peer.
        {
        LOCK(cs_main);
        UpdatePreferredDownload(pfrom, State(pfrom->GetId()));
        }

        if (!pfrom->fInbound && pfrom->IsAddrRelayPeer())
        {
            // Advertise our address
            if (fListen && !::ChainstateActive().IsInitialBlockDownload())
            {
                CAddress addr = GetLocalAddress(&pfrom->addr, pfrom->GetLocalServices());
                FastRandomContext insecure_rand;
                if (addr.IsRoutable())
                {
                    LogPrint(BCLog::NET, "ProcessMessages: advertising address %s\n", addr.ToString());
                    pfrom->PushAddress(addr, insecure_rand);
                } else if (IsPeerAddrLocalGood(pfrom)) {
                    addr.SetIP(addrMe);
                    LogPrint(BCLog::NET, "ProcessMessages: advertising address %s\n", addr.ToString());
                    pfrom->PushAddress(addr, insecure_rand);
                }
            }

            // Get recent addresses
            if (pfrom->fOneShot || pfrom->nVersion >= CADDR_TIME_VERSION || connman->GetAddressCount() < 1000)
            {
                connman->PushMessage(pfrom, CNetMsgMaker(nSendVersion).Make(NetMsgType::GETADDR));
                pfrom->fGetAddr = true;
            }
            connman->MarkAddressGood(pfrom->addr);
        }

        std::string remoteAddr;
        if (fLogIPs)
            remoteAddr = ", peeraddr=" + pfrom->addr.ToString();

        LogPrint(BCLog::NET, "receive version message: %s: version %d, blocks=%d, us=%s, peer=%d%s\n",
                  cleanSubVer, pfrom->nVersion,
                  pfrom->nStartingHeight, addrMe.ToString(), pfrom->GetId(),
                  remoteAddr);

        int64_t nTimeOffset = nTime - GetTime();
        pfrom->nTimeOffset = nTimeOffset;
        AddTimeData(pfrom->addr, nTimeOffset);

        // If the peer is old enough to have the old alert system, send it the final alert.
        if (pfrom->nVersion <= 70012) {
            CDataStream finalAlert(ParseHex("60010000000000000000000000ffffff7f00000000ffffff7ffeffff7f01ffffff7f00000000ffffff7f00ffffff7f002f555247454e543a20416c657274206b657920636f6d70726f6d697365642c2075706772616465207265717569726564004630440220653febd6410f470f6bae11cad19c48413becb1ac2c17f908fd0fd53bdc3abd5202206d0e9c96fe88d4a0f01ed9dedae2b6f9e00da94cad0fecaae66ecf689bf71b50"), SER_NETWORK, PROTOCOL_VERSION);
            connman->PushMessage(pfrom, CNetMsgMaker(nSendVersion).Make("alert", finalAlert));
        }

        // Feeler connections exist only to verify if address is online.
        if (pfrom->fFeeler) {
            assert(pfrom->fInbound == false);
            pfrom->fDisconnect = true;
        }
        return true;
    }

    if (pfrom->nVersion == 0) {
        // Must have a version message before anything else
        LOCK(cs_main);
        Misbehaving(pfrom->GetId(), 1);
        return false;
    }

    // At this point, the outgoing message serialization version can't change.
    const CNetMsgMaker msgMaker(pfrom->GetSendVersion());

    if (strCommand == NetMsgType::VERACK)
    {
        pfrom->SetRecvVersion(std::min(pfrom->nVersion.load(), PROTOCOL_VERSION));

        if (!pfrom->fInbound) {
            // Mark this node as currently connected, so we update its timestamp later.
            LOCK(cs_main);
            State(pfrom->GetId())->fCurrentlyConnected = true;
            LogPrintf("New outbound peer connected: version: %d, blocks=%d, peer=%d%s (%s)\n",
                      pfrom->nVersion.load(), pfrom->nStartingHeight,
                      pfrom->GetId(), (fLogIPs ? strprintf(", peeraddr=%s", pfrom->addr.ToString()) : ""),
                      pfrom->m_tx_relay == nullptr ? "block-relay" : "full-relay");
        }

        if (pfrom->nVersion >= SENDHEADERS_VERSION) {
            // Tell our peer we prefer to receive headers rather than inv's
            // We send this to non-NODE NETWORK peers as well, because even
            // non-NODE NETWORK peers can announce blocks (such as pruning
            // nodes)
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SENDHEADERS));
        }
        if (pfrom->nVersion >= SHORT_IDS_BLOCKS_VERSION) {
            // Tell our peer we are willing to provide version 1 or 2 cmpctblocks
            // However, we do not request new block announcements using
            // cmpctblock messages.
            // We send this to non-NODE NETWORK peers as well, because
            // they may wish to request compact blocks from us
            bool fAnnounceUsingCMPCTBLOCK = false;
            uint64_t nCMPCTBLOCKVersion = 2;
            if (pfrom->GetLocalServices() & NODE_WITNESS)
                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion));
            nCMPCTBLOCKVersion = 1;
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion));
        }
        pfrom->fSuccessfullyConnected = true;
        return true;
    }

    if (!pfrom->fSuccessfullyConnected) {
        // Must have a verack message before anything else
        LOCK(cs_main);
        Misbehaving(pfrom->GetId(), 1);
        return false;
    }

    if (strCommand == NetMsgType::ADDR) {
        std::vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Don't want addr from older versions unless seeding
        if (pfrom->nVersion < CADDR_TIME_VERSION && connman->GetAddressCount() > 1000)
            return true;
        if (!pfrom->IsAddrRelayPeer()) {
            return true;
        }
        if (vAddr.size() > 1000)
        {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 20, strprintf("message addr size() = %u", vAddr.size()));
            return false;
        }

        // Store the new addresses
        std::vector<CAddress> vAddrOk;
        int64_t nNow = GetAdjustedTime();
        int64_t nSince = nNow - 10 * 60;
        for (CAddress& addr : vAddr)
        {
            if (interruptMsgProc)
                return true;

            // We only bother storing full nodes, though this may include
            // things which we would not make an outbound connection to, in
            // part because we may make feeler connections to them.
            if (!MayHaveUsefulAddressDB(addr.nServices) && !HasAllDesirableServiceFlags(addr.nServices))
                continue;

            if (addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
                addr.nTime = nNow - 5 * 24 * 60 * 60;
            pfrom->AddAddressKnown(addr);
            if (banman->IsBanned(addr)) continue; // Do not process banned addresses beyond remembering we received them
            bool fReachable = IsReachable(addr);
            if (addr.nTime > nSince && !pfrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
            {
                // Relay to a limited number of other nodes
                RelayAddress(addr, fReachable, connman);
            }
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        connman->AddNewAddresses(vAddrOk, pfrom->addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom->fGetAddr = false;
        if (pfrom->fOneShot)
            pfrom->fDisconnect = true;
        return true;
    }

    if (strCommand == NetMsgType::SENDHEADERS) {
        LOCK(cs_main);
        State(pfrom->GetId())->fPreferHeaders = true;
        return true;
    }

    if (strCommand == NetMsgType::SENDCMPCT) {
        bool fAnnounceUsingCMPCTBLOCK = false;
        uint64_t nCMPCTBLOCKVersion = 0;
        vRecv >> fAnnounceUsingCMPCTBLOCK >> nCMPCTBLOCKVersion;
        if (nCMPCTBLOCKVersion == 1 || ((pfrom->GetLocalServices() & NODE_WITNESS) && nCMPCTBLOCKVersion == 2)) {
            LOCK(cs_main);
            // fProvidesHeaderAndIDs is used to "lock in" version of compact blocks we send (fWantsCmpctWitness)
            if (!State(pfrom->GetId())->fProvidesHeaderAndIDs) {
                State(pfrom->GetId())->fProvidesHeaderAndIDs = true;
                State(pfrom->GetId())->fWantsCmpctWitness = nCMPCTBLOCKVersion == 2;
            }
            if (State(pfrom->GetId())->fWantsCmpctWitness == (nCMPCTBLOCKVersion == 2)) // ignore later version announces
                State(pfrom->GetId())->fPreferHeaderAndIDs = fAnnounceUsingCMPCTBLOCK;
            if (!State(pfrom->GetId())->fSupportsDesiredCmpctVersion) {
                if (pfrom->GetLocalServices() & NODE_WITNESS)
                    State(pfrom->GetId())->fSupportsDesiredCmpctVersion = (nCMPCTBLOCKVersion == 2);
                else
                    State(pfrom->GetId())->fSupportsDesiredCmpctVersion = (nCMPCTBLOCKVersion == 1);
            }
        }
        return true;
    }

    if (strCommand == NetMsgType::INV) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 20, strprintf("message inv size() = %u", vInv.size()));
            return false;
        }

        // We won't accept tx inv's if we're in blocks-only mode, or this is a
        // block-relay-only peer
        bool fBlocksOnly = !g_relay_txes || (pfrom->m_tx_relay == nullptr);

        // Allow whitelisted peers to send data other than blocks in blocks only mode if whitelistrelay is true
        if (pfrom->HasPermission(PF_RELAY))
            fBlocksOnly = false;

        LOCK(cs_main);

        uint32_t nFetchFlags = GetFetchFlags(pfrom);
        const auto current_time = GetTime<std::chrono::microseconds>();

        for (CInv &inv : vInv)
        {
            if (interruptMsgProc)
                return true;

            bool fAlreadyHave = AlreadyHave(inv);
            LogPrint(BCLog::NET, "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom->GetId());

            if (inv.type == MSG_TX) {
                inv.type |= nFetchFlags;
            }

            if (inv.type == MSG_BLOCK) {
                UpdateBlockAvailability(pfrom->GetId(), inv.hash);
                if (!fAlreadyHave && !fImporting && !fReindex && !mapBlocksInFlight.count(inv.hash)) {
                    // We used to request the full block here, but since headers-announcements are now the
                    // primary method of announcement on the network, and since, in the case that a node
                    // fell back to inv we probably have a reorg which we should get the headers for first,
                    // we now only provide a getheaders response here. When we receive the headers, we will
                    // then ask for the blocks we need.
                    connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(pindexBestHeader), inv.hash));
                    LogPrint(BCLog::NET, "getheaders (%d) %s to peer=%d\n", pindexBestHeader->nHeight, inv.hash.ToString(), pfrom->GetId());
                }
            }
            else
            {
                pfrom->AddInventoryKnown(inv);
                if (fBlocksOnly) {
                    LogPrint(BCLog::NET, "transaction (%s) inv sent in violation of protocol, disconnecting peer=%d\n", inv.hash.ToString(), pfrom->GetId());
                    pfrom->fDisconnect = true;
                    return true;
                } else if (!fAlreadyHave && !fImporting && !fReindex && !::ChainstateActive().IsInitialBlockDownload()) {
                    RequestTx(State(pfrom->GetId()), inv.hash, current_time);
                }
            }
        }
        return true;
    }

    if (strCommand == NetMsgType::GETDATA) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 20, strprintf("message getdata size() = %u", vInv.size()));
            return false;
        }

        LogPrint(BCLog::NET, "received getdata (%u invsz) peer=%d\n", vInv.size(), pfrom->GetId());

        if (vInv.size() > 0) {
            LogPrint(BCLog::NET, "received getdata for: %s peer=%d\n", vInv[0].ToString(), pfrom->GetId());
        }

        pfrom->vRecvGetData.insert(pfrom->vRecvGetData.end(), vInv.begin(), vInv.end());
        ProcessGetData(pfrom, chainparams, connman, interruptMsgProc);
        return true;
    }

    if (strCommand == NetMsgType::GETBLOCKS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "getblocks locator size %lld > %d, disconnect peer=%d\n", locator.vHave.size(), MAX_LOCATOR_SZ, pfrom->GetId());
            pfrom->fDisconnect = true;
            return true;
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
            if (!ActivateBestChain(state, Params(), a_recent_block)) {
                LogPrint(BCLog::NET, "failed to activate chain (%s)\n", FormatStateMessage(state));
            }
        }

        LOCK(cs_main);

        // Find the last block the caller has in the main chain
        const CBlockIndex* pindex = FindForkInGlobalIndex(::ChainActive(), locator);

        // Send the rest of the chain
        if (pindex)
            pindex = ::ChainActive().Next(pindex);
        int nLimit = 500;
        LogPrint(BCLog::NET, "getblocks %d to %s limit %d from peer=%d\n", (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), nLimit, pfrom->GetId());
        for (; pindex; pindex = ::ChainActive().Next(pindex))
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrint(BCLog::NET, "  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            // If pruning, don't inv blocks unless we have on disk and are likely to still have
            // for some reasonable time window (1 hour) that block relay might require.
            const int nPrunedBlocksLikelyToHave = MIN_BLOCKS_TO_KEEP - 3600 / chainparams.GetConsensus().nPowTargetSpacing;
            if (fPruneMode && (!(pindex->nStatus & BLOCK_HAVE_DATA) || pindex->nHeight <= ::ChainActive().Tip()->nHeight - nPrunedBlocksLikelyToHave))
            {
                LogPrint(BCLog::NET, " getblocks stopping, pruned or too old block at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll
                // trigger the peer to getblocks the next batch of inventory.
                LogPrint(BCLog::NET, "  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                pfrom->hashContinue = pindex->GetBlockHash();
                break;
            }
        }
        return true;
    }

    if (strCommand == NetMsgType::GETBLOCKTXN) {
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
            SendBlockTransactions(*recent_block, req, pfrom, connman);
            return true;
        }

        LOCK(cs_main);

        const CBlockIndex* pindex = LookupBlockIndex(req.blockhash);
        if (!pindex || !(pindex->nStatus & BLOCK_HAVE_DATA)) {
            LogPrint(BCLog::NET, "Peer %d sent us a getblocktxn for a block we don't have\n", pfrom->GetId());
            return true;
        }

        if (pindex->nHeight < ::ChainActive().Height() - MAX_BLOCKTXN_DEPTH) {
            // If an older block is requested (should never happen in practice,
            // but can happen in tests) send a block response instead of a
            // blocktxn response. Sending a full block response instead of a
            // small blocktxn response is preferable in the case where a peer
            // might maliciously send lots of getblocktxn requests to trigger
            // expensive disk reads, because it will require the peer to
            // actually receive all the data read from disk over the network.
            LogPrint(BCLog::NET, "Peer %d sent us a getblocktxn for a block > %i deep\n", pfrom->GetId(), MAX_BLOCKTXN_DEPTH);
            CInv inv;
            inv.type = State(pfrom->GetId())->fWantsCmpctWitness ? MSG_WITNESS_BLOCK : MSG_BLOCK;
            inv.hash = req.blockhash;
            pfrom->vRecvGetData.push_back(inv);
            // The message processing loop will go around again (without pausing) and we'll respond then (without cs_main)
            return true;
        }

        CBlock block;
        bool ret = ReadBlockFromDisk(block, pindex, chainparams.GetConsensus());
        assert(ret);

        SendBlockTransactions(block, req, pfrom, connman);
        return true;
    }

    if (strCommand == NetMsgType::GETHEADERS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "getheaders locator size %lld > %d, disconnect peer=%d\n", locator.vHave.size(), MAX_LOCATOR_SZ, pfrom->GetId());
            pfrom->fDisconnect = true;
            return true;
        }

        LOCK(cs_main);
        if (::ChainstateActive().IsInitialBlockDownload() && !pfrom->HasPermission(PF_NOBAN)) {
            LogPrint(BCLog::NET, "Ignoring getheaders from peer=%d because node is in initial block download\n", pfrom->GetId());
            return true;
        }

        CNodeState *nodestate = State(pfrom->GetId());
        const CBlockIndex* pindex = nullptr;
        if (locator.IsNull())
        {
            // If locator is null, return the hashStop block
            pindex = LookupBlockIndex(hashStop);
            if (!pindex) {
                return true;
            }

            if (!BlockRequestAllowed(pindex, chainparams.GetConsensus())) {
                LogPrint(BCLog::NET, "%s: ignoring request from peer=%i for old block header that isn't in the main chain\n", __func__, pfrom->GetId());
                return true;
            }
        }
        else
        {
            // Find the last block the caller has in the main chain
            pindex = FindForkInGlobalIndex(::ChainActive(), locator);
            if (pindex)
                pindex = ::ChainActive().Next(pindex);
        }

        // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
        std::vector<CBlock> vHeaders;
        int nLimit = MAX_HEADERS_RESULTS;
        LogPrint(BCLog::NET, "getheaders %d to %s from peer=%d\n", (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), pfrom->GetId());
        for (; pindex; pindex = ::ChainActive().Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        }
        // pindex can be nullptr either if we sent ::ChainActive().Tip() OR
        // if our peer has ::ChainActive().Tip() (and thus we are sending an empty
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
        nodestate->pindexBestHeaderSent = pindex ? pindex : ::ChainActive().Tip();
        connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
        return true;
    }

    if (strCommand == NetMsgType::TX) {
        // Stop processing the transaction early if
        // We are in blocks only mode and peer is either not whitelisted or whitelistrelay is off
        // or if this peer is supposed to be a block-relay-only peer
        if ((!g_relay_txes && !pfrom->HasPermission(PF_RELAY)) || (pfrom->m_tx_relay == nullptr))
        {
            LogPrint(BCLog::NET, "transaction sent in violation of protocol peer=%d\n", pfrom->GetId());
            pfrom->fDisconnect = true;
            return true;
        }

        CTransactionRef ptx;
        vRecv >> ptx;
        const CTransaction& tx = *ptx;

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK2(cs_main, g_cs_orphans);

        TxValidationState state;

        CNodeState* nodestate = State(pfrom->GetId());
        nodestate->m_tx_download.m_tx_announced.erase(inv.hash);
        nodestate->m_tx_download.m_tx_in_flight.erase(inv.hash);
        EraseTxRequest(inv.hash);

        std::list<CTransactionRef> lRemovedTxn;

        if (!AlreadyHave(inv) &&
            AcceptToMemoryPool(mempool, state, ptx, &lRemovedTxn, false /* bypass_limits */, 0 /* nAbsurdFee */)) {
            mempool.check(&::ChainstateActive().CoinsTip());
            RelayTransaction(tx.GetHash(), *connman);
            for (unsigned int i = 0; i < tx.vout.size(); i++) {
                auto it_by_prev = mapOrphanTransactionsByPrev.find(COutPoint(inv.hash, i));
                if (it_by_prev != mapOrphanTransactionsByPrev.end()) {
                    for (const auto& elem : it_by_prev->second) {
                        pfrom->orphan_work_set.insert(elem->first);
                    }
                }
            }

            pfrom->nLastTXTime = GetTime();

            LogPrint(BCLog::MEMPOOL, "AcceptToMemoryPool: peer=%d: accepted %s (poolsz %u txn, %u kB)\n",
                pfrom->GetId(),
                tx.GetHash().ToString(),
                mempool.size(), mempool.DynamicMemoryUsage() / 1000);

            // Recursively process any orphan transactions that depended on this one
            ProcessOrphanTx(connman, pfrom->orphan_work_set, lRemovedTxn);
        }
        else if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS)
        {
            bool fRejectedParents = false; // It may be the case that the orphans parents have all been rejected
            for (const CTxIn& txin : tx.vin) {
                if (recentRejects->contains(txin.prevout.hash)) {
                    fRejectedParents = true;
                    break;
                }
            }
            if (!fRejectedParents) {
                uint32_t nFetchFlags = GetFetchFlags(pfrom);
                const auto current_time = GetTime<std::chrono::microseconds>();

                for (const CTxIn& txin : tx.vin) {
                    CInv _inv(MSG_TX | nFetchFlags, txin.prevout.hash);
                    pfrom->AddInventoryKnown(_inv);
                    if (!AlreadyHave(_inv)) RequestTx(State(pfrom->GetId()), _inv.hash, current_time);
                }
                AddOrphanTx(ptx, pfrom->GetId());

                // DoS prevention: do not allow mapOrphanTransactions to grow unbounded (see CVE-2012-3789)
                unsigned int nMaxOrphanTx = (unsigned int)std::max((int64_t)0, gArgs.GetArg("-maxorphantx", DEFAULT_MAX_ORPHAN_TRANSACTIONS));
                unsigned int nEvicted = LimitOrphanTxSize(nMaxOrphanTx);
                if (nEvicted > 0) {
                    LogPrint(BCLog::MEMPOOL, "mapOrphan overflow, removed %u tx\n", nEvicted);
                }
            } else {
                LogPrint(BCLog::MEMPOOL, "not keeping orphan with rejected parents %s\n",tx.GetHash().ToString());
                // We will continue to reject this tx since it has rejected
                // parents so avoid re-requesting it from other peers.
                recentRejects->insert(tx.GetHash());
            }
        } else {
            if (!tx.HasWitness() && state.GetResult() != TxValidationResult::TX_WITNESS_MUTATED) {
                // Do not use rejection cache for witness transactions or
                // witness-stripped transactions, as they can have been malleated.
                // See https://github.com/bitcoin/bitcoin/issues/8279 for details.
                assert(recentRejects);
                recentRejects->insert(tx.GetHash());
                if (RecursiveDynamicUsage(*ptx) < 100000) {
                    AddToCompactExtraTransactions(ptx);
                }
            } else if (tx.HasWitness() && RecursiveDynamicUsage(*ptx) < 100000) {
                AddToCompactExtraTransactions(ptx);
            }

            if (pfrom->HasPermission(PF_FORCERELAY)) {
                // Always relay transactions received from whitelisted peers, even
                // if they were already in the mempool or rejected from it due
                // to policy, allowing the node to function as a gateway for
                // nodes hidden behind it.
                //
                // Never relay transactions that might result in being
                // disconnected (or banned).
                if (state.IsInvalid() && TxRelayMayResultInDisconnect(state)) {
                    LogPrintf("Not relaying invalid transaction %s from whitelisted peer=%d (%s)\n", tx.GetHash().ToString(), pfrom->GetId(), FormatStateMessage(state));
                } else {
                    LogPrintf("Force relaying tx %s from whitelisted peer=%d\n", tx.GetHash().ToString(), pfrom->GetId());
                    RelayTransaction(tx.GetHash(), *connman);
                }
            }
        }

        for (const CTransactionRef& removedTx : lRemovedTxn)
            AddToCompactExtraTransactions(removedTx);

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

        if (state.IsInvalid())
        {
            LogPrint(BCLog::MEMPOOLREJ, "%s from peer=%d was not accepted: %s\n", tx.GetHash().ToString(),
                pfrom->GetId(),
                FormatStateMessage(state));
            MaybePunishNodeForTx(pfrom->GetId(), state);
        }
        return true;
    }

    if (strCommand == NetMsgType::CMPCTBLOCK)
    {
        // Ignore cmpctblock received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected cmpctblock message received from peer %d\n", pfrom->GetId());
            return true;
        }

        CBlockHeaderAndShortTxIDs cmpctblock;
        vRecv >> cmpctblock;

        bool received_new_header = false;

        {
        LOCK(cs_main);

        if (!LookupBlockIndex(cmpctblock.header.hashPrevBlock)) {
            // Doesn't connect (or is genesis), instead of DoSing in AcceptBlockHeader, request deeper headers
            if (!::ChainstateActive().IsInitialBlockDownload())
                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(pindexBestHeader), uint256()));
            return true;
        }

        if (!LookupBlockIndex(cmpctblock.header.GetHash())) {
            received_new_header = true;
        }
        }

        const CBlockIndex *pindex = nullptr;
        BlockValidationState state;
        if (!ProcessNewBlockHeaders({cmpctblock.header}, state, chainparams, &pindex)) {
            if (state.IsInvalid()) {
                MaybePunishNodeForBlock(pfrom->GetId(), state, /*via_compact_block*/ true, "invalid header via cmpctblock");
                return true;
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
        UpdateBlockAvailability(pfrom->GetId(), pindex->GetBlockHash());

        CNodeState *nodestate = State(pfrom->GetId());

        // If this was a new header with more work than our tip, update the
        // peer's last block announcement time
        if (received_new_header && pindex->nChainWork > ::ChainActive().Tip()->nChainWork) {
            nodestate->m_last_block_announcement = GetTime();
        }

        std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator blockInFlightIt = mapBlocksInFlight.find(pindex->GetBlockHash());
        bool fAlreadyInFlight = blockInFlightIt != mapBlocksInFlight.end();

        if (pindex->nStatus & BLOCK_HAVE_DATA) // Nothing to do here
            return true;

        if (pindex->nChainWork <= ::ChainActive().Tip()->nChainWork || // We know something better
                pindex->nTx != 0) { // We had this block at some point, but pruned it
            if (fAlreadyInFlight) {
                // We requested this block for some reason, but our mempool will probably be useless
                // so we just grab the block via normal getdata
                std::vector<CInv> vInv(1);
                vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(pfrom), cmpctblock.header.GetHash());
                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
            }
            return true;
        }

        // If we're not close to tip yet, give up and let parallel block fetch work its magic
        if (!fAlreadyInFlight && !CanDirectFetch(chainparams.GetConsensus()))
            return true;

        if (IsWitnessEnabled(pindex->pprev, chainparams.GetConsensus()) && !nodestate->fSupportsDesiredCmpctVersion) {
            // Don't bother trying to process compact blocks from v1 peers
            // after segwit activates.
            return true;
        }

        // We want to be a bit conservative just to be extra careful about DoS
        // possibilities in compact block processing...
        if (pindex->nHeight <= ::ChainActive().Height() + 2) {
            if ((!fAlreadyInFlight && nodestate->nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) ||
                 (fAlreadyInFlight && blockInFlightIt->second.first == pfrom->GetId())) {
                std::list<QueuedBlock>::iterator* queuedBlockIt = nullptr;
                if (!MarkBlockAsInFlight(pfrom->GetId(), pindex->GetBlockHash(), pindex, &queuedBlockIt)) {
                    if (!(*queuedBlockIt)->partialBlock)
                        (*queuedBlockIt)->partialBlock.reset(new PartiallyDownloadedBlock(&mempool));
                    else {
                        // The block was already in flight using compact blocks from the same peer
                        LogPrint(BCLog::NET, "Peer sent us compact block we were already syncing!\n");
                        return true;
                    }
                }

                PartiallyDownloadedBlock& partialBlock = *(*queuedBlockIt)->partialBlock;
                ReadStatus status = partialBlock.InitData(cmpctblock, vExtraTxnForCompact);
                if (status == READ_STATUS_INVALID) {
                    MarkBlockAsReceived(pindex->GetBlockHash()); // Reset in-flight state in case of whitelist
                    Misbehaving(pfrom->GetId(), 100, strprintf("Peer %d sent us invalid compact block\n", pfrom->GetId()));
                    return true;
                } else if (status == READ_STATUS_FAILED) {
                    // Duplicate txindexes, the block is now in-flight, so just request it
                    std::vector<CInv> vInv(1);
                    vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(pfrom), cmpctblock.header.GetHash());
                    connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                    return true;
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
                    txn.popData = cmpctblock.popData;
                    blockTxnMsg << txn;
                    fProcessBLOCKTXN = true;
                } else {
                    req.blockhash = pindex->GetBlockHash();
                    connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETBLOCKTXN, req));
                }
            } else {
                // This block is either already in flight from a different
                // peer, or this peer has too many blocks outstanding to
                // download from.
                // Optimistically try to reconstruct anyway since we might be
                // able to without any round trips.
                PartiallyDownloadedBlock tempBlock(&mempool);
                ReadStatus status = tempBlock.InitData(cmpctblock, vExtraTxnForCompact);
                if (status != READ_STATUS_OK) {
                    // TODO: don't ignore failures
                    return true;
                }
                std::vector<CTransactionRef> dummy;
                status = tempBlock.FillBlock(*pblock, dummy);
                if (status == READ_STATUS_OK) {
                    fBlockReconstructed = true;
                    if(pblock && pblock->nVersion & VeriBlock::POP_BLOCK_VERSION_BIT) {
                        assert(!pblock->popData.empty() && "POP bit is set and POP data is empty");
                    }
                }
            }
        } else {
            if (fAlreadyInFlight) {
                // We requested this block, but its far into the future, so our
                // mempool will probably be useless - request the block normally
                std::vector<CInv> vInv(1);
                vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(pfrom), cmpctblock.header.GetHash());
                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                return true;
            } else {
                // If this was an announce-cmpctblock, we want the same treatment as a header message
                fRevertToHeaderProcessing = true;
            }
        }
        } // cs_main

        if (fProcessBLOCKTXN)
            return ProcessMessage(pfrom, NetMsgType::BLOCKTXN, blockTxnMsg, nTimeReceived, chainparams, connman, banman, interruptMsgProc);

        if (fRevertToHeaderProcessing) {
            // Headers received from HB compact block peers are permitted to be
            // relayed before full validation (see BIP 152), so we don't want to disconnect
            // the peer if the header turns out to be for an invalid block.
            // Note that if a peer tries to build on an invalid chain, that
            // will be detected and the peer will be banned.
            return ProcessHeadersMessage(pfrom, connman, {cmpctblock.header}, chainparams, /*via_compact_block=*/true);
        }

        if (fBlockReconstructed) {
            // If we got here, we were able to optimistically reconstruct a
            // block that is in flight from some other peer.
            {
                LOCK(cs_main);
                mapBlockSource.emplace(pblock->GetHash(), std::make_pair(pfrom->GetId(), false));
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
            ProcessNewBlock(chainparams, pblock, /*fForceProcessing=*/true, &fNewBlock);
            if (fNewBlock) {
                pfrom->nLastBlockTime = GetTime();
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
        return true;
    }

    if (strCommand == NetMsgType::BLOCKTXN)
    {
        // Ignore blocktxn received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected blocktxn message received from peer %d\n", pfrom->GetId());
            return true;
        }

        BlockTransactions resp;
        vRecv >> resp;
        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        bool fBlockRead = false;
        {
            LOCK(cs_main);

            std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator it = mapBlocksInFlight.find(resp.blockhash);
            if (it == mapBlocksInFlight.end() || !it->second.second->partialBlock ||
                    it->second.first != pfrom->GetId()) {
                LogPrint(BCLog::NET, "Peer %d sent us block transactions for block we weren't expecting\n", pfrom->GetId());
                return true;
            }

            PartiallyDownloadedBlock& partialBlock = *it->second.second->partialBlock;
            ReadStatus status = partialBlock.FillBlock(*pblock, resp.txn, resp.popData);
            if (status == READ_STATUS_INVALID) {
                MarkBlockAsReceived(resp.blockhash); // Reset in-flight state in case of whitelist
                Misbehaving(pfrom->GetId(), 100, strprintf("Peer %d sent us invalid compact block/non-matching block transactions\n", pfrom->GetId()));
                return true;
            } else if (status == READ_STATUS_FAILED) {
                // Might have collided, fall back to getdata now :(
                std::vector<CInv> invs;
                invs.push_back(CInv(MSG_BLOCK | GetFetchFlags(pfrom), resp.blockhash));
                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::GETDATA, invs));
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
                // Under BIP 152, we don't DoS-ban unless proof of work is
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
                mapBlockSource.emplace(resp.blockhash, std::make_pair(pfrom->GetId(), false));
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
            ProcessNewBlock(chainparams, pblock, /*fForceProcessing=*/true, &fNewBlock);
            if (fNewBlock) {
                pfrom->nLastBlockTime = GetTime();
            } else {
                LOCK(cs_main);
                mapBlockSource.erase(pblock->GetHash());
            }
        }
        return true;
    }

    if (strCommand == NetMsgType::HEADERS)
    {
        // Ignore headers received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected headers message received from peer %d\n", pfrom->GetId());
            return true;
        }

        std::vector<CBlockHeader> headers;
        // Bypass the normal CBlock deserialization, as we don't want to risk deserializing 2000 full blocks.
        unsigned int nCount = ReadCompactSize(vRecv);
        if (nCount > MAX_HEADERS_RESULTS) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 20, strprintf("headers message size = %u", nCount));
            return false;
        }
        headers.resize(nCount);
        for (unsigned int n = 0; n < nCount; n++) {
            vRecv >> headers[n];
            ReadCompactSize(vRecv); // ignore tx count; assume it is 0.
            if (headers[n].nVersion & VeriBlock::POP_BLOCK_VERSION_BIT) {
                altintegration::PopData tmp;
                vRecv >> tmp;
            }
        }

        return ProcessHeadersMessage(pfrom, connman, headers, chainparams, /*via_compact_block=*/false);
    }

    if (strCommand == NetMsgType::BLOCK)
    {
        // Ignore block received while importing
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected block message received from peer %d\n", pfrom->GetId());
            return true;
        }

        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        vRecv >> *pblock;

        LogPrint(BCLog::NET, "received block %s peer=%d\n", pblock->GetHash().ToString(), pfrom->GetId());

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
            mapBlockSource.emplace(hash, std::make_pair(pfrom->GetId(), true));
        }
        bool fNewBlock = false;
        ProcessNewBlock(chainparams, pblock, forceProcessing, &fNewBlock);
        if (fNewBlock) {
            pfrom->nLastBlockTime = GetTime();
        } else {
            LOCK(cs_main);
            mapBlockSource.erase(pblock->GetHash());
        }
        return true;
    }

    if (strCommand == NetMsgType::GETADDR) {
        // This asymmetric behavior for inbound and outbound connections was introduced
        // to prevent a fingerprinting attack: an attacker can send specific fake addresses
        // to users' AddrMan and later request them by sending getaddr messages.
        // Making nodes which are behind NAT and can only make outgoing connections ignore
        // the getaddr message mitigates the attack.
        if (!pfrom->fInbound) {
            LogPrint(BCLog::NET, "Ignoring \"getaddr\" from outbound connection. peer=%d\n", pfrom->GetId());
            return true;
        }
        if (!pfrom->IsAddrRelayPeer()) {
            LogPrint(BCLog::NET, "Ignoring \"getaddr\" from block-relay-only connection. peer=%d\n", pfrom->GetId());
            return true;
        }

        // Only send one GetAddr response per connection to reduce resource waste
        //  and discourage addr stamping of INV announcements.
        if (pfrom->fSentAddr) {
            LogPrint(BCLog::NET, "Ignoring repeated \"getaddr\". peer=%d\n", pfrom->GetId());
            return true;
        }
        pfrom->fSentAddr = true;

        pfrom->vAddrToSend.clear();
        std::vector<CAddress> vAddr = connman->GetAddresses();
        FastRandomContext insecure_rand;
        for (const CAddress &addr : vAddr) {
            if (!banman->IsBanned(addr)) {
                pfrom->PushAddress(addr, insecure_rand);
            }
        }
        return true;
    }

    if (strCommand == NetMsgType::MEMPOOL) {
        if (!(pfrom->GetLocalServices() & NODE_BLOOM) && !pfrom->HasPermission(PF_MEMPOOL))
        {
            if (!pfrom->HasPermission(PF_NOBAN))
            {
                LogPrint(BCLog::NET, "mempool request with bloom filters disabled, disconnect peer=%d\n", pfrom->GetId());
                pfrom->fDisconnect = true;
            }
            return true;
        }

        if (connman->OutboundTargetReached(false) && !pfrom->HasPermission(PF_MEMPOOL))
        {
            if (!pfrom->HasPermission(PF_NOBAN))
            {
                LogPrint(BCLog::NET, "mempool request with bandwidth limit reached, disconnect peer=%d\n", pfrom->GetId());
                pfrom->fDisconnect = true;
            }
            return true;
        }

        if (pfrom->m_tx_relay != nullptr) {
            LOCK(pfrom->m_tx_relay->cs_tx_inventory);
            pfrom->m_tx_relay->fSendMempool = true;
        }
        return true;
    }

    if (strCommand == NetMsgType::PING) {
        if (pfrom->nVersion > BIP0031_VERSION)
        {
            uint64_t nonce = 0;
            vRecv >> nonce;

            if(pfrom->nVersion > PING_BESTCHAIN_VERSION) {
                // VeriBlock: immediately after nonce, receive best block hash
                LOCK(cs_main);
                uint256 bestHash;
                vRecv >> bestHash;
                UpdateBestChainTip(pfrom->GetId(), bestHash);

                connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::PONG, nonce, ::ChainActive().Tip()->GetBlockHash()));
                return true;
            }

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
            connman->PushMessage(pfrom, msgMaker.Make(NetMsgType::PONG, nonce));
        }
        return true;
    }

    if (strCommand == NetMsgType::PONG) {
        int64_t pingUsecEnd = nTimeReceived;
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;

        if (nAvail >= sizeof(nonce)) {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (pfrom->nPingNonceSent != 0) {
                if (nonce == pfrom->nPingNonceSent) {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished = true;
                    int64_t pingUsecTime = pingUsecEnd - pfrom->nPingUsecStart;
                    if (pingUsecTime > 0) {
                        // Successful ping time measurement, replace previous
                        pfrom->nPingUsecTime = pingUsecTime;
                        pfrom->nMinPingUsecTime = std::min(pfrom->nMinPingUsecTime.load(), pingUsecTime);
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

                if(pfrom->nVersion > PING_BESTCHAIN_VERSION) {
                    LOCK(cs_main);
                    uint256 bestHash;
                    vRecv >> bestHash;
                    UpdateBestChainTip(pfrom->GetId(), bestHash);
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
                pfrom->GetId(),
                sProblem,
                pfrom->nPingNonceSent,
                nonce,
                nAvail);
        }
        if (bPingFinished) {
            pfrom->nPingNonceSent = 0;
        }
        return true;
    }

    if (strCommand == NetMsgType::FILTERLOAD) {
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints())
        {
            // There is no excuse for sending a too-large filter
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 100);
        }
        else if (pfrom->m_tx_relay != nullptr)
        {
            LOCK(pfrom->m_tx_relay->cs_filter);
            pfrom->m_tx_relay->pfilter.reset(new CBloomFilter(filter));
            pfrom->m_tx_relay->pfilter->UpdateEmptyFull();
            pfrom->m_tx_relay->fRelayTxes = true;
        }
        return true;
    }

    if (strCommand == NetMsgType::FILTERADD) {
        std::vector<unsigned char> vData;
        vRecv >> vData;

        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        bool bad = false;
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            bad = true;
        } else if (pfrom->m_tx_relay != nullptr) {
            LOCK(pfrom->m_tx_relay->cs_filter);
            if (pfrom->m_tx_relay->pfilter) {
                pfrom->m_tx_relay->pfilter->insert(vData);
            } else {
                bad = true;
            }
        }
        if (bad) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 100);
        }
        return true;
    }

    if (strCommand == NetMsgType::FILTERCLEAR) {
        if (pfrom->m_tx_relay == nullptr) {
            return true;
        }
        LOCK(pfrom->m_tx_relay->cs_filter);
        if (pfrom->GetLocalServices() & NODE_BLOOM) {
            pfrom->m_tx_relay->pfilter.reset(new CBloomFilter());
        }
        pfrom->m_tx_relay->fRelayTxes = true;
        return true;
    }

    if (strCommand == NetMsgType::FEEFILTER) {
        CAmount newFeeFilter = 0;
        vRecv >> newFeeFilter;
        if (MoneyRange(newFeeFilter)) {
            if (pfrom->m_tx_relay != nullptr) {
                LOCK(pfrom->m_tx_relay->cs_feeFilter);
                pfrom->m_tx_relay->minFeeFilter = newFeeFilter;
            }
            LogPrint(BCLog::NET, "received: feefilter of %s from peer=%d\n", CFeeRate(newFeeFilter).ToString(), pfrom->GetId());
        }
        return true;
    }

    if (strCommand == NetMsgType::NOTFOUND) {
        // Remove the NOTFOUND transactions from the peer
        LOCK(cs_main);
        CNodeState *state = State(pfrom->GetId());
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() <= MAX_PEER_TX_IN_FLIGHT + MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            for (CInv &inv : vInv) {
                if (inv.type == MSG_TX || inv.type == MSG_WITNESS_TX) {
                    // If we receive a NOTFOUND message for a txid we requested, erase
                    // it from our data structures for this peer.
                    auto in_flight_it = state->m_tx_download.m_tx_in_flight.find(inv.hash);
                    if (in_flight_it == state->m_tx_download.m_tx_in_flight.end()) {
                        // Skip any further work if this is a spurious NOTFOUND
                        // message.
                        continue;
                    }
                    state->m_tx_download.m_tx_in_flight.erase(in_flight_it);
                    state->m_tx_download.m_tx_announced.erase(inv.hash);
                }
            }
        }
        return true;
    }

    // Ignore unknown commands for extensibility
    LogPrint(BCLog::NET, "Unknown command \"%s\" from peer=%d\n", SanitizeString(strCommand), pfrom->GetId());
    return true;
}

bool PeerLogicValidation::CheckIfBanned(CNode* pnode)
{
    AssertLockHeld(cs_main);
    CNodeState &state = *State(pnode->GetId());

    if (state.fShouldBan) {
        state.fShouldBan = false;
        if (pnode->HasPermission(PF_NOBAN))
            LogPrintf("Warning: not punishing whitelisted peer %s!\n", pnode->addr.ToString());
        else if (pnode->m_manual_connection)
            LogPrintf("Warning: not punishing manually-connected peer %s!\n", pnode->addr.ToString());
        else if (pnode->addr.IsLocal()) {
            // Disconnect but don't ban _this_ local node
            LogPrintf("Warning: disconnecting but not banning local peer %s!\n", pnode->addr.ToString());
            pnode->fDisconnect = true;
        } else {
            // Disconnect and ban all nodes sharing the address
            if (m_banman) {
                m_banman->Ban(pnode->addr, BanReasonNodeMisbehaving);
            }
            connman->DisconnectNode(pnode->addr);
        }
        return true;
    }
    return false;
}

bool PeerLogicValidation::ProcessMessages(CNode* pfrom, std::atomic<bool>& interruptMsgProc)
{
    const CChainParams& chainparams = Params();
    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fMoreWork = false;

    if (!pfrom->vRecvGetData.empty())
        ProcessGetData(pfrom, chainparams, connman, interruptMsgProc);

    if (!pfrom->orphan_work_set.empty()) {
        std::list<CTransactionRef> removed_txn;
        LOCK2(cs_main, g_cs_orphans);
        ProcessOrphanTx(connman, pfrom->orphan_work_set, removed_txn);
        for (const CTransactionRef& removedTx : removed_txn) {
            AddToCompactExtraTransactions(removedTx);
        }
    }

    if (pfrom->fDisconnect)
        return false;

    // this maintains the order of responses
    // and prevents vRecvGetData to grow unbounded
    if (!pfrom->vRecvGetData.empty()) return true;
    if (!pfrom->orphan_work_set.empty()) return true;

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
        pfrom->fPauseRecv = pfrom->nProcessQueueSize > connman->GetReceiveFloodSize();
        fMoreWork = !pfrom->vProcessMsg.empty();
    }
    CNetMessage& msg(msgs.front());

    msg.SetVersion(pfrom->GetRecvVersion());
    // Check network magic
    if (!msg.m_valid_netmagic) {
        LogPrint(BCLog::NET, "PROCESSMESSAGE: INVALID MESSAGESTART %s peer=%d\n", SanitizeString(msg.m_command), pfrom->GetId());
        pfrom->fDisconnect = true;
        return false;
    }

    // Check header
    if (!msg.m_valid_header)
    {
        LogPrint(BCLog::NET, "PROCESSMESSAGE: ERRORS IN HEADER %s peer=%d\n", SanitizeString(msg.m_command), pfrom->GetId());
        return fMoreWork;
    }
    const std::string& strCommand = msg.m_command;

    // Message size
    unsigned int nMessageSize = msg.m_message_size;

    // Checksum
    CDataStream& vRecv = msg.m_recv;
    if (!msg.m_valid_checksum)
    {
        LogPrint(BCLog::NET, "%s(%s, %u bytes): CHECKSUM ERROR peer=%d\n", __func__,
           SanitizeString(strCommand), nMessageSize, pfrom->GetId());
        return fMoreWork;
    }

    // Process message
    bool fRet = false;
    try
    {
        fRet = ProcessMessage(pfrom, strCommand, vRecv, msg.m_time, chainparams, connman, m_banman, interruptMsgProc);
        if (interruptMsgProc)
            return false;
        if (!pfrom->vRecvGetData.empty())
            fMoreWork = true;
    }
    catch (const std::ios_base::failure& e)
    {
        if (strstr(e.what(), "end of data")) {
            // Allow exceptions from under-length message on vRecv
            LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' caught, normally caused by a message being shorter than its stated length\n", __func__, SanitizeString(strCommand), nMessageSize, e.what());
        } else if (strstr(e.what(), "size too large")) {
            // Allow exceptions from over-long size
            LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' caught\n", __func__, SanitizeString(strCommand), nMessageSize, e.what());
        } else if (strstr(e.what(), "non-canonical ReadCompactSize()")) {
            // Allow exceptions from non-canonical encoding
            LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' caught\n", __func__, SanitizeString(strCommand), nMessageSize, e.what());
        } else if (strstr(e.what(), "Superfluous witness record")) {
            // Allow exceptions from illegal witness encoding
            LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' caught\n", __func__, SanitizeString(strCommand), nMessageSize, e.what());
        } else if (strstr(e.what(), "Unknown transaction optional data")) {
            // Allow exceptions from unknown witness encoding
            LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' caught\n", __func__, SanitizeString(strCommand), nMessageSize, e.what());
        } else {
            PrintExceptionContinue(&e, "ProcessMessages()");
        }
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "ProcessMessages()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "ProcessMessages()");
    }

    if (!fRet) {
        LogPrint(BCLog::NET, "%s(%s, %u bytes) FAILED peer=%d\n", __func__, SanitizeString(strCommand), nMessageSize, pfrom->GetId());
    }

    LOCK(cs_main);
    CheckIfBanned(pfrom);

    return fMoreWork;
}

void PeerLogicValidation::ConsiderEviction(CNode *pto, int64_t time_in_seconds)
{
    AssertLockHeld(cs_main);

    CNodeState &state = *State(pto->GetId());
    const CNetMsgMaker msgMaker(pto->GetSendVersion());

    if (!state.m_chain_sync.m_protect && IsOutboundDisconnectionCandidate(pto) && state.fSyncStarted) {
        // This is an outbound peer subject to disconnection if they don't
        // announce a block with as much work as the current tip within
        // CHAIN_SYNC_TIMEOUT + HEADERS_RESPONSE_TIME seconds (note: if
        // their chain has more work than ours, we should sync to it,
        // unless it's invalid, in which case we should find that out and
        // disconnect from them elsewhere).
        if (state.pindexBestKnownBlock != nullptr && state.pindexBestKnownBlock->nChainWork >= ::ChainActive().Tip()->nChainWork) {
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
            state.m_chain_sync.m_work_header = ::ChainActive().Tip();
            state.m_chain_sync.m_sent_getheaders = false;
        } else if (state.m_chain_sync.m_timeout > 0 && time_in_seconds > state.m_chain_sync.m_timeout) {
            // No evidence yet that our peer has synced to a chain with work equal to that
            // of our tip, when we first detected it was behind. Send a single getheaders
            // message to give the peer a chance to update us.
            if (state.m_chain_sync.m_sent_getheaders) {
                // They've run out of time to catch up!
                LogPrintf("Disconnecting outbound peer %d for old chain, best known block = %s\n", pto->GetId(), state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>");
                pto->fDisconnect = true;
            } else {
                assert(state.m_chain_sync.m_work_header);
                LogPrint(BCLog::NET, "sending getheaders to outbound peer=%d to verify chain work (current best known block:%s, benchmark blockhash: %s)\n", pto->GetId(), state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>", state.m_chain_sync.m_work_header->GetBlockHash().ToString());
                connman->PushMessage(pto, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(state.m_chain_sync.m_work_header->pprev), uint256()));
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

void PeerLogicValidation::EvictExtraOutboundPeers(int64_t time_in_seconds)
{
    // Check whether we have too many outbound peers
    int extra_peers = connman->GetExtraOutboundCount();
    if (extra_peers > 0) {
        // If we have more outbound peers than we target, disconnect one.
        // Pick the outbound peer that least recently announced
        // us a new block, with ties broken by choosing the more recent
        // connection (higher node id)
        NodeId worst_peer = -1;
        int64_t oldest_block_announcement = std::numeric_limits<int64_t>::max();

        connman->ForEachNode([&](CNode* pnode) {
            AssertLockHeld(cs_main);

            // Ignore non-outbound peers, or nodes marked for disconnect already
            if (!IsOutboundDisconnectionCandidate(pnode) || pnode->fDisconnect) return;
            CNodeState *state = State(pnode->GetId());
            if (state == nullptr) return; // shouldn't be possible, but just in case
            // Don't evict our protected peers
            if (state->m_chain_sync.m_protect) return;
            // Don't evict our block-relay-only peers.
            if (pnode->m_tx_relay == nullptr) return;
            if (state->m_last_block_announcement < oldest_block_announcement || (state->m_last_block_announcement == oldest_block_announcement && pnode->GetId() > worst_peer)) {
                worst_peer = pnode->GetId();
                oldest_block_announcement = state->m_last_block_announcement;
            }
        });
        if (worst_peer != -1) {
            bool disconnected = connman->ForNode(worst_peer, [&](CNode *pnode) {
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
                connman->SetTryNewOutboundPeer(false);
            }
        }
    }
}

void PeerLogicValidation::CheckForStaleTipAndEvictPeers(const Consensus::Params &consensusParams)
{
    LOCK(cs_main);

    if (connman == nullptr) return;

    int64_t time_in_seconds = GetTime();

    EvictExtraOutboundPeers(time_in_seconds);

    if (time_in_seconds > m_stale_tip_check_time) {
        // Check whether our tip is stale, and if so, allow using an extra
        // outbound peer
        if (!fImporting && !fReindex && connman->GetNetworkActive() && connman->GetUseAddrmanOutgoing() && TipMayBeStale(consensusParams)) {
            LogPrintf("Potential stale tip detected, will try using extra outbound peer (last tip update: %d seconds ago)\n", time_in_seconds - g_last_tip_update);
            connman->SetTryNewOutboundPeer(true);
        } else if (connman->GetTryNewOutboundPeer()) {
            connman->SetTryNewOutboundPeer(false);
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

bool PeerLogicValidation::SendMessages(CNode* pto)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    {
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
            if (pto->nVersion > BIP0031_VERSION) {
                pto->nPingNonceSent = nonce;
                if(pto->nVersion > PING_BESTCHAIN_VERSION) {
                    connman->PushMessage(pto, msgMaker.Make(NetMsgType::PING, nonce, ::ChainActive().Tip()->GetBlockHash()));
                } else {
                    connman->PushMessage(pto, msgMaker.Make(NetMsgType::PING, nonce));
                }
            } else {
                // Peer is too old to support ping command with nonce, pong will never arrive.
                pto->nPingNonceSent = 0;
                connman->PushMessage(pto, msgMaker.Make(NetMsgType::PING));
            }
        }

        TRY_LOCK(cs_main, lockMain);
        if (!lockMain)
            return true;

        if (CheckIfBanned(pto)) return true;

        CNodeState &state = *State(pto->GetId());

        // Address refresh broadcast
        int64_t nNow = GetTimeMicros();
        auto current_time = GetTime<std::chrono::microseconds>();

        if (pto->IsAddrRelayPeer() && !::ChainstateActive().IsInitialBlockDownload() && pto->nNextLocalAddrSend < nNow) {
            AdvertiseLocal(pto);
            pto->nNextLocalAddrSend = PoissonNextSend(nNow, AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL);
        }

        //
        // Message: addr
        //
        if (pto->IsAddrRelayPeer() && pto->nNextAddrSend < nNow) {
            pto->nNextAddrSend = PoissonNextSend(nNow, AVG_ADDRESS_BROADCAST_INTERVAL);
            std::vector<CAddress> vAddr;
            vAddr.reserve(pto->vAddrToSend.size());
            assert(pto->m_addr_known);
            for (const CAddress& addr : pto->vAddrToSend)
            {
                if (!pto->m_addr_known->contains(addr.GetKey()))
                {
                    pto->m_addr_known->insert(addr.GetKey());
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than 1000
                    if (vAddr.size() >= 1000)
                    {
                        connman->PushMessage(pto, msgMaker.Make(NetMsgType::ADDR, vAddr));
                        vAddr.clear();
                    }
                }
            }
            pto->vAddrToSend.clear();
            if (!vAddr.empty())
                connman->PushMessage(pto, msgMaker.Make(NetMsgType::ADDR, vAddr));
            // we only send the big addr message once
            if (pto->vAddrToSend.capacity() > 40)
                pto->vAddrToSend.shrink_to_fit();
        }

        // Start block sync
        if (pindexBestHeader == nullptr)
            pindexBestHeader = ::ChainActive().Tip();
        bool fFetch = state.fPreferredDownload || (nPreferredDownload == 0 && !pto->fClient && !pto->fOneShot); // Download if this is a nice peer, or we have no nice peers and this one might do.
        if (!state.fSyncStarted && !pto->fClient && !fImporting && !fReindex) {
            // Only actively request headers from a single peer, unless we're close to today.
            if ((nSyncStarted == 0 && fFetch) || pindexBestHeader->GetBlockTime() > GetAdjustedTime() - 24 * 60 * 60) {
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
                LogPrint(BCLog::NET, "initial getheaders (%d) to peer=%d (startheight:%d)\n", pindexStart->nHeight, pto->GetId(), pto->nStartingHeight);
                connman->PushMessage(pto, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(pindexStart), uint256()));
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
            LOCK(pto->cs_inventory);
            std::vector<CBlock> vHeaders;
            bool fRevertToInv = ((!state.fPreferHeaders &&
                                 (!state.fPreferHeaderAndIDs || pto->vBlockHashesToAnnounce.size() > 1)) ||
                                pto->vBlockHashesToAnnounce.size() > MAX_BLOCKS_TO_ANNOUNCE);
            const CBlockIndex *pBestIndex = nullptr; // last header queued for delivery
            ProcessBlockAvailability(pto->GetId()); // ensure pindexBestKnownBlock is up-to-date

            if (!fRevertToInv) {
                bool fFoundStartingHeader = false;
                // Try to find first header that our peer doesn't have, and
                // then send all headers past that one.  If we come across any
                // headers that aren't on ::ChainActive(), give up.
                for (const uint256 &hash : pto->vBlockHashesToAnnounce) {
                    const CBlockIndex* pindex = LookupBlockIndex(hash);
                    assert(pindex);
                    if (::ChainActive()[pindex->nHeight] != pindex) {
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
                                connman->PushMessage(pto, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, *most_recent_compact_block));
                            else {
                                CBlockHeaderAndShortTxIDs cmpctblock(*most_recent_block, state.fWantsCmpctWitness);
                                connman->PushMessage(pto, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, cmpctblock));
                            }
                            fGotBlockFromCache = true;
                        }
                    }
                    if (!fGotBlockFromCache) {
                        CBlock block;
                        bool ret = ReadBlockFromDisk(block, pBestIndex, consensusParams);
                        assert(ret);
                        CBlockHeaderAndShortTxIDs cmpctblock(block, state.fWantsCmpctWitness);
                        connman->PushMessage(pto, msgMaker.Make(nSendFlags, NetMsgType::CMPCTBLOCK, cmpctblock));
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
                    connman->PushMessage(pto, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
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
                    const CBlockIndex* pindex = LookupBlockIndex(hashToAnnounce);
                    assert(pindex);

                    // Warn if we're announcing a block that is not on the main chain.
                    // This should be very rare and could be optimized out.
                    // Just log for now.
                    if (::ChainActive()[pindex->nHeight] != pindex) {
                        LogPrint(BCLog::NET, "Announcing block %s not on main chain (tip=%s)\n",
                            hashToAnnounce.ToString(), ::ChainActive().Tip()->GetBlockHash().ToString());
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
            LOCK(pto->cs_inventory);
            vInv.reserve(std::max<size_t>(pto->vInventoryBlockToSend.size(), INVENTORY_BROADCAST_MAX));

            // Add blocks
            for (const uint256& hash : pto->vInventoryBlockToSend) {
                vInv.push_back(CInv(MSG_BLOCK, hash));
                if (vInv.size() == MAX_INV_SZ) {
                    connman->PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                    vInv.clear();
                }
            }
            pto->vInventoryBlockToSend.clear();

            if (pto->m_tx_relay != nullptr) {
                LOCK(pto->m_tx_relay->cs_tx_inventory);
                // Check whether periodic sends should happen
                bool fSendTrickle = pto->HasPermission(PF_NOBAN);
                if (pto->m_tx_relay->nNextInvSend < current_time) {
                    fSendTrickle = true;
                    if (pto->fInbound) {
                        pto->m_tx_relay->nNextInvSend = std::chrono::microseconds{connman->PoissonNextSendInbound(nNow, INVENTORY_BROADCAST_INTERVAL)};
                    } else {
                        // Use half the delay for outbound peers, as there is less privacy concern for them.
                        pto->m_tx_relay->nNextInvSend = PoissonNextSend(current_time, std::chrono::seconds{INVENTORY_BROADCAST_INTERVAL >> 1});
                    }
                }

                // Time to send but the peer has requested we not relay transactions.
                if (fSendTrickle) {
                    LOCK(pto->m_tx_relay->cs_filter);
                    if (!pto->m_tx_relay->fRelayTxes) pto->m_tx_relay->setInventoryTxToSend.clear();
                }

                // Respond to BIP35 mempool requests
                if (fSendTrickle && pto->m_tx_relay->fSendMempool) {
                    auto vtxinfo = mempool.infoAll();
                    pto->m_tx_relay->fSendMempool = false;
                    CFeeRate filterrate;
                    {
                        LOCK(pto->m_tx_relay->cs_feeFilter);
                        filterrate = CFeeRate(pto->m_tx_relay->minFeeFilter);
                    }

                    LOCK(pto->m_tx_relay->cs_filter);

                    for (const auto& txinfo : vtxinfo) {
                        const uint256& hash = txinfo.tx->GetHash();
                        CInv inv(MSG_TX, hash);
                        pto->m_tx_relay->setInventoryTxToSend.erase(hash);
                        // Don't send transactions that peers will not put into their mempool
                        if (txinfo.fee < filterrate.GetFee(txinfo.vsize)) {
                            continue;
                        }
                        if (pto->m_tx_relay->pfilter) {
                            if (!pto->m_tx_relay->pfilter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        }
                        pto->m_tx_relay->filterInventoryKnown.insert(hash);
                        vInv.push_back(inv);
                        if (vInv.size() == MAX_INV_SZ) {
                            connman->PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
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
                    CFeeRate filterrate;
                    {
                        LOCK(pto->m_tx_relay->cs_feeFilter);
                        filterrate = CFeeRate(pto->m_tx_relay->minFeeFilter);
                    }
                    // Topologically and fee-rate sort the inventory we send for privacy and priority reasons.
                    // A heap is used so that not all items need sorting if only a few are being sent.
                    CompareInvMempoolOrder compareInvMempoolOrder(&mempool);
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
                        // Remove it from the to-be-sent set
                        pto->m_tx_relay->setInventoryTxToSend.erase(it);
                        // Check if not in the filter already
                        if (pto->m_tx_relay->filterInventoryKnown.contains(hash)) {
                            continue;
                        }
                        // Not in the mempool anymore? don't bother sending it.
                        auto txinfo = mempool.info(hash);
                        if (!txinfo.tx) {
                            continue;
                        }
                        // Peer told you to not send transactions at that feerate? Don't bother sending it.
                        if (txinfo.fee < filterrate.GetFee(txinfo.vsize)) {
                            continue;
                        }
                        if (pto->m_tx_relay->pfilter && !pto->m_tx_relay->pfilter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        // Send
                        vInv.push_back(CInv(MSG_TX, hash));
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
                        if (vInv.size() == MAX_INV_SZ) {
                            connman->PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                            vInv.clear();
                        }
                        pto->m_tx_relay->filterInventoryKnown.insert(hash);
                    }
                }
            }
        }
        if (!vInv.empty())
            connman->PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));

        // VeriBlock offer Pop Data
        if (VeriBlock::isPopEnabled()) {
            VeriBlock::p2p::offerPopData<altintegration::ATV>(pto, connman, msgMaker);
            VeriBlock::p2p::offerPopData<altintegration::VTB>(pto, connman, msgMaker);
            VeriBlock::p2p::offerPopData<altintegration::VbkBlock>(pto, connman, msgMaker);
        }

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
            if (pindexBestHeader->GetBlockTime() <= GetAdjustedTime() - 24*60*60) {
                if (nNow > state.nHeadersSyncTimeout && nSyncStarted == 1 && (nPreferredDownload - state.fPreferredDownload >= 1)) {
                    // Disconnect a (non-whitelisted) peer if it is our only sync peer,
                    // and we have others we could be using instead.
                    // Note: If all our peers are inbound, then we won't
                    // disconnect our sync peer for stalling; we have bigger
                    // problems if we can't get any outbound peers.
                    if (!pto->HasPermission(PF_NOBAN)) {
                        LogPrintf("Timeout downloading headers from peer=%d, disconnecting\n", pto->GetId());
                        pto->fDisconnect = true;
                        return true;
                    } else {
                        LogPrintf("Timeout downloading headers from whitelisted peer=%d, not disconnecting\n", pto->GetId());
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
        ConsiderEviction(pto, GetTime());

        //
        // Message: getdata (blocks)
        //
        std::vector<CInv> vGetData;
        if (!pto->fClient && ((fFetch && !pto->m_limited_node) || !::ChainstateActive().IsInitialBlockDownload()) && state.nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            std::vector<const CBlockIndex*> vToDownload;
            NodeId staller = -1;
            // VeriBlock: find "blocks to download" in 2 chains: one that has "best chainwork", and second that is reported by peer as best.
            ProcessBlockAvailability(pto->GetId());
            // always download chain with higher chainwork
            if(state.pindexBestKnownBlock) {
                FindNextBlocksToDownload(pto->GetId(), MAX_BLOCKS_IN_TRANSIT_PER_PEER - state.nBlocksInFlight, vToDownload, staller, consensusParams, state.pindexBestKnownBlock, &state.pindexLastCommonBlock);
            }
            // should we fetch announced chain?
            if(state.pindexLastAnnouncedBlock && state.pindexBestKnownBlock) {
                // last announced block is by definition always <= chainwork than best known block by chainwork
                assert(state.pindexLastAnnouncedBlock->nChainWork <= state.pindexBestKnownBlock->nChainWork);

                // are they in the same chain?
                if (state.pindexBestKnownBlock->GetAncestor(state.pindexLastAnnouncedBlock->nHeight) != state.pindexLastAnnouncedBlock) {
                    // no, additionally sync 'announced' chain
                    LogPrint(BCLog::NET, "Requesting announced best chain %d:%s from peer=%d\n", state.pindexLastAnnouncedBlock->GetBlockHash().ToString(),
                        state.pindexLastAnnouncedBlock->nHeight, pto->GetId());
                    FindNextBlocksToDownload(pto->GetId(), MAX_BLOCKS_IN_TRANSIT_PER_PEER - state.nBlocksInFlight, vToDownload, staller, consensusParams, state.pindexLastAnnouncedBlock, &state.pindexLastCommonAnnouncedBlock);
                }
            }
            for (const CBlockIndex *pindex : vToDownload) {
                uint32_t nFetchFlags = GetFetchFlags(pto);
                vGetData.push_back(CInv(MSG_BLOCK | nFetchFlags, pindex->GetBlockHash()));
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
        // we can resume downloading transactions from a peer even if they
        // were unresponsive in the past.
        // Eventually we should consider disconnecting peers, but this is
        // conservative.
        if (state.m_tx_download.m_check_expiry_timer <= current_time) {
            for (auto it=state.m_tx_download.m_tx_in_flight.begin(); it != state.m_tx_download.m_tx_in_flight.end();) {
                if (it->second <= current_time - TX_EXPIRY_INTERVAL) {
                    LogPrint(BCLog::NET, "timeout of inflight tx %s from peer=%d\n", it->first.ToString(), pto->GetId());
                    state.m_tx_download.m_tx_announced.erase(it->first);
                    state.m_tx_download.m_tx_in_flight.erase(it++);
                } else {
                    ++it;
                }
            }
            // On average, we do this check every TX_EXPIRY_INTERVAL. Randomize
            // so that we're not doing this for all peers at the same time.
            state.m_tx_download.m_check_expiry_timer = current_time + TX_EXPIRY_INTERVAL / 2 + GetRandMicros(TX_EXPIRY_INTERVAL);
        }

        auto& tx_process_time = state.m_tx_download.m_tx_process_time;
        while (!tx_process_time.empty() && tx_process_time.begin()->first <= current_time && state.m_tx_download.m_tx_in_flight.size() < MAX_PEER_TX_IN_FLIGHT) {
            const uint256 txid = tx_process_time.begin()->second;
            // Erase this entry from tx_process_time (it may be added back for
            // processing at a later time, see below)
            tx_process_time.erase(tx_process_time.begin());
            CInv inv(MSG_TX | GetFetchFlags(pto), txid);
            if (!AlreadyHave(inv)) {
                // If this transaction was last requested more than 1 minute ago,
                // then request.
                const auto last_request_time = GetTxRequestTime(inv.hash);
                if (last_request_time <= current_time - GETDATA_TX_INTERVAL) {
                    LogPrint(BCLog::NET, "Requesting %s peer=%d\n", inv.ToString(), pto->GetId());
                    vGetData.push_back(inv);
                    if (vGetData.size() >= MAX_GETDATA_SZ) {
                        connman->PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));
                        vGetData.clear();
                    }
                    UpdateTxRequestTime(inv.hash, current_time);
                    state.m_tx_download.m_tx_in_flight.emplace(inv.hash, current_time);
                } else {
                    // This transaction is in flight from someone else; queue
                    // up processing to happen after the download times out
                    // (with a slight delay for inbound peers, to prefer
                    // requests to outbound peers).
                    const auto next_process_time = CalculateTxGetDataTime(txid, current_time, !state.fPreferredDownload);
                    tx_process_time.emplace(next_process_time, txid);
                }
            } else {
                // We have already seen this transaction, no need to download.
                state.m_tx_download.m_tx_announced.erase(inv.hash);
                state.m_tx_download.m_tx_in_flight.erase(inv.hash);
            }
        }


        if (!vGetData.empty())
            connman->PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));

        //
        // Message: feefilter
        //
        // We don't want white listed peers to filter txs to us if we have -whitelistforcerelay
        if (pto->m_tx_relay != nullptr && pto->nVersion >= FEEFILTER_VERSION && gArgs.GetBoolArg("-feefilter", DEFAULT_FEEFILTER) &&
            !pto->HasPermission(PF_FORCERELAY)) {
            CAmount currentFilter = mempool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFeePerK();
            int64_t timeNow = GetTimeMicros();
            if (timeNow > pto->m_tx_relay->nextSendTimeFeeFilter) {
                static CFeeRate default_feerate(DEFAULT_MIN_RELAY_TX_FEE);
                static FeeFilterRounder filterRounder(default_feerate);
                CAmount filterToSend = filterRounder.round(currentFilter);
                // We always have a fee filter of at least minRelayTxFee
                filterToSend = std::max(filterToSend, ::minRelayTxFee.GetFeePerK());
                if (filterToSend != pto->m_tx_relay->lastSentFeeFilter) {
                    connman->PushMessage(pto, msgMaker.Make(NetMsgType::FEEFILTER, filterToSend));
                    pto->m_tx_relay->lastSentFeeFilter = filterToSend;
                }
                pto->m_tx_relay->nextSendTimeFeeFilter = PoissonNextSend(timeNow, AVG_FEEFILTER_BROADCAST_INTERVAL);
            }
            // If the fee filter has changed substantially and it's still more than MAX_FEEFILTER_CHANGE_DELAY
            // until scheduled broadcast, then move the broadcast to within MAX_FEEFILTER_CHANGE_DELAY.
            else if (timeNow + MAX_FEEFILTER_CHANGE_DELAY * 1000000 < pto->m_tx_relay->nextSendTimeFeeFilter &&
                     (currentFilter < 3 * pto->m_tx_relay->lastSentFeeFilter / 4 || currentFilter > 4 * pto->m_tx_relay->lastSentFeeFilter / 3)) {
                pto->m_tx_relay->nextSendTimeFeeFilter = timeNow + GetRandInt(MAX_FEEFILTER_CHANGE_DELAY) * 1000000;
            }
        }
    }

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
    }
};
static CNetProcessingCleanup instance_of_cnetprocessingcleanup;
