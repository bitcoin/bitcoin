// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Copyright (c) 2016 Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include "addrman.h"
#include "arith_uint256.h"
#include "buip055fork.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "checkqueue.h"
#include "connmgr.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "dosman.h"
#include "expedited.h"
#include "hash.h"
#include "init.h"
#include "merkleblock.h"
#include "net.h"
#include "nodestate.h"
#include "parallel.h"
#include "policy/policy.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "requestManager.h"
#include "script/script.h"
#include "script/sigcache.h"
#include "script/standard.h"
#include "thinblock.h"
#include "tinyformat.h"
#include "txdb.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "undo.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"
#include "validationinterface.h"
#include "versionbits.h"

#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread.hpp>
#include <sstream>

#if defined(NDEBUG)
#error "Bitcoin cannot be compiled without assertions."
#endif

/**
 * Global state
 */

// BU variables moved to globals.cpp
// BU moved CCriticalSection cs_main;

// BU moved BlockMap mapBlockIndex;
// BU movedCChain chainActive;
CBlockIndex *pindexBestHeader = NULL;
int64_t nTimeBestReceived = 0;
// BU moved CWaitableCriticalSection csBestBlock;
// BU moved CConditionVariable cvBlockChange;
bool fImporting = false;
bool fReindex = false;
bool fTxIndex = false;
bool fHavePruned = false;
bool fPruneMode = false;
bool fIsBareMultisigStd = DEFAULT_PERMIT_BAREMULTISIG;
bool fRequireStandard = true;
unsigned int nBytesPerSigOp = DEFAULT_BYTES_PER_SIGOP;
bool fCheckBlockIndex = false;
bool fCheckpointsEnabled = DEFAULT_CHECKPOINTS_ENABLED;
size_t nCoinCacheUsage = 5000 * 300;
uint64_t nPruneTarget = 0;

CFeeRate minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);

// BU: Move global objects to a single file
extern CTxMemPool mempool;

// BU: change locking of orphan map from using cs_main to cs_orphancache.
// There is too much dependance on cs_main locks which are generally too
// broad in scope.
// Move globals to a single file
extern CCriticalSection cs_orphancache;
extern std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(cs_orphancache);
extern std::map<uint256, std::set<uint256> > mapOrphanTransactionsByPrev GUARDED_BY(cs_orphancache);

int64_t nLastOrphanCheck = GetTime(); // Used in EraseOrphansByTime()
static uint64_t nBytesOrphanPool = 0; // Current in memory size of the orphan pool.

// BU: start block download at low numbers in case our peers are slow when we start
/** Number of blocks that can be requested at any given time from a single peer. */
static unsigned int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 1;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and in the future perhaps pruning
 *  harder). We'll probably want to make this a per-peer adaptive value at some point. */
static unsigned int BLOCK_DOWNLOAD_WINDOW = 256;

extern CTweak<unsigned int> maxBlocksInTransitPerPeer; // override the above
extern CTweak<unsigned int> blockDownloadWindow;
extern CTweak<uint64_t> reindexTypicalBlockSize;

extern std::map<CNetAddr, ConnectionHistory> mapInboundConnectionTracker;
extern CCriticalSection cs_mapInboundConnectionTracker;

/** A cache to store headers that have arrived but can not yet be connected **/
std::map<uint256, std::pair<CBlockHeader, int64_t> > mapUnConnectedHeaders;

/**
 * Returns true if there are nRequired or more blocks of minVersion or above
 * in the last Consensus::Params::nMajorityWindow blocks, starting at pstart and going backwards.
 */
static bool IsSuperMajority(int minVersion,
    const CBlockIndex *pstart,
    unsigned nRequired,
    const Consensus::Params &consensusParams);
static void CheckBlockIndex(const Consensus::Params &consensusParams);

/** Constant stuff for coinbase transactions we create: */
CScript COINBASE_FLAGS;

const std::string strMessageMagic = "Bitcoin Signed Message:\n";

extern CStatHistory<uint64_t> nTxValidationTime;
extern CStatHistory<uint64_t> nBlockValidationTime;
extern CCriticalSection cs_blockvalidationtime;

extern CCriticalSection cs_LastBlockFile;
extern CCriticalSection cs_nBlockSequenceId;


// Internal stuff
namespace
{
struct CBlockIndexWorkComparator
{
    bool operator()(CBlockIndex *pa, CBlockIndex *pb) const
    {
        // First sort by most total work, ...
        if (pa->nChainWork > pb->nChainWork)
            return false;
        if (pa->nChainWork < pb->nChainWork)
            return true;

        // ... then by earliest time received, ...
        if (pa->nSequenceId < pb->nSequenceId)
            return false;
        if (pa->nSequenceId > pb->nSequenceId)
            return true;

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those all have id 0).
        if (pa < pb)
            return false;
        if (pa > pb)
            return true;

        // Identical blocks.
        return false;
    }
};

CBlockIndex *pindexBestInvalid;

/**
 * The set of all CBlockIndex entries with BLOCK_VALID_TRANSACTIONS (for itself and all ancestors) and
 * as good as our current tip or better. Entries may be failed, though, and pruning nodes may be
 * missing the data for the block.
 */
std::set<CBlockIndex *, CBlockIndexWorkComparator> setBlockIndexCandidates;
/** Number of nodes with fSyncStarted. */
int nSyncStarted = 0;
/** All pairs A->B, where A (or one of its ancestors) misses transactions, but B has transactions.
 * Pruned nodes may have entries where B is missing data.
 */
std::multimap<CBlockIndex *, CBlockIndex *> mapBlocksUnlinked;

std::vector<CBlockFileInfo> vinfoBlockFile;
int nLastBlockFile = 0;
/** Global flag to indicate we should check to see if there are
 *  block/undo files that should be deleted.  Set on startup
 *  or if we allocate more file space when we're in prune mode
 */
bool fCheckForPruning = false;

/**
 * Every received block is assigned a unique and increasing identifier, so we
 * know which one to give priority in case of a fork.
 */
/** Blocks loaded from disk are assigned id 0, so start the counter at 1. */
uint32_t nBlockSequenceId = 1;

/**
 * Sources of received blocks, saved to be able to send them reject
 * messages or ban them when processing happens afterwards. Protected by
 * cs_main.
 */
std::map<uint256, NodeId> mapBlockSource;

/**
 * Filter for transactions that were recently rejected by
 * AcceptToMemoryPool. These are not rerequested until the chain tip
 * changes, at which point the entire filter is reset. Protected by
 * cs_main.
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
 * Memory used: 1.7MB
 */
boost::scoped_ptr<CRollingBloomFilter> recentRejects;
uint256 hashRecentRejectsChainTip;

/** Number of preferable block download peers. */
int nPreferredDownload = 0;

/** Dirty block file entries. */
std::set<int> setDirtyFileInfo;

/** Number of peers from which we're downloading blocks. */
int nPeersWithValidatedDownloads = 0;
} // anon namespace

/** Dirty block index entries. */
std::set<CBlockIndex *> setDirtyBlockIndex;

//////////////////////////////////////////////////////////////////////////////
//
// Registration of network node signals.
//

namespace
{
int GetHeight()
{
    LOCK(cs_main);
    return chainActive.Height();
}

void UpdatePreferredDownload(CNode *node, CNodeState *state)
{
    nPreferredDownload -= state->fPreferredDownload;

    // Whether this node should be marked as a preferred download node.
    state->fPreferredDownload = (!node->fInbound || node->fWhitelisted) && !node->fOneShot && !node->fClient;

    nPreferredDownload += state->fPreferredDownload;
}

void InitializeNode(NodeId nodeid, const CNode *pnode)
{
    LOCK(cs_main);
    CNodeState &state = mapNodeState.insert(std::make_pair(nodeid, CNodeState())).first->second;
    state.name = pnode->addrName;
    state.address = pnode->addr;
}

void FinalizeNode(NodeId nodeid)
{
    LOCK(cs_main);
    CNodeState *state = State(nodeid);

    if (state->fSyncStarted)
        nSyncStarted--;

    BOOST_FOREACH (const QueuedBlock &entry, state->vBlocksInFlight)
    {
        mapBlocksInFlight.erase(entry.hash);
    }
    nPreferredDownload -= state->fPreferredDownload;
    nPeersWithValidatedDownloads -= (state->nBlocksInFlightValidHeaders != 0);
    DbgAssert(nPeersWithValidatedDownloads >= 0, nPeersWithValidatedDownloads = 0);

    mapNodeState.erase(nodeid);

    if (mapNodeState.empty())
    {
        // Do a consistency check after the last peer is removed.  Force consistent state if production code
        DbgAssert(mapBlocksInFlight.empty(), mapBlocksInFlight.clear());
        DbgAssert(nPreferredDownload == 0, nPreferredDownload = 0);
        DbgAssert(nPeersWithValidatedDownloads == 0, nPeersWithValidatedDownloads = 0);
    }
}

// Requires cs_main.
// Returns a bool indicating whether we requested this block.
bool MarkBlockAsReceived(const uint256 &hash)
{
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator itInFlight =
        mapBlocksInFlight.find(hash);
    if (itInFlight != mapBlocksInFlight.end())
    {
        // BUIP010 Xtreme Thinblocks: begin section
        int64_t getdataTime = itInFlight->second.second->nTime;
        int64_t now = GetTimeMicros();
        double nResponseTime = (double)(now - getdataTime) / 1000000.0;

        // BU:  calculate avg block response time over last 20 blocks to be used for IBD tuning
        // start at a higher number so that we don't start jamming IBD when we restart a node sync
        static double avgResponseTime = 5;
        static uint8_t blockRange = 20;
        if (avgResponseTime > 0)
            avgResponseTime -= (avgResponseTime / blockRange);
        avgResponseTime += nResponseTime / blockRange;
        if (avgResponseTime < 0.2)
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = 32;
        }
        else if (avgResponseTime < 0.5)
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
        }
        else if (avgResponseTime < 0.9)
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = 8;
        }
        else if (avgResponseTime < 1.4)
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = 4;
        }
        else if (avgResponseTime < 2.0)
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = 2;
        }
        else
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = 1;
        }

        LogPrint("thin", "Received block %s in %.2f seconds\n", hash.ToString(), nResponseTime);
        LogPrint("thin", "Average block response time is %.2f seconds\n", avgResponseTime);
        if (maxBlocksInTransitPerPeer.value != 0)
        {
            MAX_BLOCKS_IN_TRANSIT_PER_PEER = maxBlocksInTransitPerPeer.value;
        }
        if (blockDownloadWindow.value != 0)
        {
            BLOCK_DOWNLOAD_WINDOW = blockDownloadWindow.value;
        }
        LogPrint("thin", "BLOCK_DOWNLOAD_WINDOW is %d MAX_BLOCKS_IN_TRANSIT_PER_PEER is %d\n", BLOCK_DOWNLOAD_WINDOW,
            MAX_BLOCKS_IN_TRANSIT_PER_PEER);

        {
            LOCK(cs_vNodes);
            BOOST_FOREACH (CNode *pnode, vNodes)
            {
                if (pnode->mapThinBlocksInFlight.size() > 0)
                {
                    LOCK(pnode->cs_mapthinblocksinflight);
                    if (pnode->mapThinBlocksInFlight.count(hash))
                    {
                        // Only update thinstats if this is actually a thinblock and not a regular block.
                        // Sometimes we request a thinblock but then revert to requesting a regular block
                        // as can happen when the thinblock preferential timer is exceeded.
                        thindata.UpdateResponseTime(nResponseTime);
                        break;
                    }
                }
            }
        }
        // BUIP010 Xtreme Thinblocks: end section
        CNodeState *state = State(itInFlight->second.first);
        state->nBlocksInFlightValidHeaders -= itInFlight->second.second->fValidatedHeaders;
        if (state->nBlocksInFlightValidHeaders == 0 && itInFlight->second.second->fValidatedHeaders)
        {
            // Last validated block on the queue was received.
            nPeersWithValidatedDownloads--;
        }
        if (state->vBlocksInFlight.begin() == itInFlight->second.second)
        {
            // First block on the queue was received, update the start download time for the next one
            state->nDownloadingSince = std::max(state->nDownloadingSince, GetTimeMicros());
        }
        state->vBlocksInFlight.erase(itInFlight->second.second);
        state->nBlocksInFlight--;
        mapBlocksInFlight.erase(itInFlight);
        return true;
    }
    return false;
}

// BU MarkBlockAsInFlight moved out of anonymous namespace

/** Check whether the last unknown block a peer advertised is not yet known. */
void ProcessBlockAvailability(NodeId nodeid)
{
    CNodeState *state = State(nodeid);
    DbgAssert(state != NULL, return ); // node already destructed, nothing to do in production mode

    if (!state->hashLastUnknownBlock.IsNull())
    {
        BlockMap::iterator itOld = mapBlockIndex.find(state->hashLastUnknownBlock);
        if (itOld != mapBlockIndex.end() && itOld->second->nChainWork > 0)
        {
            if (state->pindexBestKnownBlock == NULL ||
                itOld->second->nChainWork >= state->pindexBestKnownBlock->nChainWork)
                state->pindexBestKnownBlock = itOld->second;
            state->hashLastUnknownBlock.SetNull();
        }
    }
}


// Requires cs_main
bool PeerHasHeader(CNodeState *state, CBlockIndex *pindex)
{
    if (state->pindexBestKnownBlock && pindex == state->pindexBestKnownBlock->GetAncestor(pindex->nHeight))
        return true;
    if (state->pindexBestHeaderSent && pindex == state->pindexBestHeaderSent->GetAncestor(pindex->nHeight))
        return true;
    return false;
}

/** Find the last common ancestor two blocks have.
 *  Both pa and pb must be non-NULL. */
CBlockIndex *LastCommonAncestor(CBlockIndex *pa, CBlockIndex *pb)
{
    if (pa->nHeight > pb->nHeight)
    {
        pa = pa->GetAncestor(pb->nHeight);
    }
    else if (pb->nHeight > pa->nHeight)
    {
        pb = pb->GetAncestor(pa->nHeight);
    }

    while (pa != pb && pa && pb)
    {
        pa = pa->pprev;
        pb = pb->pprev;
    }

    // Eventually all chain branches meet at the genesis block.
    assert(pa == pb);
    return pa;
}

/** Update pindexLastCommonBlock and add not-in-flight missing successors to vBlocks, until it has
 *  at most count entries. */
static void FindNextBlocksToDownload(NodeId nodeid, unsigned int count, std::vector<CBlockIndex *> &vBlocks)
{
    if (count == 0)
        return;

    vBlocks.reserve(vBlocks.size() + count);
    CNodeState *state = State(nodeid);
    DbgAssert(state != NULL, return );

    // Make sure pindexBestKnownBlock is up to date, we'll need it.
    ProcessBlockAvailability(nodeid);

    if (state->pindexBestKnownBlock == NULL || state->pindexBestKnownBlock->nChainWork < chainActive.Tip()->nChainWork)
    {
        // This peer has nothing interesting.
        return;
    }

    if (state->pindexLastCommonBlock == NULL)
    {
        // Bootstrap quickly by guessing a parent of our best tip is the forking point.
        // Guessing wrong in either direction is not a problem.
        state->pindexLastCommonBlock =
            chainActive[std::min(state->pindexBestKnownBlock->nHeight, chainActive.Height())];
    }

    // If the peer reorganized, our previous pindexLastCommonBlock may not be an ancestor
    // of its current tip anymore. Go back enough to fix that.
    state->pindexLastCommonBlock = LastCommonAncestor(state->pindexLastCommonBlock, state->pindexBestKnownBlock);
    if (state->pindexLastCommonBlock == state->pindexBestKnownBlock)
        return;

    std::vector<CBlockIndex *> vToFetch;
    CBlockIndex *pindexWalk = state->pindexLastCommonBlock;
    // Never fetch further than the current chain tip + the block download window.  We need to ensure
    // the if running in pruning mode we don't download too many blocks ahead and as a result use to
    // much disk space to store unconnected blocks.
    int nWindowEnd = chainActive.Height() + BLOCK_DOWNLOAD_WINDOW;

    int nMaxHeight = std::min<int>(state->pindexBestKnownBlock->nHeight, nWindowEnd + 1);
    while (pindexWalk->nHeight < nMaxHeight)
    {
        // Read up to 128 (or more, if more blocks than that are needed) successors of pindexWalk (towards
        // pindexBestKnownBlock) into vToFetch. We fetch 128, because CBlockIndex::GetAncestor may be as expensive
        // as iterating over ~100 CBlockIndex* entries anyway.
        int nToFetch = std::min(nMaxHeight - pindexWalk->nHeight, std::max<int>(count - vBlocks.size(), 128));
        vToFetch.resize(nToFetch);
        pindexWalk = state->pindexBestKnownBlock->GetAncestor(pindexWalk->nHeight + nToFetch);
        vToFetch[nToFetch - 1] = pindexWalk;
        for (unsigned int i = nToFetch - 1; i > 0; i--)
        {
            vToFetch[i - 1] = vToFetch[i]->pprev;
        }

        // Iterate over those blocks in vToFetch (in forward direction), adding the ones that
        // are not yet downloaded and not in flight to vBlocks. In the mean time, update
        // pindexLastCommonBlock as long as all ancestors are already downloaded, or if it's
        // already part of our chain (and therefore don't need it even if pruned).
        BOOST_FOREACH (CBlockIndex *pindex, vToFetch)
        {
            if (!pindex->IsValid(BLOCK_VALID_TREE))
            {
                // We consider the chain that this peer is on invalid.
                return;
            }
            if (pindex->nStatus & BLOCK_HAVE_DATA || chainActive.Contains(pindex))
            {
                if (pindex->nChainTx)
                    state->pindexLastCommonBlock = pindex;
            }
            else
            {
                // Return if we've reached the end of the download window.
                if (pindex->nHeight > nWindowEnd)
                {
                    return;
                }

                // Return if we've reached the end of the number of blocks we can download for this peer.
                vBlocks.push_back(pindex);
                if (vBlocks.size() == count)
                {
                    return;
                }
            }
        }
    }
}

} // anon namespace

/** Update tracking information about which blocks a peer is assumed to have. */
void UpdateBlockAvailability(NodeId nodeid, const uint256 &hash)
{
    CNodeState *state = State(nodeid);
    DbgAssert(state != NULL, return ); // node already destructed, nothing to do in production mode

    ProcessBlockAvailability(nodeid);

    BlockMap::iterator it = mapBlockIndex.find(hash);
    if (it != mapBlockIndex.end() && it->second->nChainWork > 0)
    {
        // An actually better block was announced.
        if (state->pindexBestKnownBlock == NULL || it->second->nChainWork >= state->pindexBestKnownBlock->nChainWork)
            state->pindexBestKnownBlock = it->second;
    }
    else
    {
        // An unknown block was announced; just assume that the latest one is the best one.
        state->hashLastUnknownBlock = hash;
    }
}

void MarkBlockAsInFlight(NodeId nodeid,
    const uint256 &hash,
    const Consensus::Params &consensusParams,
    CBlockIndex *pindex = NULL)
{
    LOCK(cs_main);
    CNodeState *state = State(nodeid);
    DbgAssert(state != NULL, return );

    // If started then clear the thinblock timer used for preferential downloading
    thindata.ClearThinBlockTimer(hash);

    // BU why mark as received? because this erases it from the inflight list.  Instead we'll check for it
    // BU removed: MarkBlockAsReceived(hash);
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator itInFlight =
        mapBlocksInFlight.find(hash);
    if (itInFlight == mapBlocksInFlight.end()) // If it hasn't already been marked inflight...
    {
        int64_t nNow = GetTimeMicros();
        QueuedBlock newentry = {hash, pindex, nNow, pindex != NULL};
        std::list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(), newentry);
        state->nBlocksInFlight++;
        state->nBlocksInFlightValidHeaders += newentry.fValidatedHeaders;
        if (state->nBlocksInFlight == 1)
        {
            // We're starting a block download (batch) from this peer.
            state->nDownloadingSince = GetTimeMicros();
        }
        if (state->nBlocksInFlightValidHeaders == 1 && pindex != NULL)
        {
            nPeersWithValidatedDownloads++;
        }
        mapBlocksInFlight[hash] = std::make_pair(nodeid, it);
    }
}

// Requires cs_main
bool CanDirectFetch(const Consensus::Params &consensusParams)
{
    return chainActive.Tip()->GetBlockTime() > GetAdjustedTime() - consensusParams.nPowTargetSpacing * 20;
}

bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats)
{
    CNodeRef node(connmgr->FindNodeFromId(nodeid));
    if (!node)
        return false;

    LOCK(cs_main);
    CNodeState *state = State(nodeid);
    if (state == NULL)
        return false;
    stats.nMisbehavior = node->nMisbehavior;
    stats.nSyncHeight = state->pindexBestKnownBlock ? state->pindexBestKnownBlock->nHeight : -1;
    stats.nCommonHeight = state->pindexLastCommonBlock ? state->pindexLastCommonBlock->nHeight : -1;
    BOOST_FOREACH (const QueuedBlock &queue, state->vBlocksInFlight)
    {
        if (queue.pindex)
            stats.vHeightInFlight.push_back(queue.pindex->nHeight);
    }
    return true;
}

void RegisterNodeSignals(CNodeSignals &nodeSignals)
{
    nodeSignals.GetHeight.connect(&GetHeight);
    nodeSignals.ProcessMessages.connect(&ProcessMessages);
    nodeSignals.SendMessages.connect(&SendMessages);
    nodeSignals.InitializeNode.connect(&InitializeNode);
    nodeSignals.FinalizeNode.connect(&FinalizeNode);
}

void UnregisterNodeSignals(CNodeSignals &nodeSignals)
{
    nodeSignals.GetHeight.disconnect(&GetHeight);
    nodeSignals.ProcessMessages.disconnect(&ProcessMessages);
    nodeSignals.SendMessages.disconnect(&SendMessages);
    nodeSignals.InitializeNode.disconnect(&InitializeNode);
    nodeSignals.FinalizeNode.disconnect(&FinalizeNode);
}

CBlockIndex *FindForkInGlobalIndex(const CChain &chain, const CBlockLocator &locator)
{
    // Find the first block the caller has in the main chain
    BOOST_FOREACH (const uint256 &hash, locator.vHave)
    {
        BlockMap::iterator mi = mapBlockIndex.find(hash);
        if (mi != mapBlockIndex.end())
        {
            CBlockIndex *pindex = (*mi).second;
            if (chain.Contains(pindex))
                return pindex;
        }
    }
    return chain.Genesis();
}

CCoinsViewCache *pcoinsTip = NULL;
CBlockTreeDB *pblocktree = NULL;

//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//
static bool AlreadyHaveOrphan(uint256 hash)
{
    LOCK(cs_orphancache);
    if (mapOrphanTransactions.count(hash))
        return true;
    return false;
}
bool AddOrphanTx(const CTransaction &tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(cs_orphancache)
{
    AssertLockHeld(cs_orphancache);

    if (mapOrphanTransactions.empty())
        DbgAssert(nBytesOrphanPool == 0, nBytesOrphanPool = 0);

    uint256 hash = tx.GetHash();
    if (mapOrphanTransactions.count(hash))
        return false;

    // Ignore orphans larger than the largest txn size allowed.
    unsigned int sz = tx.GetSerializeSize(SER_NETWORK, CTransaction::CURRENT_VERSION);
    if (sz > MAX_STANDARD_TX_SIZE)
    {
        LogPrint("mempool", "ignoring large orphan tx (size: %u, hash: %s)\n", sz, hash.ToString());
        return false;
    }

    uint64_t txSize = RecursiveDynamicUsage(tx);
    mapOrphanTransactions[hash].tx = tx;
    mapOrphanTransactions[hash].fromPeer = peer;
    mapOrphanTransactions[hash].nEntryTime = GetTime(); // BU - Xtreme Thinblocks;
    mapOrphanTransactions[hash].nOrphanTxSize = txSize;
    BOOST_FOREACH (const CTxIn &txin, tx.vin)
        mapOrphanTransactionsByPrev[txin.prevout.hash].insert(hash);

    nBytesOrphanPool += txSize;
    LogPrint("mempool", "stored orphan tx %s bytes:%ld (mapsz %u prevsz %u), orphan pool bytes:%ld\n", hash.ToString(),
        txSize, mapOrphanTransactions.size(), mapOrphanTransactionsByPrev.size(), nBytesOrphanPool);
    return true;
}

void EraseOrphanTx(uint256 hash) EXCLUSIVE_LOCKS_REQUIRED(cs_orphancache)
{
    AssertLockHeld(cs_orphancache);

    std::map<uint256, COrphanTx>::iterator it = mapOrphanTransactions.find(hash);
    if (it == mapOrphanTransactions.end())
        return;
    BOOST_FOREACH (const CTxIn &txin, it->second.tx.vin)
    {
        std::map<uint256, std::set<uint256> >::iterator itPrev = mapOrphanTransactionsByPrev.find(txin.prevout.hash);
        if (itPrev == mapOrphanTransactionsByPrev.end())
            continue;
        itPrev->second.erase(hash);
        if (itPrev->second.empty())
            mapOrphanTransactionsByPrev.erase(itPrev);
    }

    nBytesOrphanPool -= it->second.nOrphanTxSize;
    LogPrint("mempool", "Erased orphan tx %s of size %ld bytes, orphan pool bytes:%ld\n",
        it->second.tx.GetHash().ToString(), it->second.nOrphanTxSize, nBytesOrphanPool);
    mapOrphanTransactions.erase(it);
}

// BU - Xtreme Thinblocks: begin
void EraseOrphansByTime() EXCLUSIVE_LOCKS_REQUIRED(cs_orphancache)
{
    AssertLockHeld(cs_orphancache);

    // Because we have to iterate through the entire orphan cache which can be large we don't want to check this
    // every time a tx enters the mempool but just once every 5 minutes is good enough.
    if (GetTime() < nLastOrphanCheck + 5 * 60)
        return;
    int64_t nOrphanTxCutoffTime = GetTime() - GetArg("-orphanpoolexpiry", DEFAULT_ORPHANPOOL_EXPIRY) * 60 * 60;
    std::map<uint256, COrphanTx>::iterator iter = mapOrphanTransactions.begin();
    while (iter != mapOrphanTransactions.end())
    {
        std::map<uint256, COrphanTx>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        int64_t nEntryTime = mi->second.nEntryTime;
        if (nEntryTime < nOrphanTxCutoffTime)
        {
            uint256 txHash = mi->second.tx.GetHash();
            LogPrint(
                "mempool", "Erased old orphan tx %s of age %d seconds\n", txHash.ToString(), GetTime() - nEntryTime);
            EraseOrphanTx(txHash);
        }
    }

    nLastOrphanCheck = GetTime();
}
// BU - Xtreme Thinblocks: end

unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans, uint64_t nMaxBytes) EXCLUSIVE_LOCKS_REQUIRED(cs_orphancache)
{
    AssertLockHeld(cs_orphancache);

    // Limit the orphan pool size by either number of transactions or the max orphan pool size allowed.
    // Limiting by pool size to 1/10th the size of the maxmempool alone is not enough because the total number
    // of txns in the pool can adversely effect the size of the bloom filter in a get_xthin message.
    unsigned int nEvicted = 0;
    while (mapOrphanTransactions.size() > nMaxOrphans || nBytesOrphanPool > nMaxBytes)
    {
        // Evict a random orphan:
        uint256 randomhash = GetRandHash();
        std::map<uint256, COrphanTx>::iterator it = mapOrphanTransactions.lower_bound(randomhash);
        if (it == mapOrphanTransactions.end())
            it = mapOrphanTransactions.begin();
        EraseOrphanTx(it->first);
        ++nEvicted;
    }
    return nEvicted;
}

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    BOOST_FOREACH (const CTxIn &txin, tx.vin)
    {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

bool CheckFinalTx(const CTransaction &tx, int flags)
{
    AssertLockHeld(cs_main);

    // By convention a negative value for flags indicates that the
    // current network-enforced consensus rules should be used. In
    // a future soft-fork scenario that would mean checking which
    // rules would be enforced for the next block and setting the
    // appropriate flags. At the present time no soft-forks are
    // scheduled, so no flags are set.
    flags = std::max(flags, 0);

    // CheckFinalTx() uses chainActive.Height()+1 to evaluate
    // nLockTime because when IsFinalTx() is called within
    // CBlock::AcceptBlock(), the height of the block *being*
    // evaluated is what is used. Thus if we want to know if a
    // transaction can be part of the *next* block, we need to call
    // IsFinalTx() with one more than chainActive.Height().
    const int nBlockHeight = chainActive.Height() + 1;

    // BIP113 will require that time-locked transactions have nLockTime set to
    // less than the median time of the previous block they're contained in.
    // When the next block is created its previous block will be the current
    // chain tip, so we use that to calculate the median time passed to
    // IsFinalTx() if LOCKTIME_MEDIAN_TIME_PAST is set.
    const int64_t nBlockTime =
        (flags & LOCKTIME_MEDIAN_TIME_PAST) ? chainActive.Tip()->GetMedianTimePast() : GetAdjustedTime();

    return IsFinalTx(tx, nBlockHeight, nBlockTime);
}

/**
 * Calculates the block height and previous block's median time past at
 * which the transaction will be considered final in the context of BIP 68.
 * Also removes from the vector of input heights any entries which did not
 * correspond to sequence locked inputs as they do not affect the calculation.
 */
static std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx,
    int flags,
    std::vector<int> *prevHeights,
    const CBlockIndex &block)
{
    assert(prevHeights->size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = static_cast<uint32_t>(tx.nVersion) >= 2 && flags & LOCKTIME_VERIFY_SEQUENCE;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68)
    {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++)
    {
        const CTxIn &txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG)
        {
            // The height of this input is not relevant for sequence locks
            (*prevHeights)[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = (*prevHeights)[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG)
        {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight - 1, 0))->GetMedianTimePast();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK)
                                                                << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) -
                                              1);
        }
        else
        {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

static bool EvaluateSequenceLocks(const CBlockIndex &block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetMedianTimePast();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int> *prevHeights, const CBlockIndex &block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

bool TestLockPointValidity(const LockPoints *lp)
{
    AssertLockHeld(cs_main);
    assert(lp);
    // If there are relative lock times then the maxInputBlock will be set
    // If there are no relative lock times, the LockPoints don't depend on the chain
    if (lp->maxInputBlock)
    {
        // Check whether chainActive is an extension of the block at which the LockPoints
        // calculation was valid.  If not LockPoints are no longer valid
        if (!chainActive.Contains(lp->maxInputBlock))
        {
            return false;
        }
    }

    // LockPoints still valid
    return true;
}

bool CheckSequenceLocks(const CTransaction &tx, int flags, LockPoints *lp, bool useExistingLockPoints)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(mempool.cs);

    CBlockIndex *tip = chainActive.Tip();
    CBlockIndex index;
    index.pprev = tip;
    // CheckSequenceLocks() uses chainActive.Height()+1 to evaluate
    // height based locks because when SequenceLocks() is called within
    // ConnectBlock(), the height of the block *being*
    // evaluated is what is used.
    // Thus if we want to know if a transaction can be part of the
    // *next* block, we need to use one more than chainActive.Height()
    index.nHeight = tip->nHeight + 1;

    std::pair<int, int64_t> lockPair;
    if (useExistingLockPoints)
    {
        assert(lp);
        lockPair.first = lp->height;
        lockPair.second = lp->time;
    }
    else
    {
        // pcoinsTip contains the UTXO set for chainActive.Tip()
        CCoinsViewMemPool viewMemPool(pcoinsTip, mempool);
        std::vector<int> prevheights;
        prevheights.resize(tx.vin.size());
        for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++)
        {
            const CTxIn &txin = tx.vin[txinIndex];
            CCoins coins;
            if (!viewMemPool.GetCoins(txin.prevout.hash, coins))
            {
                return error("%s: Missing input", __func__);
            }
            if (coins.nHeight == MEMPOOL_HEIGHT)
            {
                // Assume all mempool transaction confirm in the next block
                prevheights[txinIndex] = tip->nHeight + 1;
            }
            else
            {
                prevheights[txinIndex] = coins.nHeight;
            }
        }
        lockPair = CalculateSequenceLocks(tx, flags, &prevheights, index);
        if (lp)
        {
            lp->height = lockPair.first;
            lp->time = lockPair.second;
            // Also store the hash of the block with the highest height of
            // all the blocks which have sequence locked prevouts.
            // This hash needs to still be on the chain
            // for these LockPoint calculations to be valid
            // Note: It is impossible to correctly calculate a maxInputBlock
            // if any of the sequence locked inputs depend on unconfirmed txs,
            // except in the special case where the relative lock time/height
            // is 0, which is equivalent to no sequence lock. Since we assume
            // input height of tip+1 for mempool txs and test the resulting
            // lockPair from CalculateSequenceLocks against tip+1.  We know
            // EvaluateSequenceLocks will fail if there was a non-zero sequence
            // lock on a mempool input, so we can use the return value of
            // CheckSequenceLocks to indicate the LockPoints validity
            int maxInputHeight = 0;
            BOOST_FOREACH (int height, prevheights)
            {
                // Can ignore mempool inputs since we'll fail if they had non-zero locks
                if (height != tip->nHeight + 1)
                {
                    maxInputHeight = std::max(maxInputHeight, height);
                }
            }
            lp->maxInputBlock = tip->GetAncestor(maxInputHeight);
        }
    }
    return EvaluateSequenceLocks(index, lockPair);
}


