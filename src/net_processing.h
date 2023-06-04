// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_PROCESSING_H
#define BITCOIN_NET_PROCESSING_H

#include <net.h>
#include <sync.h>
#include <validationinterface.h>

class CAddrMan;
class CTxMemPool;
class ChainstateManager;
struct LLMQContext;

extern RecursiveMutex cs_main;
extern RecursiveMutex g_cs_orphans;

/** Default for -maxorphantxsize, maximum size in megabytes the orphan map can grow before entries are removed */
static const unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS_SIZE = 10; // this allows around 100 TXs of max size (and many more of normal size)
/** Default number of orphan+recently-replaced txn to keep around for block reconstruction */
static const unsigned int DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN = 100;
static const bool DEFAULT_PEERBLOOMFILTERS = true;
static const bool DEFAULT_PEERBLOCKFILTERS = false;

struct CNodeStateStats {
    int m_misbehavior_score = 0;
    int nSyncHeight = -1;
    int nCommonHeight = -1;
    std::vector<int> vHeightInFlight;
};

class PeerManager : public CValidationInterface, public NetEventsInterface
{
public:
    static std::unique_ptr<PeerManager> make(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman,
                                             BanMan* banman, CScheduler &scheduler, ChainstateManager& chainman,
                                             CTxMemPool& pool, const std::unique_ptr<LLMQContext>& llmq_ctx, bool ignore_incoming_txs);
    virtual ~PeerManager() { }

    /** Get statistics from node state */
    virtual bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) = 0;

    /** Whether this node ignores txs received over p2p. */
    virtual bool IgnoresIncomingTxs() = 0;

    /** Set the best height */
    virtual void SetBestHeight(int height) = 0;

    /**
     * Increment peer's misbehavior score. If the new value surpasses banscore (specified on startup or by default), mark node to be discouraged, meaning the peer might be disconnected & added to the discouragement filter.
     */
    virtual void Misbehaving(const NodeId pnode, const int howmuch, const std::string& message = "") = 0;

    /**
     * Evict extra outbound peers. If we think our tip may be stale, connect to an extra outbound.
     * Public for unit testing.
     */
    virtual void CheckForStaleTipAndEvictPeers(const Consensus::Params &consensusParams) = 0;

    /** Process a single message from a peer. Public for fuzz testing */
    virtual void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                                int64_t nTimeReceived, const std::atomic<bool>& interruptMsgProc) = 0;

    virtual bool IsBanned(NodeId pnode) = 0;
};

void EraseObjectRequest(NodeId nodeId, const CInv& inv) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
void RequestObject(NodeId nodeId, const CInv& inv, std::chrono::microseconds current_time, bool fForce=false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
size_t GetRequestedObjectCount(NodeId nodeId) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Relay transaction to every node */
void RelayTransaction(const uint256&, const CConnman& connman);

#endif // BITCOIN_NET_PROCESSING_H
