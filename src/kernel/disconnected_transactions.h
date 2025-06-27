// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_DISCONNECTED_TRANSACTIONS_H
#define BITCOIN_KERNEL_DISCONNECTED_TRANSACTIONS_H

#include <primitives/transaction.h>
#include <util/hasher.h>

#include <list>
#include <unordered_map>
#include <vector>

/** Maximum bytes for transactions to store for processing during reorg */
static const unsigned int MAX_DISCONNECTED_TX_POOL_BYTES{20'000'000};
/**
 * DisconnectedBlockTransactions

 * During the reorg, it's desirable to re-add previously confirmed transactions
 * to the mempool, so that anything not re-confirmed in the new chain is
 * available to be mined. However, it's more efficient to wait until the reorg
 * is complete and process all still-unconfirmed transactions at that time,
 * since we expect most confirmed transactions to (typically) still be
 * confirmed in the new chain, and re-accepting to the memory pool is expensive
 * (and therefore better to not do in the middle of reorg-processing).
 * Instead, store the disconnected transactions (in order!) as we go, remove any
 * that are included in blocks in the new chain, and then process the remaining
 * still-unconfirmed transactions at the end.
 *
 * Order of queuedTx:
 * The front of the list should be the most recently-confirmed transactions (transactions at the
 * end of vtx of blocks closer to the tip). If memory usage grows too large, we trim from the front
 * of the list. After trimming, transactions can be re-added to the mempool from the back of the
 * list to the front without running into missing inputs.
 */
class DisconnectedBlockTransactions {
private:
    /** Cached dynamic memory usage for the `CTransactionRef`s */
    uint64_t cachedInnerUsage = 0;
    const size_t m_max_mem_usage;
    std::list<CTransactionRef> queuedTx;
    using TxList = decltype(queuedTx);
    std::unordered_map<Txid, TxList::iterator, SaltedTxidHasher> iters_by_txid;

    /** Trim the earliest-added entries until we are within memory bounds. */
    std::vector<CTransactionRef> LimitMemoryUsage();

public:
    DisconnectedBlockTransactions(size_t max_mem_usage)
        : m_max_mem_usage{max_mem_usage} {}

    ~DisconnectedBlockTransactions();

    size_t DynamicMemoryUsage() const;

    /** Add transactions from the block, iterating through vtx in reverse order. Callers should call
     * this function for blocks in descending order by block height.
     * We assume that callers never pass multiple transactions with the same txid, otherwise things
     * can go very wrong in removeForBlock due to queuedTx containing an item without a
     * corresponding entry in iters_by_txid.
     * @returns vector of transactions that were evicted for size-limiting.
     */
    [[nodiscard]] std::vector<CTransactionRef> AddTransactionsFromBlock(const std::vector<CTransactionRef>& vtx);

    /** Remove any entries that are in this block. */
    void removeForBlock(const std::vector<CTransactionRef>& vtx);

    size_t size() const { return queuedTx.size(); }

    void clear();

    /** Clear all data structures and return the list of transactions. */
    std::list<CTransactionRef> take();
};
#endif // BITCOIN_KERNEL_DISCONNECTED_TRANSACTIONS_H