// BU: This code is completely inaccurate if its used to determine the approximate time of transaction
// validation!!!  The sigop count in the output transactions are irrelevant, and the sigop count of the
// previous outputs are the most relevant, but not actually checked.
// The purpose of this is to limit the outputs of transactions so that other transactions' "prevout"
// is reasonably sized.
unsigned int GetLegacySigOpCount(const CTransaction &tx)
{
    unsigned int nSigOps = 0;
    BOOST_FOREACH (const CTxIn &txin, tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    BOOST_FOREACH (const CTxOut &txout, tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction &tx, const CCoinsViewCache &inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut &prevout = inputs.GetOutputFor(tx.vin[i]);
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}


bool CheckTransaction(const CTransaction &tx, CValidationState &state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits
    // BU: size limits removed
    // if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
    //    return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    BOOST_FOREACH (const CTxOut &txout, tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs
    std::set<COutPoint> vInOutPoints;
    BOOST_FOREACH (const CTxIn &txin, tx.vin)
    {
        if (vInOutPoints.count(txin.prevout))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        vInOutPoints.insert(txin.prevout);
    }

    if (tx.IsCoinBase())
    {
        // BU convert 100 to a constant so we can use it during generation
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > MAX_COINBASE_SCRIPTSIG_SIZE)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        BOOST_FOREACH (const CTxIn &txin, tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
    }

    return true;
}

void LimitMempoolSize(CTxMemPool &pool, size_t limit, unsigned long age)
{
    int expired = pool.Expire(GetTime() - age);
    if (expired != 0)
        LogPrint("mempool", "Expired %i transactions from the memory pool\n", expired);

    std::vector<uint256> vNoSpendsRemaining;
    pool.TrimToSize(limit, &vNoSpendsRemaining);
    BOOST_FOREACH (const uint256 &removed, vNoSpendsRemaining)
        pcoinsTip->Uncache(removed);
}

/** Convert CValidationState to a human-readable message for logging */
std::string FormatStateMessage(const CValidationState &state)
{
    return strprintf("%s%s (code %i)", state.GetRejectReason(),
        state.GetDebugMessage().empty() ? "" : ", " + state.GetDebugMessage(), state.GetRejectCode());
}

bool AcceptToMemoryPoolWorker(CTxMemPool &pool,
    CValidationState &state,
    const CTransaction &consttx,
    bool fLimitFree,
    bool *pfMissingInputs,
    bool fOverrideMempoolLimit,
    bool fRejectAbsurdFee,
    std::vector<uint256> &vHashTxnToUncache)
{
    unsigned int forkVerifyFlags = 0;
    CTransaction tx = consttx;
    unsigned int nSigOps = 0;
    ValidationResourceTracker resourceTracker;
    unsigned int nSize = 0;
    uint64_t start = GetTimeMicros();
    AssertLockHeld(cs_main);
    if (pfMissingInputs)
        *pfMissingInputs = false;

    if (!CheckTransaction(tx, state))
        return false;

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.DoS(100, false, REJECT_INVALID, "coinbase");

    // BUIP055: reject transactions that won't work on the fork.
    // This code uses the system time to determine when to start rejecting which is inaccurate relative to the
    // actual activation time (defined by times in the blocks).
    // But its ok to reject these transactions from the mempool a little early (or late).
    if ((miningForkTime.value != 0) && (start / 1000000 >= miningForkTime.value))
    {
        forkVerifyFlags = SCRIPT_ENABLE_SIGHASH_FORKID;
        if (IsTxOpReturnInvalid(tx))
            return state.DoS(0, false, REJECT_WRONG_FORK, "wrong-fork");
    }

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    std::string reason;
    if (fRequireStandard && !IsStandardTx(tx, reason))
        return state.DoS(0, false, REJECT_NONSTANDARD, reason);

    // Don't relay version 2 transactions until CSV is active, and we can be
    // sure that such transactions will be mined (unless we're on
    // -testnet/-regtest).
    const CChainParams &chainparams = Params();
    if (fRequireStandard && tx.nVersion >= 2 &&
        VersionBitsTipState(chainparams.GetConsensus(), Consensus::DEPLOYMENT_CSV) != THRESHOLD_ACTIVE)
    {
        return state.DoS(0, false, REJECT_NONSTANDARD, "premature-version2-tx");
    }

    // Only accept nLockTime-using transactions that can be mined in the next
    // block; we don't want our mempool filled up with transactions that can't
    // be mined yet.
    if (!CheckFinalTx(tx, STANDARD_LOCKTIME_VERIFY_FLAGS))
        return state.DoS(0, false, REJECT_NONSTANDARD, "non-final");

    // is it already in the memory pool?
    uint256 hash = tx.GetHash();
    if (pool.exists(hash))
        return state.Invalid(false, REJECT_ALREADY_KNOWN, "txn-already-in-mempool");

    // Check for conflicts with in-memory transactions
    {
        LOCK(pool.cs); // protect pool.mapNextTx
        BOOST_FOREACH (const CTxIn &txin, tx.vin)
        {
            auto itConflicting = pool.mapNextTx.find(txin.prevout);
            if (itConflicting != pool.mapNextTx.end())
            {
                // Disable replacement feature for good
                return state.Invalid(false, REJECT_CONFLICT, "txn-mempool-conflict");
            }
        }
    }

    {
        CCoinsView dummy;
        CCoinsViewCache view(&dummy);

        CAmount nValueIn = 0;
        LockPoints lp;
        {
            LOCK(pool.cs);
            CCoinsViewMemPool viewMemPool(pcoinsTip, pool);
            view.SetBackend(viewMemPool);

            // do we already have it?
            bool fHadTxInCache = pcoinsTip->HaveCoinsInCache(hash);
            if (view.HaveCoins(hash))
            {
                if (!fHadTxInCache)
                    vHashTxnToUncache.push_back(hash);
                return state.Invalid(false, REJECT_ALREADY_KNOWN, "txn-already-known");
            }

            // do all inputs exist?
            // Note that this does not check for the presence of actual outputs (see the next check for that),
            // and only helps with filling in pfMissingInputs (to determine missing vs spent).
            BOOST_FOREACH (const CTxIn txin, tx.vin)
            {
                if (!pcoinsTip->HaveCoinsInCache(txin.prevout.hash))
                    vHashTxnToUncache.push_back(txin.prevout.hash);
                if (!view.HaveCoins(txin.prevout.hash))
                {
                    if (pfMissingInputs)
                        *pfMissingInputs = true;
                    // fMissingInputs and !state.IsInvalid() is used to detect this condition, don't set state.Invalid()
                    return false;
                }
            }

            // are the actual inputs available?
            if (!view.HaveInputs(tx))
                return state.Invalid(false, REJECT_DUPLICATE, "bad-txns-inputs-spent");

            // Bring the best block into scope
            view.GetBestBlock();

            nValueIn = view.GetValueIn(tx);

            // we have all inputs cached now, so switch back to dummy, so we don't need to keep lock on mempool
            view.SetBackend(dummy);

            // Only accept BIP68 sequence locked transactions that can be mined in the next
            // block; we don't want our mempool filled up with transactions that can't
            // be mined yet.
            // Must keep pool.cs for this unless we change CheckSequenceLocks to take a
            // CoinsViewCache instead of create its own
            if (!CheckSequenceLocks(tx, STANDARD_LOCKTIME_VERIFY_FLAGS, &lp))
                return state.DoS(0, false, REJECT_NONSTANDARD, "non-BIP68-final");
        }

        // Check for non-standard pay-to-script-hash in inputs
        if (fRequireStandard && !AreInputsStandard(tx, view))
            return state.Invalid(false, REJECT_NONSTANDARD, "bad-txns-nonstandard-inputs");

        nSigOps = GetLegacySigOpCount(tx);
        nSigOps += GetP2SHSigOpCount(tx, view);

        CAmount nValueOut = tx.GetValueOut();
        CAmount nFees = nValueIn - nValueOut;
        // nModifiedFees includes any fee deltas from PrioritiseTransaction
        CAmount nModifiedFees = nFees;
        double nPriorityDummy = 0;
        pool.ApplyDeltas(hash, nPriorityDummy, nModifiedFees);

        CAmount inChainInputValue;
        double dPriority = view.GetPriority(tx, chainActive.Height(), inChainInputValue);

        // Keep track of transactions that spend a coinbase, which we re-scan
        // during reorgs to ensure COINBASE_MATURITY is still met.
        bool fSpendsCoinbase = false;
        BOOST_FOREACH (const CTxIn &txin, tx.vin)
        {
            const CCoins *coins = view.AccessCoins(txin.prevout.hash);
            if (coins->IsCoinBase())
            {
                fSpendsCoinbase = true;
                break;
            }
        }

        CTxMemPoolEntry entry(tx, nFees, GetTime(), dPriority, chainActive.Height(), pool.HasNoInputsOf(tx),
            inChainInputValue, fSpendsCoinbase, nSigOps, lp);
        nSize = entry.GetTxSize();

// Check that the transaction doesn't have an excessive number of
// sigops, making it impossible to mine. Since the coinbase transaction
// itself can contain sigops MAX_STANDARD_TX_SIGOPS is less than
// MAX_BLOCK_SIGOPS; we still consider this an invalid rather than
// merely non-standard transaction.

#if 1
        // For testing blocks with larger numbers of sigops, we need to be able to create them by creating transactions
        // that miners can create.  This define won't be set outside of testing
        //#ifdef ALLOW_ALL_VALID_TX_IN_MEMPOOL
        if (nSigOps > BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS)
            return state.DoS(0, false, REJECT_NONSTANDARD, "bad-txns-too-many-sigops", false, strprintf("%d", nSigOps));
#else
        if ((nSigOps > MAX_STANDARD_TX_SIGOPS) || (nBytesPerSigOp && nSigOps > nSize / nBytesPerSigOp))
            return state.DoS(0, false, REJECT_NONSTANDARD, "bad-txns-too-many-sigops", false, strprintf("%d", nSigOps));
#endif

        CAmount mempoolRejectFee =
            pool.GetMinFee(GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFee(nSize);
        if (mempoolRejectFee > 0 && nModifiedFees < mempoolRejectFee)
        {
            return state.DoS(0, false, REJECT_INSUFFICIENTFEE, "mempool min fee not met", false,
                strprintf("%d < %d", nFees, mempoolRejectFee));
        }
        else if (GetBoolArg("-relaypriority", DEFAULT_RELAYPRIORITY) && nModifiedFees < ::minRelayTxFee.GetFee(nSize) &&
                 !AllowFree(entry.GetPriority(chainActive.Height() + 1)))
        {
            // Require that free transactions have sufficient priority to be mined in the next block.
            return state.DoS(0, false, REJECT_INSUFFICIENTFEE, "insufficient priority");
        }

        // BU - Xtreme Thinblocks Auto Mempool Limiter - begin section
        /* Continuously rate-limit free (really, very-low-fee) transactions
         * This mitigates 'penny-flooding' -- sending thousands of free transactions just to
         * be annoying or make others' transactions take longer to confirm. */
        // maximum feeCutoff in satoshi per byte
        static const double maxFeeCutoff =
            boost::lexical_cast<double>(GetArg("-maxlimitertxfee", DEFAULT_MAXLIMITERTXFEE));
        // starting value for feeCutoff in satoshi per byte
        static const double initFeeCutoff =
            boost::lexical_cast<double>(GetArg("-minlimitertxfee", DEFAULT_MINLIMITERTXFEE));
        static const int nLimitFreeRelay = GetArg("-limitfreerelay", DEFAULT_LIMITFREERELAY);

        // get current memory pool size
        uint64_t poolBytes = pool.GetTotalTxSize();

        // Calculate feeCutoff in satoshis per byte:
        //   When the feeCutoff is larger than the satoshiPerByte of the
        //   current transaction then spam blocking will be in effect. However
        //   Some free transactions will still get through based on -limitfreerelay
        static double feeCutoff;
        static double nFreeLimit = nLimitFreeRelay;
        static int64_t nLastTime;
        int64_t nNow = GetTime();

        // When the mempool starts falling use an exponentially decaying ~24 hour window:
        // nFreeLimit = nFreeLimit + ((double)(DEFAULT_LIMIT_FREE_RELAY - nFreeLimit) / pow(1.0 - 1.0/86400,
        // (double)(nNow - nLastTime)));
        nFreeLimit /= std::pow(1.0 - 1.0 / 86400, (double)(nNow - nLastTime));

        // When the mempool starts falling use an exponentially decaying ~24 hour window:
        feeCutoff *= std::pow(1.0 - 1.0 / 86400, (double)(nNow - nLastTime));

        uint64_t nLargestBlockSeen = LargestBlockSeen();

        if (poolBytes < nLargestBlockSeen)
        {
            feeCutoff = std::max(feeCutoff, initFeeCutoff);
            nFreeLimit = std::min(nFreeLimit, (double)nLimitFreeRelay);
        }
        else if (poolBytes < (nLargestBlockSeen * MAX_BLOCK_SIZE_MULTIPLIER))
        {
            // Gradually choke off what is considered a free transaction
            feeCutoff =
                std::max(feeCutoff, initFeeCutoff + ((maxFeeCutoff - initFeeCutoff) * (poolBytes - nLargestBlockSeen) /
                                                        (nLargestBlockSeen * (MAX_BLOCK_SIZE_MULTIPLIER - 1))));

            // Gradually choke off the nFreeLimit as well but leave at least DEFAULT_MIN_LIMITFREERELAY
            // So that some free transactions can still get through
            nFreeLimit = std::min(
                nFreeLimit, ((double)nLimitFreeRelay - ((double)(nLimitFreeRelay - DEFAULT_MIN_LIMITFREERELAY) *
                                                           (double)(poolBytes - nLargestBlockSeen) /
                                                           (nLargestBlockSeen * (MAX_BLOCK_SIZE_MULTIPLIER - 1)))));
            if (nFreeLimit < DEFAULT_MIN_LIMITFREERELAY)
                nFreeLimit = DEFAULT_MIN_LIMITFREERELAY;
        }
        else
        {
            feeCutoff = maxFeeCutoff;
            nFreeLimit = DEFAULT_MIN_LIMITFREERELAY;
        }

        minRelayTxFee = CFeeRate(feeCutoff * 1000);
        LogPrint("mempool",
            "MempoolBytes:%d  LimitFreeRelay:%.5g  FeeCutOff:%.4g  FeesSatoshiPerByte:%.4g  TxBytes:%d  TxFees:%d\n",
            poolBytes, nFreeLimit, ((double)::minRelayTxFee.GetFee(nSize)) / nSize, ((double)nFees) / nSize, nSize,
            nFees);
        if (fLimitFree && nFees < ::minRelayTxFee.GetFee(nSize))
        {
            static double dFreeCount;

            // Use an exponentially decaying ~10-minute window:
            dFreeCount *= std::pow(1.0 - 1.0 / 600.0, (double)(nNow - nLastTime));
            nLastTime = nNow;

            // -limitfreerelay unit is thousand-bytes-per-minute
            // At default rate it would take over a month to fill 1GB
            LogPrint("mempool", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount + nSize);
            if ((dFreeCount + nSize) >= (nFreeLimit * 10 * 1000 * nLargestBlockSeen / BLOCKSTREAM_CORE_MAX_BLOCK_SIZE))
            {
                thindata.UpdateMempoolLimiterBytesSaved(nSize);
                LogPrint(
                    "mempool", "AcceptToMemoryPool : free transaction %s rejected by rate limiter\n", hash.ToString());
                return state.DoS(0, false, REJECT_INSUFFICIENTFEE, "rate limited free transaction");
            }
            dFreeCount += nSize;
        }
        nLastTime = nNow;
        // BU - Xtreme Thinblocks Auto Mempool Limiter - end section

        // BU: we calculate the recommended fee by looking at what's in the mempool.  This starts at 0 though for an
        // empty mempool.  So set the minimum "absurd" fee to 10000 satoshies per byte.  If for some reason fees rise
        // above that, you can specify up to 100x what other txns are paying in the mempool
        if (fRejectAbsurdFee && nFees > std::max((int64_t)100L * nSize, maxTxFee.value) * 100)
            return state.Invalid(false, REJECT_HIGHFEE, "absurdly-high-fee",
                strprintf("%d > %d", nFees, std::max((int64_t)1L, maxTxFee.value) * 10000));

        // Calculate in-mempool ancestors, up to a limit.
        CTxMemPool::setEntries setAncestors;
        size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
        size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
        std::string errString;
        if (!pool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants,
                nLimitDescendantSize, errString))
        {
            return state.DoS(0, false, REJECT_NONSTANDARD, "too-long-mempool-chain", false, errString);
        }

        // Check against previous transactions
        // This is done last to help prevent CPU exhaustion denial-of-service attacks.

        if (!CheckInputsAnalyzeTx(
                tx, state, view, true, STANDARD_SCRIPT_VERIFY_FLAGS | forkVerifyFlags, true, &resourceTracker))
        {
            LogPrint("mempool", "txn CheckInputs failed");
            return false;
        }
        entry.UpdateRuntimeSigOps(resourceTracker.GetSigOps(), resourceTracker.GetSighashBytes());

        // Check again against just the consensus-critical mandatory script
        // verification flags, in case of bugs in the standard flags that cause
        // transactions to pass as valid when they're actually invalid. For
        // instance the STRICTENC flag was incorrectly allowing certain
        // CHECKSIG NOT scripts to pass, even though they were invalid.
        //
        // There is a similar check in CreateNewBlock() to prevent creating
        // invalid blocks, however allowing such transactions into the mempool
        // can be exploited as a DoS attack.
        if (!CheckInputsAnalyzeTx(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS | forkVerifyFlags, true, NULL))
        {
            return error(
                "%s: BUG! PLEASE REPORT THIS! ConnectInputs failed against MANDATORY but not STANDARD flags %s, %s",
                __func__, hash.ToString(), FormatStateMessage(state));
        }

        // Store transaction in memory
        pool.addUnchecked(hash, entry, setAncestors, !IsInitialBlockDownload());

        // trim mempool and check if tx was trimmed
        if (!fOverrideMempoolLimit)
        {
            LimitMempoolSize(pool, GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000,
                GetArg("-mempoolexpiry", DEFAULT_MEMPOOL_EXPIRY) * 60 * 60);
            if (!pool.exists(hash))
                return state.DoS(0, false, REJECT_INSUFFICIENTFEE, "mempool full");
        }
        // BU: update tx per second when a tx is valid and accepted
        pool.UpdateTransactionsPerSecond();
        // BU - Xtreme Thinblocks - trim the orphan pool by entry time and do not allow it to be overidden.
    }

    if (!fRejectAbsurdFee)
        SyncWithWallets(tx, NULL);

    int64_t end = GetTimeMicros();

    LogPrint("bench", "ValidateTransaction, time: %d, tx: %s, len: %d, sigops: %llu (legacy: %u), sighash: %llu, Vin: "
                      "%llu, Vout: %llu\n",
        end - start, tx.GetHash().ToString(), nSize, resourceTracker.GetSigOps(), (unsigned int)nSigOps,
        resourceTracker.GetSighashBytes(), tx.vin.size(), tx.vout.size());
    nTxValidationTime << (end - start);

    return true;
}

bool AcceptToMemoryPool(CTxMemPool &pool,
    CValidationState &state,
    const CTransaction &tx,
    bool fLimitFree,
    bool *pfMissingInputs,
    bool fOverrideMempoolLimit,
    bool fRejectAbsurdFee)
{
    std::vector<uint256> vHashTxToUncache;
    bool res = AcceptToMemoryPoolWorker(
        pool, state, tx, fLimitFree, pfMissingInputs, fOverrideMempoolLimit, fRejectAbsurdFee, vHashTxToUncache);
    if (!res)
    {
        BOOST_FOREACH (const uint256 &hashTx, vHashTxToUncache)
            pcoinsTip->Uncache(hashTx);
    }

    return res;
}

/** Return transaction in tx, and if it was found inside a block, its hash is placed in hashBlock */
bool GetTransaction(const uint256 &hash,
    CTransaction &txOut,
    const Consensus::Params &consensusParams,
    uint256 &hashBlock,
    bool fAllowSlow)
{
    CBlockIndex *pindexSlow = NULL;

    LOCK(cs_main);

    if (mempool.lookup(hash, txOut))
    {
        return true;
    }

    if (fTxIndex)
    {
        CDiskTxPos postx;
        if (pblocktree->ReadTxIndex(hash, postx))
        {
            CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
            if (file.IsNull())
                return error("%s: OpenBlockFile failed", __func__);
            CBlockHeader header;
            try
            {
                file >> header;
                fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
                file >> txOut;
            }
            catch (const std::exception &e)
            {
                return error("%s: Deserialize or I/O error - %s", __func__, e.what());
            }
            hashBlock = header.GetHash();
            if (txOut.GetHash() != hash)
                return error("%s: txid mismatch", __func__);
            return true;
        }
    }

    if (fAllowSlow)
    { // use coin database to locate block that contains transaction, and scan it
        int nHeight = -1;
        {
            CCoinsViewCache &view = *pcoinsTip;
            const CCoins *coins = view.AccessCoins(hash);
            if (coins)
                nHeight = coins->nHeight;
        }
        if (nHeight > 0)
            pindexSlow = chainActive[nHeight];
    }

    if (pindexSlow)
    {
        CBlock block;
        if (ReadBlockFromDisk(block, pindexSlow, consensusParams))
        {
            BOOST_FOREACH (const CTransaction &tx, block.vtx)
            {
                if (tx.GetHash() == hash)
                {
                    txOut = tx;
                    hashBlock = pindexSlow->GetBlockHash();
                    return true;
                }
            }
        }
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//

bool WriteBlockToDisk(const CBlock &block, CDiskBlockPos &pos, const CMessageHeader::MessageStartChars &messageStart)
{
    // Open history file to append
    CAutoFile fileout(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("WriteBlockToDisk: OpenBlockFile failed");

    // Write index header
    unsigned int nSize = fileout.GetSerializeSize(block);
    fileout << FLATDATA(messageStart) << nSize;

    // Write block
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0)
        return error("WriteBlockToDisk: ftell failed");
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    return true;
}

bool ReadBlockFromDisk(CBlock &block, const CDiskBlockPos &pos, const Consensus::Params &consensusParams)
{
    block.SetNull();

    // Open history file to read
    CAutoFile filein(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
        return error("ReadBlockFromDisk: OpenBlockFile failed for %s", pos.ToString());

    // Read block
    try
    {
        filein >> block;
    }
    catch (const std::exception &e)
    {
        return error("%s: Deserialize or I/O error - %s at %s", __func__, e.what(), pos.ToString());
    }

    // Check the header
    if (!CheckProofOfWork(block.GetHash(), block.nBits, consensusParams))
        return error("ReadBlockFromDisk: Errors in block header at %s", pos.ToString());

    return true;
}

bool ReadBlockFromDisk(CBlock &block, const CBlockIndex *pindex, const Consensus::Params &consensusParams)
{
    if (!ReadBlockFromDisk(block, pindex->GetBlockPos(), consensusParams))
        return false;
    if (block.GetHash() != pindex->GetBlockHash())
        return error("ReadBlockFromDisk(CBlock&, CBlockIndex*): GetHash() doesn't match index for %s at %s",
            pindex->ToString(), pindex->GetBlockPos().ToString());
    return true;
}

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params &consensusParams)
{
    int halvings = nHeight / consensusParams.nSubsidyHalvingInterval;
    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64)
        return 0;

    CAmount nSubsidy = 50 * COIN;
    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;
    return nSubsidy;
}

bool fLargeWorkForkFound = false;
bool fLargeWorkInvalidChainFound = false;
CBlockIndex *pindexBestForkTip = NULL, *pindexBestForkBase = NULL;

// Execute a command, as given by -alertnotify, on certain events such as a long fork being seen
static void AlertNotify(const std::string &strMessage)
{
    uiInterface.NotifyAlertChanged();
    std::string strCmd = GetArg("-alertnotify", "");
    if (strCmd.empty())
        return;

    // Alert text should be plain ascii coming from a trusted source, but to
    // be safe we first strip anything not in safeChars, then add single quotes around
    // the whole string before passing it to the shell:
    std::string singleQuote("'");
    std::string safeStatus = SanitizeString(strMessage);
    safeStatus = singleQuote + safeStatus + singleQuote;
    boost::replace_all(strCmd, "%s", safeStatus);

    boost::thread t(runCommand, strCmd); // thread runs free
}

void CheckForkWarningConditions()
{
    AssertLockHeld(cs_main);
    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before the last checkpoint)
    if (IsInitialBlockDownload())
        return;

    // If our best fork is no longer within 72 blocks (+/- 12 hours if no one mines it)
    // of our head, drop it
    if (pindexBestForkTip && chainActive.Height() - pindexBestForkTip->nHeight >= 72)
        pindexBestForkTip = NULL;

    if (pindexBestForkTip ||
        (pindexBestInvalid &&
            pindexBestInvalid->nChainWork > chainActive.Tip()->nChainWork + (GetBlockProof(*chainActive.Tip()) * 6)))
    {
        if (!fLargeWorkForkFound && pindexBestForkBase)
        {
            std::string warning = std::string("'Warning: Large-work fork detected, forking after block ") +
                                  pindexBestForkBase->phashBlock->ToString() + std::string("'");
            AlertNotify(warning);
        }
        if (pindexBestForkTip && pindexBestForkBase)
        {
            LogPrintf("%s: Warning: Large valid fork found\n  forking the chain at height %d (%s)\n  lasting to height "
                      "%d (%s).\nChain state database corruption likely.\n",
                __func__, pindexBestForkBase->nHeight, pindexBestForkBase->phashBlock->ToString(),
                pindexBestForkTip->nHeight, pindexBestForkTip->phashBlock->ToString());
            fLargeWorkForkFound = true;
        }
        else
        {
            LogPrintf("%s: Warning: Found invalid chain at least ~6 blocks longer than our best chain.\nChain state "
                      "database corruption likely.\n",
                __func__);
            fLargeWorkInvalidChainFound = true;
        }
    }
    else
    {
        fLargeWorkForkFound = false;
        fLargeWorkInvalidChainFound = false;
    }
}

void CheckForkWarningConditionsOnNewFork(CBlockIndex *pindexNewForkTip)
{
    AssertLockHeld(cs_main);
    // If we are on a fork that is sufficiently large, set a warning flag
    CBlockIndex *pfork = pindexNewForkTip;
    CBlockIndex *plonger = chainActive.Tip();
    while (pfork && pfork != plonger)
    {
        while (plonger && plonger->nHeight > pfork->nHeight)
            plonger = plonger->pprev;
        if (pfork == plonger)
            break;
        pfork = pfork->pprev;
    }

    // We define a condition where we should warn the user about as a fork of at least 7 blocks
    // with a tip within 72 blocks (+/- 12 hours if no one mines it) of ours
    // We use 7 blocks rather arbitrarily as it represents just under 10% of sustained network
    // hash rate operating on the fork.
    // or a chain that is entirely longer than ours and invalid (note that this should be detected by both)
    // We define it this way because it allows us to only store the highest fork tip (+ base) which meets
    // the 7-block condition and from this always have the most-likely-to-cause-warning fork
    if (pfork &&
        (!pindexBestForkTip || (pindexBestForkTip && pindexNewForkTip->nHeight > pindexBestForkTip->nHeight)) &&
        pindexNewForkTip->nChainWork - pfork->nChainWork > (GetBlockProof(*pfork) * 7) &&
        chainActive.Height() - pindexNewForkTip->nHeight < 72)
    {
        pindexBestForkTip = pindexNewForkTip;
        pindexBestForkBase = pfork;
    }

    CheckForkWarningConditions();
}

void static InvalidChainFound(CBlockIndex *pindexNew)
{
    if (!pindexBestInvalid || pindexNew->nChainWork > pindexBestInvalid->nChainWork)
        pindexBestInvalid = pindexNew;

    LogPrintf("%s: invalid block=%s  height=%d  log2_work=%.8g  date=%s\n", __func__,
        pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
        std::log(pindexNew->nChainWork.getdouble()) / std::log(2.0),
        DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pindexNew->GetBlockTime()));
    CBlockIndex *tip = chainActive.Tip();
    assert(tip);
    LogPrintf("%s:  current best=%s  height=%d  log2_work=%.8g  date=%s\n", __func__, tip->GetBlockHash().ToString(),
        chainActive.Height(), std::log(tip->nChainWork.getdouble()) / std::log(2.0),
        DateTimeStrFormat("%Y-%m-%d %H:%M:%S", tip->GetBlockTime()));
    CheckForkWarningConditions();
}


void static InvalidBlockFound(CBlockIndex *pindex, const CValidationState &state)
{
    int nDoS = 0;
    if (state.IsInvalid(nDoS))
    {
        assert(state.GetRejectCode() < REJECT_INTERNAL); // Blocks are never rejected with internal reject codes

        std::map<uint256, NodeId>::iterator it = mapBlockSource.find(pindex->GetBlockHash());
        if (it != mapBlockSource.end())
        {
            CNodeRef node(connmgr->FindNodeFromId(it->second));

            if (node)
            {
                node->PushMessage(NetMsgType::REJECT, (std::string)NetMsgType::BLOCK,
                    (unsigned char)state.GetRejectCode(), state.GetRejectReason().substr(0, MAX_REJECT_MESSAGE_LENGTH),
                    pindex->GetBlockHash());
                if (nDoS > 0)
                    dosMan.Misbehaving(node.get(), nDoS);
            }
        }
    }
    if (!state.CorruptionPossible())
    {
        pindex->nStatus |= BLOCK_FAILED_VALID;
        setDirtyBlockIndex.insert(pindex);
        setBlockIndexCandidates.erase(pindex);
        InvalidChainFound(pindex);

        // Now mark every block index on every chain that contains pindex as child of invalid
        MarkAllContainingChainsInvalid(pindex);
    }
}

void UpdateCoins(const CTransaction &tx, CValidationState &state, CCoinsViewCache &inputs, CTxUndo &txundo, int nHeight)
{
    // mark inputs spent
    if (!tx.IsCoinBase())
    {
        txundo.vprevout.reserve(tx.vin.size());
        BOOST_FOREACH (const CTxIn &txin, tx.vin)
        {
            CCoinsModifier coins = inputs.ModifyCoins(txin.prevout.hash);
            unsigned nPos = txin.prevout.n;

            if (nPos >= coins->vout.size() || coins->vout[nPos].IsNull())
                assert(false);
            // mark an outpoint spent, and construct undo information
            txundo.vprevout.push_back(CTxInUndo(coins->vout[nPos]));
            coins->Spend(nPos);
            if (coins->vout.size() == 0)
            {
                CTxInUndo &undo = txundo.vprevout.back();
                undo.nHeight = coins->nHeight;
                undo.fCoinBase = coins->fCoinBase;
                undo.nVersion = coins->nVersion;
            }
        }
        // add outputs
        inputs.ModifyNewCoins(tx.GetHash())->FromTx(tx, nHeight);
    }
    else
    {
        // add outputs for coinbase tx
        // In this case call the full ModifyCoins which will do a database
        // lookup to be sure the coins do not already exist otherwise we do not
        // know whether to mark them fresh or not.  We want the duplicate coinbases
        // before BIP30 to still be properly overwritten.
        inputs.ModifyCoins(tx.GetHash())->FromTx(tx, nHeight);
    }
}

void UpdateCoins(const CTransaction &tx, CValidationState &state, CCoinsViewCache &inputs, int nHeight)
{
    CTxUndo txundo;
    UpdateCoins(tx, state, inputs, txundo, nHeight);
}

bool CScriptCheck::operator()()
{
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    CachingTransactionSignatureChecker checker(ptxTo, nIn, amount, cacheStore);
    unsigned int sighashtype = 0;
    if (!VerifyScript(scriptSig, scriptPubKey, nFlags, checker, &error, &sighashtype))
        return false;
    if (resourceTracker)
        resourceTracker->Update(ptxTo->GetHash(), checker.GetNumSigops(), checker.GetBytesHashed());
    return true;
}

bool CScriptCheckAndAnalyze::operator()()
{
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    CachingTransactionSignatureChecker checker(ptxTo, nIn, amount, cacheStore);
    unsigned int sighashtype = 0;
    if (!VerifyScript(scriptSig, scriptPubKey, nFlags, checker, &error, &sighashtype))
        return false;
    ptxToNonConst->sighashType |= sighashtype;
    if (resourceTracker)
        resourceTracker->Update(ptxTo->GetHash(), checker.GetNumSigops(), checker.GetBytesHashed());
    return true;
}

int GetSpendHeight(const CCoinsViewCache &inputs)
{
    LOCK(cs_main);
    BlockMap::iterator i = mapBlockIndex.find(inputs.GetBestBlock());
    if (i != mapBlockIndex.end())
    {
        CBlockIndex *pindexPrev = i->second;
        if (pindexPrev)
            return pindexPrev->nHeight + 1;
        else
        {
            throw std::runtime_error("GetSpendHeight(): mapBlockIndex contains null block");
        }
    }
    throw std::runtime_error("GetSpendHeight(): best block does not exist");
}

namespace Consensus
{
bool CheckTxInputs(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &inputs, int nSpendHeight)
{
    // This doesn't trigger the DoS code on purpose; if it did, it would make it easier
    // for an attacker to attempt to split the network.
    if (!inputs.HaveInputs(tx))
        return state.Invalid(false, 0, "", "Inputs unavailable");

    CAmount nValueIn = 0;
    CAmount nFees = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const COutPoint &prevout = tx.vin[i].prevout;
        const CCoins *coins = inputs.AccessCoins(prevout.hash);
        assert(coins);

        // If prev is coinbase, check that it's matured
        if (coins->IsCoinBase())
        {
            if (nSpendHeight - coins->nHeight < COINBASE_MATURITY)
                return state.Invalid(false, REJECT_INVALID, "bad-txns-premature-spend-of-coinbase",
                    strprintf("tried to spend coinbase at depth %d", nSpendHeight - coins->nHeight));
        }

        // Check for negative or overflow input values
        nValueIn += coins->vout[prevout.n].nValue;
        if (!MoneyRange(coins->vout[prevout.n].nValue) || !MoneyRange(nValueIn))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
    }

    if (nValueIn < tx.GetValueOut())
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-in-belowout", false,
            strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(tx.GetValueOut())));

    // Tally transaction fees
    CAmount nTxFee = nValueIn - tx.GetValueOut();
    if (nTxFee < 0)
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-negative");
    nFees += nTxFee;
    if (!MoneyRange(nFees))
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-outofrange");
    return true;
}
} // namespace Consensus

bool CheckInputsAnalyzeTx(CTransaction &tx,
    CValidationState &state,
    const CCoinsViewCache &inputs,
    bool fScriptChecks,
    unsigned int flags,
    bool cacheStore,
    ValidationResourceTracker *resourceTracker,
    std::vector<CScriptCheck> *pvChecks)
{
    if (!tx.IsCoinBase())
    {
        if (!Consensus::CheckTxInputs(tx, state, inputs, GetSpendHeight(inputs)))
            return false;
        if (pvChecks)
            pvChecks->reserve(tx.vin.size());

        // The first loop above does all the inexpensive checks.
        // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
        // Helps prevent CPU exhaustion attacks.

        // Skip ECDSA signature verification when connecting blocks before the
        // last block chain checkpoint. Assuming the checkpoints are valid this
        // is safe because block merkle hashes are still computed and checked,
        // and any change will be caught at the next checkpoint. Of course, if
        // the checkpoint is for a chain that's invalid due to false scriptSigs
        // this optimisation would allow an invalid chain to be accepted.
        if (fScriptChecks)
        {
            for (unsigned int i = 0; i < tx.vin.size(); i++)
            {
                const COutPoint &prevout = tx.vin[i].prevout;
                const CCoins *coins = inputs.AccessCoins(prevout.hash);
                if (!coins)
                    LogPrintf("ASSERTION: no inputs available\n");
                assert(coins);

                // Verify signature
                CScriptCheckAndAnalyze check(resourceTracker, *coins, tx, i, flags, cacheStore);
                if (pvChecks)
                {
                    pvChecks->push_back(CScriptCheck());
                    check.swap(pvChecks->back());
                }
                else if (!check())
                {
                    if (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS)
                    {
                        // Check whether the failure was caused by a
                        // non-mandatory script verification check, such as
                        // non-standard DER encodings or non-null dummy
                        // arguments; if so, don't trigger DoS protection to
                        // avoid splitting the network between upgraded and
                        // non-upgraded nodes.
                        CScriptCheck check2(
                            NULL, *coins, tx, i, flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS, cacheStore);
                        if (check2())
                            return state.Invalid(
                                false, REJECT_NONSTANDARD, strprintf("non-mandatory-script-verify-flag (%s)",
                                                               ScriptErrorString(check.GetScriptError())));
                    }
                    // Failures of other flags indicate a transaction that is
                    // invalid in new blocks, e.g. a invalid P2SH. We DoS ban
                    // such nodes as they are not following the protocol. That
                    // said during an upgrade careful thought should be taken
                    // as to the correct behavior - we may want to continue
                    // peering with non-upgraded nodes even after a soft-fork
                    // super-majority vote has passed.
                    return state.DoS(100, false, REJECT_INVALID, strprintf("mandatory-script-verify-flag-failed (%s)",
                                                                     ScriptErrorString(check.GetScriptError())));
                }
            }
        }
    }

    return true;
}

