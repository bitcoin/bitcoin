// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_DISCONNECTED_TRANSACTIONS_H
#define BITCOIN_KERNEL_DISCONNECTED_TRANSACTIONS_H

#include <core_memusage.h>
#include <memusage.h>
#include <primitives/transaction.h>
#include <util/hasher.h>

#include <list>
#include <unordered_map>
#include <vector>

/** Maximum kilobytes for transactions to store for processing during reorg */
static const unsigned int MAX_DISCONNECTED_TX_POOL_SIZE = 20'000;
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
    /** Cached dynamic memory usage for the CTransactions (memory for the shared pointers is
     * included in the container calculations). */
    uint64_t cachedInnerUsage = 0;
    const size_t m_max_mem_usage;
    std::list<CTransactionRef> queuedTx;
    using TxList = decltype(queuedTx);
    std::unordered_map<uint256, TxList::iterator, SaltedTxidHasher> iters_by_txid;

    /** Trim the earliest-added entries until we are within memory bounds. */
    std::vector<CTransactionRef> LimitMemoryUsage()
    {
        std::vector<CTransactionRef> evicted;

        while (!queuedTx.empty() && DynamicMemoryUsage() > m_max_mem_usage) {
            evicted.emplace_back(queuedTx.front());
            cachedInnerUsage -= RecursiveDynamicUsage(*queuedTx.front());
            iters_by_txid.erase(queuedTx.front()->GetHash());
            queuedTx.pop_front();
        }
        return evicted;
    }

public:
    DisconnectedBlockTransactions(size_t max_mem_usage) : m_max_mem_usage{max_mem_usage} {}

    // It's almost certainly a logic bug if we don't clear out queuedTx before
    // destruction, as we add to it while disconnecting blocks, and then we
    // need to re-process remaining transactions to ensure mempool consistency.
    // For now, assert() that we've emptied out this object on destruction.
    // This assert() can always be removed if the reorg-processing code were
    // to be refactored such that this assumption is no longer true (for
    // instance if there was some other way we cleaned up the mempool after a
    // reorg, besides draining this object).
    ~DisconnectedBlockTransactions() {
        assert(queuedTx.empty());
        assert(iters_by_txid.empty());
        assert(cachedInnerUsage == 0);
    }

    size_t DynamicMemoryUsage() const {
        return cachedInnerUsage + memusage::DynamicUsage(iters_by_txid) + memusage::DynamicUsage(queuedTx);
    }

    /** Add transactions from the block, iterating through vtx in reverse order. Callers should call
     * this function for blocks in descending order by block height.
     * We assume that callers never pass multiple transactions with the same txid, otherwise things
     * can go very wrong in removeForBlock due to queuedTx containing an item without a
     * corresponding entry in iters_by_txid.
     * @returns vector of transactions that were evicted for size-limiting.
     */
    [[nodiscard]] std::vector<CTransactionRef> AddTransactionsFromBlock(const std::vector<CTransactionRef>& vtx)
    {
        iters_by_txid.reserve(iters_by_txid.size() + vtx.size());
        for (auto block_it = vtx.rbegin(); block_it != vtx.rend(); ++block_it) {
            auto it = queuedTx.insert(queuedTx.end(), *block_it);
            iters_by_txid.emplace((*block_it)->GetHash(), it);
            cachedInnerUsage += RecursiveDynamicUsage(**block_it);
        }
        return LimitMemoryUsage();
    }

    /** Remove any entries that are in this block. */
    void removeForBlock(const std::vector<CTransactionRef>& vtx)
    {
        // Short-circuit in the common case of a block being added to the tip
        if (queuedTx.empty()) {
            return;
        }
        for (const auto& tx : vtx) {
            auto iter = iters_by_txid.find(tx->GetHash());
            if (iter != iters_by_txid.end()) {
                auto list_iter = iter->second;
                iters_by_txid.erase(iter);
                cachedInnerUsage -= RecursiveDynamicUsage(**list_iter);
                queuedTx.erase(list_iter);
            }
        }
    }

    size_t size() const { return queuedTx.size(); }

    void clear()
    {
        cachedInnerUsage = 0;
        iters_by_txid.clear();
        queuedTx.clear();
    }

    /** Clear all data structures and return the list of transactions. */
    std::list<CTransactionRef> take()
    {
        std::list<CTransactionRef> ret = std::move(queuedTx);
        clear();
        return ret;
    }
};
#endif // BITCOIN_KERNEL_DISCONNECTED_TRANSACTIONS_H
