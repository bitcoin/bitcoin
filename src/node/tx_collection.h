// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_NODE_TX_COLLECTION_H
#define BITCOIN_NODE_TX_COLLECTION_H

#include <primitives/transaction.h>
#include <sync.h>
#include <util/hasher.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ChainstateManager;
class CTxMemPool;
class uint256;

namespace node {
struct CBlockTemplate;

/** Collects transactions in client-specified order for block template creation. */
class TxCollection
{
public:
    TxCollection(std::vector<Wtxid> wtxids, ChainstateManager& chainman, CTxMemPool& mempool);
    /** Return zero-based positions for requested transactions that are still missing. */
    std::vector<uint32_t> UnknownTxPos() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Add transactions matching previously requested wtxids. Throws on null,
     *  unexpected, or duplicate transactions within the batch, in which case
     *  nothing is added. Transactions from earlier calls may be submitted again. */
    void AddMissingTxs(const std::vector<CTransactionRef>& txs) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /**
     * Assemble and validate a block template from the collected transactions.
     * If @p coinbase is provided the block is validated with it, otherwise a
     * node-generated dummy coinbase is used.
     */
    std::unique_ptr<CBlockTemplate> MakeTemplate(const uint256& prevhash,
                                                 const CTransactionRef& coinbase,
                                                 std::string& reason,
                                                 std::string& debug) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    /** Requested transaction order as provided by the client. */
    const std::vector<Wtxid> m_wtxids;
    /** Protects m_transactions: IPC clients may call methods concurrently
     *  from different threads. */
    mutable Mutex m_mutex;
    /** Collected transactions keyed by wtxid. */
    std::unordered_map<Wtxid, CTransactionRef, SaltedWtxidHasher> m_transactions GUARDED_BY(m_mutex);
    ChainstateManager& m_chainman;
    CTxMemPool& m_mempool;
};
} // namespace node

#endif // BITCOIN_NODE_TX_COLLECTION_H
