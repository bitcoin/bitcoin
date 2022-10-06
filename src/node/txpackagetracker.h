// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXPACKAGETRACKER_H
#define BITCOIN_NODE_TXPACKAGETRACKER_H

#include <net.h>

#include <cstdint>
#include <map>
#include <vector>

class TxOrphanage;
namespace node {
static constexpr bool DEFAULT_ENABLE_PACKAGE_RELAY{false};

class TxPackageTracker {
    class Impl;
    const std::unique_ptr<Impl> m_impl;

public:
    explicit TxPackageTracker();
    ~TxPackageTracker();

    // Orphanage wrapper functions
    /** Add new tx to orphanage if it isn't already there. Returns whether the tx was added. */
    bool OrphanageAddTx(const CTransactionRef& tx, NodeId peer);

    /** Check if we already have an orphan transaction (by txid or wtxid) */
    bool OrphanageHaveTx(const GenTxid& gtxid);

    /** Extract a transaction from a peer's work set
     *  Returns nullptr if there are no transactions to work on.
     *  Otherwise returns the transaction reference, and removes
     *  it from the work set.
     */
    CTransactionRef GetTxToReconsider(NodeId peer);

    /** Erase an orphan by txid */
    int EraseOrphanTx(const uint256& txid);

    /** Erase all orphans announced by a peer (eg, after that peer disconnects) */
    void DisconnectedPeer(NodeId peer);

    /** Erase all orphans included in or invalidated by a new block */
    void BlockConnected(const CBlock& block);

    /** Limit the orphanage to the given maximum */
    void LimitOrphans(unsigned int max_orphans);

    /** Add any orphans that list a particular tx as a parent into the from peer's work set */
    void AddChildrenToWorkSet(const CTransaction& tx);

    /** Does this peer have any orphans to validate? */
    bool HaveTxToReconsider(NodeId peer);

    /** Return how many entries exist in the orphange */
    size_t OrphanageSize();

};
} // namespace node
#endif // BITCOIN_NODE_TXPACKAGETRACKER_H