bool CheckInputs(const CTransaction &tx,
    CValidationState &state,
    const CCoinsViewCache &inputs,
    bool fScriptChecks,
    unsigned int flags,
    bool cacheStore,
    ValidationResourceTracker *resourceTracker,
    std::vector<CScriptCheck> *pvChecks)
{
    if (!tx.IsCoinBase())
    {
        if (!Consensus::CheckTxInputs(tx, state, inputs, GetSpendHeight(inputs)))
            return false;
        if (pvChecks)
            pvChecks->reserve(tx.vin.size());

        // The first loop above does all the inexpensive checks.
        // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
        // Helps prevent CPU exhaustion attacks.

        // Skip ECDSA signature verification when connecting blocks before the
        // last block chain checkpoint. Assuming the checkpoints are valid this
        // is safe because block merkle hashes are still computed and checked,
        // and any change will be caught at the next checkpoint. Of course, if
        // the checkpoint is for a chain that's invalid due to false scriptSigs
        // this optimisation would allow an invalid chain to be accepted.
        if (fScriptChecks)
        {
            for (unsigned int i = 0; i < tx.vin.size(); i++)
            {
                const COutPoint &prevout = tx.vin[i].prevout;
                const CCoins *coins = inputs.AccessCoins(prevout.hash);
                if (!coins)
                    LogPrintf("ASSERTION: no inputs available\n");
                assert(coins);

                // Verify signature
                CScriptCheck check(resourceTracker, *coins, tx, i, flags, cacheStore);
                if (pvChecks)
                {
                    pvChecks->push_back(CScriptCheck());
                    check.swap(pvChecks->back());
                }
                else if (!check())
                {
                    if (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS)
                    {
                        // Check whether the failure was caused by a
                        // non-mandatory script verification check, such as
                        // non-standard DER encodings or non-null dummy
                        // arguments; if so, don't trigger DoS protection to
                        // avoid splitting the network between upgraded and
                        // non-upgraded nodes.
                        CScriptCheck check2(
                            NULL, *coins, tx, i, flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS, cacheStore);
                        if (check2())
                            return state.Invalid(
                                false, REJECT_NONSTANDARD, strprintf("non-mandatory-script-verify-flag (%s)",
                                                               ScriptErrorString(check.GetScriptError())));
                    }
                    // Failures of other flags indicate a transaction that is
                    // invalid in new blocks, e.g. a invalid P2SH. We DoS ban
                    // such nodes as they are not following the protocol. That
                    // said during an upgrade careful thought should be taken
                    // as to the correct behavior - we may want to continue
                    // peering with non-upgraded nodes even after a soft-fork
                    // super-majority vote has passed.
                    return state.DoS(100, false, REJECT_INVALID, strprintf("mandatory-script-verify-flag-failed (%s)",
                                                                     ScriptErrorString(check.GetScriptError())));
                }
            }
        }
    }

    return true;
}

namespace
{
bool UndoWriteToDisk(const CBlockUndo &blockundo,
    CDiskBlockPos &pos,
    const uint256 &hashBlock,
    const CMessageHeader::MessageStartChars &messageStart)
{
    // Open history file to append
    CAutoFile fileout(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s: OpenUndoFile failed", __func__);

    // Write index header
    unsigned int nSize = fileout.GetSerializeSize(blockundo);
    fileout << FLATDATA(messageStart) << nSize;

    // Write undo data
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0)
        return error("%s: ftell failed", __func__);
    pos.nPos = (unsigned int)fileOutPos;
    fileout << blockundo;

    // calculate & write checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << hashBlock;
    hasher << blockundo;
    fileout << hasher.GetHash();

    return true;
}

bool UndoReadFromDisk(CBlockUndo &blockundo, const CDiskBlockPos &pos, const uint256 &hashBlock)
{
    // Open history file to read
    CAutoFile filein(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
        return error("%s: OpenBlockFile failed", __func__);

    // Read block
    uint256 hashChecksum;
    try
    {
        filein >> blockundo;
        filein >> hashChecksum;
    }
    catch (const std::exception &e)
    {
        return error("%s: Deserialize or I/O error - %s", __func__, e.what());
    }

    // Verify checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << hashBlock;
    hasher << blockundo;
    if (hashChecksum != hasher.GetHash())
        return error("%s: Checksum mismatch", __func__);

    return true;
}

/** Abort with a message */
bool AbortNode(const std::string &strMessage, const std::string &userMessage = "")
{
    strMiscWarning = strMessage;
    LogPrintf("*** %s\n", strMessage);
    uiInterface.ThreadSafeMessageBox(
        userMessage.empty() ? _("Error: A fatal internal error occurred, see debug.log for details") : userMessage, "",
        CClientUIInterface::MSG_ERROR);
    StartShutdown();
    return false;
}

bool AbortNode(CValidationState &state, const std::string &strMessage, const std::string &userMessage = "")
{
    AbortNode(strMessage, userMessage);
    return state.Error(strMessage);
}

} // anon namespace

/**
 * Apply the undo operation of a CTxInUndo to the given chain state.
 * @param undo The undo object.
 * @param view The coins view to which to apply the changes.
 * @param out The out point that corresponds to the tx input.
 * @return True on success.
 */
static bool ApplyTxInUndo(const CTxInUndo &undo, CCoinsViewCache &view, const COutPoint &out)
{
    bool fClean = true;

    CCoinsModifier coins = view.ModifyCoins(out.hash);
    if (undo.nHeight != 0)
    {
        // undo data contains height: this is the last output of the prevout tx being spent
        if (!coins->IsPruned())
            fClean = fClean && error("%s: undo data overwriting existing transaction", __func__);
        coins->Clear();
        coins->fCoinBase = undo.fCoinBase;
        coins->nHeight = undo.nHeight;
        coins->nVersion = undo.nVersion;
    }
    else
    {
        if (coins->IsPruned())
            fClean = fClean && error("%s: undo data adding output to missing transaction", __func__);
    }
    if (coins->IsAvailable(out.n))
        fClean = fClean && error("%s: undo data overwriting existing output", __func__);
    if (coins->vout.size() < out.n + 1)
        coins->vout.resize(out.n + 1);
    coins->vout[out.n] = undo.txout;

    return fClean;
}

bool DisconnectBlock(const CBlock &block,
    CValidationState &state,
    const CBlockIndex *pindex,
    CCoinsViewCache &view,
    bool *pfClean)
{
    assert(pindex->GetBlockHash() == view.GetBestBlock());

    if (pfClean)
        *pfClean = false;

    bool fClean = true;

    CBlockUndo blockUndo;
    CDiskBlockPos pos = pindex->GetUndoPos();
    if (pos.IsNull())
        return error("DisconnectBlock(): no undo data available");
    if (!UndoReadFromDisk(blockUndo, pos, pindex->pprev->GetBlockHash()))
        return error("DisconnectBlock(): failure reading undo data");

    if (blockUndo.vtxundo.size() + 1 != block.vtx.size())
        return error("DisconnectBlock(): block and undo data inconsistent");

    // undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 0; i--)
    {
        const CTransaction &tx = block.vtx[i];
        uint256 hash = tx.GetHash();

        // Check that all outputs are available and match the outputs in the block itself
        // exactly.
        {
            CCoinsModifier outs = view.ModifyCoins(hash);
            outs->ClearUnspendable();

            CCoins outsBlock(tx, pindex->nHeight);
            // The CCoins serialization does not serialize negative numbers.
            // No network rules currently depend on the version here, so an inconsistency is harmless
            // but it must be corrected before txout nversion ever influences a network rule.
            if (outsBlock.nVersion < 0)
                outs->nVersion = outsBlock.nVersion;
            if (*outs != outsBlock)
                fClean = fClean && error("DisconnectBlock(): added transaction mismatch? database corrupted");

            // remove outputs
            outs->Clear();
        }

        // restore inputs
        if (i > 0)
        { // not coinbases
            const CTxUndo &txundo = blockUndo.vtxundo[i - 1];
            if (txundo.vprevout.size() != tx.vin.size())
                return error("DisconnectBlock(): transaction and undo data inconsistent");
            for (unsigned int j = tx.vin.size(); j-- > 0;)
            {
                const COutPoint &out = tx.vin[j].prevout;
                const CTxInUndo &undo = txundo.vprevout[j];
                if (!ApplyTxInUndo(undo, view, out))
                    fClean = false;
            }
        }
    }

    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());

    if (pfClean)
    {
        *pfClean = fClean;
        return true;
    }

    return fClean;
}

void static FlushBlockFile(bool fFinalize = false)
{
    LOCK(cs_LastBlockFile);

    CDiskBlockPos posOld(nLastBlockFile, 0);

    FILE *fileOld = OpenBlockFile(posOld);
    if (fileOld)
    {
        if (fFinalize)
            TruncateFile(fileOld, vinfoBlockFile[nLastBlockFile].nSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }

    fileOld = OpenUndoFile(posOld);
    if (fileOld)
    {
        if (fFinalize)
            TruncateFile(fileOld, vinfoBlockFile[nLastBlockFile].nUndoSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }
}

bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize);


//
// Called periodically asynchronously; alerts if it smells like
// we're being fed a bad chain (blocks being generated much
// too slowly or too quickly).
//
void PartitionCheck(bool (*initialDownloadCheck)(),
    CCriticalSection &cs,
    const CBlockIndex *const &bestHeader,
    int64_t nPowTargetSpacing)
{
    if (bestHeader == NULL || initialDownloadCheck())
        return;

    static int64_t lastAlertTime = 0;
    int64_t now = GetAdjustedTime();
    if (lastAlertTime > now - 60 * 60 * 24)
        return; // Alert at most once per day

    const int SPAN_HOURS = 4;
    const int SPAN_SECONDS = SPAN_HOURS * 60 * 60;
    int BLOCKS_EXPECTED = SPAN_SECONDS / nPowTargetSpacing;

    boost::math::poisson_distribution<double> poisson(BLOCKS_EXPECTED);

    std::string strWarning;
    int64_t startTime = GetAdjustedTime() - SPAN_SECONDS;

    LOCK(cs);
    const CBlockIndex *i = bestHeader;
    int nBlocks = 0;
    while (i->GetBlockTime() >= startTime)
    {
        ++nBlocks;
        i = i->pprev;
        if (i == NULL)
            return; // Ran out of chain, we must not be fully sync'ed
    }

    // How likely is it to find that many by chance?
    double p = boost::math::pdf(poisson, nBlocks);

    LogPrint("partitioncheck", "%s: Found %d blocks in the last %d hours\n", __func__, nBlocks, SPAN_HOURS);
    LogPrint("partitioncheck", "%s: likelihood: %g\n", __func__, p);

    // Aim for one false-positive about every fifty years of normal running:
    const int FIFTY_YEARS = 50 * 365 * 24 * 60 * 60;
    double alertThreshold = 1.0 / (FIFTY_YEARS / SPAN_SECONDS);

    if (p <= alertThreshold && nBlocks < BLOCKS_EXPECTED)
    {
        // Many fewer blocks than expected: alert!
        strWarning = strprintf(
            _("WARNING: check your network connection, %d blocks received in the last %d hours (%d expected)"), nBlocks,
            SPAN_HOURS, BLOCKS_EXPECTED);
    }
    else if (p <= alertThreshold && nBlocks > BLOCKS_EXPECTED)
    {
        // Many more blocks than expected: alert!
        strWarning = strprintf(_("WARNING: abnormally high number of blocks generated, %d blocks received in the last "
                                 "%d hours (%d expected)"),
            nBlocks, SPAN_HOURS, BLOCKS_EXPECTED);
    }
    if (!strWarning.empty())
    {
        strMiscWarning = strWarning;
        AlertNotify(strWarning);
        lastAlertTime = now;
    }
}

// Protected by cs_main
VersionBitsCache versionbitscache;

int32_t ComputeBlockVersion(const CBlockIndex *pindexPrev, const Consensus::Params &params)
{
    LOCK(cs_main);
    int32_t nVersion = VERSIONBITS_TOP_BITS;

    for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++)
    {
        ThresholdState state = VersionBitsState(pindexPrev, params, (Consensus::DeploymentPos)i, versionbitscache);
        if (state == THRESHOLD_LOCKED_IN || state == THRESHOLD_STARTED)
        {
            nVersion |= VersionBitsMask(params, (Consensus::DeploymentPos)i);
        }
    }

    return nVersion;
}

/**
 * Threshold condition checker that triggers when unknown versionbits are seen on the network.
 */
