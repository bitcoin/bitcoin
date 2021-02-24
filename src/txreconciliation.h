// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXRECONCILIATION_H
#define BITCOIN_TXRECONCILIATION_H

#include <net.h>
#include <sync.h>

#include <tuple>
#include <unordered_map>

/**
 * Used to track reconciliations across all peers.
 */
class TxReconciliationTracker {
    /**
     * Salt used to compute short IDs during transaction reconciliation.
     * Salt is generated randomly per-connection to prevent linking of
     * connections belonging to the same physical node.
     * Also, salts should be different per-connection to prevent halting
     * of relay of particular transactions due to collisions in short IDs.
     */
    Mutex m_local_salts_mutex;
    std::unordered_map<NodeId, uint64_t> m_local_salts GUARDED_BY(m_local_salts_mutex);

    public:

    TxReconciliationTracker() {};

    /**
     * TODO: document
     */
    std::tuple<bool, bool, uint32_t, uint64_t> SuggestReconciling(const NodeId peer_id, bool inbound);
};

#endif // BITCOIN_TXRECONCILIATION_H
