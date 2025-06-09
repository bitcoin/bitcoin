// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXORPHANAGE_H
#define BITCOIN_NODE_TXORPHANAGE_H

#include <consensus/validation.h>
#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <util/time.h>

#include <map>
#include <set>

namespace node {
/** Expiration time for orphan transactions */
static constexpr auto ORPHAN_TX_EXPIRE_TIME{20min};
/** Minimum time between orphan transactions expire time checks */
static constexpr auto ORPHAN_TX_EXPIRE_INTERVAL{5min};
/** Default maximum number of orphan transactions kept in memory */
static const uint32_t DEFAULT_MAX_ORPHAN_TRANSACTIONS{100};

/** A class to track orphan transactions (failed on TX_MISSING_INPUTS)
 * Since we cannot distinguish orphans from bad transactions with
 * non-existent inputs, we heavily limit the number of orphans
 * we keep and the duration we keep them for.
 * Not thread-safe. Requires external synchronization.
 */
class TxOrphanage {
public:
    /** Allows providing orphan information externally */
    struct OrphanTxBase {
        CTransactionRef tx;
        /** Peers added with AddTx or AddAnnouncer. */
        std::set<NodeId> announcers;

        /** Get the weight of this transaction, an approximation of its memory usage. */
        unsigned int GetUsage() const {
            return GetTransactionWeight(*tx);
        }
    };

    virtual ~TxOrphanage() = default;

    /** Add a new orphan transaction */
    virtual bool AddTx(const CTransactionRef& tx, NodeId peer) = 0;

    /** Add an additional announcer to an orphan if it exists. Otherwise, do nothing. */
    virtual bool AddAnnouncer(const Wtxid& wtxid, NodeId peer) = 0;

    /** Get a transaction by its witness txid */
    virtual CTransactionRef GetTx(const Wtxid& wtxid) const = 0;

    /** Check if we already have an orphan transaction (by wtxid only) */
    virtual bool HaveTx(const Wtxid& wtxid) const = 0;

    /** Check if a {tx, peer} exists in the orphanage.*/
    virtual bool HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const = 0;

    /** Extract a transaction from a peer's work set
     *  Returns nullptr if there are no transactions to work on.
     *  Otherwise returns the transaction reference, and removes
     *  it from the work set.
     */
    virtual CTransactionRef GetTxToReconsider(NodeId peer) = 0;

    /** Erase an orphan by wtxid, including all announcements if there are multiple.
     * Returns true if an orphan was erased, false if no tx with this wtxid exists. */
    virtual bool EraseTx(const Wtxid& wtxid) = 0;

    /** Maybe erase all orphans announced by a peer (eg, after that peer disconnects). If an orphan
     * has been announced by another peer, don't erase, just remove this peer from the list of announcers. */
    virtual void EraseForPeer(NodeId peer) = 0;

    /** Erase all orphans included in or invalidated by a new block */
    virtual void EraseForBlock(const CBlock& block) = 0;

    /** Limit the orphanage to DEFAULT_MAX_ORPHAN_TRANSACTIONS. */
    virtual void LimitOrphans(FastRandomContext& rng) = 0;

    /** Add any orphans that list a particular tx as a parent into the from peer's work set */
    virtual void AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng) = 0;

    /** Does this peer have any work to do? */
    virtual bool HaveTxToReconsider(NodeId peer) = 0;

    /** Get all children that spend from this tx and were received from nodeid. Sorted from most
     * recent to least recent. */
    virtual std::vector<CTransactionRef> GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId nodeid) const = 0;

    /** Return how many entries exist in the orphange */
    virtual size_t Size() const = 0;

    /** Get all orphan transactions */
    virtual std::vector<OrphanTxBase> GetOrphanTransactions() const = 0;

    /** Get the total usage (weight) of all orphans. If an orphan has multiple announcers, its usage is
     * only counted once within this total. */
    virtual int64_t TotalOrphanUsage() const = 0;

    /** Total usage (weight) of orphans for which this peer is an announcer. If an orphan has multiple
     * announcers, its weight will be accounted for in each PeerOrphanInfo, so the total of all
     * peers' UsageByPeer() may be larger than TotalOrphanBytes(). */
    virtual int64_t UsageByPeer(NodeId peer) const = 0;

    /** Check consistency between PeerOrphanInfo and m_orphans. Recalculate counters and ensure they
     * match what is cached. */
    virtual void SanityCheck() const = 0;
};

/** Create a new TxOrphanage instance */
std::unique_ptr<TxOrphanage> MakeTxOrphanage() noexcept;

} // namespace node
#endif // BITCOIN_NODE_TXORPHANAGE_H
