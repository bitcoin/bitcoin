// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXDOWNLOADMAN_H
#define BITCOIN_NODE_TXDOWNLOADMAN_H

#include <net.h>

#include <cstdint>
#include <memory>

class CBlock;
class CBlockIndex;
class CRollingBloomFilter;
class CTxMemPool;
class GenTxid;
class TxOrphanage;
class TxRequestTracker;
enum class ChainstateRole;
namespace node {
class TxDownloadManagerImpl;

struct TxDownloadOptions {
    /** Read-only reference to mempool. */
    const CTxMemPool& m_mempool;
};
struct TxDownloadConnectionInfo {
    /** Whether this peer is preferred for transaction download. */
    const bool m_preferred;
    /** Whether this peer has Relay permissions. */
    const bool m_relay_permissions;
    /** Whether this peer supports wtxid relay. */
    const bool m_wtxid_relay;
};

class TxDownloadManager {
    const std::unique_ptr<TxDownloadManagerImpl> m_impl;

public:
    explicit TxDownloadManager(const TxDownloadOptions& options);
    ~TxDownloadManager();

    // Get references to internal data structures. Outside access to these data structures should be
    // temporary and removed later once logic has been moved internally.
    TxOrphanage& GetOrphanageRef();
    TxRequestTracker& GetTxRequestRef();
    CRollingBloomFilter& RecentRejectsFilter();
    CRollingBloomFilter& RecentRejectsReconsiderableFilter();

    // Responses to chain events. TxDownloadManager is not an actual client of ValidationInterface, these are called through PeerManager.
    void ActiveTipChange();
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock);
    void BlockDisconnected();

    /** Check whether we already have this gtxid in:
     *  - mempool
     *  - orphanage
     *  - m_recent_rejects
     *  - m_recent_rejects_reconsiderable (if include_reconsiderable = true)
     *  - m_recent_confirmed_transactions
     *  */
    bool AlreadyHaveTx(const GenTxid& gtxid, bool include_reconsiderable);

    /** Creates a new PeerInfo. Saves the connection info to calculate tx announcement delays later. */
    void ConnectedPeer(NodeId nodeid, const TxDownloadConnectionInfo& info);

    /** Deletes all txrequest announcements and orphans for a given peer. */
    void DisconnectedPeer(NodeId nodeid);
};
} // namespace node
#endif // BITCOIN_NODE_TXDOWNLOADMAN_H
