// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_NODE_TX_COLLECTION_H
#define BITCOIN_NODE_TX_COLLECTION_H

#include <primitives/transaction.h>
#include <util/hasher.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

class CTxMemPool;

namespace node {
/** Collects transactions in client-specified order for block template creation. */
class TxCollection
{
public:
    TxCollection(std::vector<Wtxid> wtxids, CTxMemPool& mempool);
    /** Return zero-based positions for requested transactions that are still missing. */
    std::vector<uint32_t> UnknownTxPos() const;

private:
    /** Requested transaction order as provided by the client. */
    const std::vector<Wtxid> m_wtxids;
    /** Collected transactions keyed by wtxid. */
    std::unordered_map<Wtxid, CTransactionRef, SaltedWtxidHasher> m_transactions;
    CTxMemPool& m_mempool;
};
} // namespace node

#endif // BITCOIN_NODE_TX_COLLECTION_H