class WarningBitsConditionChecker : public AbstractThresholdConditionChecker
{
private:
    int bit;

public:
    WarningBitsConditionChecker(int bitIn) : bit(bitIn) {}
    int64_t BeginTime(const Consensus::Params &params) const { return 0; }
    int64_t EndTime(const Consensus::Params &params) const { return std::numeric_limits<int64_t>::max(); }
    int Period(const Consensus::Params &params) const { return params.nMinerConfirmationWindow; }
    int Threshold(const Consensus::Params &params) const { return params.nRuleChangeActivationThreshold; }
    bool Condition(const CBlockIndex *pindex, const Consensus::Params &params) const
    {
        return ((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) &&
               ((pindex->nVersion >> bit) & 1) != 0 &&
               ((UnlimitedComputeBlockVersion(pindex->pprev, params, pindex->nTime) >> bit) & 1) == 0;
    }
};

// Protected by cs_main
static ThresholdConditionCache warningcache[VERSIONBITS_NUM_BITS];

static int64_t nTimeCheck = 0;
static int64_t nTimeForks = 0;
static int64_t nTimeVerify = 0;
static int64_t nTimeConnect = 0;
static int64_t nTimeIndex = 0;
static int64_t nTimeCallbacks = 0;
static int64_t nTimeTotal = 0;

bool ConnectBlock(const CBlock &block,
    CValidationState &state,
    CBlockIndex *pindex,
    CCoinsViewCache &view,
    bool fJustCheck,
    bool fParallel)
{
    /** BU: Start Section to validate inputs - if there are parallel blocks being checked
     *      then the winner of this race will get to update the UTXO.
     */

    const CChainParams &chainparams = Params();
    AssertLockHeld(cs_main);

    int64_t nTimeStart = GetTimeMicros();

    // Check it again in case a previous version let a bad block in
    if (!CheckBlock(block, state, !fJustCheck, !fJustCheck))
        return false;

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == NULL ? uint256() : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == chainparams.GetConsensus().hashGenesisBlock)
    {
        if (!fJustCheck)
            view.SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    const int64_t timeBarrier = GetTime() - (24 * 3600 * checkScriptDays.value);
    // Blocks that have various days of POW behind them makes them secure in that
    // real online nodes have checked the scripts.  Therefore, during initial block
    // download we don't need to check most of those scripts except for the most
    // recent ones.
    bool fScriptChecks = true;
    if (pindexBestHeader)
        fScriptChecks = !fCheckpointsEnabled || block.nTime > timeBarrier ||
                        (uint32_t)pindex->nHeight > pindexBestHeader->nHeight - (144 * checkScriptDays.value);

    int64_t nTime1 = GetTimeMicros();
    nTimeCheck += nTime1 - nTimeStart;
    LogPrint("bench", "    - Sanity checks: %.2fms [%.2fs]\n", 0.001 * (nTime1 - nTimeStart), nTimeCheck * 0.000001);

    // Do not allow blocks that contain transactions which 'overwrite' older transactions,
    // unless those are already completely spent.
    // If such overwrites are allowed, coinbases and transactions depending upon those
    // can be duplicated to remove the ability to spend the first instance -- even after
    // being sent to another address.
    // See BIP30 and http://r6.ca/blog/20120206T005236Z.html for more information.
    // This logic is not necessary for memory pool transactions, as AcceptToMemoryPool
    // already refuses previously-known transaction ids entirely.
    // This rule was originally applied to all blocks with a timestamp after March 15, 2012, 0:00 UTC.
    // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
    // two in the chain that violate it. This prevents exploiting the issue against nodes during their
    // initial block download.
    bool fEnforceBIP30 = (!pindex->phashBlock) || // Enforce on CreateNewBlock invocations which don't have a hash.
                         !((pindex->nHeight == 91842 &&
                               pindex->GetBlockHash() ==
                                   uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                             (pindex->nHeight == 91880 &&
                                 pindex->GetBlockHash() ==
                                     uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));

    // Once BIP34 activated it was not possible to create new duplicate coinbases and thus other than starting
    // with the 2 existing duplicate coinbase pairs, not possible to create overwriting txs.  But by the
    // time BIP34 activated, in each of the existing pairs the duplicate coinbase had overwritten the first
    // before the first had been spent.  Since those coinbases are sufficiently buried its no longer possible to create
    // further
    // duplicate transactions descending from the known pairs either.
    // If we're on the known chain at height greater than where BIP34 activated, we can save the db accesses needed for
    // the BIP30 check.
    if (pindex->pprev) // If this isn't the genesis block
    {
        CBlockIndex *pindexBIP34height = pindex->pprev->GetAncestor(chainparams.GetConsensus().BIP34Height);
        // Only continue to enforce if we're below BIP34 activation height or the block hash at that height doesn't
        // correspond.
        fEnforceBIP30 =
            fEnforceBIP30 &&
            (!pindexBIP34height || !(pindexBIP34height->GetBlockHash() == chainparams.GetConsensus().BIP34Hash));

        if (fEnforceBIP30)
        {
            BOOST_FOREACH (const CTransaction &tx, block.vtx)
            {
                const CCoins *coins = view.AccessCoins(tx.GetHash());
                if (coins && !coins->IsPruned())
                    return state.DoS(
                        100, error("ConnectBlock(): tried to overwrite transaction"), REJECT_INVALID, "bad-txns-BIP30");
            }
        }
    }
    // BIP16 didn't become active until Apr 1 2012
    int64_t nBIP16SwitchTime = 1333238400;
    bool fStrictPayToScriptHash = (pindex->GetBlockTime() >= nBIP16SwitchTime);

    unsigned int flags = fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE;

    if (pindex->forkActivated(miningForkTime.value))
    {
        flags |= SCRIPT_ENABLE_SIGHASH_FORKID;
    }

    // Start enforcing the DERSIG (BIP66) rules, for block.nVersion=3 blocks,
    // when 75% of the network has upgraded:
    if (block.nVersion >= 3 && IsSuperMajority(3, pindex->pprev,
                                   chainparams.GetConsensus().nMajorityEnforceBlockUpgrade, chainparams.GetConsensus()))
    {
        flags |= SCRIPT_VERIFY_DERSIG;
    }

    // Start enforcing CHECKLOCKTIMEVERIFY, (BIP65) for block.nVersion=4
    // blocks, when 75% of the network has upgraded:
    if (block.nVersion >= 4 && IsSuperMajority(4, pindex->pprev,
                                   chainparams.GetConsensus().nMajorityEnforceBlockUpgrade, chainparams.GetConsensus()))
    {
        flags |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;
    }

    // Start enforcing BIP68 (sequence locks) and BIP112 (CHECKSEQUENCEVERIFY) using versionbits logic.
    int nLockTimeFlags = 0;
    if (VersionBitsState(pindex->pprev, chainparams.GetConsensus(), Consensus::DEPLOYMENT_CSV, versionbitscache) ==
        THRESHOLD_ACTIVE)
    {
        flags |= SCRIPT_VERIFY_CHECKSEQUENCEVERIFY;
        nLockTimeFlags |= LOCKTIME_VERIFY_SEQUENCE;
    }

    int64_t nTime2 = GetTimeMicros();
    nTimeForks += nTime2 - nTime1;
    LogPrint("bench", "    - Fork checks: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeForks * 0.000001);

    CBlockUndo blockundo;
    ValidationResourceTracker resourceTracker;
    std::vector<int> prevheights;
    CAmount nFees = 0;
    int nInputs = 0;
    unsigned int nSigOps = 0;
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos> > vPos;
    vPos.reserve(block.vtx.size());
    blockundo.vtxundo.reserve(block.vtx.size() - 1);
    int nChecked = 0;
    int nOrphansChecked = 0;
    const arith_uint256 nStartingChainWork = chainActive.Tip()->nChainWork;

    // Create a vector for storing hashes that will be deleted from the unverified and perverified txn sets.
    // We will delete these hashes only if and when this block is the one that is accepted saving us the unnecessary
    // repeated locking and unlocking of cs_xval.
    std::vector<uint256> vHashesToDelete;

    // Create a temporary view of the UTXO set
    CCoinsViewCache viewTempCache(pcoinsTip);

    // Section for boost scoped lock on the scriptcheck_mutex
    boost::thread::id this_id(boost::this_thread::get_id());

    // Get the next available mutex and the associated scriptcheckqueue. Then lock this thread
    // with the mutex so that the checking of inputs can be done with the chosen scriptcheckqueue.
    CCheckQueue<CScriptCheck> *pScriptQueue(PV->GetScriptCheckQueue());

    // Aquire the control that is used to wait for the script threads to finish. Do this after aquiring the
    // scoped lock to ensure the scriptqueue is free and available.
    CCheckQueueControl<CScriptCheck> control(fScriptChecks && PV->ThreadCount() ? pScriptQueue : NULL);

    // Initialize a PV session.
    if (!PV->Initialize(this_id, pindex, fParallel))
        return false;

    /*********************************************************************************************
     If in PV, unlock cs_main here so we have no contention when we're checking inputs and scripts
     *********************************************************************************************/
    if (fParallel)
        LEAVE_CRITICAL_SECTION(cs_main);

    // Begin Section for Boost Scope Guard
    {
        // Scope guard to make sure cs_main is set and resources released if we encounter an exception.
        BOOST_SCOPE_EXIT(&PV, &fParallel) { PV->SetLocks(fParallel); }
        BOOST_SCOPE_EXIT_END


        // Start checking Inputs
        bool inOrphanCache;
        bool inVerifiedCache;
        // When in parallel mode then unlock cs_main for this loop to give any other threads
        // a chance to process in parallel. This is crucial for parallel validation to work.
        // NOTE: the only place where cs_main is needed is if we hit PV->ChainWorkHasChanged, which
        //       internally grabs the cs_main lock when needed.
        for (unsigned int i = 0; i < block.vtx.size(); i++)
        {
            const CTransaction &tx = block.vtx[i];

            nInputs += tx.vin.size();
            nSigOps += GetLegacySigOpCount(tx);
            // if (nSigOps > MAX_BLOCK_SIGOPS)
            //    return state.DoS(100, error("ConnectBlock(): too many sigops"),
            //                    REJECT_INVALID, "bad-blk-sigops");

            if (!tx.IsCoinBase())
            {
                if (!viewTempCache.HaveInputs(tx))
                {
                    // If we were validating at the same time as another block and the other block wins the validation
                    // race
                    // and updates the UTXO first, then we may end up here with missing inputs.  Therefore we checke to
                    // see
                    // if the chainwork has advanced or if we recieved a quit and if so return without DOSing the node.
                    if (PV->ChainWorkHasChanged(nStartingChainWork) || PV->QuitReceived(this_id, fParallel))
                    {
                        return false;
                    }
                    return state.DoS(100, error("ConnectBlock(): inputs missing/spent"), REJECT_INVALID,
                        "bad-txns-inputs-missingorspent");
                }

                // Check that transaction is BIP68 final
                // BIP68 lock checks (as opposed to nLockTime checks) must
                // be in ConnectBlock because they require the UTXO set
                prevheights.resize(tx.vin.size());
                for (size_t j = 0; j < tx.vin.size(); j++)
                {
                    prevheights[j] = viewTempCache.AccessCoins(tx.vin[j].prevout.hash)->nHeight;
                }

                if (!SequenceLocks(tx, nLockTimeFlags, &prevheights, *pindex))
                {
                    return state.DoS(100, error("%s: contains a non-BIP68-final transaction", __func__), REJECT_INVALID,
                        "bad-txns-nonfinal");
                }

                if (fStrictPayToScriptHash)
                {
                    // Add in sigops done by pay-to-script-hash inputs;
                    // this is to prevent a "rogue miner" from creating
                    // an incredibly-expensive-to-validate block.
                    nSigOps += GetP2SHSigOpCount(tx, viewTempCache);
                    // if (nSigOps > MAX_BLOCK_SIGOPS)
                    //    return state.DoS(100, error("ConnectBlock(): too many sigops"),
                    //                     REJECT_INVALID, "bad-blk-sigops");
                }

                nFees += viewTempCache.GetValueIn(tx) - tx.GetValueOut();

                // Only check inputs when the tx hash in not in the setPreVerifiedTxHash as would only
                // happen if this were a regular block or when a tx is found within the returning XThinblock.
                uint256 hash = tx.GetHash();
                {
                    {
                        LOCK(cs_xval);
                        inOrphanCache = setUnVerifiedOrphanTxHash.count(hash);
                        inVerifiedCache = setPreVerifiedTxHash.count(hash);
                    } /* We don't want to hold the lock while inputs are being checked or we'll slow down the competing
                         thread, if there is one */

                    if ((inOrphanCache) || (!inVerifiedCache && !inOrphanCache))
                    {
                        LogPrint("parallel_2", "checking inputs for tx: %d\n", i);
                        if (inOrphanCache)
                            nOrphansChecked++;

                        std::vector<CScriptCheck> vChecks;
                        bool fCacheResults = fJustCheck; /* Don't cache results if we're actually connecting blocks
                                                            (still consult the cache, though) */
                        if (!CheckInputs(tx, state, viewTempCache, fScriptChecks, flags, fCacheResults,
                                &resourceTracker, PV->ThreadCount() ? &vChecks : NULL))
                        {
                            return error("ConnectBlock(): CheckInputs on %s failed with %s", tx.GetHash().ToString(),
                                FormatStateMessage(state));
                        }
                        control.Add(vChecks);
                        nChecked++;
                    }
                    else
                    {
                        vHashesToDelete.push_back(hash);
                    }
                }
            }

            CTxUndo undoDummy;
            if (i > 0)
            {
                blockundo.vtxundo.push_back(CTxUndo());
            }
            UpdateCoins(tx, state, viewTempCache, i == 0 ? undoDummy : blockundo.vtxundo.back(), pindex->nHeight);
            vPos.push_back(std::make_pair(tx.GetHash(), pos));
            pos.nTxOffset += ::GetSerializeSize(tx, SER_DISK, CLIENT_VERSION);

            if (PV->QuitReceived(this_id, fParallel))
            {
                return false;
            }

            // This is for testing PV and slowing down the validation of inputs. This makes it easier to create
            // and run python regression tests and is an testing feature.
            if (GetArg("-pvtest", false))
                MilliSleep(1000);
        }
        LogPrint("thin", "Number of CheckInputs() performed: %d  Orphan count: %d\n", nChecked, nOrphansChecked);


        // Wait for all sig check threads to finish before updating utxo
        LogPrint("parallel", "Waiting for script threads to finish\n");
        if (!control.Wait())
        {
            // if we end up here then the signature verification failed and we must re-lock cs_main before returning.
            return state.DoS(100, false);
        }
        if (PV->QuitReceived(this_id, fParallel))
        {
            return false;
        }

        CAmount blockReward = nFees + GetBlockSubsidy(pindex->nHeight, chainparams.GetConsensus());
        if (block.vtx[0].GetValueOut() > blockReward)
            return state.DoS(100, error("ConnectBlock(): coinbase pays too much (actual=%d vs limit=%d)",
                                      block.vtx[0].GetValueOut(), blockReward),
                REJECT_INVALID, "bad-cb-amount");

        if (fJustCheck)
            return true;


        /*****************************************************************************************************************
         *                         Start update of UTXO, if this block wins the validation race *
         *****************************************************************************************************************/
        // If in PV mode and we win the race then we lock everyone out by taking cs_main but before updating the UTXO
        // and
        // terminating any competing threads.
    } // cs_main is re-aquired automatically as we go out of scope from the BOOST scope guard

    // Last check for chain work just in case the thread manages to get here before being terminated.
    if (PV->ChainWorkHasChanged(nStartingChainWork) || PV->QuitReceived(this_id, fParallel))
    {
        return false; // no need to lock cs_main before returning as it should already be locked.
    }

    // Quit any competing threads may be validating which have the same previous block before updating the UTXO.
    PV->QuitCompetingThreads(block.GetBlockHeader().hashPrevBlock);

    // Flush the temporary UTXO view to the base view (the in memory UTXO main cache)
    int64_t nUpdateCoinsTimeBegin = GetTimeMicros();
    LogPrint("parallel", "Updating UTXO for %s\n", block.GetHash().ToString());
    viewTempCache.Flush();

    int64_t nUpdateCoinsTimeEnd = GetTimeMicros();
    LogPrint("bench", "      - Update Coins %.3fms\n", nUpdateCoinsTimeEnd - nUpdateCoinsTimeBegin);

    int64_t nTime3 = GetTimeMicros();
    nTimeConnect += nTime3 - nTime2;
    LogPrint("bench", "      - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin) [%.2fs]\n",
        (unsigned)block.vtx.size(), 0.001 * (nTime3 - nTime2), 0.001 * (nTime3 - nTime2) / block.vtx.size(),
        nInputs <= 1 ? 0 : 0.001 * (nTime3 - nTime2) / (nInputs - 1), nTimeConnect * 0.000001);

    int64_t nTime4 = GetTimeMicros();
    nTimeVerify += nTime4 - nTime2;
    LogPrint("bench", "    - Verify %u txins: %.2fms (%.3fms/txin) [%.2fs]\n", nInputs - 1, 0.001 * (nTime4 - nTime2),
        nInputs <= 1 ? 0 : 0.001 * (nTime4 - nTime2) / (nInputs - 1), nTimeVerify * 0.000001);


    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull() || !pindex->IsValid(BLOCK_VALID_SCRIPTS))
    {
        if (pindex->GetUndoPos().IsNull())
        {
            CDiskBlockPos pos;
            if (!FindUndoPos(state, pindex->nFile, pos, ::GetSerializeSize(blockundo, SER_DISK, CLIENT_VERSION) + 40))
                return error("ConnectBlock(): FindUndoPos failed");


            uint256 prevHash;
            if (pindex->pprev) // genesis block prev hash is 0
                prevHash = pindex->pprev->GetBlockHash();
            else
                prevHash.SetNull();
            if (!UndoWriteToDisk(blockundo, pos, prevHash, chainparams.MessageStart()))
                return AbortNode(state, "Failed to write undo data");


            // update nUndoPos in block index
            pindex->nUndoPos = pos.nPos;
            pindex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);
        setDirtyBlockIndex.insert(pindex);
    }

    if (fTxIndex)
        if (!pblocktree->WriteTxIndex(vPos))
            return AbortNode(state, "Failed to write transaction index");

    // add this block to the view's block chain (the main UTXO in memory cache)
    view.SetBestBlock(pindex->GetBlockHash());

    int64_t nTime5 = GetTimeMicros();
    nTimeIndex += nTime5 - nTime4;
    LogPrint("bench", "    - Index writing: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4), nTimeIndex * 0.000001);

    // Watch for changes to the previous coinbase transaction.
    static uint256 hashPrevBestCoinBase;
    GetMainSignals().UpdatedTransaction(hashPrevBestCoinBase);
    hashPrevBestCoinBase = block.vtx[0].GetHash();

    int64_t nTime6 = GetTimeMicros();
    nTimeCallbacks += nTime6 - nTime5;
    LogPrint("bench", "    - Callbacks: %.2fms [%.2fs]\n", 0.001 * (nTime6 - nTime5), nTimeCallbacks * 0.000001);

    PV->Cleanup(block, pindex); // NOTE: this must be run whether in fParallel or not!

    // Delete hashes from unverified and preverified sets that will no longer be needed after the block is accepted.
    {
        LOCK(cs_xval);
        BOOST_FOREACH (const uint256 hash, vHashesToDelete)
        {
            setPreVerifiedTxHash.erase(hash);
            setUnVerifiedOrphanTxHash.erase(hash);
        }
    }

    return true;
}

enum FlushStateMode
{
    FLUSH_STATE_NONE,
    FLUSH_STATE_IF_NEEDED,
    FLUSH_STATE_PERIODIC,
    FLUSH_STATE_ALWAYS
};

/**
 * Update the on-disk chain state.
 * The caches and indexes are flushed depending on the mode we're called with
 * if they're too large, if it's been a while since the last write,
 * or always and in all cases if we're in prune mode and are deleting files.
 */
bool static FlushStateToDisk(CValidationState &state, FlushStateMode mode)
{
    const CChainParams &chainparams = Params();
    LOCK2(cs_main, cs_LastBlockFile);
    static int64_t nLastWrite = 0;
    static int64_t nLastFlush = 0;
    static int64_t nLastSetChain = 0;
    std::set<int> setFilesToPrune;
    bool fFlushForPrune = false;
    try
    {
        if (fPruneMode && fCheckForPruning && !fReindex)
        {
            FindFilesToPrune(setFilesToPrune, chainparams.PruneAfterHeight());
            fCheckForPruning = false;
            if (!setFilesToPrune.empty())
            {
                fFlushForPrune = true;
                if (!fHavePruned)
                {
                    pblocktree->WriteFlag("prunedblockfiles", true);
                    fHavePruned = true;
                }
            }
        }
        int64_t nNow = GetTimeMicros();
        // Avoid writing/flushing immediately after startup.
        if (nLastWrite == 0)
        {
            nLastWrite = nNow;
        }
        if (nLastFlush == 0)
        {
            nLastFlush = nNow;
        }
        if (nLastSetChain == 0)
        {
            nLastSetChain = nNow;
        }
        size_t cacheSize = pcoinsTip->DynamicMemoryUsage();
        // The cache is large and close to the limit, but we have time now (not in the middle of a block processing).
        bool fCacheLarge = mode == FLUSH_STATE_PERIODIC && cacheSize * (10.0 / 9) > nCoinCacheUsage;
        // The cache is over the limit, we have to write now.
        bool fCacheCritical = mode == FLUSH_STATE_IF_NEEDED && cacheSize > nCoinCacheUsage;
        // It's been a while since we wrote the block index to disk. Do this frequently, so we don't need to redownload
        // after a crash.
        bool fPeriodicWrite =
            mode == FLUSH_STATE_PERIODIC && nNow > nLastWrite + (int64_t)DATABASE_WRITE_INTERVAL * 1000000;
        // It's been very long since we flushed the cache. Do this infrequently, to optimize cache usage.
        bool fPeriodicFlush =
            mode == FLUSH_STATE_PERIODIC && nNow > nLastFlush + (int64_t)DATABASE_FLUSH_INTERVAL * 1000000;
        // Combine all conditions that result in a full cache flush.
        bool fDoFullFlush =
            (mode == FLUSH_STATE_ALWAYS) || fCacheLarge || fCacheCritical || fPeriodicFlush || fFlushForPrune;
        // Write blocks and block index to disk.
        if (fDoFullFlush || fPeriodicWrite)
        {
            // Depend on nMinDiskSpace to ensure we can write block index
            if (!CheckDiskSpace(0))
                return state.Error("out of disk space");
            // First make sure all block and undo data is flushed to disk.
            FlushBlockFile();
            // Then update all block file information (which may refer to block and undo files).
            {
                std::vector<std::pair<int, const CBlockFileInfo *> > vFiles;
                vFiles.reserve(setDirtyFileInfo.size());
                for (std::set<int>::iterator it = setDirtyFileInfo.begin(); it != setDirtyFileInfo.end();)
                {
                    vFiles.push_back(std::make_pair(*it, &vinfoBlockFile[*it]));
                    setDirtyFileInfo.erase(it++);
                }
                std::vector<const CBlockIndex *> vBlocks;
                vBlocks.reserve(setDirtyBlockIndex.size());
                for (std::set<CBlockIndex *>::iterator it = setDirtyBlockIndex.begin(); it != setDirtyBlockIndex.end();)
                {
                    vBlocks.push_back(*it);
                    setDirtyBlockIndex.erase(it++);
                }
                if (!pblocktree->WriteBatchSync(vFiles, nLastBlockFile, vBlocks))
                {
                    return AbortNode(state, "Files to write to block index database");
                }
            }
            // Finally remove any pruned files
            if (fFlushForPrune)
                UnlinkPrunedFiles(setFilesToPrune);
            nLastWrite = nNow;
        }
        // Flush best chain related state. This can only be done if the blocks / block index write was also done.
        if (fDoFullFlush)
        {
            // Typical CCoins structures on disk are around 128 bytes in size.
            // Pushing a new one to the database can cause it to be written
            // twice (once in the log, and once in the tables). This is already
            // an overestimation, as most will delete an existing entry or
            // overwrite one. Still, use a conservative safety factor of 2.
            if (!CheckDiskSpace(128 * 2 * 2 * pcoinsTip->GetCacheSize()))
                return state.Error("out of disk space");
            // Flush the chainstate (which may refer to block index entries).
            if (!pcoinsTip->Flush())
                return AbortNode(state, "Failed to write to coin database");
            nLastFlush = nNow;
        }
        if (fDoFullFlush || ((mode == FLUSH_STATE_ALWAYS || mode == FLUSH_STATE_PERIODIC) &&
                                nNow > nLastSetChain + (int64_t)DATABASE_WRITE_INTERVAL * 1000000))
        {
            // Update best block in wallet (so we can detect restored wallets).
            GetMainSignals().SetBestChain(chainActive.GetLocator());
            nLastSetChain = nNow;
        }
    }
    catch (const std::runtime_error &e)
    {
        return AbortNode(state, std::string("System error while flushing: ") + e.what());
    }
    return true;
}

void FlushStateToDisk()
{
    CValidationState state;
    FlushStateToDisk(state, FLUSH_STATE_ALWAYS);
}

void PruneAndFlush()
{
    CValidationState state;
    fCheckForPruning = true;
    FlushStateToDisk(state, FLUSH_STATE_NONE);
}

/** Update chainActive and related internal data structures. */
void static UpdateTip(CBlockIndex *pindexNew)
{
    const CChainParams &chainParams = Params();
    chainActive.SetTip(pindexNew);

    // New best block
    nTimeBestReceived = GetTime();
    mempool.AddTransactionsUpdated(1);

    LogPrintf("%s: new best=%s  height=%d bits=%d log2_work=%.8g  tx=%lu  date=%s progress=%f  cache=%.1fMiB(%utx)\n",
        __func__, chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(), chainActive.Tip()->nBits,
        log(chainActive.Tip()->nChainWork.getdouble()) / log(2.0), (unsigned long)chainActive.Tip()->nChainTx,
        DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
        Checkpoints::GuessVerificationProgress(chainParams.Checkpoints(), chainActive.Tip()),
        pcoinsTip->DynamicMemoryUsage() * (1.0 / (1 << 20)), pcoinsTip->GetCacheSize());

    UpdateBUIP055Globals(pindexNew);
    cvBlockChange.notify_all();

    // Check the version of the last 100 blocks to see if we need to upgrade:
    static bool fWarned = false;
    if (!IsInitialBlockDownload())
    {
        int nUpgraded = 0;
        const CBlockIndex *pindex = chainActive.Tip();
        for (int bit = 0; bit < VERSIONBITS_NUM_BITS; bit++)
        {
            WarningBitsConditionChecker checker(bit);
            ThresholdState state = checker.GetStateFor(pindex, chainParams.GetConsensus(), warningcache[bit]);
            if (state == THRESHOLD_ACTIVE || state == THRESHOLD_LOCKED_IN)
            {
                if (state == THRESHOLD_ACTIVE)
                {
                    strMiscWarning = strprintf(_("Warning: unknown new rules activated (versionbit %i)"), bit);
                    if (!fWarned)
                    {
                        AlertNotify(strMiscWarning);
                        fWarned = true;
                    }
                }
                else
                {
                    LogPrintf("%s: unknown new rules are about to activate (versionbit %i)\n", __func__, bit);
                }
            }
        }
        int32_t anUnexpectedVersion = 0;
        for (int i = 0; i < 100 && pindex != NULL; i++)
        {
            int32_t nExpectedVersion =
                UnlimitedComputeBlockVersion(pindex->pprev, chainParams.GetConsensus(), pindex->nTime);
            if (pindex->nVersion > VERSIONBITS_LAST_OLD_BLOCK_VERSION && (pindex->nVersion & ~nExpectedVersion) != 0)
            {
                anUnexpectedVersion = pindex->nVersion;
                ++nUpgraded;
            }
            pindex = pindex->pprev;
        }
        if (nUpgraded > 0)
            LogPrintf("%s: %d of last 100 blocks have unexpected version. One example: 0x%x\n", __func__, nUpgraded,
                anUnexpectedVersion);
        if (nUpgraded > 100 / 2)
        {
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning =
                _("Warning: Unknown block versions being mined! It's possible unknown rules are in effect");
            if (!fWarned)
            {
                AlertNotify(strMiscWarning);
                fWarned = true;
            }
        }
    }
}

/** Disconnect chainActive's tip. You probably want to call mempool.removeForReorg and manually re-limit mempool size
 * after this, with cs_main held. */
bool static DisconnectTip(CValidationState &state, const Consensus::Params &consensusParams)
{
    AssertLockHeld(cs_main);

    CBlockIndex *pindexDelete = chainActive.Tip();
    assert(pindexDelete);
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(block, pindexDelete, consensusParams))
        return AbortNode(state, "Failed to read block");
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CCoinsViewCache view(pcoinsTip);
        if (!DisconnectBlock(block, state, pindexDelete, view))
            return error("DisconnectTip(): DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
        bool result = view.Flush();
        assert(result);
    }
    LogPrint("bench", "- Disconnect block: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FLUSH_STATE_IF_NEEDED))
        return false;
    // Resurrect mempool transactions from the disconnected block.
    std::vector<uint256> vHashUpdate;
    BOOST_FOREACH (const CTransaction &tx, block.vtx)
    {
        // ignore validation errors in resurrected transactions
        std::list<CTransaction> removed;
        CValidationState stateDummy;
        if (tx.IsCoinBase() || !AcceptToMemoryPool(mempool, stateDummy, tx, false, NULL, true))
        {
            mempool.remove(tx, removed, true);
        }
        else if (mempool.exists(tx.GetHash()))
        {
            vHashUpdate.push_back(tx.GetHash());
        }
    }
    // AcceptToMemoryPool/addUnchecked all assume that new mempool entries have
    // no in-mempool children, which is generally not true when adding
    // previously-confirmed transactions back to the mempool.
    // UpdateTransactionsFromBlock finds descendants of any transactions in this
    // block that were added back and cleans up the mempool state.
    mempool.UpdateTransactionsFromBlock(vHashUpdate);
    // Update chainActive and related variables.
    UpdateTip(pindexDelete->pprev);
    // Let wallets know transactions went from 1-confirmed to
    // 0-confirmed or conflicted:
    BOOST_FOREACH (const CTransaction &tx, block.vtx)
    {
        SyncWithWallets(tx, NULL);
    }

    return true;
}

static int64_t nTimeReadFromDisk = 0;
static int64_t nTimeConnectTotal = 0;
static int64_t nTimeFlush = 0;
static int64_t nTimeChainState = 0;
static int64_t nTimePostConnect = 0;

/**
 * Connect a new block to chainActive. pblock is either NULL or a pointer to a CBlock
 * corresponding to pindexNew, to bypass loading it again from disk.
 */
bool static ConnectTip(CValidationState &state,
    const CChainParams &chainparams,
    CBlockIndex *pindexNew,
    const CBlock *pblock,
    bool fParallel)
{
    AssertLockHeld(cs_main);

    // With PV there is a special case where one chain may be in the process of connecting several blocks but then
    // a second chain also begins to connect blocks and its block beat the first chains block to advance the tip.
    // As a result pindexNew->prev on the first chain will no longer match the chaintip as the second chain continues
    // connecting blocks. Therefore we must return "false" rather than "assert" as was previously the case.
    // assert(pindexNew->pprev == chainActive.Tip());
    if (pindexNew->pprev != chainActive.Tip())
        return false;

    // Read block from disk.
    int64_t nTime1 = GetTimeMicros();
    CBlock block;
    if (!pblock)
    {
        if (!ReadBlockFromDisk(block, pindexNew, chainparams.GetConsensus()))
            return AbortNode(state, "Failed to read block");
        pblock = &block;
    }
    // Apply the block atomically to the chain state.
    int64_t nTime2 = GetTimeMicros();
    nTimeReadFromDisk += nTime2 - nTime1;
    int64_t nTime3;
    LogPrint(
        "bench", "  - Load block from disk: %.2fms [%.2fs]\n", (nTime2 - nTime1) * 0.001, nTimeReadFromDisk * 0.000001);
    {
        CCoinsViewCache view(pcoinsTip);
        bool rv = ConnectBlock(*pblock, state, pindexNew, view, false, fParallel);
        GetMainSignals().BlockChecked(*pblock, state);
        if (!rv)
        {
            if (state.IsInvalid())
            {
                InvalidBlockFound(pindexNew, state);
                return error("ConnectTip(): ConnectBlock %s failed", pindexNew->GetBlockHash().ToString());
            }
            return false;
        }
        mapBlockSource.erase(pindexNew->GetBlockHash());
        nTime3 = GetTimeMicros();
        nTimeConnectTotal += nTime3 - nTime2;
        LogPrint(
            "bench", "  - Connect total: %.2fms [%.2fs]\n", (nTime3 - nTime2) * 0.001, nTimeConnectTotal * 0.000001);
        bool result = view.Flush();
        assert(result);
    }
    int64_t nTime4 = GetTimeMicros();
    nTimeFlush += nTime4 - nTime3;
    LogPrint("bench", "  - Flush: %.2fms [%.2fs]\n", (nTime4 - nTime3) * 0.001, nTimeFlush * 0.000001);
    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FLUSH_STATE_IF_NEEDED))
        return false;
    int64_t nTime5 = GetTimeMicros();
    nTimeChainState += nTime5 - nTime4;
    LogPrint(
        "bench", "  - Writing chainstate: %.2fms [%.2fs]\n", (nTime5 - nTime4) * 0.001, nTimeChainState * 0.000001);
    // Remove conflicting transactions from the mempool.
    std::list<CTransaction> txConflicted;
    mempool.removeForBlock(pblock->vtx, pindexNew->nHeight, txConflicted, !IsInitialBlockDownload());
    // Update chainActive & related variables.
    UpdateTip(pindexNew);
    // Tell wallet about transactions that went from mempool
    // to conflicted:
    BOOST_FOREACH (const CTransaction &tx, txConflicted)
    {
        SyncWithWallets(tx, NULL);
    }
    // ... and about transactions that got confirmed:
    BOOST_FOREACH (const CTransaction &tx, pblock->vtx)
    {
        SyncWithWallets(tx, pblock);
    }

    int64_t nTime6 = GetTimeMicros();
    nTimePostConnect += nTime6 - nTime5;
    nTimeTotal += nTime6 - nTime1;
    LogPrint(
        "bench", "  - Connect postprocess: %.2fms [%.2fs]\n", (nTime6 - nTime5) * 0.001, nTimePostConnect * 0.000001);
    LogPrint("bench", "- Connect block: %.2fms [%.2fs]\n", (nTime6 - nTime1) * 0.001, nTimeTotal * 0.000001);
    return true;
}

/**
 * Return the tip of the chain with the most work in it, that isn't
 * known to be invalid (it's however far from certain to be valid).
 */
CBlockIndex *FindMostWorkChain()
{
    do
    {
        CBlockIndex *pindexNew = NULL;

        // Find the best candidate header.
        {
            std::set<CBlockIndex *, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexCandidates.rbegin();
            if (it == setBlockIndexCandidates.rend())
                return NULL;
            pindexNew = *it;
        }

        // Check whether all blocks on the path between the currently active chain and the candidate are valid.
        // Just going until the active chain is an optimization, as we know all blocks in it are valid already.
        CBlockIndex *pindexTest = pindexNew;
        bool fInvalidAncestor = false;
        uint64_t depth = 0;
        bool fFailedChain = false;
        bool fMissingData = false;
        bool fRecentExcessive = false; // Has there been a excessive block within our accept depth?
        // Was there an excessive block prior to our accept depth (if so we ignore the accept depth -- this chain has
        // already been accepted as valid)
        bool fOldExcessive = false;
        // follow the chain all the way back to where it joins the current active chain.
        while (pindexTest && !chainActive.Contains(pindexTest))
        {
            assert(pindexTest->nChainTx || pindexTest->nHeight == 0);

            // Pruned nodes may have entries in setBlockIndexCandidates for
            // which block files have been deleted.  Remove those as candidates
            // for the most work chain if we come across them; we can't switch
            // to a chain unless we have all the non-active-chain parent blocks.
            fFailedChain = pindexTest->nStatus & BLOCK_FAILED_MASK;
            fMissingData = !(pindexTest->nStatus & BLOCK_HAVE_DATA);
            if (depth < excessiveAcceptDepth)
            {
                // Unlimited: deny this candidate chain if there's a recent excessive block
                fRecentExcessive |= ((pindexTest->nStatus & BLOCK_EXCESSIVE) != 0);
            }
            else
            {
                // Unlimited: unless there is an even older excessive block
                fOldExcessive |= ((pindexTest->nStatus & BLOCK_EXCESSIVE) != 0);
            }

            if (fFailedChain | fMissingData | fRecentExcessive)
                break;
            pindexTest = pindexTest->pprev;
            depth++;
        }

        // If there was a recent excessive block, check a certain distance beyond the acceptdepth to see if this chain
        // has already seen an excessive block... if it has then allow the chain.
        // This stops the client from always tracking excessiveDepth blocks behind the chain tip in a situation where
        // lots of excessive blocks are being created.
        // But after a while with no excessive blocks, we reset and our reluctance to accept an excessive block resumes
        // on this chain.
        // An alternate algorithm would be to move the excessive block size up to match the size of the accepted block,
        // but this changes a user-defined field and is awkward to code because
        // block sizes are not saved.
        if ((fRecentExcessive && !fOldExcessive) && (depth < excessiveAcceptDepth + EXCESSIVE_BLOCK_CHAIN_RESET))
        {
            CBlockIndex *chain = pindexTest;
            // skip accept depth blocks, we are looking for an older excessive
            while (chain && (depth < excessiveAcceptDepth))
            {
                chain = chain->pprev;
                depth++;
            }

            while (chain && (depth < excessiveAcceptDepth + EXCESSIVE_BLOCK_CHAIN_RESET))
            {
                fOldExcessive |= ((chain->nStatus & BLOCK_EXCESSIVE) != 0);
                chain = chain->pprev;
                depth++;
            }
        }

        // Conditions where we want to reject the chain
        if (fFailedChain || fMissingData || (fRecentExcessive && !fOldExcessive))
        {
            // Candidate chain is not usable (either invalid or missing data)
            if (fFailedChain && (pindexBestInvalid == NULL || pindexNew->nChainWork > pindexBestInvalid->nChainWork))
                pindexBestInvalid = pindexNew;
            CBlockIndex *pindexFailed = pindexNew;
            // Remove the entire chain from the set.
            while (pindexTest != pindexFailed)
            {
                if (fFailedChain)
                {
                    pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                }
                else if (fMissingData || (fRecentExcessive && !fOldExcessive))
                {
                    // If we're missing data, then add back to mapBlocksUnlinked,
                    // so that if the block arrives in the future we can try adding
                    // to setBlockIndexCandidates again.
                    mapBlocksUnlinked.insert(std::make_pair(pindexFailed->pprev, pindexFailed));
                }
                setBlockIndexCandidates.erase(pindexFailed);
                pindexFailed = pindexFailed->pprev;
            }
            setBlockIndexCandidates.erase(pindexTest);
            fInvalidAncestor = true;
        }

        if (!fInvalidAncestor)
            return pindexNew;
    } while (true);
    DbgAssert(0, return NULL); // should never get here
}

/** Delete all entries in setBlockIndexCandidates that are worse than the current tip. */
static void PruneBlockIndexCandidates()
{
    // Note that we can't delete the current block itself, as we may need to return to it later in case a
    // reorganization to a better block fails.
    std::set<CBlockIndex *, CBlockIndexWorkComparator>::iterator it = setBlockIndexCandidates.begin();
    while (it != setBlockIndexCandidates.end() && setBlockIndexCandidates.value_comp()(*it, chainActive.Tip()))
    {
        setBlockIndexCandidates.erase(it++);
    }
    // Either the current tip or a successor of it we're working towards is left in setBlockIndexCandidates.
    assert(!setBlockIndexCandidates.empty());
}

/**
 * Try to make some progress towards making pindexMostWork the active block.
 * pblock is either NULL or a pointer to a CBlock corresponding to pindexMostWork.
 */
static bool ActivateBestChainStep(CValidationState &state,
    const CChainParams &chainparams,
    CBlockIndex *pindexMostWork,
    const CBlock *pblock,
    bool fParallel)
{
    AssertLockHeld(cs_main);
    bool fInvalidFound = false;
    const CBlockIndex *pindexOldTip = chainActive.Tip();
    const CBlockIndex *pindexFork = chainActive.FindFork(pindexMostWork);
    CBlockIndex *pindexNewMostWork;

    bool fBlocksDisconnected = false;
    boost::thread::id this_id(boost::this_thread::get_id()); // get this thread's id

    while (chainActive.Tip() && chainActive.Tip() != pindexFork)
    {
        // When running in parallel block validation mode it is possible that this competing block could get to this
        // point just after the chaintip had already been advanced.  If that were to happen then it could initiate a
        // re-org when in fact a Quit had already been called on this thread.  So we do a check if Quit was previously
        // called and return if true.
        if (PV->QuitReceived(this_id, fParallel))
            return false;

        PV->IsReorgInProgress(this_id, true, fParallel); // indicate that this thread has now initiated a re-org

        // Disconnect active blocks which are no longer in the best chain.
        if (!DisconnectTip(state, chainparams.GetConsensus()))
            return false;

        if (fParallel && !fBlocksDisconnected)
            PV->StopAllValidationThreads(this_id);

        fBlocksDisconnected = true;
    }

    // Build list of new blocks to connect.
    std::vector<CBlockIndex *> vpindexToConnect;
    bool fContinue = true;
    /** Parallel Validation: fBlock determines whether we pass a block or NULL to ConnectTip().
     *  If the pindexMostWork has been extended while we have been validating the last block then we
     *  want to pass a NULL so that the next block is read from disk, because we will definitely not
     *  have the block.
     */
    bool fBlock = true;
    int nHeight = pindexFork ? pindexFork->nHeight : -1;
    while (fContinue && nHeight < pindexMostWork->nHeight)
    {
        // During IBD if there are many blocks to connect still it could be a while before shutting down
        // and the user may think the shutdown has hung, so return here and stop connecting any remaining
        // blocks.
        if (ShutdownRequested())
            return false;

        // Don't iterate the entire list of potential improvements toward the best tip, as we likely only need
        // a few blocks along the way.
        int nTargetHeight = std::min(nHeight + (int)BLOCK_DOWNLOAD_WINDOW, pindexMostWork->nHeight);
        vpindexToConnect.clear();
        CBlockIndex *pindexIter = pindexMostWork->GetAncestor(nTargetHeight);
        while (pindexIter && pindexIter->nHeight != nHeight)
        {
            vpindexToConnect.push_back(pindexIter);
            pindexIter = pindexIter->pprev;
        }
        nHeight = nTargetHeight;

        // Connect new blocks.
        CBlockIndex *pindexNewTip;
        BOOST_REVERSE_FOREACH (CBlockIndex *pindexConnect, vpindexToConnect)
        {
            // Check if the best chain has changed while we were disconnecting or processing blocks.
            // If so then we need to return and continue processing the newer chain.
            pindexNewMostWork = FindMostWorkChain();
            if (!pindexMostWork)
                return false;

            if (pindexNewMostWork->nChainWork > pindexMostWork->nChainWork)
            {
                LogPrint("parallel", "Returning because chain work has changed while connecting blocks\n");
                return true;
            }

            if (!ConnectTip(state, chainparams, pindexConnect,
                    pindexConnect == pindexMostWork && fBlock ? pblock : NULL, fParallel))
            {
                if (state.IsInvalid())
                {
                    // The block violates a consensus rule.
                    if (!state.CorruptionPossible())
                        InvalidChainFound(vpindexToConnect.back());
                    state = CValidationState();
                    fInvalidFound = true;
                    fContinue = false;
                    break;
                }
                else
                {
                    // A system error occurred (disk space, database error, ...) or a Parallel Validation was
                    // terminated.
                    return false;
                }
            }
            else
            {
                pindexNewTip = pindexConnect;

                // Update the syncd status after each block is handled
                IsChainNearlySyncdInit();
                IsInitialBlockDownloadInit();

                if (!IsInitialBlockDownload())
                {
                    // Notify external zmq listeners about the new tip.
                    GetMainSignals().UpdatedBlockTip(pindexConnect);
                }

                // Update the UI at least every 5 seconds just in case we get in a long loop
                // as can happen during IBD.  We need an atomic here because there may be other
                // threads running concurrently.
                static std::atomic<int64_t> nLastUpdate = {GetTime()};
                if (nLastUpdate.load() < GetTime() - 5)
                {
                    uiInterface.NotifyBlockTip(true, pindexNewTip);
                    nLastUpdate.store(GetTime());
                }

                PruneBlockIndexCandidates();
                if (!pindexOldTip || chainActive.Tip()->nChainWork > pindexOldTip->nChainWork)
                {
                    /* BU: these are commented out for parallel validation:
                           We must always continue so as to find if the pindexMostWork has advanced while we've
                           been trying to connect the last block.
                    // We're in a better position than we were. Return temporarily to release the lock.
                    fContinue = false;
                    break;
                    */
                }
            }
        }

        if (fInvalidFound)
            break; // stop processing more blocks if the last one was invalid.

        // Notify the UI with the new block tip information.
        if (pindexMostWork->nHeight >= nHeight && pindexNewTip != NULL)
            uiInterface.NotifyBlockTip(true, pindexNewTip);

        if (fContinue)
        {
            pindexMostWork = FindMostWorkChain();
            if (!pindexMostWork)
                return false;
        }
        fBlock = false; // read next blocks from disk

        // Update the syncd status after each block is handled
        IsChainNearlySyncdInit();
        IsInitialBlockDownloadInit();
    }


    // Relay Inventory
    CBlockIndex *pindexNewTip = chainActive.Tip();
    if (pindexFork != pindexNewTip)
    {
        if (!IsInitialBlockDownload())
        {
            // Find the hashes of all blocks that weren't previously in the best chain.
            std::vector<uint256> vHashes;
            CBlockIndex *pindexToAnnounce = pindexNewTip;
            while (pindexToAnnounce != pindexFork)
            {
                vHashes.push_back(pindexToAnnounce->GetBlockHash());
                pindexToAnnounce = pindexToAnnounce->pprev;
                if (vHashes.size() == MAX_BLOCKS_TO_ANNOUNCE)
                {
                    // Limit announcements in case of a huge reorganization.
                    // Rely on the peer's synchronization mechanism in that case.
                    break;
                }
            }
            // Relay inventory, but don't relay old inventory during initial block download.
            int nBlockEstimate = 0;
            if (fCheckpointsEnabled)
                nBlockEstimate = Checkpoints::GetTotalBlocksEstimate(chainparams.Checkpoints());
            {
                LOCK(cs_vNodes);
                BOOST_FOREACH (CNode *pnode, vNodes)
                {
                    if (chainActive.Height() >
                        (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : nBlockEstimate))
                    {
                        BOOST_REVERSE_FOREACH (const uint256 &hash, vHashes)
                        {
                            pnode->PushBlockHash(hash);
                        }
                    }
                }
            }
        }
    }

    if (fBlocksDisconnected)
    {
        mempool.removeForReorg(pcoinsTip, chainActive.Tip()->nHeight + 1, STANDARD_LOCKTIME_VERIFY_FLAGS);
        LimitMempoolSize(mempool, GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000,
            GetArg("-mempoolexpiry", DEFAULT_MEMPOOL_EXPIRY) * 60 * 60);
    }
    mempool.check(pcoinsTip);

    // Callbacks/notifications for a new best chain.
    if (fInvalidFound)
    {
        CheckForkWarningConditionsOnNewFork(vpindexToConnect.back());
        return false;
    }
    else
        CheckForkWarningConditions();

    return true;
}

/**
 * Make the best chain active, in multiple steps. The result is either failure
 * or an activated best chain. pblock is either NULL or a pointer to a block
 * that is already loaded (to avoid loading it again from disk).
 */
bool ActivateBestChain(CValidationState &state, const CChainParams &chainparams, const CBlock *pblock, bool fParallel)
{
    CBlockIndex *pindexMostWork = NULL;
    LOCK(cs_main);

    bool fOneDone = false;
    do
    {
        boost::this_thread::interruption_point();
        if (ShutdownRequested())
            return false;

        CBlockIndex *pindexOldTip = chainActive.Tip();
        pindexMostWork = FindMostWorkChain();
        if (!pindexMostWork)
            return true;


        // This is needed for PV because FindMostWorkChain does not necessarily return the block with the lowest
        // nSequenceId
        if (fParallel && pblock)
        {
            std::set<CBlockIndex *, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexCandidates.rbegin();
            while (it != setBlockIndexCandidates.rend())
            {
                if ((*it)->nChainWork == pindexMostWork->nChainWork)
                    if ((*it)->nSequenceId < pindexMostWork->nSequenceId)
                        pindexMostWork = *it;
                it++;
            }
        }

        // Whether we have anything to do at all.
        if (chainActive.Tip() != NULL)
        {
            if (pindexMostWork->nChainWork <= chainActive.Tip()->nChainWork)
                return true;
        }

        //** PARALLEL BLOCK VALIDATION
        // Find the CBlockIndex of this block if this blocks previous hash matches the old chaintip.  In the
        // case of parallel block validation we may have two or more blocks processing at the same time however
        // their block headers may not represent what is considered the best block as returned by pindexMostWork.
        // Therefore we must supply the blockindex of this block explicitly as being the one with potentially
        // the most work and which will subsequently advance the chain tip if it wins the validation race.
        if (pblock != NULL && pindexOldTip != NULL && chainActive.Tip() != chainActive.Genesis() && fParallel)
        {
            if (pblock->GetBlockHeader().hashPrevBlock == *pindexOldTip->phashBlock)
            {
                BlockMap::iterator mi = mapBlockIndex.find(pblock->GetHash());
                if (mi == mapBlockIndex.end())
                {
                    LogPrintf("Could not find block in mapBlockIndex: %s\n", pblock->GetHash().ToString());
                    return false;
                }
                else
                {
                    // Because we are potentially working with a block that is not the pindexMostWork as returned by
                    // FindMostWorkChain() but rather are forcing it to point to this block we must check again if
                    // this block has enough work to advance the tip.
                    pindexMostWork = (*mi).second;
                    if (pindexMostWork->nChainWork <= pindexOldTip->nChainWork)
                    {
                        return false;
                    }
                }
            }
        }

        // If there is a reorg happening then we can not activate this chain *unless* it
        // has more work that the currently processing reorg chain.  In that case we must terminate the reorg
        // extend this chain instead.
        if (!fOneDone && PV && PV->IsReorgInProgress())
        {
            // find out if this block and chain are more work than the chain
            // being reorg'd to.  If not then just return.  If so then kill the reorg and
            // start connecting this chain.
            if (pindexMostWork->nChainWork > PV->MaxWorkChainBeingProcessed())
            {
                // kill all validating threads except our own.
                boost::thread::id this_id(boost::this_thread::get_id());
                PV->StopAllValidationThreads(this_id);
            }
            else
                return true;
        }

        if (!ActivateBestChainStep(state, chainparams, pindexMostWork,
                ((pblock) && pblock->GetHash() == pindexMostWork->GetBlockHash() ? pblock : NULL), fParallel))
            return false;

        // Check if the best chain has changed while we were processing blocks.  If so then we need to
        // continue processing the newer chain.  This satisfies a rare edge case where we have initiated
        // a reorg to another chain but before the reorg is complete we end up reorging to a different
        // chain. Set pblock to NULL here to make sure as we continue we get blocks from disk.
        pindexMostWork = FindMostWorkChain();
        if (!pindexMostWork)
            return false;
        pblock = NULL;
        fOneDone = true;
    } while (pindexMostWork->nChainWork > chainActive.Tip()->nChainWork);
    CheckBlockIndex(chainparams.GetConsensus());

    // Write changes periodically to disk.
    if (!FlushStateToDisk(state, FLUSH_STATE_PERIODIC))
        return false;

    return true;
}

bool InvalidateBlock(CValidationState &state, const Consensus::Params &consensusParams, CBlockIndex *pindex)
{
    AssertLockHeld(cs_main);

    // Mark the block itself as invalid.
    pindex->nStatus |= BLOCK_FAILED_VALID;
    setDirtyBlockIndex.insert(pindex);
    setBlockIndexCandidates.erase(pindex);

    while (chainActive.Contains(pindex))
    {
        CBlockIndex *pindexWalk = chainActive.Tip();
        pindexWalk->nStatus |= BLOCK_FAILED_CHILD;
        setDirtyBlockIndex.insert(pindexWalk);
        setBlockIndexCandidates.erase(pindexWalk);
        // ActivateBestChain considers blocks already in chainActive
        // unconditionally valid already, so force disconnect away from it.
        if (!DisconnectTip(state, consensusParams))
        {
            mempool.removeForReorg(pcoinsTip, chainActive.Tip()->nHeight + 1, STANDARD_LOCKTIME_VERIFY_FLAGS);
            return false;
        }
    }

    LimitMempoolSize(mempool, GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000,
        GetArg("-mempoolexpiry", DEFAULT_MEMPOOL_EXPIRY) * 60 * 60);

    // The resulting new best tip may not be in setBlockIndexCandidates anymore, so
    // add it again.
    BlockMap::iterator it = mapBlockIndex.begin();
    while (it != mapBlockIndex.end())
    {
        if (it->second->IsValid(BLOCK_VALID_TRANSACTIONS) && it->second->nChainTx &&
            !setBlockIndexCandidates.value_comp()(it->second, chainActive.Tip()))
        {
            setBlockIndexCandidates.insert(it->second);
        }
        it++;
    }

    InvalidChainFound(pindex);
    // Now mark every block index on every chain that contains pindex as child of invalid
    MarkAllContainingChainsInvalid(pindex);
    mempool.removeForReorg(pcoinsTip, chainActive.Tip()->nHeight + 1, STANDARD_LOCKTIME_VERIFY_FLAGS);
    uiInterface.NotifyBlockTip(IsInitialBlockDownload(), pindex->pprev);
    return true;
}

bool ReconsiderBlock(CValidationState &state, CBlockIndex *pindex)
{
    AssertLockHeld(cs_main);

    int nHeight = pindex->nHeight;

    // Remove the invalidity flag from this block and all its descendants.
    BlockMap::iterator it = mapBlockIndex.begin();
    while (it != mapBlockIndex.end())
    {
        if (!it->second->IsValid() && it->second->GetAncestor(nHeight) == pindex)
        {
            it->second->nStatus &= ~BLOCK_FAILED_MASK;
            setDirtyBlockIndex.insert(it->second);
            if (it->second->IsValid(BLOCK_VALID_TRANSACTIONS) && it->second->nChainTx &&
                setBlockIndexCandidates.value_comp()(chainActive.Tip(), it->second))
            {
                setBlockIndexCandidates.insert(it->second);
            }
            if (it->second == pindexBestInvalid)
            {
                // Reset invalid block marker if it was pointing to one of those.
                pindexBestInvalid = NULL;
            }
        }
        it++;
    }

    // Remove the invalidity flag from all ancestors too.
    while (pindex != NULL)
    {
        if (pindex->nStatus & BLOCK_FAILED_MASK)
        {
            pindex->nStatus &= ~BLOCK_FAILED_MASK;
            setDirtyBlockIndex.insert(pindex);
        }
        pindex = pindex->pprev;
    }
    return true;
}

CBlockIndex *AddToBlockIndex(const CBlockHeader &block)
{
    // Check for duplicate
    uint256 hash = block.GetHash();
    BlockMap::iterator it = mapBlockIndex.find(hash);
    if (it != mapBlockIndex.end())
        return it->second;

    // Construct new block index object
    CBlockIndex *pindexNew = new CBlockIndex(block);
    assert(pindexNew);
    // We assign the sequence id to blocks only when the full data is available,
    // to avoid miners withholding blocks but broadcasting headers, to get a
    // competitive advantage.
    pindexNew->nSequenceId = 0;
    BlockMap::iterator mi = mapBlockIndex.insert(std::make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);
    BlockMap::iterator miPrev = mapBlockIndex.find(block.hashPrevBlock);
    if (miPrev != mapBlockIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
        pindexNew->BuildSkip();
        // BU If the prior block or an ancestor has failed, mark this one failed
        if (pindexNew->pprev && pindexNew->pprev->nStatus & BLOCK_FAILED_MASK)
            pindexNew->nStatus |= BLOCK_FAILED_CHILD;
    }
    pindexNew->nChainWork = (pindexNew->pprev ? pindexNew->pprev->nChainWork : 0) + GetBlockProof(*pindexNew);
    pindexNew->RaiseValidity(BLOCK_VALID_TREE);

    if ((!(pindexNew->nStatus & BLOCK_FAILED_MASK)) &&
        (pindexBestHeader == NULL || pindexBestHeader->nChainWork < pindexNew->nChainWork))
        pindexBestHeader = pindexNew;

    setDirtyBlockIndex.insert(pindexNew);

    return pindexNew;
}

/** Mark a block as having its data received and checked (up to BLOCK_VALID_TRANSACTIONS). */
bool ReceivedBlockTransactions(const CBlock &block,
    CValidationState &state,
    CBlockIndex *pindexNew,
    const CDiskBlockPos &pos)
{
    pindexNew->nTx = block.vtx.size();
    pindexNew->nChainTx = 0;
    pindexNew->nFile = pos.nFile;
    pindexNew->nDataPos = pos.nPos;
    pindexNew->nUndoPos = 0;
    pindexNew->nStatus |= BLOCK_HAVE_DATA;
    if (block.fExcessive)
        pindexNew->nStatus |= BLOCK_EXCESSIVE;
    pindexNew->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    setDirtyBlockIndex.insert(pindexNew);

    if (pindexNew->pprev == NULL || pindexNew->pprev->nChainTx)
    {
        // If pindexNew is the genesis block or all parents are BLOCK_VALID_TRANSACTIONS.
        std::deque<CBlockIndex *> queue;
        queue.push_back(pindexNew);

        // Recursively process any descendant blocks that now may be eligible to be connected.
        while (!queue.empty())
        {
            CBlockIndex *pindex = queue.front();
            queue.pop_front();
            pindex->nChainTx = (pindex->pprev ? pindex->pprev->nChainTx : 0) + pindex->nTx;
            {
                LOCK(cs_nBlockSequenceId);
                pindex->nSequenceId = nBlockSequenceId++;
            }
            if (chainActive.Tip() == NULL || !setBlockIndexCandidates.value_comp()(pindex, chainActive.Tip()))
            {
                setBlockIndexCandidates.insert(pindex);
            }
            std::pair<std::multimap<CBlockIndex *, CBlockIndex *>::iterator,
                std::multimap<CBlockIndex *, CBlockIndex *>::iterator>
                range = mapBlocksUnlinked.equal_range(pindex);
            while (range.first != range.second)
            {
                std::multimap<CBlockIndex *, CBlockIndex *>::iterator it = range.first;
                queue.push_back(it->second);
                range.first++;
                mapBlocksUnlinked.erase(it);
            }
        }
    }
    else
    {
        if (pindexNew->pprev && pindexNew->pprev->IsValid(BLOCK_VALID_TREE))
        {
            mapBlocksUnlinked.insert(std::make_pair(pindexNew->pprev, pindexNew));
        }
    }

    return true;
}

bool FindBlockPos(CValidationState &state,
    CDiskBlockPos &pos,
    unsigned int nAddSize,
    unsigned int nHeight,
    uint64_t nTime,
    bool fKnown = false)
{
    LOCK(cs_LastBlockFile);

    unsigned int nFile = fKnown ? pos.nFile : nLastBlockFile;
    if (vinfoBlockFile.size() <= nFile)
    {
        vinfoBlockFile.resize(nFile + 1);
    }

    if (!fKnown)
    {
        while (vinfoBlockFile[nFile].nSize + nAddSize >= MAX_BLOCKFILE_SIZE)
        {
            nFile++;
            if (vinfoBlockFile.size() <= nFile)
            {
                vinfoBlockFile.resize(nFile + 1);
            }
        }
        pos.nFile = nFile;
        pos.nPos = vinfoBlockFile[nFile].nSize;
    }

    if ((int)nFile != nLastBlockFile)
    {
        if (!fKnown)
        {
            LogPrintf("Leaving block file %i: %s\n", nLastBlockFile, vinfoBlockFile[nLastBlockFile].ToString());
        }
        FlushBlockFile(!fKnown);
        nLastBlockFile = nFile;
    }

    vinfoBlockFile[nFile].AddBlock(nHeight, nTime);
    if (fKnown)
        vinfoBlockFile[nFile].nSize = std::max(pos.nPos + nAddSize, vinfoBlockFile[nFile].nSize);
    else
        vinfoBlockFile[nFile].nSize += nAddSize;

    if (!fKnown)
    {
        unsigned int nOldChunks = (pos.nPos + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        unsigned int nNewChunks = (vinfoBlockFile[nFile].nSize + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        if (nNewChunks > nOldChunks)
        {
            if (fPruneMode)
                fCheckForPruning = true;
            if (CheckDiskSpace(nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos))
            {
                FILE *file = OpenBlockFile(pos);
                if (file)
                {
                    LogPrintf("Pre-allocating up to position 0x%x in blk%05u.dat\n", nNewChunks * BLOCKFILE_CHUNK_SIZE,
                        pos.nFile);
                    AllocateFileRange(file, pos.nPos, nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos);
                    fclose(file);
                }
            }
            else
                return state.Error("out of disk space");
        }
    }

    setDirtyFileInfo.insert(nFile);
    return true;
}

bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    unsigned int nNewSize;
    pos.nPos = vinfoBlockFile[nFile].nUndoSize;
    nNewSize = vinfoBlockFile[nFile].nUndoSize += nAddSize;
    setDirtyFileInfo.insert(nFile);

    unsigned int nOldChunks = (pos.nPos + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    unsigned int nNewChunks = (nNewSize + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks)
    {
        if (fPruneMode)
            fCheckForPruning = true;
        if (CheckDiskSpace(nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos))
        {
            FILE *file = OpenUndoFile(pos);
            if (file)
            {
                LogPrintf(
                    "Pre-allocating up to position 0x%x in rev%05u.dat\n", nNewChunks * UNDOFILE_CHUNK_SIZE, pos.nFile);
                AllocateFileRange(file, pos.nPos, nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos);
                fclose(file);
            }
        }
        else
            return state.Error("out of disk space");
    }

    return true;
}

bool CheckBlockHeader(const CBlockHeader &block, CValidationState &state, bool fCheckPOW)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, Params().GetConsensus()))
        return state.DoS(50, error("CheckBlockHeader(): proof of work failed"), REJECT_INVALID, "high-hash");

