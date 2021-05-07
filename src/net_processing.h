// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NET_PROCESSING_H
#define SYSCOIN_NET_PROCESSING_H

#include <net.h>
#include <validationinterface.h>

class CAddrMan;
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
};

class PeerManager : public CValidationInterface, public NetEventsInterface
{
public:
    static std::unique_ptr<PeerManager> make(const CChainParams& chainparams, CConnman& connman, CAddrMan& addrman,
                                             BanMan* banman, CScheduler& scheduler, ChainstateManager& chainman,
                                             CTxMemPool& pool, bool ignore_incoming_txs);
    virtual ~PeerManager() { }

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
    virtual size_t GetRequestedCount(NodeId nodeId) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main) = 0;
    virtual void ReceivedResponse(NodeId nodeId, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) = 0;
    virtual void ForgetTxHash(NodeId nodeId, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) = 0;
    /**
     * Increment peer's misbehavior score. If the new value >= DISCOURAGEMENT_THRESHOLD, mark the node
     * to be discouraged, meaning the peer might be disconnected and added to the discouragement filter.
     * Public for unit testing.
     */
    virtual void Misbehaving(const NodeId pnode, const int howmuch, const std::string& message) = 0;

    /**
     * Evict extra outbound peers. If we think our tip may be stale, connect to an extra outbound.
     * Public for unit testing.
     */
    virtual void CheckForStaleTipAndEvictPeers() = 0;

    /** Process a single message from a peer. Public for fuzz testing */
    virtual void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                                const std::chrono::microseconds time_received, const std::atomic<bool>& interruptMsgProc) = 0;
};

// SYSCOIN
// Upstream moved this into net_processing.cpp (13417), however since we use Misbehaving in a number of syscoin specific
// files such as mnauth.cpp and governance.cpp it makes sense to keep it in the header
/** Increase a node's misbehavior score. */
bool IsBanned(NodeId nodeid, BanMan& banman);
unsigned int GetMaxInv();

#endif // SYSCOIN_NET_PROCESSING_H
