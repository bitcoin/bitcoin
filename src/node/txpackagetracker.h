// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXPACKAGETRACKER_H
#define BITCOIN_NODE_TXPACKAGETRACKER_H

#include <net.h>

#include <cstdint>
#include <map>
#include <vector>

class CBlock;
class TxOrphanage;
namespace node {
static constexpr bool DEFAULT_ENABLE_PACKAGE_RELAY{false};
static constexpr size_t MAX_PKGTXNS_COUNT{100};
enum PackageRelayVersions : uint64_t {
    PKG_RELAY_NONE = 0,
    // BIP331 Ancestor Package Information
    PKG_RELAY_ANCPKG = (1 << 0),
};

class TxPackageTracker {
    class Impl;
    const std::unique_ptr<Impl> m_impl;

public:
    explicit TxPackageTracker();
    ~TxPackageTracker();

    // Orphanage wrapper functions
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

    PackageRelayVersions GetSupportedVersions() const;

    // We expect this to be called only once
    void ReceivedVersion(NodeId nodeid);
    void ReceivedSendpackages(NodeId nodeid, PackageRelayVersions versions);
    // Finalize the registration state.
    bool ReceivedVerack(NodeId nodeid, bool inbound, bool txrelay, bool wtxidrelay);

    /** Received an announcement from this peer for a tx we already know is an orphan; should be
     * called for every peer that announces the tx, even if they are not a package relay peer.
     * The orphan request tracker will decide when to request what from which peer - use
     * GetOrphanRequests().
     */
    void AddOrphanTx(NodeId nodeid, const uint256& wtxid, const CTransactionRef& tx, bool is_preferred, std::chrono::microseconds reqtime);

    /** Number of packages we are working on with this peer. Includes any entries in the orphan
     * tracker, in-flight orphan parent requests (1 per orphan regardless of how many missing
     * parents were requested), package info requests, tx data download, and packages in the
     * validation queue. */
    size_t Count(NodeId nodeid) const;

    /** Number of packages we are currently working on with this peer (i.e. reserving memory for
     * storing orphan(s)). Includes in-flight package info requests, tx data download, and packages
     * in the validation queue. Excludes entries in the orphan tracker that are just candidates. */
    size_t CountInFlight(NodeId nodeid) const;

    /** Get list of requests that should be sent to resolve orphans. These may be wtxids to send
     * getdata(ANCPKGINFO) or txids corresponding to parents. Automatically marks the orphans as
     * having outgoing requests. */
    std::vector<GenTxid> GetOrphanRequests(NodeId nodeid, std::chrono::microseconds current_time);

    /** Update transactions for which we have made "final" decisions: transactions that have
     * confirmed in a block, conflicted due to a block, or added to the mempool already.
     * Should be called on new block: valid=block transactions, invalid=conflicts.
     * Should be called when tx is added to mempool.
     * Should not be called when a tx fails validation.
     * */
    void FinalizeTransactions(const std::set<uint256>& valid, const std::set<uint256>& invalid);
};
} // namespace node
#endif // BITCOIN_NODE_TXPACKAGETRACKER_H