    // Check timestamp
    if (block.GetBlockTime() > GetAdjustedTime() + 2 * 60 * 60)
        return state.Invalid(
            error("CheckBlockHeader(): block timestamp too far in the future"), REJECT_INVALID, "time-too-new");

    return true;
}

bool CheckBlock(const CBlock &block, CValidationState &state, bool fCheckPOW, bool fCheckMerkleRoot, bool fConservative)
{
    // These are checks that are independent of context.

    if (block.fChecked)
        return true;

    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!CheckBlockHeader(block, state, fCheckPOW))
        return false;

    // Check the merkle root.
    if (fCheckMerkleRoot)
    {
        bool mutated;
        uint256 hashMerkleRoot2 = BlockMerkleRoot(block, &mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2)
            return state.DoS(
                100, error("CheckBlock(): hashMerkleRoot mismatch"), REJECT_INVALID, "bad-txnmrklroot", true);

        // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
        // of transactions in a block without affecting the merkle root of a block,
        // while still invalidating it.
        if (mutated)
            return state.DoS(
                100, error("CheckBlock(): duplicate transaction"), REJECT_INVALID, "bad-txns-duplicate", true);
    }

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.

    // Size limits
    if (block.nBlockSize == 0)
        block.nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);

    // || block.vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) >
    // MAX_BLOCK_SIZE)
    if (block.vtx.empty())
        return state.DoS(100, error("CheckBlock(): size limits failed"), REJECT_INVALID, "bad-blk-length");

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0].IsCoinBase())
        return state.DoS(100, error("CheckBlock(): first tx is not coinbase"), REJECT_INVALID, "bad-cb-missing");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i].IsCoinBase())
            return state.DoS(100, error("CheckBlock(): more than one coinbase"), REJECT_INVALID, "bad-cb-multiple");

    // Check transactions
    BOOST_FOREACH (const CTransaction &tx, block.vtx)
        if (!CheckTransaction(tx, state))
            return error("CheckBlock(): CheckTransaction of %s failed with %s", tx.GetHash().ToString(),
                FormatStateMessage(state));

    uint64_t nSigOps = 0;
    // BU: count the number of transactions in case the CheckExcessive function wants to use this as criteria
    uint64_t nTx = 0;
    uint64_t nLargestTx = 0; // BU: track the longest transaction

    BOOST_FOREACH (const CTransaction &tx, block.vtx)
    {
        nTx++;
        nSigOps += GetLegacySigOpCount(tx);
        uint64_t nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
        if (nTxSize > nLargestTx)
            nLargestTx = nTxSize;
    }

    // BU only enforce sigops during block generation not acceptance
    if (fConservative && (nSigOps > BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS))
        return state.DoS(100, error("CheckBlock(): out-of-bounds SigOpCount"), REJECT_INVALID, "bad-blk-sigops", true);

    if (fCheckPOW && fCheckMerkleRoot)
        block.fChecked = true;

    // BU: Check whether this block exceeds what we want to relay.
    block.fExcessive = CheckExcessive(block, block.nBlockSize, nSigOps, nTx, nLargestTx);

    return true;
}

bool CheckIndexAgainstCheckpoint(const CBlockIndex *pindexPrev,
    CValidationState &state,
    const CChainParams &chainparams,
    const uint256 &hash)
{
    if (*pindexPrev->phashBlock == chainparams.GetConsensus().hashGenesisBlock)
        return true;

    int nHeight = pindexPrev->nHeight + 1;
    // Don't accept any forks from the main chain prior to last checkpoint
    CBlockIndex *pcheckpoint = Checkpoints::GetLastCheckpoint(chainparams.Checkpoints());
    if (pcheckpoint && nHeight < pcheckpoint->nHeight)
        return state.DoS(100, error("%s: forked chain older than last checkpoint (height %d)", __func__, nHeight));

    return true;
}

bool ContextualCheckBlockHeader(const CBlockHeader &block, CValidationState &state, CBlockIndex *const pindexPrev)
{
    const Consensus::Params &consensusParams = Params().GetConsensus();
    // Check proof of work
    if (block.nBits != GetNextWorkRequired(pindexPrev, &block, consensusParams))
        return state.DoS(100, error("%s: incorrect proof of work", __func__), REJECT_INVALID, "bad-diffbits");

    // Check timestamp against prev
    if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
        return state.Invalid(error("%s: block's timestamp is too early", __func__), REJECT_INVALID, "time-too-old");

    // Reject block.nVersion=1 blocks when 95% (75% on testnet) of the network has upgraded:
    if (block.nVersion < 2 &&
        IsSuperMajority(2, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams))
        return state.Invalid(error("%s: rejected nVersion=1 block", __func__), REJECT_OBSOLETE, "bad-version");

    // Reject block.nVersion=2 blocks when 95% (75% on testnet) of the network has upgraded:
    if (block.nVersion < 3 &&
        IsSuperMajority(3, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams))
        return state.Invalid(error("%s: rejected nVersion=2 block", __func__), REJECT_OBSOLETE, "bad-version");

    // Reject block.nVersion=3 blocks when 95% (75% on testnet) of the network has upgraded:
    if (block.nVersion < 4 &&
        IsSuperMajority(4, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams))
        return state.Invalid(error("%s : rejected nVersion=3 block", __func__), REJECT_OBSOLETE, "bad-version");

    return true;
}

bool ContextualCheckBlock(const CBlock &block, CValidationState &state, CBlockIndex *const pindexPrev)
{
    const int nHeight = pindexPrev == NULL ? 0 : pindexPrev->nHeight + 1;
    const Consensus::Params &consensusParams = Params().GetConsensus();

    // Start enforcing BIP113 (Median Time Past) using versionbits logic.
    int nLockTimeFlags = 0;
    if (VersionBitsState(pindexPrev, consensusParams, Consensus::DEPLOYMENT_CSV, versionbitscache) == THRESHOLD_ACTIVE)
    {
        nLockTimeFlags |= LOCKTIME_MEDIAN_TIME_PAST;
    }

    int64_t nLockTimeCutoff =
        (nLockTimeFlags & LOCKTIME_MEDIAN_TIME_PAST) ? pindexPrev->GetMedianTimePast() : block.GetBlockTime();

    // Check that all transactions are finalized
    BOOST_FOREACH (const CTransaction &tx, block.vtx)
    {
        if (!IsFinalTx(tx, nHeight, nLockTimeCutoff))
        {
            return state.DoS(
                10, error("%s: contains a non-final transaction", __func__), REJECT_INVALID, "bad-txns-nonfinal");
        }
    }

    // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
    // if 750 of the last 1,000 blocks are version 2 or greater (51/100 if testnet):
    if (block.nVersion >= 2 &&
        IsSuperMajority(2, pindexPrev, consensusParams.nMajorityEnforceBlockUpgrade, consensusParams))
    {
        CScript expect = CScript() << nHeight;
        if (block.vtx[0].vin[0].scriptSig.size() < expect.size() ||
            !std::equal(expect.begin(), expect.end(), block.vtx[0].vin[0].scriptSig.begin()))
        {
            int blockCoinbaseHeight = block.GetHeight();
            uint256 hashp = block.hashPrevBlock;
            uint256 hash = block.GetHash();
            return state.DoS(100, error("%s: block height mismatch in coinbase, expected %d, got %d, block is %s, "
                                        "parent block is %s, pprev is %s",
                                      __func__, nHeight, blockCoinbaseHeight, hash.ToString(), hashp.ToString(),
                                      pindexPrev->phashBlock->ToString()),
                REJECT_INVALID, "bad-cb-height");
        }
    }

    // BUIP055 enforce that the fork block is > 1MB
    // (note subsequent blocks can be <= 1MB...)
    if (pindexPrev && pindexPrev->forkAtNextBlock(miningForkTime.value))
    {
        DbgAssert(block.nBlockSize, );
        if (block.nBlockSize <= BLOCKSTREAM_CORE_MAX_BLOCK_SIZE)
        {
            uint256 hash = block.GetHash();
            return state.DoS(100,
                error("%s: BUIP055 fork block (%s, height %d) must exceed %d, but this block is %d bytes", __func__,
                                 hash.ToString(), nHeight, BLOCKSTREAM_CORE_MAX_BLOCK_SIZE, block.nBlockSize),
                REJECT_INVALID, "bad-blk-too-small");
        }
    }
    // BUIP055 check soft-fork items, such as tx targeted to the 1MB chain
    if (pindexPrev && pindexPrev->IsforkActiveOnNextBlock(miningForkTime.value))
    {
        return ValidateBUIP055Block(block, state, nHeight);
    }

    return true;
}

bool AcceptBlockHeader(const CBlockHeader &block,
    CValidationState &state,
    const CChainParams &chainparams,
    CBlockIndex **ppindex)
{
    AssertLockHeld(cs_main);
    // Check for duplicate
    uint256 hash = block.GetHash();
    CBlockIndex *pindex = NULL;
    if (hash != chainparams.GetConsensus().hashGenesisBlock)
    {
        BlockMap::iterator miSelf = mapBlockIndex.find(hash);
        if (miSelf != mapBlockIndex.end())
        {
            // Block header is already known.
            pindex = miSelf->second;
            if (ppindex)
                *ppindex = pindex;
            if (pindex->nStatus & BLOCK_FAILED_MASK)
                return state.Invalid(
                    error("%s: block %s height %d is marked invalid", __func__, hash.ToString(), pindex->nHeight), 0,
                    "duplicate");
            return true;
        }

        if (!CheckBlockHeader(block, state))
            return false;

        // Get prev block index
        CBlockIndex *pindexPrev = NULL;
        BlockMap::iterator mi = mapBlockIndex.find(block.hashPrevBlock);
        if (mi == mapBlockIndex.end())
            return state.DoS(10, error("%s: previous block %s not found while accepting %s", __func__,
                                     block.hashPrevBlock.ToString(), hash.ToString()),
                0, "bad-prevblk");
        pindexPrev = (*mi).second;
        if (pindexPrev->nStatus & BLOCK_FAILED_MASK)
            return state.DoS(100, error("%s: previous block invalid", __func__), REJECT_INVALID, "bad-prevblk");

        assert(pindexPrev);
        if (fCheckpointsEnabled && !CheckIndexAgainstCheckpoint(pindexPrev, state, chainparams, hash))
            return error("%s: CheckIndexAgainstCheckpoint(): %s", __func__, state.GetRejectReason().c_str());

        if (!ContextualCheckBlockHeader(block, state, pindexPrev))
            return false;
    }
    if (pindex == NULL)
        pindex = AddToBlockIndex(block);

    if (ppindex)
        *ppindex = pindex;

    return true;
}

/** Store block on disk. If dbp is non-NULL, the file is known to already reside on disk */
static bool AcceptBlock(const CBlock &block,
    CValidationState &state,
    const CChainParams &chainparams,
    CBlockIndex **ppindex,
    bool fRequested,
    CDiskBlockPos *dbp)
{
    AssertLockHeld(cs_main);

    CBlockIndex *&pindex = *ppindex;

    if (!AcceptBlockHeader(block, state, chainparams, &pindex))
        return false;

    LogPrint("parallel", "Check Block %s with chain work %s block height %d\n", pindex->phashBlock->ToString(),
        pindex->nChainWork.ToString(), pindex->nHeight);

    // Try to process all requested blocks that we don't have, but only
    // process an unrequested block if it's new and has enough work to
    // advance our tip, and isn't too many blocks ahead.
    bool fAlreadyHave = pindex->nStatus & BLOCK_HAVE_DATA;
    bool fHasMoreWork = (chainActive.Tip() ? pindex->nChainWork > chainActive.Tip()->nChainWork : true);
    // Blocks that are too out-of-order needlessly limit the effectiveness of
    // pruning, because pruning will not delete block files that contain any
    // blocks which are too close in height to the tip.  Apply this test
    // regardless of whether pruning is enabled; it should generally be safe to
    // not process unrequested blocks.
    bool fTooFarAhead = (pindex->nHeight > int(chainActive.Height() + MIN_BLOCKS_TO_KEEP));

    // TODO: deal better with return value and error conditions for duplicate
    // and unrequested blocks.
    if (fAlreadyHave)
        return true;
    if (!fRequested)
    { // If we didn't ask for it:
        if (pindex->nTx != 0)
            return true; // This is a previously-processed block that was pruned
        if (!fHasMoreWork)
            return true; // Don't process less-work chains
        if (fTooFarAhead)
            return true; // Block height is too high
    }

    if ((!CheckBlock(block, state)) || !ContextualCheckBlock(block, state, pindex->pprev))
    {
        if (state.IsInvalid() && !state.CorruptionPossible())
        {
            pindex->nStatus |= BLOCK_FAILED_VALID;
            setDirtyBlockIndex.insert(pindex);
            // Now mark every block index on every chain that contains pindex as child of invalid
            MarkAllContainingChainsInvalid(pindex);
        }
        return false;
    }

    int nHeight = pindex->nHeight;

    // Write block to history file
    try
    {
        unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
        CDiskBlockPos blockPos;
        if (dbp != NULL)
            blockPos = *dbp;
        if (!FindBlockPos(state, blockPos, nBlockSize + 8, nHeight, block.GetBlockTime(), dbp != NULL))
            return error("AcceptBlock(): FindBlockPos failed");
        if (dbp == NULL)
            if (!WriteBlockToDisk(block, blockPos, chainparams.MessageStart()))
                AbortNode(state, "Failed to write block");
        if (!ReceivedBlockTransactions(block, state, pindex, blockPos))
            return error("AcceptBlock(): ReceivedBlockTransactions failed");
    }
    catch (const std::runtime_error &e)
    {
        return AbortNode(state, std::string("System error: ") + e.what());
    }

    if (fCheckForPruning)
        FlushStateToDisk(state, FLUSH_STATE_NONE); // we just allocated more disk space for block files

    return true;
}

static bool IsSuperMajority(int minVersion,
    const CBlockIndex *pstart,
    unsigned nRequired,
    const Consensus::Params &consensusParams)
{
    unsigned int nFound = 0;
    for (int i = 0; i < consensusParams.nMajorityWindow && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }
    return (nFound >= nRequired);
}


bool ProcessNewBlock(CValidationState &state,
    const CChainParams &chainparams,
    CNode *pfrom,
    const CBlock *pblock,
    bool fForceProcessing,
    CDiskBlockPos *dbp,
    bool fParallel)
{
    int64_t start = GetTimeMicros();
    LogPrint("thin", "Processing new block %s from peer %s (%d).\n", pblock->GetHash().ToString(),
        pfrom ? pfrom->addrName.c_str() : "myself", pfrom ? pfrom->id : 0);
    // Preliminary checks
    if (!CheckBlockHeader(*pblock, state, true))
    { // block header is bad
        // demerit the sender
        return error("%s: CheckBlockHeader FAILED", __func__);
    }
    if (IsChainNearlySyncd())
        SendExpeditedBlock(*pblock, pfrom);

    bool checked = CheckBlock(*pblock, state);
    if (!checked)
    {
        int byteLen = ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION);
        LogPrintf("Invalid block: ver:%x time:%d Tx size:%d len:%d\n", pblock->nVersion, pblock->nTime,
            pblock->vtx.size(), byteLen);
    }

    // WARNING: cs_main is not locked here throughout but is released and then re-locked during ActivateBestChain
    //          If you lock cs_main throughout ProcessNewBlock then you will in effect prevent PV from happening.
    //          TODO: in order to lock cs_main all the way through we must remove the locking from ActivateBestChain
    //                but it will require great care because ActivateBestChain requires cs_main however it is also
    //                called from other places.  Currently it seems best to leave cs_main here as is.
    {
        LOCK(cs_main);
        uint256 hash = pblock->GetHash();
        bool fRequested = MarkBlockAsReceived(hash);
        fRequested |= fForceProcessing;
        if (!checked)
        {
            return error("%s: CheckBlock FAILED", __func__);
        }

        // Store to disk
        CBlockIndex *pindex = NULL;
        bool ret = AcceptBlock(*pblock, state, chainparams, &pindex, fRequested, dbp);
        if (pindex && pfrom)
        {
            mapBlockSource[pindex->GetBlockHash()] = pfrom->GetId();
        }
        CheckBlockIndex(chainparams.GetConsensus());
        if (!ret)
        {
            // BU TODO: if block comes out of order (before its parent) this will happen.  We should cache the block
            // until the parents arrive.
            return error("%s: AcceptBlock FAILED", __func__);
        }

        // We must indicate to the request manager that the block was received only after it has
        // been stored to disk. Doing so prevents unnecessary re-requests.
        CInv inv(MSG_BLOCK, hash);
        requester.Received(inv, pfrom);
    }

    if (!ActivateBestChain(state, chainparams, pblock, fParallel))
    {
        if (state.IsInvalid() || state.IsError())
            return error("%s: ActivateBestChain failed", __func__);
        else
            return false;
    }

    int64_t end = GetTimeMicros();

    uint64_t maxTxSize = 0;
    uint64_t maxVin = 0;
    uint64_t maxVout = 0;
    CTransaction txIn;
    CTransaction txOut;
    CTransaction txLen;

    for (unsigned int i = 0; i < pblock->vtx.size(); i++)
    {
        if (pblock->vtx[i].vin.size() > maxVin)
        {
            maxVin = pblock->vtx[i].vin.size();
            txIn = pblock->vtx[i];
        }
        if (pblock->vtx[i].vout.size() > maxVout)
        {
            maxVout = pblock->vtx[i].vout.size();
            txOut = pblock->vtx[i];
        }
        uint64_t len = ::GetSerializeSize(pblock->vtx[i], SER_NETWORK, PROTOCOL_VERSION);
        if (len > maxTxSize)
        {
            maxTxSize = len;
            txLen = pblock->vtx[i];
        }
    }

    LogPrint("bench",
        "ProcessNewBlock, time: %d, block: %s, len: %d, numTx: %d, maxVin: %llu, maxVout: %llu, maxTx:%llu\n",
        end - start, pblock->GetHash().ToString(), pblock->nBlockSize, pblock->vtx.size(), maxVin, maxVout, maxTxSize);
    LogPrint("bench", "tx: %s, vin: %llu, vout: %llu, len: %d\n", txIn.GetHash().ToString(), txIn.vin.size(),
        txIn.vout.size(), ::GetSerializeSize(txIn, SER_NETWORK, PROTOCOL_VERSION));
    LogPrint("bench", "tx: %s, vin: %llu, vout: %llu, len: %d\n", txOut.GetHash().ToString(), txOut.vin.size(),
        txOut.vout.size(), ::GetSerializeSize(txOut, SER_NETWORK, PROTOCOL_VERSION));
    LogPrint("bench", "tx: %s, vin: %llu, vout: %llu, len: %d\n", txLen.GetHash().ToString(), txLen.vin.size(),
        txLen.vout.size(), ::GetSerializeSize(txLen, SER_NETWORK, PROTOCOL_VERSION));

    LOCK(cs_blockvalidationtime);
    nBlockValidationTime << (end - start);
    return true;
}

bool TestBlockValidity(CValidationState &state,
    const CChainParams &chainparams,
    const CBlock &block,
    CBlockIndex *pindexPrev,
    bool fCheckPOW,
    bool fCheckMerkleRoot)
{
    AssertLockHeld(cs_main);
    assert(pindexPrev && pindexPrev == chainActive.Tip());
    if (fCheckpointsEnabled && !CheckIndexAgainstCheckpoint(pindexPrev, state, chainparams, block.GetHash()))
        return error("%s: CheckIndexAgainstCheckpoint(): %s", __func__, state.GetRejectReason().c_str());

    CCoinsViewCache viewNew(pcoinsTip);
    CBlockIndex indexDummy(block);
    indexDummy.pprev = pindexPrev;
    indexDummy.nHeight = pindexPrev->nHeight + 1;

    // NOTE: CheckBlockHeader is called by CheckBlock
    if (!ContextualCheckBlockHeader(block, state, pindexPrev))
        return false;
    if (!CheckBlock(block, state, fCheckPOW, fCheckMerkleRoot))
        return false;
    if (!ContextualCheckBlock(block, state, pindexPrev))
        return false;
    if (!ConnectBlock(block, state, &indexDummy, viewNew, true))
        return false;
    assert(state.IsValid());

    return true;
}

/**
 * BLOCK PRUNING CODE
 */

/* Calculate the amount of disk space the block & undo files currently use */
uint64_t CalculateCurrentUsage()
{
    uint64_t retval = 0;
    BOOST_FOREACH (const CBlockFileInfo &file, vinfoBlockFile)
    {
        retval += file.nSize + file.nUndoSize;
    }
    return retval;
}

/* Prune a block file (modify associated database entries)*/
void PruneOneBlockFile(const int fileNumber)
{
    for (BlockMap::iterator it = mapBlockIndex.begin(); it != mapBlockIndex.end(); ++it)
    {
        CBlockIndex *pindex = it->second;
        if (pindex->nFile == fileNumber)
        {
            pindex->nStatus &= ~BLOCK_HAVE_DATA;
            pindex->nStatus &= ~BLOCK_HAVE_UNDO;
            pindex->nFile = 0;
            pindex->nDataPos = 0;
            pindex->nUndoPos = 0;
            setDirtyBlockIndex.insert(pindex);

            // Prune from mapBlocksUnlinked -- any block we prune would have
            // to be downloaded again in order to consider its chain, at which
            // point it would be considered as a candidate for
            // mapBlocksUnlinked or setBlockIndexCandidates.
            std::pair<std::multimap<CBlockIndex *, CBlockIndex *>::iterator,
                std::multimap<CBlockIndex *, CBlockIndex *>::iterator>
                range = mapBlocksUnlinked.equal_range(pindex->pprev);
            while (range.first != range.second)
            {
                std::multimap<CBlockIndex *, CBlockIndex *>::iterator it = range.first;
                range.first++;
                if (it->second == pindex)
                {
                    mapBlocksUnlinked.erase(it);
                }
            }
        }
    }

    vinfoBlockFile[fileNumber].SetNull();
    setDirtyFileInfo.insert(fileNumber);
}


void UnlinkPrunedFiles(std::set<int> &setFilesToPrune)
{
    for (std::set<int>::iterator it = setFilesToPrune.begin(); it != setFilesToPrune.end(); ++it)
    {
        CDiskBlockPos pos(*it, 0);
        boost::filesystem::remove(GetBlockPosFilename(pos, "blk"));
        boost::filesystem::remove(GetBlockPosFilename(pos, "rev"));
        LogPrintf("Prune: %s deleted blk/rev (%05u)\n", __func__, *it);
    }
}

/* Calculate the block/rev files that should be deleted to remain under target*/
void FindFilesToPrune(std::set<int> &setFilesToPrune, uint64_t nPruneAfterHeight)
{
    LOCK2(cs_main, cs_LastBlockFile);
    if (chainActive.Tip() == NULL || nPruneTarget == 0)
    {
        return;
    }
    if ((uint64_t)chainActive.Tip()->nHeight <= nPruneAfterHeight)
    {
        return;
    }

    unsigned int nLastBlockWeCanPrune = chainActive.Tip()->nHeight - MIN_BLOCKS_TO_KEEP;
    uint64_t nCurrentUsage = CalculateCurrentUsage();
    // We don't check to prune until after we've allocated new space for files
    // So we should leave a buffer under our target to account for another allocation
    // before the next pruning.
    uint64_t nBuffer = BLOCKFILE_CHUNK_SIZE + UNDOFILE_CHUNK_SIZE;
    uint64_t nBytesToPrune;
    int count = 0;

    if (nCurrentUsage + nBuffer >= nPruneTarget)
    {
        for (int fileNumber = 0; fileNumber < nLastBlockFile; fileNumber++)
        {
            nBytesToPrune = vinfoBlockFile[fileNumber].nSize + vinfoBlockFile[fileNumber].nUndoSize;

            if (vinfoBlockFile[fileNumber].nSize == 0)
                continue;

            if (nCurrentUsage + nBuffer < nPruneTarget) // are we below our target?
                break;

            // don't prune files that could have a block within MIN_BLOCKS_TO_KEEP of the main chain's tip but keep
            // scanning
            if (vinfoBlockFile[fileNumber].nHeightLast > nLastBlockWeCanPrune)
                continue;

            PruneOneBlockFile(fileNumber);
            // Queue up the files for removal
            setFilesToPrune.insert(fileNumber);
            nCurrentUsage -= nBytesToPrune;
            count++;
        }
    }

    LogPrint("prune", "Prune: target=%dMiB actual=%dMiB diff=%dMiB max_prune_height=%d removed %d blk/rev pairs\n",
        nPruneTarget / 1024 / 1024, nCurrentUsage / 1024 / 1024,
        ((int64_t)nPruneTarget - (int64_t)nCurrentUsage) / 1024 / 1024, nLastBlockWeCanPrune, count);
}

bool CheckDiskSpace(uint64_t nAdditionalBytes)
{
    uint64_t nFreeBytesAvailable = boost::filesystem::space(GetDataDir()).available;

    // Check for nMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < nMinDiskSpace + nAdditionalBytes)
        return AbortNode("Disk space is low!", _("Error: Disk space is low!"));

    return true;
}

FILE *OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly)
{
    if (pos.IsNull())
        return NULL;
    boost::filesystem::path path = GetBlockPosFilename(pos, prefix);
    boost::filesystem::create_directories(path.parent_path());
    FILE *file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file)
    {
        LogPrintf("Unable to open file %s\n", path.string());
        return NULL;
    }
    if (pos.nPos)
    {
        if (fseek(file, pos.nPos, SEEK_SET))
        {
            LogPrintf("Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return NULL;
        }
    }
    return file;
}

FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly) { return OpenDiskFile(pos, "blk", fReadOnly); }
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly) { return OpenDiskFile(pos, "rev", fReadOnly); }
boost::filesystem::path GetBlockPosFilename(const CDiskBlockPos &pos, const char *prefix)
{
    return GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
}

CBlockIndex *InsertBlockIndex(uint256 hash)
{
    if (hash.IsNull())
        return NULL;

    // Return existing
    BlockMap::iterator mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    // Create new
    CBlockIndex *pindexNew = new CBlockIndex();
    if (!pindexNew)
        throw std::runtime_error("LoadBlockIndex(): new CBlockIndex failed");
    mi = mapBlockIndex.insert(std::make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}

bool static LoadBlockIndexDB()
{
    const CChainParams &chainparams = Params();
    if (!pblocktree->LoadBlockIndexGuts())
        return false;

    boost::this_thread::interruption_point();

    // Calculate nChainWork
    std::vector<std::pair<int, CBlockIndex *> > vSortedByHeight;
    vSortedByHeight.reserve(mapBlockIndex.size());
    BOOST_FOREACH (const PAIRTYPE(uint256, CBlockIndex *) & item, mapBlockIndex)
    {
        CBlockIndex *pindex = item.second;
        vSortedByHeight.push_back(std::make_pair(pindex->nHeight, pindex));
    }
    std::sort(vSortedByHeight.begin(), vSortedByHeight.end());
    BOOST_FOREACH (const PAIRTYPE(int, CBlockIndex *) & item, vSortedByHeight)
    {
        CBlockIndex *pindex = item.second;
        pindex->nChainWork = (pindex->pprev ? pindex->pprev->nChainWork : 0) + GetBlockProof(*pindex);
        // We can link the chain of blocks for which we've received transactions at some point.
        // Pruned nodes may have deleted the block.
        if (pindex->nTx > 0)
        {
            if (pindex->pprev)
            {
                if (pindex->pprev->nChainTx)
                {
                    pindex->nChainTx = pindex->pprev->nChainTx + pindex->nTx;
                }
                else
                {
                    pindex->nChainTx = 0;
                    mapBlocksUnlinked.insert(std::make_pair(pindex->pprev, pindex));
                }
            }
            else
            {
                pindex->nChainTx = pindex->nTx;
            }
        }
        if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && (pindex->nChainTx || pindex->pprev == NULL))
            setBlockIndexCandidates.insert(pindex);
        if (pindex->nStatus & BLOCK_FAILED_MASK &&
            (!pindexBestInvalid || pindex->nChainWork > pindexBestInvalid->nChainWork))
            pindexBestInvalid = pindex;
        if (pindex->pprev)
            pindex->BuildSkip();
        if (pindex->IsValid(BLOCK_VALID_TREE) &&
            (pindexBestHeader == NULL || CBlockIndexWorkComparator()(pindexBestHeader, pindex)))
            pindexBestHeader = pindex;
    }

    // Load block file info
    pblocktree->ReadLastBlockFile(nLastBlockFile);
    vinfoBlockFile.resize(nLastBlockFile + 1);
    LogPrintf("%s: last block file = %i\n", __func__, nLastBlockFile);
    for (int nFile = 0; nFile <= nLastBlockFile; nFile++)
    {
        pblocktree->ReadBlockFileInfo(nFile, vinfoBlockFile[nFile]);
    }
    LogPrintf("%s: last block file info: %s\n", __func__, vinfoBlockFile[nLastBlockFile].ToString());
    for (int nFile = nLastBlockFile + 1; true; nFile++)
    {
        CBlockFileInfo info;
        if (pblocktree->ReadBlockFileInfo(nFile, info))
        {
            vinfoBlockFile.push_back(info);
        }
        else
        {
            break;
        }
    }

    // Check presence of blk files
    LogPrintf("Checking all blk files are present...\n");
    std::set<int> setBlkDataFiles;
    BOOST_FOREACH (const PAIRTYPE(uint256, CBlockIndex *) & item, mapBlockIndex)
    {
        CBlockIndex *pindex = item.second;
        if (pindex->nStatus & BLOCK_HAVE_DATA)
        {
            setBlkDataFiles.insert(pindex->nFile);
        }
    }
    for (std::set<int>::iterator it = setBlkDataFiles.begin(); it != setBlkDataFiles.end(); it++)
    {
        CDiskBlockPos pos(*it, 0);
        if (CAutoFile(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION).IsNull())
        {
            return false;
        }
    }

    // Check whether we have ever pruned block & undo files
    pblocktree->ReadFlag("prunedblockfiles", fHavePruned);
    if (fHavePruned)
        LogPrintf("LoadBlockIndexDB(): Block files have previously been pruned\n");

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    pblocktree->ReadReindexing(fReindexing);
    fReindex |= fReindexing;

    // Check whether we have a transaction index
    pblocktree->ReadFlag("txindex", fTxIndex);
    LogPrintf("%s: transaction index %s\n", __func__, fTxIndex ? "enabled" : "disabled");

    // Load pointer to end of best chain
    BlockMap::iterator it = mapBlockIndex.find(pcoinsTip->GetBestBlock());
    if (it == mapBlockIndex.end())
        return true;
    chainActive.SetTip(it->second);

    PruneBlockIndexCandidates();

    LogPrintf("%s: hashBestChain=%s height=%d date=%s progress=%f\n", __func__,
        chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
        DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
        Checkpoints::GuessVerificationProgress(chainparams.Checkpoints(), chainActive.Tip()));

    return true;
}

CVerifyDB::CVerifyDB() { uiInterface.ShowProgress(_("Verifying blocks..."), 0); }
CVerifyDB::~CVerifyDB() { uiInterface.ShowProgress("", 100); }
bool CVerifyDB::VerifyDB(const CChainParams &chainparams, CCoinsView *coinsview, int nCheckLevel, int nCheckDepth)
{
    LOCK(cs_main);
    if (chainActive.Tip() == NULL || chainActive.Tip()->pprev == NULL)
        return true;

    // Verify blocks in the best chain
    if (nCheckDepth <= 0)
        nCheckDepth = 1000000000; // suffices until the year 19000
    if (nCheckDepth > chainActive.Height())
        nCheckDepth = chainActive.Height();
    nCheckLevel = std::max(0, std::min(4, nCheckLevel));
    LogPrintf("Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    CCoinsViewCache coins(coinsview);
    CBlockIndex *pindexState = chainActive.Tip();
    CBlockIndex *pindexFailure = NULL;
    int nGoodTransactions = 0;
    CValidationState state;
    for (CBlockIndex *pindex = chainActive.Tip(); pindex && pindex->pprev; pindex = pindex->pprev)
    {
        boost::this_thread::interruption_point();
        uiInterface.ShowProgress(_("Verifying blocks..."),
            std::max(1, std::min(99, (int)(((double)(chainActive.Height() - pindex->nHeight)) / (double)nCheckDepth *
                                           (nCheckLevel >= 4 ? 50 : 100)))));
        if (pindex->nHeight < chainActive.Height() - nCheckDepth)
            break;
        if (fPruneMode && !(pindex->nStatus & BLOCK_HAVE_DATA))
        {
            // If pruning, only go back as far as we have data.
            LogPrintf("VerifyDB(): block verification stopping at height %d (pruning, no data)\n", pindex->nHeight);
            break;
        }
        CBlock block;
        // check level 0: read from disk
        if (!ReadBlockFromDisk(block, pindex, chainparams.GetConsensus()))
            return error("VerifyDB(): *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight,
                pindex->GetBlockHash().ToString());
        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !CheckBlock(block, state))
            return error(
                "VerifyDB(): *** found bad block at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pindex)
        {
            CBlockUndo undo;
            CDiskBlockPos pos = pindex->GetUndoPos();
            if (!pos.IsNull())
            {
                if (!UndoReadFromDisk(undo, pos, pindex->pprev->GetBlockHash()))
                    return error("VerifyDB(): *** found bad undo data at %d, hash=%s\n", pindex->nHeight,
                        pindex->GetBlockHash().ToString());
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        if (nCheckLevel >= 3 && pindex == pindexState &&
            (coins.DynamicMemoryUsage() + pcoinsTip->DynamicMemoryUsage()) <= nCoinCacheUsage)
        {
            bool fClean = true;
            if (!DisconnectBlock(block, state, pindex, coins, &fClean))
                return error("VerifyDB(): *** irrecoverable inconsistency in block data at %d, hash=%s",
                    pindex->nHeight, pindex->GetBlockHash().ToString());
            pindexState = pindex->pprev;
            if (!fClean)
            {
                nGoodTransactions = 0;
                pindexFailure = pindex;
            }
            else
                nGoodTransactions += block.vtx.size();
        }
        if (ShutdownRequested())
            return true;
    }
    if (pindexFailure)
        return error(
            "VerifyDB(): *** coin database inconsistencies found (last %i blocks, %i good transactions before that)\n",
            chainActive.Height() - pindexFailure->nHeight + 1, nGoodTransactions);

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4)
    {
        CBlockIndex *pindex = pindexState;
        while (pindex != chainActive.Tip())
        {
            boost::this_thread::interruption_point();
            uiInterface.ShowProgress(_("Verifying blocks..."),
                std::max(1, std::min(99, 100 - (int)(((double)(chainActive.Height() - pindex->nHeight)) /
                                                     (double)nCheckDepth * 50))));
            pindex = chainActive.Next(pindex);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, chainparams.GetConsensus()))
                return error("VerifyDB(): *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight,
                    pindex->GetBlockHash().ToString());
            if (!ConnectBlock(block, state, pindex, coins))
                return error("VerifyDB(): *** found unconnectable block at %d, hash=%s", pindex->nHeight,
                    pindex->GetBlockHash().ToString());
        }
    }

    LogPrintf("No coin database inconsistencies in last %i blocks (%i transactions)\n",
        chainActive.Height() - pindexState->nHeight, nGoodTransactions);

    return true;
}

void UnloadBlockIndex()
{
    {
        LOCK(cs_orphancache);
        mapOrphanTransactions.clear();
        mapOrphanTransactionsByPrev.clear();
        nBytesOrphanPool = 0;
    }

    LOCK(cs_main);
    mapUnConnectedHeaders.clear();
    setBlockIndexCandidates.clear();
    chainActive.SetTip(NULL);
    pindexBestInvalid = NULL;
    pindexBestHeader = NULL;
    mempool.clear();
    nSyncStarted = 0;
    mapBlocksUnlinked.clear();
    vinfoBlockFile.clear();
    nLastBlockFile = 0;
    nBlockSequenceId = 1;
    mapBlockSource.clear();
    mapBlocksInFlight.clear();
    nPreferredDownload = 0;
    setDirtyBlockIndex.clear();
    setDirtyFileInfo.clear();
    mapNodeState.clear();
    recentRejects.reset(NULL);
    versionbitscache.Clear();
    for (int b = 0; b < VERSIONBITS_NUM_BITS; b++)
    {
        warningcache[b].clear();
    }

    BOOST_FOREACH (BlockMap::value_type &entry, mapBlockIndex)
    {
        delete entry.second;
    }
    mapBlockIndex.clear();
    fHavePruned = false;
}

bool LoadBlockIndex()
{
    // Load block index from databases
    if (!fReindex && !LoadBlockIndexDB())
        return false;
    return true;
}

bool InitBlockIndex(const CChainParams &chainparams)
{
    LOCK(cs_main);

    // Initialize global variables that cannot be constructed at startup.
    recentRejects.reset(new CRollingBloomFilter(120000, 0.000001));

    // Check whether we're already initialized
    if (chainActive.Genesis() != NULL)
        return true;

    // Use the provided setting for -txindex in the new database
    fTxIndex = GetBoolArg("-txindex", DEFAULT_TXINDEX);
    pblocktree->WriteFlag("txindex", fTxIndex);
    LogPrintf("Initializing databases...\n");

    // Only add the genesis block if not reindexing (in which case we reuse the one already on disk)
    if (!fReindex)
    {
        try
        {
            CBlock &block = const_cast<CBlock &>(chainparams.GenesisBlock());
            // Start new block file
            unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
            CDiskBlockPos blockPos;
            CValidationState state;
            if (!FindBlockPos(state, blockPos, nBlockSize + 8, 0, block.GetBlockTime()))
                return error("LoadBlockIndex(): FindBlockPos failed");
            if (!WriteBlockToDisk(block, blockPos, chainparams.MessageStart()))
                return error("LoadBlockIndex(): writing genesis block to disk failed");
            CBlockIndex *pindex = AddToBlockIndex(block);
            if (!ReceivedBlockTransactions(block, state, pindex, blockPos))
                return error("LoadBlockIndex(): genesis block not accepted");
            if (!ActivateBestChain(state, chainparams, &block, false))
                return error("LoadBlockIndex(): genesis block cannot be activated");
            // Force a chainstate write so that when we VerifyDB in a moment, it doesn't check stale data
            return FlushStateToDisk(state, FLUSH_STATE_ALWAYS);
        }
        catch (const std::runtime_error &e)
        {
            return error("LoadBlockIndex(): failed to initialize block database: %s", e.what());
        }
    }

    return true;
}

bool LoadExternalBlockFile(const CChainParams &chainparams, FILE *fileIn, CDiskBlockPos *dbp)
{
    // Map of disk positions for blocks with unknown parent (only used for reindex)
    static std::multimap<uint256, CDiskBlockPos> mapBlocksUnknownParent;
    int64_t nStart = GetTimeMillis();

    int nLoaded = 0;
    try
    {
        // This takes over fileIn and calls fclose() on it in the CBufferedFile destructor
        CBufferedFile blkdat(fileIn, 2 * (reindexTypicalBlockSize.value + MESSAGE_START_SIZE + sizeof(unsigned int)),
            reindexTypicalBlockSize.value + MESSAGE_START_SIZE + sizeof(unsigned int), SER_DISK, CLIENT_VERSION);
        uint64_t nRewind = blkdat.GetPos();
        while (!blkdat.eof())
        {
            boost::this_thread::interruption_point();

            blkdat.SetPos(nRewind);
            nRewind++; // start one byte further next time, in case of failure
            blkdat.SetLimit(); // remove former limit
            unsigned int nSize = 0;
            try
            {
                // locate a header
                unsigned char buf[MESSAGE_START_SIZE];
                blkdat.FindByte(chainparams.MessageStart()[0]);
                // FindByte peeks 1 ahead and locates the file pointer AT the byte, not at the next one as is typical
                // for file ops.  So if we rewind, we want to go one further.
                nRewind = blkdat.GetPos() + 1;
                blkdat >> FLATDATA(buf);
                if (memcmp(buf, chainparams.MessageStart(), MESSAGE_START_SIZE))
                    continue;
                // read size
                // BU NOTE: if we ever get to 4GB blocks the block size data structure will overflow since this is
                // defined as unsigned int (32 bits)
                blkdat >> nSize;
                if (nSize < 80) // BU allow variable block size || nSize > BU_MAX_BLOCK_SIZE)
                {
                    LogPrint("reindex", "Reindex error: Short block: %d\n", nSize);
                    continue;
                }
                if (nSize > 256 * 1024 * 1024)
                {
                    LogPrint("reindex", "Reindex warning: Gigantic block: %d\n", nSize);
                }
                blkdat.GrowTo(2 * (nSize + MESSAGE_START_SIZE + sizeof(unsigned int)));
            }
            catch (const std::exception &)
            {
                // no valid block header found; don't complain
                break;
            }
            try
            {
                // read block
                uint64_t nBlockPos = blkdat.GetPos();
                if (dbp)
                    dbp->nPos = nBlockPos;
                blkdat.SetLimit(nBlockPos + nSize);
                blkdat.SetPos(nBlockPos); // Unnecessary, I just got the position
                CBlock block;
                blkdat >> block;
                nRewind = blkdat.GetPos();

                // detect out of order blocks, and store them for later
                uint256 hash = block.GetHash();
                if (hash != chainparams.GetConsensus().hashGenesisBlock &&
                    mapBlockIndex.find(block.hashPrevBlock) == mapBlockIndex.end())
                {
                    LogPrint("reindex", "%s: Out of order block %s (created %s), parent %s not known\n", __func__,
                        hash.ToString(), DateTimeStrFormat("%Y-%m-%d", block.nTime), block.hashPrevBlock.ToString());
                    if (dbp)
                        mapBlocksUnknownParent.insert(std::make_pair(block.hashPrevBlock, *dbp));
                    continue;
                }

                // process in case the block isn't known yet
                if (mapBlockIndex.count(hash) == 0 || (mapBlockIndex[hash]->nStatus & BLOCK_HAVE_DATA) == 0)
                {
                    CValidationState state;
                    if (ProcessNewBlock(state, chainparams, NULL, &block, true, dbp, false))
                        nLoaded++;
                    if (state.IsError())
                        break;
                }
                else if (hash != chainparams.GetConsensus().hashGenesisBlock &&
                         mapBlockIndex[hash]->nHeight % 1000 == 0)
                {
                    LogPrint("reindex", "Block Import: already had block %s at height %d\n", hash.ToString(),
                        mapBlockIndex[hash]->nHeight);
                }

                // Recursively process earlier encountered successors of this block
                std::deque<uint256> queue;
                queue.push_back(hash);
                while (!queue.empty())
                {
                    uint256 head = queue.front();
                    queue.pop_front();
                    std::pair<std::multimap<uint256, CDiskBlockPos>::iterator,
                        std::multimap<uint256, CDiskBlockPos>::iterator>
                        range = mapBlocksUnknownParent.equal_range(head);
                    while (range.first != range.second)
                    {
                        std::multimap<uint256, CDiskBlockPos>::iterator it = range.first;
                        if (ReadBlockFromDisk(block, it->second, chainparams.GetConsensus()))
                        {
                            LogPrintf("%s: Processing out of order child %s of %s\n", __func__,
                                block.GetHash().ToString(), head.ToString());
                            CValidationState dummy;
                            if (ProcessNewBlock(dummy, chainparams, NULL, &block, true, &it->second, false))
                            {
                                nLoaded++;
                                queue.push_back(block.GetHash());
                            }
                        }
                        range.first++;
                        mapBlocksUnknownParent.erase(it);
                    }
                }
            }
            catch (const std::exception &e)
            {
                LogPrintf("%s: Deserialize or I/O error - %s\n", __func__, e.what());
            }
        }
    }
    catch (const std::runtime_error &e)
    {
        AbortNode(std::string("System error: ") + e.what());
    }
    if (nLoaded > 0)
        LogPrintf("Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
    return nLoaded > 0;
}

void static CheckBlockIndex(const Consensus::Params &consensusParams)
{
    if (!fCheckBlockIndex)
    {
        return;
    }

    LOCK(cs_main);

    // During a reindex, we read the genesis block and call CheckBlockIndex before ActivateBestChain,
    // so we have the genesis block in mapBlockIndex but no active chain.  (A few of the tests when
    // iterating the block tree require that chainActive has been initialized.)
    if (chainActive.Height() < 0)
    {
        assert(mapBlockIndex.size() <= 1);
        return;
    }
    // Build forward-pointing map of the entire block tree.
    std::multimap<CBlockIndex *, CBlockIndex *> forward;
    for (BlockMap::iterator it = mapBlockIndex.begin(); it != mapBlockIndex.end(); it++)
    {
        forward.insert(std::make_pair(it->second->pprev, it->second));
    }

    assert(forward.size() == mapBlockIndex.size());

    std::pair<std::multimap<CBlockIndex *, CBlockIndex *>::iterator,
        std::multimap<CBlockIndex *, CBlockIndex *>::iterator>
        rangeGenesis = forward.equal_range(NULL);
    CBlockIndex *pindex = rangeGenesis.first->second;
    rangeGenesis.first++;
    assert(rangeGenesis.first == rangeGenesis.second); // There is only one index entry with parent NULL.

    // Iterate over the entire block tree, using depth-first search.
    // Along the way, remember whether there are blocks on the path from genesis
    // block being explored which are the first to have certain properties.
    size_t nNodes = 0;
    int nHeight = 0;
    CBlockIndex *pindexFirstInvalid = NULL; // Oldest ancestor of pindex which is invalid.
    CBlockIndex *pindexFirstMissing = NULL; // Oldest ancestor of pindex which does not have BLOCK_HAVE_DATA.
    CBlockIndex *pindexFirstNeverProcessed = NULL; // Oldest ancestor of pindex for which nTx == 0.
    // Oldest ancestor of pindex which does not have BLOCK_VALID_TREE (regardless of being valid or not).
    CBlockIndex *pindexFirstNotTreeValid = NULL;
    // Oldest ancestor of pindex which does not have BLOCK_VALID_TRANSACTIONS (regardless of being valid or not).
    CBlockIndex *pindexFirstNotTransactionsValid = NULL;
    // Oldest ancestor of pindex which does not have BLOCK_VALID_CHAIN (regardless of being valid or not).
    CBlockIndex *pindexFirstNotChainValid = NULL;
    // Oldest ancestor of pindex which does not have BLOCK_VALID_SCRIPTS (regardless of being valid or not).
    CBlockIndex *pindexFirstNotScriptsValid = NULL;
    while (pindex != NULL)
    {
        nNodes++;
        if (pindexFirstInvalid == NULL && pindex->nStatus & BLOCK_FAILED_VALID)
            pindexFirstInvalid = pindex;
        if (pindexFirstMissing == NULL && !(pindex->nStatus & BLOCK_HAVE_DATA))
            pindexFirstMissing = pindex;
        if (pindexFirstNeverProcessed == NULL && pindex->nTx == 0)
            pindexFirstNeverProcessed = pindex;
        if (pindex->pprev != NULL && pindexFirstNotTreeValid == NULL &&
            (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TREE)
            pindexFirstNotTreeValid = pindex;
        if (pindex->pprev != NULL && pindexFirstNotTransactionsValid == NULL &&
            (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TRANSACTIONS)
            pindexFirstNotTransactionsValid = pindex;
        if (pindex->pprev != NULL && pindexFirstNotChainValid == NULL &&
            (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_CHAIN)
            pindexFirstNotChainValid = pindex;
        if (pindex->pprev != NULL && pindexFirstNotScriptsValid == NULL &&
            (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS)
            pindexFirstNotScriptsValid = pindex;

        // Begin: actual consistency checks.
        if (pindex->pprev == NULL)
        {
            // Genesis block checks.
            assert(pindex->GetBlockHash() == consensusParams.hashGenesisBlock); // Genesis block's hash must match.
            assert(pindex == chainActive.Genesis()); // The current active chain's genesis block must be this block.
        }
        // nSequenceId can't be set for blocks that aren't linked
        if (pindex->nChainTx == 0)
            assert(pindex->nSequenceId == 0);
        // VALID_TRANSACTIONS is equivalent to nTx > 0 for all nodes (whether or not pruning has occurred).
        // HAVE_DATA is only equivalent to nTx > 0 (or VALID_TRANSACTIONS) if no pruning has occurred.
        if (!fHavePruned)
        {
            // If we've never pruned, then HAVE_DATA should be equivalent to nTx > 0
            assert(!(pindex->nStatus & BLOCK_HAVE_DATA) == (pindex->nTx == 0));
            assert(pindexFirstMissing == pindexFirstNeverProcessed);
        }
        else
        {
            // If we have pruned, then we can only say that HAVE_DATA implies nTx > 0
            if (pindex->nStatus & BLOCK_HAVE_DATA)
                assert(pindex->nTx > 0);
        }
        if (pindex->nStatus & BLOCK_HAVE_UNDO)
            assert(pindex->nStatus & BLOCK_HAVE_DATA);
        // This is pruning-independent.
        assert(((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS) == (pindex->nTx > 0));
        // All parents having had data (at some point) is equivalent to all parents being VALID_TRANSACTIONS, which is
        // equivalent to nChainTx being set.
        // nChainTx != 0 is used to signal that all parent blocks have been processed (but may have been pruned).
        assert((pindexFirstNeverProcessed != NULL) == (pindex->nChainTx == 0));
        assert((pindexFirstNotTransactionsValid != NULL) == (pindex->nChainTx == 0));
        assert(pindex->nHeight == nHeight); // nHeight must be consistent.
        // For every block except the genesis block, the chainwork must be larger than the parent's.
        assert(pindex->pprev == NULL || pindex->nChainWork >= pindex->pprev->nChainWork);
        // The pskip pointer must point back for all but the first 2 blocks.
        assert(nHeight < 2 || (pindex->pskip && (pindex->pskip->nHeight < nHeight)));
        assert(pindexFirstNotTreeValid == NULL); // All mapBlockIndex entries must at least be TREE valid
        // TREE valid implies all parents are TREE valid
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TREE)
            assert(pindexFirstNotTreeValid == NULL);
        // CHAIN valid implies all parents are CHAIN valid
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_CHAIN)
            assert(pindexFirstNotChainValid == NULL);
        // SCRIPTS valid implies all parents are SCRIPTS valid
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_SCRIPTS)
            assert(pindexFirstNotScriptsValid == NULL);
        if (pindexFirstInvalid == NULL)
        {
            // Checks for not-invalid blocks.
            // The failed mask cannot be set for blocks without invalid parents.
            assert((pindex->nStatus & BLOCK_FAILED_MASK) == 0);
        }

        /*  This section does not apply to PV since blocks can arrive and be processed in potentially any order.
            Leaving the commented section for now for further review.
        if (!CBlockIndexWorkComparator()(pindex, chainActive.Tip()) && pindexFirstNeverProcessed == NULL) {
            if (pindexFirstInvalid == NULL) {
                // If this block sorts at least as good as the current tip and
                // is valid and we have all data for its parents, it must be in
                // setBlockIndexCandidates.  chainActive.Tip() must also be there
                // even if some data has been pruned.

                // PV:  this is no longer true under certain condition for PV - leaving it in here for further review
                // BU: if the chain is excessive it won't be on the list of active chain candidates
                //if ((!chainContainsExcessive(pindex)) && (pindexFirstMissing == NULL || pindex == chainActive.Tip()) )
                //    assert(setBlockIndexCandidates.count(pindex));

                    // If some parent is missing, then it could be that this block was in
                    // setBlockIndexCandidates but had to be removed because of the missing data.
                    // In this case it must be in mapBlocksUnlinked -- see test below.
            }
        // If this block sorts worse than the current tip or some ancestor's block has never been seen, it cannot be in
        setBlockIndexCandidates.
        } else {
            assert(setBlockIndexCandidates.count(pindex) == 0);
        }
        */

        // Check whether this block is in mapBlocksUnlinked.
        std::pair<std::multimap<CBlockIndex *, CBlockIndex *>::iterator,
            std::multimap<CBlockIndex *, CBlockIndex *>::iterator>
            rangeUnlinked = mapBlocksUnlinked.equal_range(pindex->pprev);
        bool foundInUnlinked = false;
        while (rangeUnlinked.first != rangeUnlinked.second)
        {
            assert(rangeUnlinked.first->first == pindex->pprev);
            if (rangeUnlinked.first->second == pindex)
            {
                foundInUnlinked = true;
                break;
            }
            rangeUnlinked.first++;
        }
        if (pindex->pprev && (pindex->nStatus & BLOCK_HAVE_DATA) && pindexFirstNeverProcessed != NULL &&
            pindexFirstInvalid == NULL)
        {
            // If this block has block data available, some parent was never received, and has no invalid parents, it
            // must be in mapBlocksUnlinked.
            assert(foundInUnlinked);
        }
        // Can't be in mapBlocksUnlinked if we don't HAVE_DATA
        if (!(pindex->nStatus & BLOCK_HAVE_DATA))
            assert(!foundInUnlinked);
        // BU: blocks that are excessive are placed in the unlinked map
        if ((pindexFirstMissing == NULL) && (!chainContainsExcessive(pindex)))
        {
            assert(!foundInUnlinked); // We aren't missing data for any parent -- cannot be in mapBlocksUnlinked.
        }
        if (pindex->pprev && (pindex->nStatus & BLOCK_HAVE_DATA) && pindexFirstNeverProcessed == NULL &&
            pindexFirstMissing != NULL)
        {
            // We HAVE_DATA for this block, have received data for all parents at some point, but we're currently
            // missing data for some parent.
            assert(fHavePruned); // We must have pruned.
            // This block may have entered mapBlocksUnlinked if:
            //  - it has a descendant that at some point had more work than the
            //    tip, and
            //  - we tried switching to that descendant but were missing
            //    data for some intermediate block between chainActive and the
            //    tip.
            // So if this block is itself better than chainActive.Tip() and it wasn't in
            // setBlockIndexCandidates, then it must be in mapBlocksUnlinked.
            if (!CBlockIndexWorkComparator()(pindex, chainActive.Tip()) && setBlockIndexCandidates.count(pindex) == 0)
            {
                if (pindexFirstInvalid == NULL)
                {
                    assert(foundInUnlinked);
                }
            }
        }
        // assert(pindex->GetBlockHash() == pindex->GetBlockHeader().GetHash()); // Perhaps too slow
        // End: actual consistency checks.

        // Try descending into the first subnode.
        std::pair<std::multimap<CBlockIndex *, CBlockIndex *>::iterator,
            std::multimap<CBlockIndex *, CBlockIndex *>::iterator>
            range = forward.equal_range(pindex);
        if (range.first != range.second)
        {
            // A subnode was found.
            pindex = range.first->second;
            nHeight++;
            continue;
        }
        // This is a leaf node.
        // Move upwards until we reach a node of which we have not yet visited the last child.
        while (pindex)
        {
            // We are going to either move to a parent or a sibling of pindex.
            // If pindex was the first with a certain property, unset the corresponding variable.
            if (pindex == pindexFirstInvalid)
                pindexFirstInvalid = NULL;
            if (pindex == pindexFirstMissing)
                pindexFirstMissing = NULL;
            if (pindex == pindexFirstNeverProcessed)
                pindexFirstNeverProcessed = NULL;
            if (pindex == pindexFirstNotTreeValid)
                pindexFirstNotTreeValid = NULL;
            if (pindex == pindexFirstNotTransactionsValid)
                pindexFirstNotTransactionsValid = NULL;
            if (pindex == pindexFirstNotChainValid)
                pindexFirstNotChainValid = NULL;
            if (pindex == pindexFirstNotScriptsValid)
                pindexFirstNotScriptsValid = NULL;
            // Find our parent.
            CBlockIndex *pindexPar = pindex->pprev;
            // Find which child we just visited.
            std::pair<std::multimap<CBlockIndex *, CBlockIndex *>::iterator,
                std::multimap<CBlockIndex *, CBlockIndex *>::iterator>
                rangePar = forward.equal_range(pindexPar);
            while (rangePar.first->second != pindex)
            {
                // Our parent must have at least the node we're coming from as child.
                assert(rangePar.first != rangePar.second);
                rangePar.first++;
            }
            // Proceed to the next one.
            rangePar.first++;
            if (rangePar.first != rangePar.second)
            {
                // Move to the sibling.
                pindex = rangePar.first->second;
                break;
            }
            else
            {
                // Move up further.
                pindex = pindexPar;
                nHeight--;
                continue;
            }
        }
    }

    // Check that we actually traversed the entire map.
    assert(nNodes == forward.size());
}

std::string GetWarnings(const std::string &strFor)
{
    std::string strStatusBar;
    std::string strRPC;
    std::string strGUI;

    if (!CLIENT_VERSION_IS_RELEASE)
    {
        strStatusBar =
            "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        strGUI = _(
            "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");
    }

    if (GetBoolArg("-testsafemode", DEFAULT_TESTSAFEMODE))
        strStatusBar = strRPC = strGUI = "testsafemode enabled";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        strStatusBar = strGUI = strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        strStatusBar = strRPC =
            "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.";
        strGUI =
            _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (fLargeWorkInvalidChainFound)
    {
        strStatusBar = strRPC = "Warning: We do not appear to fully agree with our peers! You may need to upgrade, or "
                                "other nodes may need to upgrade.";
        strGUI = _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes "
                   "may need to upgrade.");
    }

    if (strFor == "gui")
        return strGUI;
    else if (strFor == "statusbar")
        return strStatusBar;
    else if (strFor == "rpc")
        return strRPC;
    assert(!"GetWarnings(): invalid parameter");
    return "error";
}


//////////////////////////////////////////////////////////////////////////////
//
// Messages
//


bool AlreadyHave(const CInv &inv) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    switch (inv.type)
    {
    case MSG_TX:
    {
        // remove assertions from P2P code, but this should hold: assert(recentRejects);
        if (chainActive.Tip()->GetBlockHash() != hashRecentRejectsChainTip)
        {
            // If the chain tip has changed previously rejected transactions
            // might be now valid, e.g. due to a nLockTime'd tx becoming valid,
            // or a double-spend. Reset the rejects filter and give those
            // txs a second chance.
            hashRecentRejectsChainTip = chainActive.Tip()->GetBlockHash();
            if (recentRejects)
            {
                recentRejects->reset();
            }
            else
            {
                recentRejects.reset(new CRollingBloomFilter(120000, 0.000001));
            }
        }
        bool rrc = recentRejects ? recentRejects->contains(inv.hash) : false;
        return rrc || mempool.exists(inv.hash) || AlreadyHaveOrphan(inv.hash) || pcoinsTip->HaveCoins(inv.hash);
    }
    case MSG_BLOCK:
    case MSG_XTHINBLOCK:
    case MSG_THINBLOCK:
    {
        // The Request Manager functionality requires that we return true only when we actually have received
        // the block and not when we have received the header only.  Otherwise the request manager may not
        // be able to update its block source in order to make re-requests.
        BlockMap::iterator mi = mapBlockIndex.find(inv.hash);
        if (mi == mapBlockIndex.end())
            return false;
        if (!(mi->second->nStatus & BLOCK_HAVE_DATA))
            return false;
        return true;
    }
    }
    // Don't know what it is, just say we already got one
    return true;
}

void static ProcessGetData(CNode *pfrom, const Consensus::Params &consensusParams)
{
    std::deque<CInv>::iterator it = pfrom->vRecvGetData.begin();

    std::vector<CInv> vNotFound;

    LOCK(cs_main);

    while (it != pfrom->vRecvGetData.end())
    {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;

        const CInv &inv = *it;
        {
            boost::this_thread::interruption_point();
            it++;

            // BUIP010 Xtreme Thinblocks: if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK || inv.type == MSG_THINBLOCK ||
                inv.type == MSG_XTHINBLOCK)
            {
                bool send = false;
                BlockMap::iterator mi = mapBlockIndex.find(inv.hash);
                if (mi != mapBlockIndex.end())
                {
                    if (chainActive.Contains(mi->second))
                    {
                        send = true;
                    }
                    else
                    {
                        static const int nOneMonth = 30 * 24 * 60 * 60;
                        // To prevent fingerprinting attacks, only send blocks outside of the active
                        // chain if they are valid, and no more than a month older (both in time, and in
                        // best equivalent proof of work) than the best header chain we know about.
                        send = mi->second->IsValid(BLOCK_VALID_SCRIPTS) && (pindexBestHeader != NULL) &&
                               (pindexBestHeader->GetBlockTime() - mi->second->GetBlockTime() < nOneMonth) &&
                               (GetBlockProofEquivalentTime(
                                    *pindexBestHeader, *mi->second, *pindexBestHeader, consensusParams) < nOneMonth);
                        if (!send)
                        {
                            LogPrintf("%s: ignoring request from peer=%i for old block that isn't in the main chain\n",
                                __func__, pfrom->GetId());
                        }
                        else
                        { // BU: don't relay excessive blocks
                            if (mi->second->nStatus & BLOCK_EXCESSIVE)
                                send = false;
                            if (!send)
                                LogPrintf("%s: ignoring request from peer=%i for excessive block of height %d not on "
                                          "the main chain\n",
                                    __func__, pfrom->GetId(), mi->second->nHeight);
                        }
                        // BU: in the future we can throttle old block requests by setting send=false if we are out of
                        // bandwidth
                    }
                }
                // disconnect node in case we have reached the outbound limit for serving historical blocks
                // never disconnect whitelisted nodes
                static const int nOneWeek = 7 * 24 * 60 * 60; // assume > 1 week = historical
                if (send && CNode::OutboundTargetReached(true) &&
                    (((pindexBestHeader != NULL) &&
                         (pindexBestHeader->GetBlockTime() - mi->second->GetBlockTime() > nOneWeek)) ||
                        inv.type == MSG_FILTERED_BLOCK) &&
                    !pfrom->fWhitelisted)
                {
                    LogPrint("net", "historical block serving limit reached, disconnect peer=%d\n", pfrom->GetId());

                    // disconnect node
                    pfrom->fDisconnect = true;
                    send = false;
                }
                // Pruned nodes may have deleted the block, so check whether
                // it's available before trying to send.
                if (send && (mi->second->nStatus & BLOCK_HAVE_DATA))
                {
                    // Send block from disk
                    CBlock block;
                    if (!ReadBlockFromDisk(block, (*mi).second, consensusParams))
                    {
                        // its possible that I know about it but haven't stored it yet
                        LogPrint("thin", "unable to load block %s from disk\n",
                            (*mi).second->phashBlock ? (*mi).second->phashBlock->ToString() : "");
                        // no response
                    }
                    else
                    {
                        if (inv.type == MSG_BLOCK)
                        {
                            pfrom->blocksSent += 1;
                            pfrom->PushMessage(NetMsgType::BLOCK, block);
                        }

                        // BUIP010 Xtreme Thinblocks: begin section
                        else if (inv.type == MSG_THINBLOCK || inv.type == MSG_XTHINBLOCK)
                        {
                            LogPrint("thin", "Sending xthin by INV queue getdata message\n");
                            SendXThinBlock(block, pfrom, inv);
                        }
                        // BUIP010 Xtreme Thinblocks: end section

                        else // MSG_FILTERED_BLOCK)
                        {
                            LOCK(pfrom->cs_filter);
                            if (pfrom->pfilter)
                            {
                                CMerkleBlock merkleBlock(block, *pfrom->pfilter);
                                pfrom->PushMessage(NetMsgType::MERKLEBLOCK, merkleBlock);
                                pfrom->blocksSent += 1;
                                // CMerkleBlock just contains hashes, so also push any transactions in the block the
                                // client did not see
                                // This avoids hurting performance by pointlessly requiring a round-trip
                                // Note that there is currently no way for a node to request any single transactions we
                                // didn't send here -
                                // they must either disconnect and retry or request the full block.
                                // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                                // however we MUST always provide at least what the remote peer needs
                                typedef std::pair<unsigned int, uint256> PairType;
                                BOOST_FOREACH (PairType &pair, merkleBlock.vMatchedTxn)
                                {
                                    pfrom->txsSent += 1;
                                    pfrom->PushMessage(NetMsgType::TX, block.vtx[pair.first]);
                                }
                            }
                            // else
                            // no response
                        }

                        // Trigger the peer node to send a getblocks request for the next batch of inventory
                        if (inv.hash == pfrom->hashContinue)
                        {
                            // Bypass PushInventory, this must send even if redundant,
                            // and we want it right after the last block so they don't
                            // wait for other stuff first.
                            std::vector<CInv> vInv;
                            vInv.push_back(CInv(MSG_BLOCK, chainActive.Tip()->GetBlockHash()));
                            pfrom->PushMessage(NetMsgType::INV, vInv);
                            pfrom->hashContinue.SetNull();
                        }
                    }
                }
            }
            else if (inv.IsKnownType())
            {
                // Send stream from relay memory
                bool pushed = false;
                {
                    CDataStream cd(0, 0);
                    if (1)
                    {
                        // BU: We need to release this lock before push message or there is a potential deadlock because
                        // cs_vSend is often taken before cs_mapRelay
                        LOCK(cs_mapRelay);
                        std::map<CInv, CDataStream>::iterator mi = mapRelay.find(inv);
                        if (mi != mapRelay.end())
                        {
                            cd = (*mi).second; // I have to copy, because .second may be deleted once lock is released
                            pushed = true;
                        }
                    }
                    if (pushed)
                    {
                        pfrom->PushMessage(inv.GetCommand(), cd);
                    }
                }
                if (!pushed && inv.type == MSG_TX)
                {
                    CTransaction tx;
                    if (mempool.lookup(inv.hash, tx))
                    {
                        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                        ss.reserve(1000);
                        ss << tx;
                        pfrom->PushMessage(NetMsgType::TX, ss);
                        pushed = true;
                        pfrom->txsSent += 1;
                    }
                }
                if (!pushed)
                {
                    vNotFound.push_back(inv);
                }
            }

            // Track requests for our stuff.
            GetMainSignals().Inventory(inv.hash);

            // BUIP010 Xtreme Thinblocks: if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK || inv.type == MSG_THINBLOCK ||
                inv.type == MSG_XTHINBLOCK)
                break;
        }
    }

    pfrom->vRecvGetData.erase(pfrom->vRecvGetData.begin(), it);

    if (!vNotFound.empty())
    {
        // Let the peer know that we didn't find what it asked for, so it doesn't
        // have to wait around forever. Currently only SPV clients actually care
        // about this message: it's needed when they are recursively walking the
        // dependencies of relevant unconfirmed transactions. SPV clients want to
        // do that because they want to know about (and store and rebroadcast and
        // risk analyze) the dependencies of transactions relevant to them, without
        // having to download the entire memory pool.
        pfrom->PushMessage(NetMsgType::NOTFOUND, vNotFound);
    }
}

bool ProcessMessage(CNode *pfrom, std::string strCommand, CDataStream &vRecv, int64_t nTimeReceived)
{
    int64_t receiptTime = GetTime();
    const CChainParams &chainparams = Params();
    RandAddSeedPerfmon();
    unsigned int msgSize = vRecv.size(); // BU for statistics
    UpdateRecvStats(pfrom, strCommand, msgSize, nTimeReceived);
    LogPrint("net", "received: %s (%u bytes) peer=%d\n", SanitizeString(strCommand), msgSize, pfrom->id);
    if (mapArgs.count("-dropmessagestest") && GetRand(atoi(mapArgs["-dropmessagestest"])) == 0)
    {
        LogPrintf("dropmessagestest DROPPING RECV MESSAGE\n");
        return true;
    }

    if (!(nLocalServices & NODE_BLOOM) &&
        (strCommand == NetMsgType::FILTERLOAD || strCommand == NetMsgType::FILTERADD ||
            strCommand == NetMsgType::FILTERCLEAR))
    {
        if (pfrom->nVersion >= NO_BLOOM_VERSION)
        {
            dosMan.Misbehaving(pfrom, 100);
            return false;
        }
        else
        {
            pfrom->fDisconnect = true;
            return false;
        }
    }


    if (strCommand == NetMsgType::VERSION)
    {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0)
        {
            pfrom->PushMessage(
                NetMsgType::REJECT, strCommand, REJECT_DUPLICATE, std::string("Duplicate version message"));
            pfrom->fDisconnect = true;
            return error("Duplicate version message received - disconnecting peer=%s version=%s", pfrom->GetLogName(),
                pfrom->cleanSubVer);
        }

        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        vRecv >> pfrom->nVersion >> pfrom->nServices >> nTime >> addrMe;

        CheckNodeSupportForThinBlocks(); // BUIP010 Xtreme Thinblocks

        if (pfrom->nVersion < MIN_PEER_PROTO_VERSION)
        {
            // ban peers older than this proto version
            pfrom->PushMessage(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
                strprintf("Protocol Version must be %d or greater", MIN_PEER_PROTO_VERSION));
            dosMan.Misbehaving(pfrom, 100);
            return error("Using obsolete protocol version %i - banning peer=%d version=%s ip=%s", pfrom->nVersion,
                pfrom->GetId(), pfrom->cleanSubVer, pfrom->addrName.c_str());
        }

        if (pfrom->nVersion == 10300)
            pfrom->nVersion = 300;
        if (!vRecv.empty())
            vRecv >> addrFrom >> nNonce;
        if (!vRecv.empty())
        {
            vRecv >> LIMITED_STRING(pfrom->strSubVer, MAX_SUBVERSION_LENGTH);
            pfrom->cleanSubVer = SanitizeString(pfrom->strSubVer);
        }
        if (!vRecv.empty())
            vRecv >> pfrom->nStartingHeight;
        if (!vRecv.empty())
            vRecv >> pfrom->fRelayTxes; // set to true after we get the first filter* message
        else
            pfrom->fRelayTxes = true;

        // Disconnect if we connected to ourself
        if (nNonce == nLocalHostNonce && nNonce > 1)
        {
            LogPrintf("connected to self at %s, disconnecting\n", pfrom->addr.ToString());
            pfrom->fDisconnect = true;
            return true;
        }

        pfrom->addrLocal = addrMe;
        if (pfrom->fInbound && addrMe.IsRoutable())
        {
            SeenLocal(addrMe);
        }

        // Be shy and don't send version until we hear
        if (pfrom->fInbound)
            pfrom->PushVersion();

        pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);

        // Potentially mark this peer as a preferred download peer.
        UpdatePreferredDownload(pfrom, State(pfrom->GetId()));

        // Send VERACK handshake message
        pfrom->PushMessage(NetMsgType::VERACK);
        pfrom->fVerackSent = true;

        // Change version
        pfrom->ssSend.SetVersion(std::min(pfrom->nVersion, PROTOCOL_VERSION));

        if (!pfrom->fInbound)
        {
            // Advertise our address
            if (fListen && !IsInitialBlockDownload())
            {
                CAddress addr = GetLocalAddress(&pfrom->addr);
                if (addr.IsRoutable())
                {
                    LogPrint("net", "ProcessMessages: advertising address %s\n", addr.ToString());
                    pfrom->PushAddress(addr);
                }
                else if (IsPeerAddrLocalGood(pfrom))
                {
                    addr.SetIP(pfrom->addrLocal);
                    LogPrint("net", "ProcessMessages: advertising address %s\n", addr.ToString());
                    pfrom->PushAddress(addr);
                }
            }

            // Get recent addresses
            if (pfrom->fOneShot || pfrom->nVersion >= CADDR_TIME_VERSION || addrman.size() < 1000)
            {
                pfrom->PushMessage(NetMsgType::GETADDR);
                pfrom->fGetAddr = true;
            }
            addrman.Good(pfrom->addr);
        }
        else
        {
            if (((CNetAddr)pfrom->addr) == (CNetAddr)addrFrom)
            {
                addrman.Add(addrFrom, addrFrom);
                addrman.Good(addrFrom);
            }
        }

        std::string remoteAddr;
        if (fLogIPs)
            remoteAddr = ", peeraddr=" + pfrom->addr.ToString();

        LogPrint("net", "receive version message: %s: version %d, blocks=%d, us=%s, peer=%d%s\n", pfrom->cleanSubVer,
            pfrom->nVersion, pfrom->nStartingHeight, addrMe.ToString(), pfrom->id, remoteAddr);

        int64_t nTimeOffset = nTime - GetTime();
        pfrom->nTimeOffset = nTimeOffset;
        AddTimeData(pfrom->addr, nTimeOffset);

        // Feeler connections exist only to verify if address is online.
        if (pfrom->fFeeler)
        {
            // Should never occur but if it does correct the value.
            // We can't have an inbound "feeler" connection, so the value must be improperly set.
            DbgAssert(pfrom->fInbound == false, pfrom->fFeeler = false);
            if (pfrom->fInbound == false)
                pfrom->fDisconnect = true;
        }
    }


    else if (pfrom->nVersion == 0 && !pfrom->fWhitelisted)
    {
        // Must have version message before anything else (Although we may send our VERSION before
        // we receive theirs, it would not be possible to receive their VERACK before their VERSION).
        pfrom->fDisconnect = true;
        return error("%s receieved before VERSION message - disconnecting peer=%s", strCommand, pfrom->GetLogName());
    }


    else if (strCommand == NetMsgType::VERACK)
    {
        // If we haven't sent a VERSION message yet then we should not get a VERACK message.
        if (pfrom->tVersionSent < 0)
        {
            pfrom->fDisconnect = true;
            return error("VERACK received but we never sent a VERSION message - disconnecting peer=%s version=%s",
                pfrom->GetLogName(), pfrom->cleanSubVer);
        }
        if (pfrom->fSuccessfullyConnected)
        {
            pfrom->fDisconnect = true;
            return error("duplicate VERACK received - disconnecting peer=%s version=%s", pfrom->GetLogName(),
                pfrom->cleanSubVer);
        }

        pfrom->fSuccessfullyConnected = true;
        pfrom->SetRecvVersion(std::min(pfrom->nVersion, PROTOCOL_VERSION));

        // Mark this node as currently connected, so we update its timestamp later.
        if (pfrom->fNetworkNode)
            pfrom->fCurrentlyConnected = true;

        if (pfrom->nVersion >= SENDHEADERS_VERSION)
        {
            // Tell our peer we prefer to receive headers rather than inv's
            // We send this to non-NODE NETWORK peers as well, because even
            // non-NODE NETWORK peers can announce blocks (such as pruning
            // nodes)

            pfrom->PushMessage(NetMsgType::SENDHEADERS);
        }

        // BU expedited procecessing requires the exchange of the listening port id but we have to send it in a separate
        // version
        // message because we don't know if in the future Core will append more data to the end of the current VERSION
        // message.
        // The BUVERSION should be after the VERACK message otherwise Core may flag an error if another messaged shows
        // up before the VERACK is received.
        // The BUVERSION message is active from the protocol EXPEDITED_VERSION onwards.
        if (pfrom->nVersion >= EXPEDITED_VERSION)
        {
            pfrom->PushMessage(NetMsgType::BUVERSION, GetListenPort());
            pfrom->fBUVersionSent = true;
        }
    }


    else if (!pfrom->fSuccessfullyConnected && GetTime() - pfrom->tVersionSent > VERACK_TIMEOUT &&
             pfrom->tVersionSent >= 0)
    {
        // If verack is not received within timeout then disconnect.
        // The peer may be slow so disconnect them only, to give them another chance if they try to re-connect.
        // If they are a bad peer and keep trying to reconnect and still do not VERACK, they will eventually
        // get banned by the connection slot algorithm which tracks disconnects and reconnects.
        pfrom->fDisconnect = true;
        LogPrint("net", "ERROR: disconnecting - VERACK not received within %d seconds for peer=%s version=%s\n",
            VERACK_TIMEOUT, pfrom->GetLogName(), pfrom->cleanSubVer);

        // update connection tracker which is used by the connection slot algorithm.
        LOCK(cs_mapInboundConnectionTracker);
        CNetAddr ipAddress = (CNetAddr)pfrom->addr;
        mapInboundConnectionTracker[ipAddress].nEvictions += 1;
        mapInboundConnectionTracker[ipAddress].nLastEvictionTime = GetTime();

        return true; // return true so we don't get any process message failures in the log.
    }


    else if (strCommand == NetMsgType::ADDR)
    {
        std::vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Don't want addr from older versions unless seeding
        if (pfrom->nVersion < CADDR_TIME_VERSION && addrman.size() > 1000)
            return true;
        if (vAddr.size() > 1000)
        {
            dosMan.Misbehaving(pfrom, 20);
            return error("message addr size() = %u", vAddr.size());
        }

        // Store the new addresses
        std::vector<CAddress> vAddrOk;
        int64_t nNow = GetAdjustedTime();
        int64_t nSince = nNow - 10 * 60;
        BOOST_FOREACH (CAddress &addr, vAddr)
        {
            boost::this_thread::interruption_point();

            if (addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
                addr.nTime = nNow - 5 * 24 * 60 * 60;
            pfrom->AddAddressKnown(addr);
            bool fReachable = IsReachable(addr);
            if (addr.nTime > nSince && !pfrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
            {
                // Relay to a limited number of other nodes
                {
                    LOCK(cs_vNodes);
                    // Use deterministic randomness to send to the same nodes for 24 hours
                    // at a time so the addrKnowns of the chosen nodes prevent repeats
                    static uint256 hashSalt;
                    if (hashSalt.IsNull())
                        hashSalt = GetRandHash();
                    uint64_t hashAddr = addr.GetHash();
                    uint256 hashRand = ArithToUint256(
                        UintToArith256(hashSalt) ^ (hashAddr << 32) ^ ((GetTime() + hashAddr) / (24 * 60 * 60)));
                    hashRand = Hash(BEGIN(hashRand), END(hashRand));
                    std::multimap<uint256, CNode *> mapMix;
                    BOOST_FOREACH (CNode *pnode, vNodes)
                    {
                        if (pnode->nVersion < CADDR_TIME_VERSION)
                            continue;
                        unsigned int nPointer;
                        memcpy(&nPointer, &pnode, sizeof(nPointer));
                        uint256 hashKey = ArithToUint256(UintToArith256(hashRand) ^ nPointer);
                        hashKey = Hash(BEGIN(hashKey), END(hashKey));
                        mapMix.insert(std::make_pair(hashKey, pnode));
                    }
                    int nRelayNodes = fReachable ? 2 : 1; // limited relaying of addresses outside our network(s)
                    for (std::multimap<uint256, CNode *>::iterator mi = mapMix.begin();
                         mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
                        ((*mi).second)->PushAddress(addr);
                }
            }
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        addrman.Add(vAddrOk, pfrom->addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom->fGetAddr = false;
        if (pfrom->fOneShot)
            pfrom->fDisconnect = true;
    }

    else if (strCommand == NetMsgType::SENDHEADERS)
    {
        LOCK(cs_main);
        State(pfrom->GetId())->fPreferHeaders = true;
    }


    else if (strCommand == NetMsgType::INV)
    {
        std::vector<CInv> vInv;
        vRecv >> vInv;

        // Message Consistency Checking
        //   Check size == 0 to be intolerant of an empty and useless request.
        //   Validate that INVs are a valid type and not null.
        if (vInv.size() > MAX_INV_SZ || vInv.empty())
        {
            dosMan.Misbehaving(pfrom, 20);
            return error("message inv size() = %u", vInv.size());
        }
        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
        {
            const CInv &inv = vInv[nInv];
            if (!((inv.type == MSG_TX) || (inv.type == MSG_BLOCK)) || inv.hash.IsNull())
            {
                dosMan.Misbehaving(pfrom, 20);
                return error("message inv invalid type = %u or is null hash %s", inv.type, inv.hash.ToString());
            }
        }

        bool fBlocksOnly = GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY);

        // Allow whitelisted peers to send data other than blocks in blocks only mode if whitelistrelay is true
        if (pfrom->fWhitelisted && GetBoolArg("-whitelistrelay", DEFAULT_WHITELISTRELAY))
            fBlocksOnly = false;

        LOCK(cs_main);

        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
        {
            const CInv &inv = vInv[nInv];

            boost::this_thread::interruption_point();

            bool fAlreadyHave = AlreadyHave(inv);
            LogPrint("net", "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom->id);

            if (inv.type == MSG_BLOCK)
            {
                UpdateBlockAvailability(pfrom->GetId(), inv.hash);
                // RE !IsInitialBlockDownload(): We do not want to get the block if the system is executing the initial
                // block download because
                // blocks are stored in block files in the order of arrival.  So grabbing blocks "early" will cause new
                // blocks to be sprinkled
                // throughout older block files.  This will stop those files from being pruned.
                // !IsInitialBlockDownload() can be removed if
                // a better block storage system is devised.
                if ((!fAlreadyHave && !fImporting && !fReindex && !IsInitialBlockDownload()) ||
                    // BU request && !mapBlocksInFlight.count(inv.hash)) {
                    (!fAlreadyHave && !fImporting && !fReindex && Params().NetworkIDString() == "regtest"))
                {
                    requester.AskFor(inv, pfrom);
                }
                else
                {
                    LogPrint("net", "skipping request of block %s.  already have: %d  importing: %d  reindex: %d  "
                                    "isChainNearlySyncd: %d\n",
                        inv.hash.ToString(), fAlreadyHave, fImporting, fReindex, IsChainNearlySyncd());
                }
            }
            else
            {
                pfrom->AddInventoryKnown(inv);
                if (fBlocksOnly)
                    LogPrint("net", "transaction (%s) inv sent in violation of protocol peer=%d\n", inv.hash.ToString(),
                        pfrom->id);
                // RE !IsInitialBlockDownload(): during IBD, its a waste of bandwidth to grab transactions, they will
                // likely be included
                // in blocks that we IBD download anyway.  This is especially important as transaction volumes increase.
                else if (!fAlreadyHave && !fImporting && !fReindex && !IsInitialBlockDownload())
                    requester.AskFor(inv, pfrom); // BU manage outgoing requests.  was: pfrom->AskFor(inv);
            }

            // Track requests for our stuff
            GetMainSignals().Inventory(inv.hash);

            if (pfrom->nSendSize > (SendBufferSize() * 2))
            {
                dosMan.Misbehaving(pfrom, 50);
                return error("send buffer size() = %u", pfrom->nSendSize);
            }
        }
    }


    else if (strCommand == NetMsgType::GETDATA)
    {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        // BU check size == 0 to be intolerant of an empty and useless request
        if ((vInv.size() > MAX_INV_SZ) || (vInv.size() == 0))
        {
            dosMan.Misbehaving(pfrom, 20);
            return error("message getdata size() = %u", vInv.size());
        }

        // Validate that INVs are a valid type
        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
        {
            const CInv &inv = vInv[nInv];
            if (!((inv.type == MSG_TX) || (inv.type == MSG_BLOCK) || (inv.type == MSG_FILTERED_BLOCK) ||
                    (inv.type == MSG_THINBLOCK) || (inv.type == MSG_XTHINBLOCK)))
            {
                dosMan.Misbehaving(pfrom, 20);
                return error("message inv invalid type = %u", inv.type);
            }
            // inv.hash does not need validation, since SHA2556 hash can be any value
        }


        if (fDebug || (vInv.size() != 1))
            LogPrint("net", "received getdata (%u invsz) peer=%d\n", vInv.size(), pfrom->id);

        if ((fDebug && vInv.size() > 0) || (vInv.size() == 1))
            LogPrint("net", "received getdata for: %s peer=%d\n", vInv[0].ToString(), pfrom->id);

        pfrom->vRecvGetData.insert(pfrom->vRecvGetData.end(), vInv.begin(), vInv.end());
        ProcessGetData(pfrom, chainparams.GetConsensus());
    }


    else if (strCommand == NetMsgType::GETBLOCKS)
    {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        LOCK(cs_main);

        // Find the last block the caller has in the main chain
        CBlockIndex *pindex = FindForkInGlobalIndex(chainActive, locator);

        // Send the rest of the chain
        if (pindex)
            pindex = chainActive.Next(pindex);
        int nLimit = 500;
        LogPrint("net", "getblocks %d to %s limit %d from peer=%d\n", (pindex ? pindex->nHeight : -1),
            hashStop.IsNull() ? "end" : hashStop.ToString(), nLimit, pfrom->id);
        for (; pindex; pindex = chainActive.Next(pindex))
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrint("net", "  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            // If pruning, don't inv blocks unless we have on disk and are likely to still have
            // for some reasonable time window (1 hour) that block relay might require.
            const int nPrunedBlocksLikelyToHave =
                MIN_BLOCKS_TO_KEEP - 3600 / chainparams.GetConsensus().nPowTargetSpacing;
            if (fPruneMode && (!(pindex->nStatus & BLOCK_HAVE_DATA) ||
                                  pindex->nHeight <= chainActive.Tip()->nHeight - nPrunedBlocksLikelyToHave))
            {
                LogPrint("net", " getblocks stopping, pruned or too old block at %d %s\n", pindex->nHeight,
                    pindex->GetBlockHash().ToString());
                break;
            }
            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll
                // trigger the peer to getblocks the next batch of inventory.
                LogPrint(
                    "net", "  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                pfrom->hashContinue = pindex->GetBlockHash();
                break;
            }
        }
    }


    else if (strCommand == NetMsgType::GETHEADERS)
    {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        if (IsInitialBlockDownload() && !pfrom->fWhitelisted)
        {
            LogPrint("net", "Ignoring getheaders from peer=%d because node is in initial block download\n", pfrom->id);
            return true;
        }

        LOCK(cs_main);
        CNodeState *nodestate = State(pfrom->GetId());
        CBlockIndex *pindex = NULL;
        if (locator.IsNull())
        {
            // If locator is null, return the hashStop block
            BlockMap::iterator mi = mapBlockIndex.find(hashStop);
            if (mi == mapBlockIndex.end())
                return true;
            pindex = (*mi).second;
        }
        else
        {
            // Find the last block the caller has in the main chain
            pindex = FindForkInGlobalIndex(chainActive, locator);
            if (pindex)
                pindex = chainActive.Next(pindex);
        }

        // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
        std::vector<CBlock> vHeaders;
        int nLimit = MAX_HEADERS_RESULTS;
        LogPrint("net", "getheaders height %d for block %s from peer %s\n", (pindex ? pindex->nHeight : -1),
            hashStop.ToString(), pfrom->GetLogName());
        for (; pindex; pindex = chainActive.Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        }
        // pindex can be NULL either if we sent chainActive.Tip() OR
        // if our peer has chainActive.Tip() (and thus we are sending an empty
        // headers message). In both cases it's safe to update
        // pindexBestHeaderSent to be our tip.
        nodestate->pindexBestHeaderSent = pindex ? pindex : chainActive.Tip();
        pfrom->PushMessage(NetMsgType::HEADERS, vHeaders);
    }


    else if (strCommand == NetMsgType::TX)
    {
        // Stop processing the transaction early if
        // We are in blocks only mode and peer is either not whitelisted or whitelistrelay is off
        if (GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) &&
            (!pfrom->fWhitelisted || !GetBoolArg("-whitelistrelay", DEFAULT_WHITELISTRELAY)))
        {
            LogPrint("net", "transaction sent in violation of protocol peer=%d\n", pfrom->id);
            return true;
        }

        std::vector<uint256> vWorkQueue;
        std::vector<uint256> vEraseQueue;
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);
        requester.Received(inv, pfrom, msgSize);

        LOCK(cs_main);

        bool fMissingInputs = false;
        CValidationState state;

        pfrom->setAskFor.erase(inv.hash);
        mapAlreadyAskedFor.erase(inv.hash);

        // Check for recently rejected (and do other quick existence checks)
        if (!AlreadyHave(inv) && AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs))
        {
            mempool.check(pcoinsTip);
            RelayTransaction(tx);
            vWorkQueue.push_back(inv.hash);

            LogPrint("mempool", "AcceptToMemoryPool: peer=%d: accepted %s (poolsz %u txn, %u kB)\n", pfrom->id,
                tx.GetHash().ToString(), mempool.size(), mempool.DynamicMemoryUsage() / 1000);

            // Recursively process any orphan transactions that depended on this one
            LOCK(cs_orphancache);
            std::set<NodeId> setMisbehaving;
            for (unsigned int i = 0; i < vWorkQueue.size(); i++)
            {
                std::map<uint256, std::set<uint256> >::iterator itByPrev =
                    mapOrphanTransactionsByPrev.find(vWorkQueue[i]);
                if (itByPrev == mapOrphanTransactionsByPrev.end())
                    continue;
                for (std::set<uint256>::iterator mi = itByPrev->second.begin(); mi != itByPrev->second.end(); ++mi)
                {
                    const uint256 &orphanHash = *mi;

                    // Make sure we actually have an entry on the orphan cache. While this should never fail because
                    // we always erase orphans and any mapOrphanTransactionsByPrev at the same time, still we need to
                    // be sure.
                    bool fOk = true;
                    DbgAssert(mapOrphanTransactions.count(orphanHash), fOk = false);
                    if (!fOk)
                        continue;

                    const CTransaction &orphanTx = mapOrphanTransactions[orphanHash].tx;
                    NodeId fromPeer = mapOrphanTransactions[orphanHash].fromPeer;
                    bool fMissingInputs2 = false;
                    // Use a dummy CValidationState so someone can't setup nodes to counter-DoS based on orphan
                    // resolution (that is, feeding people an invalid transaction based on LegitTxX in order to get
                    // anyone relaying LegitTxX banned)
                    CValidationState stateDummy;


                    if (setMisbehaving.count(fromPeer))
                        continue;
                    if (AcceptToMemoryPool(mempool, stateDummy, orphanTx, true, &fMissingInputs2))
                    {
                        LogPrint("mempool", "   accepted orphan tx %s\n", orphanHash.ToString());
                        RelayTransaction(orphanTx);
                        vWorkQueue.push_back(orphanHash);
                        vEraseQueue.push_back(orphanHash);
                    }
                    else if (!fMissingInputs2)
                    {
                        int nDos = 0;
                        if (stateDummy.IsInvalid(nDos) && nDos > 0)
                        {
                            // Punish peer that gave us an invalid orphan tx
                            dosMan.Misbehaving(fromPeer, nDos);
                            setMisbehaving.insert(fromPeer);
                            LogPrint("mempool", "   invalid orphan tx %s\n", orphanHash.ToString());
                        }
                        // Has inputs but not accepted to mempool
                        // Probably non-standard or insufficient fee/priority
                        LogPrint("mempool", "   removed orphan tx %s\n", orphanHash.ToString());
                        vEraseQueue.push_back(orphanHash);
                        if (recentRejects)
                            recentRejects->insert(orphanHash); // should always be true
                    }
                    mempool.check(pcoinsTip);
                }
            }
            BOOST_FOREACH (uint256 hash, vEraseQueue)
                EraseOrphanTx(hash);

            //  BU: Xtreme thinblocks - purge orphans that are too old
            EraseOrphansByTime();
        }
        else if (fMissingInputs)
        {
            LOCK(cs_orphancache);
            AddOrphanTx(tx, pfrom->GetId());

            // DoS prevention: do not allow mapOrphanTransactions to grow unbounded
            static unsigned int nMaxOrphanTx =
                (unsigned int)std::max((int64_t)0, GetArg("-maxorphantx", DEFAULT_MAX_ORPHAN_TRANSACTIONS));
            static uint64_t nMaxOrphanPoolSize =
                (uint64_t)std::max((int64_t)0, (GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000 / 10));
            unsigned int nEvicted = LimitOrphanTxSize(nMaxOrphanTx, nMaxOrphanPoolSize);
            if (nEvicted > 0)
                LogPrint("mempool", "mapOrphan overflow, removed %u tx\n", nEvicted);
        }
        else
        {
            if (recentRejects)
                recentRejects->insert(tx.GetHash()); // should always be true

            if (pfrom->fWhitelisted && GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY))
            {
                // Always relay transactions received from whitelisted peers, even
                // if they were already in the mempool or rejected from it due
                // to policy, allowing the node to function as a gateway for
                // nodes hidden behind it.
                //
                // Never relay transactions that we would assign a non-zero DoS
                // score for, as we expect peers to do the same with us in that
                // case.
                int nDoS = 0;
                if (!state.IsInvalid(nDoS) || nDoS == 0)
                {
                    LogPrintf("Force relaying tx %s from whitelisted peer=%d\n", tx.GetHash().ToString(), pfrom->id);
                    RelayTransaction(tx);
                }
                else
                {
                    LogPrintf("Not relaying invalid transaction %s from whitelisted peer=%d (%s)\n",
                        tx.GetHash().ToString(), pfrom->id, FormatStateMessage(state));
                }
            }
        }
        int nDoS = 0;
        if (state.IsInvalid(nDoS))
        {
            LogPrint("mempoolrej", "%s from peer=%d was not accepted: %s\n", tx.GetHash().ToString(), pfrom->id,
                FormatStateMessage(state));
            if (state.GetRejectCode() < REJECT_INTERNAL) // Never send AcceptToMemoryPool's internal codes over P2P
                pfrom->PushMessage(NetMsgType::REJECT, strCommand, (unsigned char)state.GetRejectCode(),
                    state.GetRejectReason().substr(0, MAX_REJECT_MESSAGE_LENGTH), inv.hash);
            if (nDoS > 0)
                dosMan.Misbehaving(pfrom, nDoS);
        }
        FlushStateToDisk(state, FLUSH_STATE_PERIODIC);
    }


    else if (strCommand == NetMsgType::HEADERS && !fImporting && !fReindex) // Ignore headers received while importing
    {
        std::vector<CBlockHeader> headers;

        // Bypass the normal CBlock deserialization, as we don't want to risk deserializing 2000 full blocks.
        unsigned int nCount = ReadCompactSize(vRecv);
        if (nCount > MAX_HEADERS_RESULTS)
        {
            dosMan.Misbehaving(pfrom, 20);
            return error("headers message size = %u", nCount);
        }
        headers.resize(nCount);
        for (unsigned int n = 0; n < nCount; n++)
        {
            vRecv >> headers[n];
            ReadCompactSize(vRecv); // ignore tx count; assume it is 0.
        }

        LOCK(cs_main);

        // Nothing interesting. Stop asking this peers for more headers.
        if (nCount == 0)
            return true;

        // Check all headers to make sure they are continuous before attempting to accept them.
        // This prevents and attacker from keeping us from doing direct fetch by giving us out
        // of order headers.
        bool fNewUnconnectedHeaders = false;
        uint256 hashLastBlock;
        hashLastBlock.SetNull();
        for (const CBlockHeader &header : headers)
        {
            // check that the first header has a previous block in the blockindex.
            if (hashLastBlock.IsNull())
            {
                BlockMap::iterator mi = mapBlockIndex.find(header.hashPrevBlock);
                if (mi != mapBlockIndex.end())
                    hashLastBlock = header.hashPrevBlock;
            }

            // Add this header to the map if it doesn't connect to a previous header
            if (header.hashPrevBlock != hashLastBlock)
            {
                // If we still haven't finished downloading the initial headers during node sync and we get
                // an out of order header then we must disconnect the node so that we can finish downloading
                // initial headers from a diffeent peer. An out of order header at this point is likely an attack
                // to prevent the node from syncing.
                if (header.GetBlockTime() < GetAdjustedTime() - 24 * 60 * 60)
                {
                    pfrom->fDisconnect = true;
                    return error("non-continuous-headers sequence during node sync - disconnecting peer=%s",
                        pfrom->GetLogName());
                }
                fNewUnconnectedHeaders = true;
            }

            // if we have an unconnected header then add every following header to the unconnected headers cache.
            if (fNewUnconnectedHeaders)
            {
                uint256 hash = header.GetHash();
                if (mapUnConnectedHeaders.size() < MAX_UNCONNECTED_HEADERS)
                    mapUnConnectedHeaders[hash] = std::make_pair(header, GetTime());

                // update hashLastUnknownBlock so that we'll be able to download the block from this peer even
                // if we receive the headers, which will connect this one, from a different peer.
                UpdateBlockAvailability(pfrom->GetId(), hash);
            }

            hashLastBlock = header.GetHash();
        }
        // return without error if we have an unconnected header.  This way we can try to connect it when the next
        // header arrives.
        if (fNewUnconnectedHeaders)
            return true;

        // If possible add any previously unconnected headers to the headers vector and remove any expired entries.
        std::map<uint256, std::pair<CBlockHeader, int64_t> >::iterator mi = mapUnConnectedHeaders.begin();
        while (mi != mapUnConnectedHeaders.end())
        {
            std::map<uint256, std::pair<CBlockHeader, int64_t> >::iterator toErase = mi;

            // Add the header if it connects to the previous header
            if (headers.back().GetHash() == (*mi).second.first.hashPrevBlock)
            {
                headers.push_back((*mi).second.first);
                mapUnConnectedHeaders.erase(toErase);

                // if you found one to connect then search from the beginning again in case there is another
                // that will connect to this new header that was added.
                mi = mapUnConnectedHeaders.begin();
                continue;
            }

            // Remove any entries that have been in the cache too long.  Unconnected headers should only exist
            // for a very short while, typically just a second or two.
            int64_t nTimeHeaderArrived = (*mi).second.second;
            uint256 headerHash = (*mi).first;
            mi++;
            if (GetTime() - nTimeHeaderArrived >= UNCONNECTED_HEADERS_TIMEOUT)
            {
                mapUnConnectedHeaders.erase(toErase);
            }
            // At this point we know the headers in the list received are known to be in order, therefore,
            // check if the header is equal to some other header in the list. If so then remove it from the cache.
            else
            {
                for (const CBlockHeader &header : headers)
                {
                    if (header.GetHash() == headerHash)
                    {
                        mapUnConnectedHeaders.erase(toErase);
                        break;
                    }
                }
            }
        }

        // Check and accept each header in order from youngest block to oldest
        CBlockIndex *pindexLast = NULL;
        for (const CBlockHeader &header : headers)
        {
            CValidationState state;
            if (!AcceptBlockHeader(header, state, chainparams, &pindexLast))
            {
                int nDoS;
                if (state.IsInvalid(nDoS))
                {
                    if (nDoS > 0)
                        dosMan.Misbehaving(pfrom, nDoS);
                    return error("invalid header received");
                }
            }
            PV->UpdateMostWorkOurFork(header);
        }

        if (pindexLast)
            UpdateBlockAvailability(pfrom->GetId(), pindexLast->GetBlockHash());

        if (nCount == MAX_HEADERS_RESULTS && pindexLast)
        {
            // Headers message had its maximum size; the peer may have more headers.
            // TODO: optimize: if pindexLast is an ancestor of chainActive.Tip or pindexBestHeader, continue
            // from there instead.
            LogPrint("net", "more getheaders (%d) to end to peer=%s (startheight:%d)\n", pindexLast->nHeight,
                pfrom->GetLogName(), pfrom->nStartingHeight);
            pfrom->PushMessage(NetMsgType::GETHEADERS, chainActive.GetLocator(pindexLast), uint256());
        }

        bool fCanDirectFetch = CanDirectFetch(chainparams.GetConsensus());
        CNodeState *nodestate = State(pfrom->GetId());

        // During the initial peer handshake we must receive the initial headers which should be greater
        // than or equal to our block height at the time of requesting GETHEADERS. This is because the peer has
        // advertised a height >= to our own. Furthermore, because the headers max returned is as much as 2000 this
        // could not be a mainnet re-org.
        if (!nodestate->fFirstHeadersReceived)
        {
            // We want to make sure that the peer doesn't just send us any old valid header. The block height of the
            // last header they send us should be equal to our block height at the time we made the GETHEADERS request.
            if (pindexLast && nodestate->nFirstHeadersExpectedHeight <= pindexLast->nHeight)
            {
                nodestate->fFirstHeadersReceived = true;
                LogPrint("net", "Initial headers received for peer=%s\n", pfrom->GetLogName());
            }

            // Allow for very large reorgs (> 2000 blocks) on the nol test chain or other test net.
            if (Params().NetworkIDString() != "main" && Params().NetworkIDString() != "regtest")
                nodestate->fFirstHeadersReceived = true;
        }

        // update the syncd status.  This should come before we make calls to requester.AskFor().
        IsChainNearlySyncdInit();
        IsInitialBlockDownloadInit();

        // If this set of headers is valid and ends in a block with at least as
        // much work as our tip, download as much as possible.
        if (fCanDirectFetch && pindexLast && pindexLast->IsValid(BLOCK_VALID_TREE) &&
            chainActive.Tip()->nChainWork <= pindexLast->nChainWork)
        {
            // Set tweak value.  Mostly used in testing direct fetch.
            if (maxBlocksInTransitPerPeer.value != 0)
                MAX_BLOCKS_IN_TRANSIT_PER_PEER = maxBlocksInTransitPerPeer.value;

            std::vector<CBlockIndex *> vToFetch;
            CBlockIndex *pindexWalk = pindexLast;
            // Calculate all the blocks we'd need to switch to pindexLast.
            while (pindexWalk && !chainActive.Contains(pindexWalk))
            {
                vToFetch.push_back(pindexWalk);
                pindexWalk = pindexWalk->pprev;
            }

            // Download as much as possible, from earliest to latest.
            unsigned int nAskFor = 0;
            BOOST_REVERSE_FOREACH (CBlockIndex *pindex, vToFetch)
            {
                // pindex must be nonnull because we populated vToFetch a few lines above
                CInv inv(MSG_BLOCK, pindex->GetBlockHash());
                if (!AlreadyHave(inv))
                {
                    requester.AskFor(inv, pfrom);
                    LogPrint("req", "AskFor block via headers direct fetch %s (%d) peer=%d\n",
                        pindex->GetBlockHash().ToString(), pindex->nHeight, pfrom->id);
                    nAskFor++;
                }
                // We don't care about how many blocks are in flight.  We just need to make sure we don't
                // ask for more than the maximum allowed per peer because the request manager will take care
                // of any duplicate requests.
                if (nAskFor >= MAX_BLOCKS_IN_TRANSIT_PER_PEER)
                {
                    LogPrint("net", "Large reorg, could only direct fetch %d blocks\n", nAskFor);
                    break;
                }
            }
            if (nAskFor > 1)
            {
                LogPrint("net", "Downloading blocks toward %s (%d) via headers direct fetch\n",
                    pindexLast->GetBlockHash().ToString(), pindexLast->nHeight);
            }
        }

        CheckBlockIndex(chainparams.GetConsensus());
    }

    // BUIP010 Xtreme Thinblocks: begin section
    else if (strCommand == NetMsgType::GET_XTHIN && !fImporting && !fReindex && IsThinBlocksEnabled())
    {
        if (!pfrom->ThinBlockCapable())
        {
            dosMan.Misbehaving(pfrom, 100);
            return error("Thinblock message received from a non thinblock node, peer=%d", pfrom->GetId());
        }

        // Check for Misbehaving and DOS
        // If they make more than 20 requests in 10 minutes then disconnect them
        {
            LOCK(cs_vNodes);
            if (pfrom->nGetXthinLastTime <= 0)
                pfrom->nGetXthinLastTime = GetTime();
            uint64_t nNow = GetTime();
            pfrom->nGetXthinCount *= std::pow(1.0 - 1.0 / 600.0, (double)(nNow - pfrom->nGetXthinLastTime));
            pfrom->nGetXthinLastTime = nNow;
            pfrom->nGetXthinCount += 1;
            LogPrint("thin", "nGetXthinCount is %f\n", pfrom->nGetXthinCount);
            if (chainparams.NetworkIDString() == "main") // other networks have variable mining rates
            {
                if (pfrom->nGetXthinCount >= 20)
                {
                    dosMan.Misbehaving(pfrom, 100); // If they exceed the limit then disconnect them
                    return error("requesting too many get_xthin");
                }
            }
        }

        CBloomFilter filterMemPool;
        CInv inv;
        vRecv >> inv >> filterMemPool;

        // Message consistency checking
        if (!((inv.type == MSG_XTHINBLOCK) || (inv.type == MSG_THINBLOCK)) || inv.hash.IsNull())
        {
            dosMan.Misbehaving(pfrom, 100);
            return error("invalid get_xthin type=%u hash=%s", inv.type, inv.hash.ToString());
        }


        // Validates that the filter is reasonably sized.
        LoadFilter(pfrom, &filterMemPool);
        {
            LOCK(cs_main);
            BlockMap::iterator mi = mapBlockIndex.find(inv.hash);
            if (mi == mapBlockIndex.end())
            {
                dosMan.Misbehaving(pfrom, 100);
                return error("Peer %s (%d) requested nonexistent block %s", pfrom->addrName.c_str(), pfrom->id,
                    inv.hash.ToString());
            }

            CBlock block;
            const Consensus::Params &consensusParams = Params().GetConsensus();
            if (!ReadBlockFromDisk(block, (*mi).second, consensusParams))
            {
                // We don't have the block yet, although we know about it.
                return error("Peer %s (%d) requested block %s that cannot be read", pfrom->addrName.c_str(), pfrom->id,
                    inv.hash.ToString());
            }
            else
            {
                SendXThinBlock(block, pfrom, inv);
            }
        }
    }


    else if (strCommand == NetMsgType::XPEDITEDREQUEST)
    {
        return HandleExpeditedRequest(vRecv, pfrom);
    }


    else if (strCommand == NetMsgType::XPEDITEDBLK)
    {
        // ignore the expedited message unless we are at the chain tip...
        if (!fImporting && !fReindex && !IsInitialBlockDownload())
        {
            if (!HandleExpeditedBlock(vRecv, pfrom))
            {
                dosMan.Misbehaving(pfrom, 5);
                return false;
            }
        }
    }


    // BUVERSION is used to pass BU specific version information similar to NetMsgType::VERSION
    // and is exchanged after the VERSION and VERACK are both sent and received.
    else if (strCommand == NetMsgType::BUVERSION)
    {
        // If we never sent a VERACK message then we should not get a BUVERSION message.
        if (!pfrom->fVerackSent)
        {
            dosMan.Misbehaving(pfrom, 100);
            return error("BUVERSION received but we never sent a VERACK message - banning peer=%d version=%s ip=%s",
                pfrom->GetId(), pfrom->cleanSubVer, pfrom->addrName.c_str());
        }
        // Each connection can only send one version message
        if (pfrom->addrFromPort != 0)
        {
            pfrom->PushMessage(
                NetMsgType::REJECT, strCommand, REJECT_DUPLICATE, std::string("Duplicate BU version message"));
            dosMan.Misbehaving(pfrom, 100);
            return error("Duplicate BU version message received from peer=%d version=%s ip=%s", pfrom->GetId(),
                pfrom->cleanSubVer, pfrom->addrName.c_str());
        }

        // addrFromPort is needed for connecting and initializing Xpedited forwarding.
        vRecv >> pfrom->addrFromPort;
        pfrom->PushMessage(NetMsgType::BUVERACK);
    }
    // Final handshake for BU specific version information similar to NetMsgType::VERACK
    else if (strCommand == NetMsgType::BUVERACK)
    {
        // If we never sent a BUVERSION message then we should not get a VERACK message.
        if (!pfrom->fBUVersionSent)
        {
            dosMan.Misbehaving(pfrom, 100);
            return error("BUVERACK received but we never sent a BUVERSION message - banning peer=%d version=%s ip=%s",
                pfrom->GetId(), pfrom->cleanSubVer, pfrom->addrName.c_str());
        }

        // This step done after final handshake
        CheckAndRequestExpeditedBlocks(pfrom);
    }

    else if (strCommand == NetMsgType::XTHINBLOCK && !fImporting && !fReindex && !IsInitialBlockDownload() &&
             IsThinBlocksEnabled())
    {
        return CXThinBlock::HandleMessage(vRecv, pfrom, strCommand, 0);
    }


    else if (strCommand == NetMsgType::THINBLOCK && !fImporting && !fReindex && !IsInitialBlockDownload() &&
             IsThinBlocksEnabled())
    {
        return CThinBlock::HandleMessage(vRecv, pfrom);
    }


    else if (strCommand == NetMsgType::GET_XBLOCKTX && !fImporting && !fReindex && !IsInitialBlockDownload() &&
             IsThinBlocksEnabled())
    {
        return CXRequestThinBlockTx::HandleMessage(vRecv, pfrom);
    }


    else if (strCommand == NetMsgType::XBLOCKTX && !fImporting && !fReindex && !IsInitialBlockDownload() &&
             IsThinBlocksEnabled())
    {
        return CXThinBlockTx::HandleMessage(vRecv, pfrom);
    }
    // BUIP010 Xtreme Thinblocks: end section


    else if (strCommand == NetMsgType::BLOCK && !fImporting && !fReindex) // Ignore blocks received while importing
    {
        CBlock block;
        vRecv >> block;

        CInv inv(MSG_BLOCK, block.GetHash());
        LogPrint("blk", "received block %s peer=%d\n", inv.hash.ToString(), pfrom->id);
        UnlimitedLogBlock(block, inv.hash.ToString(), receiptTime);

        if (IsChainNearlySyncd()) // BU send the received block out expedited channels quickly
        {
            CValidationState state;
            if (CheckBlockHeader(block, state, true)) // block header is fine
                SendExpeditedBlock(block, pfrom);
        }

        // Message consistency checking
        // NOTE: consistency checking is handled by checkblock() which is called during
        //       ProcessNewBlock() during HandleBlockMessage.
        PV->HandleBlockMessage(pfrom, strCommand, block, inv);
    }


    else if (strCommand == NetMsgType::GETADDR)
    {
        // This asymmetric behavior for inbound and outbound connections was introduced
        // to prevent a fingerprinting attack: an attacker can send specific fake addresses
        // to users' AddrMan and later request them by sending getaddr messages.
        // Making nodes which are behind NAT and can only make outgoing connections ignore
        // the getaddr message mitigates the attack.
        if (!pfrom->fInbound)
        {
            LogPrint("net", "Ignoring \"getaddr\" from outbound connection. peer=%d\n", pfrom->id);
            return true;
        }

        // Only send one GetAddr response per connection to reduce resource waste
        //  and discourage addr stamping of INV announcements.
        if (pfrom->fSentAddr)
        {
            LogPrint("net", "Ignoring repeated \"getaddr\". peer=%d\n", pfrom->id);
            return true;
        }
        pfrom->fSentAddr = true;

        pfrom->vAddrToSend.clear();
        std::vector<CAddress> vAddr = addrman.GetAddr();
        BOOST_FOREACH (const CAddress &addr, vAddr)
            pfrom->PushAddress(addr);
    }


    else if (strCommand == NetMsgType::MEMPOOL)
    {
        if (CNode::OutboundTargetReached(false) && !pfrom->fWhitelisted)
        {
            LogPrint("net", "mempool request with bandwidth limit reached, disconnect peer=%d\n", pfrom->GetId());
            pfrom->fDisconnect = true;
            return true;
        }
        std::vector<uint256> vtxid;
        if (1) // Keep this lock for as short as possible, causing 2.6 second locks
        {
            LOCK2(cs_main, pfrom->cs_filter);
            mempool.queryHashes(vtxid);
        }
        std::vector<CInv> vInv;
        BOOST_FOREACH (uint256 &hash, vtxid)
        {
            CInv inv(MSG_TX, hash);
            if (pfrom->pfilter)
            {
                CTransaction tx;
                bool fInMemPool = mempool.lookup(hash, tx);
                if (!fInMemPool)
                    continue; // another thread removed since queryHashes, maybe...
                if (!pfrom->pfilter->IsRelevantAndUpdate(tx))
                    continue;
            }
            vInv.push_back(inv);
            if (vInv.size() == MAX_INV_SZ)
            {
                pfrom->PushMessage(NetMsgType::INV, vInv);
                vInv.clear();
            }
        }
        if (vInv.size() > 0)
            pfrom->PushMessage(NetMsgType::INV, vInv);
    }


    else if (strCommand == NetMsgType::PING)
    {
        if (pfrom->nVersion > BIP0031_VERSION)
        {
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
            pfrom->PushMessage(NetMsgType::PONG, nonce);
        }
    }


    else if (strCommand == NetMsgType::PONG)
    {
        int64_t pingUsecEnd = nTimeReceived;
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;

        if (nAvail >= sizeof(nonce))
        {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (pfrom->nPingNonceSent != 0)
            {
                if (nonce == pfrom->nPingNonceSent)
                {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished = true;
                    int64_t pingUsecTime = pingUsecEnd - pfrom->nPingUsecStart;
                    if (pingUsecTime > 0)
                    {
                        // Successful ping time measurement, replace previous
                        pfrom->nPingUsecTime = pingUsecTime;
                        pfrom->nMinPingUsecTime = std::min(pfrom->nMinPingUsecTime, pingUsecTime);
                    }
                    else
                    {
                        // This should never happen
                        sProblem = "Timing mishap";
                    }
                }
                else
                {
                    // Nonce mismatches are normal when pings are overlapping
                    sProblem = "Nonce mismatch";
                    if (nonce == 0)
                    {
                        // This is most likely a bug in another implementation somewhere; cancel this ping
                        bPingFinished = true;
                        sProblem = "Nonce zero";
                    }
                }
            }
            else
            {
                sProblem = "Unsolicited pong without ping";
            }
        }
        else
        {
            // This is most likely a bug in another implementation somewhere; cancel this ping
            bPingFinished = true;
            sProblem = "Short payload";
        }

        if (!(sProblem.empty()))
        {
            LogPrint("net", "pong peer=%d: %s, %x expected, %x received, %u bytes\n", pfrom->id, sProblem,
                pfrom->nPingNonceSent, nonce, nAvail);
        }
        if (bPingFinished)
        {
            pfrom->nPingNonceSent = 0;
        }
    }


    else if (strCommand == NetMsgType::FILTERLOAD)
    {
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints())
        {
            // There is no excuse for sending a too-large filter
            dosMan.Misbehaving(pfrom, 100);
            return false;
        }
        else
        {
            LOCK(pfrom->cs_filter);
            delete pfrom->pfilter;
            pfrom->pfilter = new CBloomFilter(filter);
        }
        pfrom->fRelayTxes = true;
    }


    else if (strCommand == NetMsgType::FILTERADD)
    {
        std::vector<unsigned char> vData;
        vRecv >> vData;

        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE)
        {
            dosMan.Misbehaving(pfrom, 100);
        }
        else
        {
            LOCK(pfrom->cs_filter);
            if (pfrom->pfilter)
                pfrom->pfilter->insert(vData);
            else
                dosMan.Misbehaving(pfrom, 100);
        }
    }


    else if (strCommand == NetMsgType::FILTERCLEAR)
    {
        LOCK(pfrom->cs_filter);
        delete pfrom->pfilter;
        pfrom->pfilter = new CBloomFilter();
        pfrom->fRelayTxes = true;
    }


    else if (strCommand == NetMsgType::REJECT)
    {
        // BU: Request manager: this was restructured to not just be active in fDebug mode so that the request manager
        // can be notified of request rejections.
        try
        {
            std::string strMsg;
            unsigned char ccode;
            std::string strReason;
            uint256 hash;

            vRecv >> LIMITED_STRING(strMsg, CMessageHeader::COMMAND_SIZE) >> ccode >>
                LIMITED_STRING(strReason, MAX_REJECT_MESSAGE_LENGTH);
            std::ostringstream ss;
            ss << strMsg << " code " << itostr(ccode) << ": " << strReason;

            // BU: Check request manager reject codes
            if (strMsg == NetMsgType::BLOCK || strMsg == NetMsgType::TX)
            {
                vRecv >> hash;
                ss << ": hash " << hash.ToString();

                // We need to see this reject message in either "req" or "net" debug mode
                LogPrint("req", "Reject %s\n", SanitizeString(ss.str()));
                LogPrint("net", "Reject %s\n", SanitizeString(ss.str()));

                if (strMsg == NetMsgType::BLOCK)
                {
                    requester.Rejected(CInv(MSG_BLOCK, hash), pfrom, ccode);
                }
                else if (strMsg == NetMsgType::TX)
                {
                    requester.Rejected(CInv(MSG_TX, hash), pfrom, ccode);
                }
            }
            // if (fDebug) {
            // ostringstream ss;
            // ss << strMsg << " code " << itostr(ccode) << ": " << strReason;

            // if (strMsg == NetMsgType::BLOCK || strMsg == NetMsgType::TX)
            //  {
            //    ss << ": hash " << hash.ToString();
            //  }
            // LogPrint("net", "Reject %s\n", SanitizeString(ss.str()));
            // }
        }
        catch (const std::ios_base::failure &)
        {
            // Avoid feedback loops by preventing reject messages from triggering a new reject message.
            LogPrint("net", "Unparseable reject message received\n");
            LogPrint("req", "Unparseable reject message received\n");
        }
    }

    else
    {
        // Ignore unknown commands for extensibility
        LogPrint("net", "Unknown command \"%s\" from peer=%d\n", SanitizeString(strCommand), pfrom->id);
    }

    return true;
}


bool ProcessMessages(CNode *pfrom)
{
    AssertLockHeld(pfrom->cs_vRecvMsg);
    const CChainParams &chainparams = Params();
    // if (fDebug)
    //    LogPrintf("%s(%u messages)\n", __func__, pfrom->vRecvMsg.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fOk = true;

    if (!pfrom->vRecvGetData.empty())
        ProcessGetData(pfrom, chainparams.GetConsensus());

    // this maintains the order of responses
    if (!pfrom->vRecvGetData.empty())
        return fOk;

    std::deque<CNetMessage>::iterator it = pfrom->vRecvMsg.begin();
    while (!pfrom->fDisconnect && it != pfrom->vRecvMsg.end())
    {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;

        // get next message
        CNetMessage &msg = *it;

        // if (fDebug)
        //    LogPrintf("%s(message %u msgsz, %u bytes, complete:%s)\n", __func__,
        //            msg.hdr.nMessageSize, msg.vRecv.size(),
        //            msg.complete() ? "Y" : "N");

        // end, if an incomplete message is found
        if (!msg.complete())
            break;

        // at this point, any failure means we can delete the current message
        it++;

        // Scan for message start
        if (memcmp(msg.hdr.pchMessageStart, chainparams.MessageStart(), MESSAGE_START_SIZE) != 0)
        {
            LogPrintf("PROCESSMESSAGE: INVALID MESSAGESTART %s peer=%d ip=%s\n", SanitizeString(msg.hdr.GetCommand()),
                pfrom->id, pfrom->addrName.c_str());
            if (!pfrom->fWhitelisted)
            {
                dosMan.Ban(pfrom->addr, BanReasonNodeMisbehaving, 4 * 60 * 60); // ban for 4 hours
            }
            fOk = false;
            break;
        }

        // Read header
        CMessageHeader &hdr = msg.hdr;
        if (!hdr.IsValid(chainparams.MessageStart()))
        {
            LogPrintf("PROCESSMESSAGE: ERRORS IN HEADER %s peer=%d\n", SanitizeString(hdr.GetCommand()), pfrom->id);
            continue;
        }
        std::string strCommand = hdr.GetCommand();

        // Message size
        unsigned int nMessageSize = hdr.nMessageSize;

        // Checksum
        CDataStream &vRecv = msg.vRecv;
        uint256 hash = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        unsigned int nChecksum = ReadLE32((unsigned char *)&hash);
        if (nChecksum != hdr.nChecksum)
        {
            LogPrintf("%s(%s, %u bytes): CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n", __func__,
                SanitizeString(strCommand), nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Process message
        bool fRet = false;
        try
        {
            fRet = ProcessMessage(pfrom, strCommand, vRecv, msg.nTime);
            boost::this_thread::interruption_point();
        }
        catch (const std::ios_base::failure &e)
        {
            pfrom->PushMessage(NetMsgType::REJECT, strCommand, REJECT_MALFORMED, std::string("error parsing message"));
            if (strstr(e.what(), "end of data"))
            {
                // Allow exceptions from under-length message on vRecv
                LogPrintf("%s(%s, %u bytes): Exception '%s' caught, normally caused by a message being shorter than "
                          "its stated length\n",
                    __func__, SanitizeString(strCommand), nMessageSize, e.what());
            }
            else if (strstr(e.what(), "size too large"))
            {
                // Allow exceptions from over-long size
                LogPrintf("%s(%s, %u bytes): Exception '%s' caught\n", __func__, SanitizeString(strCommand),
                    nMessageSize, e.what());
            }
            else
            {
                PrintExceptionContinue(&e, "ProcessMessages()");
            }
        }
        catch (const boost::thread_interrupted &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            PrintExceptionContinue(&e, "ProcessMessages()");
        }
        catch (...)
        {
            PrintExceptionContinue(NULL, "ProcessMessages()");
        }

        if (!fRet)
            LogPrintf(
                "%s(%s, %u bytes) FAILED peer=%d\n", __func__, SanitizeString(strCommand), nMessageSize, pfrom->id);

        break;
    }

    // In case the connection got shut down, its receive buffer was wiped
    if (!pfrom->fDisconnect)
        pfrom->vRecvMsg.erase(pfrom->vRecvMsg.begin(), it);

    return fOk;
}


bool SendMessages(CNode *pto)
{
    const Consensus::Params &consensusParams = Params().GetConsensus();
    {
        // First set fDisconnect if appropriate.
        pto->DisconnectIfBanned();

        // Now exit early if disconnecting or the version handshake is not complete.  We must not send PING or other
        // connection maintenance messages before the handshake is done.
        if (pto->fDisconnect || !pto->fSuccessfullyConnected)
            return true;

        //
        // Message: ping
        //
        bool pingSend = false;
        if (pto->fPingQueued)
        {
            // RPC ping request by user
            pingSend = true;
        }
        if (pto->nPingNonceSent == 0 && pto->nPingUsecStart + PING_INTERVAL * 1000000 < GetTimeMicros())
        {
            // Ping automatically sent as a latency probe & keepalive.
            pingSend = true;
        }
        if (pingSend)
        {
            uint64_t nonce = 0;
            while (nonce == 0)
            {
                GetRandBytes((unsigned char *)&nonce, sizeof(nonce));
            }
            pto->fPingQueued = false;
            pto->nPingUsecStart = GetTimeMicros();
            if (pto->nVersion > BIP0031_VERSION)
            {
                pto->nPingNonceSent = nonce;
                pto->PushMessage(NetMsgType::PING, nonce);
            }
            else
            {
                // Peer is too old to support ping command with nonce, pong will never arrive.
                pto->nPingNonceSent = 0;
                pto->PushMessage(NetMsgType::PING);
            }
        }

        // Check to see if there are any thinblocks in flight that have gone beyond the timeout interval.
        // If so then we need to disconnect them so that the thinblock data is nullified.  We coud null
        // the thinblock data here but that would possible cause a node to be baneed later if the thinblock
        // finally did show up. Better to just disconnect this slow node instead.
        if (pto->mapThinBlocksInFlight.size() > 0)
        {
            LOCK(pto->cs_mapthinblocksinflight);
            std::map<uint256, CNode::CThinBlockInFlight>::iterator iter = pto->mapThinBlocksInFlight.begin();
            while (iter != pto->mapThinBlocksInFlight.end())
            {
                if (!(*iter).second.fReceived && (GetTime() - (*iter).second.nRequestTime) > THINBLOCK_DOWNLOAD_TIMEOUT)
                {
                    if (!pto->fWhitelisted && Params().NetworkIDString() != "regtest")
                    {
                        LogPrint("thin", "ERROR: Disconnecting peer=%d due to download timeout exceeded "
                                         "(%d secs)\n",
                            pto->GetId(), (GetTime() - (*iter).second.nRequestTime));
                        pto->fDisconnect = true;
                        break;
                    }
                }
                iter++;
            }
        }

        TRY_LOCK(cs_main, lockMain); // Acquire cs_main for IsInitialBlockDownload() and CNodeState()
        if (!lockMain)
        {
            LogPrint("net", "skipping SendMessages to %s, cs_main is locked\n", pto->addr.ToString());
            return true;
        }
        TRY_LOCK(pto->cs_vSend, lockSend);
        if (!lockSend)
        {
            LogPrint("net", "skipping SendMessages to %s, pto->cs_vSend is locked\n", pto->addr.ToString());
            return true;
        }

        // Address refresh broadcast
        int64_t nNow = GetTimeMicros();
        if (!IsInitialBlockDownload() && pto->nNextLocalAddrSend < nNow)
        {
            AdvertiseLocal(pto);
            pto->nNextLocalAddrSend = PoissonNextSend(nNow, AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL);
        }

        //
        // Message: addr
        //
        if (pto->nNextAddrSend < nNow)
        {
            pto->nNextAddrSend = PoissonNextSend(nNow, AVG_ADDRESS_BROADCAST_INTERVAL);
            std::vector<CAddress> vAddr;
            vAddr.reserve(pto->vAddrToSend.size());
            BOOST_FOREACH (const CAddress &addr, pto->vAddrToSend)
            {
                if (!pto->addrKnown.contains(addr.GetKey()))
                {
                    pto->addrKnown.insert(addr.GetKey());
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than 1000
                    if (vAddr.size() >= 1000)
                    {
                        pto->PushMessage(NetMsgType::ADDR, vAddr);
                        vAddr.clear();
                    }
                }
            }
            pto->vAddrToSend.clear();
            if (!vAddr.empty())
                pto->PushMessage(NetMsgType::ADDR, vAddr);
        }

        CNodeState &state = *State(pto->GetId());

        // If a sync has been started check whether we received the first batch of headers requested within the timeout
        // period.
        // If not then disconnect and ban the node and a new node will automatically be selected to start the headers
        // download.
        if ((state.fSyncStarted) && (state.fSyncStartTime < GetTime() - INITIAL_HEADERS_TIMEOUT) &&
            (!state.fFirstHeadersReceived) && !pto->fWhitelisted)
        {
            pto->fDisconnect = true;
            LogPrintf(
                "Initial headers were either not received or not received before the timeout - disconnecting peer=%s\n",
                pto->GetLogName());
        }

        // Start block sync
        if (pindexBestHeader == NULL)
            pindexBestHeader = chainActive.Tip();
        // Download if this is a nice peer, or we have no nice peers and this one might do.
        bool fFetch = state.fPreferredDownload || (nPreferredDownload == 0 && !pto->fClient && !pto->fOneShot);
        if (!state.fSyncStarted && !pto->fClient && !fImporting && !fReindex)
        {
            // Only actively request headers from a single peer, unless we're close to today.
            if ((nSyncStarted == 0 && fFetch) ||
                chainActive.Tip()->GetBlockTime() > GetAdjustedTime() - SINGLE_PEER_REQUEST_MODE_AGE)
            {
                const CBlockIndex *pindexStart = chainActive.Tip(); // pindexBestHeader;
                /* If possible, start at the block preceding the currently
                   best known header.  This ensures that we always get a
                   non-empty list of headers back as long as the peer
                   is up-to-date.  With a non-empty response, we can initialise
                   the peer's known best block.  This wouldn't be possible
                   if we requested starting at pindexBestHeader and
                   got back an empty response.  */
                if (pindexStart->pprev)
                    pindexStart = pindexStart->pprev;
                // BU Bug fix for Core:  Don't start downloading headers unless our chain is shorter
                if (pindexStart->nHeight < pto->nStartingHeight)
                {
                    state.fSyncStarted = true;
                    state.fSyncStartTime = GetTime();
                    state.fFirstHeadersReceived = false;
                    state.nFirstHeadersExpectedHeight = pindexBestHeader->nHeight;
                    nSyncStarted++;

                    LogPrint("net", "initial getheaders (%d) to peer=%s (startheight:%d)\n", pindexStart->nHeight,
                        pto->GetLogName(), pto->nStartingHeight);
                    pto->PushMessage(NetMsgType::GETHEADERS, chainActive.GetLocator(pindexStart), uint256());
                }
            }
        }

        // Resend wallet transactions that haven't gotten in a block yet
        // Except during reindex, importing and IBD, when old wallet
        // transactions become unconfirmed and spams other nodes.
        if (!fReindex && !fImporting && !IsInitialBlockDownload())
        {
            GetMainSignals().Broadcast(nTimeBestReceived);
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
            bool fRevertToInv = (!state.fPreferHeaders || pto->vBlockHashesToAnnounce.size() > MAX_BLOCKS_TO_ANNOUNCE);
            CBlockIndex *pBestIndex = NULL; // last header queued for delivery
            ProcessBlockAvailability(pto->id); // ensure pindexBestKnownBlock is up-to-date

            if (!fRevertToInv)
            {
                bool fFoundStartingHeader = false;
                // Try to find first header that our peer doesn't have, and
                // then send all headers past that one.  If we come across any
                // headers that aren't on chainActive, give up.
                BOOST_FOREACH (const uint256 &hash, pto->vBlockHashesToAnnounce)
                {
                    BlockMap::iterator mi = mapBlockIndex.find(hash);
                    // BU skip blocks that we don't know about.  was: assert(mi != mapBlockIndex.end());
                    if (mi == mapBlockIndex.end())
                        continue;
                    CBlockIndex *pindex = mi->second;
                    if (chainActive[pindex->nHeight] != pindex)
                    {
                        // Bail out if we reorged away from this block
                        fRevertToInv = true;
                        break;
                    }
                    if (pBestIndex != NULL && pindex->pprev != pBestIndex)
                    {
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
                    if (fFoundStartingHeader)
                    {
                        // add this to the headers message
                        vHeaders.push_back(pindex->GetBlockHeader());
                    }
                    else if (PeerHasHeader(&state, pindex))
                    {
                        continue; // keep looking for the first new block
                    }
                    else if (pindex->pprev == NULL || PeerHasHeader(&state, pindex->pprev))
                    {
                        // Peer doesn't have this header but they do have the prior one.
                        // Start sending headers.
                        fFoundStartingHeader = true;
                        vHeaders.push_back(pindex->GetBlockHeader());
                    }
                    else
                    {
                        // Peer doesn't have this header or the prior one -- nothing will
                        // connect, so bail out.
                        fRevertToInv = true;
                        break;
                    }
                }
            }
            if (fRevertToInv)
            {
                // If falling back to using an inv, just try to inv the tip.
                // The last entry in vBlockHashesToAnnounce was our tip at some point
                // in the past.
                if (!pto->vBlockHashesToAnnounce.empty())
                {
                    BOOST_FOREACH (const uint256 &hashToAnnounce, pto->vBlockHashesToAnnounce)
                    {
                        BlockMap::iterator mi = mapBlockIndex.find(hashToAnnounce);
                        if (mi != mapBlockIndex.end())
                        {
                            CBlockIndex *pindex = mi->second;

                            // Warn if we're announcing a block that is not on the main chain.
                            // This should be very rare and could be optimized out.
                            // Just log for now.
                            if (chainActive[pindex->nHeight] != pindex)
                            {
                                LogPrint("net", "Announcing block %s not on main chain (tip=%s)\n",
                                    hashToAnnounce.ToString(), chainActive.Tip()->GetBlockHash().ToString());
                            }

                            // If the peer announced this block to us, don't inv it back.
                            // (Since block announcements may not be via inv's, we can't solely rely on
                            // setInventoryKnown to track this.)
                            if (!PeerHasHeader(&state, pindex))
                            {
                                pto->PushInventory(CInv(MSG_BLOCK, hashToAnnounce));
                                LogPrint("net", "%s: sending inv peer=%d hash=%s\n", __func__, pto->id,
                                    hashToAnnounce.ToString());
                            }
                        }
                    }
                }
            }
            else if (!vHeaders.empty())
            {
                if (vHeaders.size() > 1)
                {
                    LogPrint("net", "%s: %u headers, range (%s, %s), to peer=%d\n", __func__, vHeaders.size(),
                        vHeaders.front().GetHash().ToString(), vHeaders.back().GetHash().ToString(), pto->id);
                }
                else
                {
                    LogPrint("net", "%s: sending header %s to peer=%d\n", __func__,
                        vHeaders.front().GetHash().ToString(), pto->id);
                }
                pto->PushMessage(NetMsgType::HEADERS, vHeaders);
                state.pindexBestHeaderSent = pBestIndex;
            }
            pto->vBlockHashesToAnnounce.clear();
        }

        //
        // Message: inventory
        //
        std::vector<CInv> vInv;
        std::vector<CInv> vInvWait;
        {
            bool fSendTrickle = pto->fWhitelisted;
            if (pto->nNextInvSend < nNow)
            {
                fSendTrickle = true;
                pto->nNextInvSend = PoissonNextSend(nNow, AVG_INVENTORY_BROADCAST_INTERVAL);
            }
            LOCK(pto->cs_inventory);
            vInv.reserve(std::min<size_t>(1000, pto->vInventoryToSend.size()));
            vInvWait.reserve(pto->vInventoryToSend.size());
            BOOST_FOREACH (const CInv &inv, pto->vInventoryToSend)
            {
                if (inv.type == MSG_TX && pto->filterInventoryKnown.contains(inv.hash))
                    continue;

                // BU - here we only want to forward message inventory if our peer has actually been requesting useful
                // data or
                //      giving us useful data.  We give them 2 minutes to be useful but then choke off their inventory.
                //      This
                //      prevents fake peers from connecting and listening to our inventory while providing no value to
                //      the network.
                //      However we will still send them block inventory in the case they are a pruned node or wallet
                //      waiting for block
                //      announcements, therefore we have to check each inv in pto->vInventoryToSend.
                if (inv.type == MSG_TX && pto->nActivityBytes == 0 && (GetTime() - pto->nTimeConnected) > 120)
                {
                    LogPrint("evict", "choking off tx inventory for %s time connected %d\n", pto->addr.ToString(),
                        GetTime() - pto->nTimeConnected);
                    continue;
                }

                // trickle out tx inv to protect privacy
                if (inv.type == MSG_TX && !fSendTrickle)
                {
                    // 1/4 of tx invs blast to all immediately
                    static uint256 hashSalt;
                    if (hashSalt.IsNull())
                        hashSalt = GetRandHash();
                    uint256 hashRand = ArithToUint256(UintToArith256(inv.hash) ^ UintToArith256(hashSalt));
                    hashRand = Hash(BEGIN(hashRand), END(hashRand));
                    bool fTrickleWait = ((UintToArith256(hashRand) & 3) != 0);

                    if (fTrickleWait)
                    {
                        vInvWait.push_back(inv);
                        continue;
                    }
                }

                pto->filterInventoryKnown.insert(inv.hash);

                vInv.push_back(inv);
                if (vInv.size() >= 1000)
                {
                    pto->PushMessage(NetMsgType::INV, vInv);
                    vInv.clear();
                }
            }
            pto->vInventoryToSend = vInvWait;
        }
        if (!vInv.empty())
            pto->PushMessage(NetMsgType::INV, vInv);

        // In case there is a block that has been in flight from this peer for 2 + 0.5 * N times the block interval
        // (with N the number of peers from which we're downloading validated blocks), disconnect due to timeout.
        // We compensate for other peers to prevent killing off peers due to our own downstream link
        // being saturated. We only count validated in-flight blocks so peers can't advertise non-existing block hashes
        // to unreasonably increase our timeout.
        if (!pto->fDisconnect && state.vBlocksInFlight.size() > 0)
        {
            QueuedBlock &queuedBlock = state.vBlocksInFlight.front();
            int nOtherPeersWithValidatedDownloads =
                nPeersWithValidatedDownloads - (state.nBlocksInFlightValidHeaders > 0);
            if (nNow > state.nDownloadingSince +
                           consensusParams.nPowTargetSpacing *
                               (BLOCK_DOWNLOAD_TIMEOUT_BASE +
                                   BLOCK_DOWNLOAD_TIMEOUT_PER_PEER * nOtherPeersWithValidatedDownloads))
            {
                LogPrintf(
                    "Timeout downloading block %s from peer=%d, disconnecting\n", queuedBlock.hash.ToString(), pto->id);
                pto->fDisconnect = true;
            }
        }

        //
        // Message: getdata (blocks)
        //
        std::vector<CInv> vGetData;
        if (!pto->fDisconnect && !pto->fClient && (fFetch || !IsInitialBlockDownload()) &&
            state.nBlocksInFlight < (int)MAX_BLOCKS_IN_TRANSIT_PER_PEER)
        {
            std::vector<CBlockIndex *> vToDownload;
            FindNextBlocksToDownload(pto->GetId(), MAX_BLOCKS_IN_TRANSIT_PER_PEER - state.nBlocksInFlight, vToDownload);
            BOOST_FOREACH (CBlockIndex *pindex, vToDownload)
            {
                CInv inv(MSG_BLOCK, pindex->GetBlockHash());
                if (!AlreadyHave(inv))
                {
                    requester.AskFor(inv, pto);
                    LogPrint("req", "AskFor block %s (%d) peer=%d\n", pindex->GetBlockHash().ToString(),
                        pindex->nHeight, pto->id);
                }
            }
        }

        //
        // Message: getdata (non-blocks)
        //
        while (!pto->fDisconnect && !pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
        {
            const CInv &inv = (*pto->mapAskFor.begin()).second;
            if (!AlreadyHave(inv))
            {
                if (fDebug)
                    LogPrint("net", "Requesting %s peer=%d\n", inv.ToString(), pto->id);
                vGetData.push_back(inv);
                if (vGetData.size() >= 1000)
                {
                    pto->PushMessage(NetMsgType::GETDATA, vGetData);
                    vGetData.clear();
                }
            }
            else
            {
                // If we're not going to ask, don't expect a response.
                pto->setAskFor.erase(inv.hash);
                requester.AlreadyReceived(inv); // BU indicate that we already got this item
            }
            pto->mapAskFor.erase(pto->mapAskFor.begin());
        }
        if (!vGetData.empty())
            pto->PushMessage(NetMsgType::GETDATA, vGetData);
    }
    return true;
}

std::string CBlockFileInfo::ToString() const
{
    return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks, nSize, nHeightFirst,
        nHeightLast, DateTimeStrFormat("%Y-%m-%d", nTimeFirst), DateTimeStrFormat("%Y-%m-%d", nTimeLast));
}

ThresholdState VersionBitsTipState(const Consensus::Params &params, Consensus::DeploymentPos pos)
{
    LOCK(cs_main);
    return VersionBitsState(chainActive.Tip(), params, pos, versionbitscache);
}

void MainCleanup()
{
    if (1)
    {
        LOCK(cs_main); // BU apply the appropriate lock so no contention during destruction
        // block headers
        BlockMap::iterator it1 = mapBlockIndex.begin();
        for (; it1 != mapBlockIndex.end(); it1++)
            delete (*it1).second;
        mapBlockIndex.clear();
    }

    if (1)
    {
        LOCK(cs_orphancache); // BU apply the appropriate lock so no contention during destruction
        // orphan transactions
        mapOrphanTransactions.clear();
        mapOrphanTransactionsByPrev.clear();
    }
}
