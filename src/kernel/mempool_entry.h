// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_MEMPOOL_ENTRY_H
#define BITCOIN_KERNEL_MEMPOOL_ENTRY_H

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <txgraph.h>
#include <util/epochguard.h>
#include <util/overflow.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>

class CBlockIndex;

struct LockPoints {
    // Will be set to the blockchain height and median time past
    // values that would be necessary to satisfy all relative locktime
    // constraints (BIP68) of this tx given our view of block chain history
    int height{0};
    int64_t time{0};
    // As long as the current chain descends from the highest height block
    // containing one of the inputs used in the calculation, then the cached
    // values are still valid even after a reorg.
    CBlockIndex* maxInputBlock{nullptr};
};

struct CompareIteratorByHash {
    // SFINAE for T where T is either a pointer type (e.g., a txiter) or a reference_wrapper<T>
    // (e.g. a wrapped CTxMemPoolEntry&)
    template <typename T>
    bool operator()(const std::reference_wrapper<T>& a, const std::reference_wrapper<T>& b) const
    {
        return a.get().GetTx().GetHash() < b.get().GetTx().GetHash();
    }
    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
        return a->GetTx().GetHash() < b->GetTx().GetHash();
    }
};

/** \class CTxMemPoolEntry
 *
 * CTxMemPoolEntry stores data about the corresponding transaction, as well
 * as data about all in-mempool transactions that depend on the transaction
 * ("descendant" transactions).
 *
 * When a new entry is added to the mempool, we update the descendant state
 * (m_count_with_descendants, nSizeWithDescendants, and nModFeesWithDescendants) for
 * all ancestors of the newly added transaction.
 *
 */

class CTxMemPoolEntry : public TxGraph::Ref
{
public:
    typedef std::reference_wrapper<const CTxMemPoolEntry> CTxMemPoolEntryRef;

private:
    CTxMemPoolEntry(const CTxMemPoolEntry&) = delete;

    const CTransactionRef tx;
    const CAmount nFee;             //!< Cached to avoid expensive parent-transaction lookups
    const int32_t nTxWeight;         //!< ... and avoid recomputing tx weight (also used for GetTxSize())
    const size_t nUsageSize;        //!< ... and total memory usage
    const int64_t nTime;            //!< Local time when entering the mempool
    const uint64_t entry_sequence;  //!< Sequence number used to determine whether this transaction is too recent for relay
    const unsigned int entryHeight; //!< Chain height when entering the mempool
    const bool spendsCoinbase;      //!< keep track of transactions that spend a coinbase
    const int64_t sigOpCost;        //!< Total sigop cost
    CAmount m_modified_fee;         //!< Used for determining the priority of the transaction for mining in a block
    mutable LockPoints lockPoints;  //!< Track the height and time at which tx was final

public:
    virtual ~CTxMemPoolEntry() = default;
    CTxMemPoolEntry(TxGraph::Ref&& ref, const CTransactionRef& tx, CAmount fee,
                    int64_t time, unsigned int entry_height, uint64_t entry_sequence,
                    bool spends_coinbase,
                    int64_t sigops_cost, LockPoints lp)
        : TxGraph::Ref(std::move(ref)),
          tx{tx},
          nFee{fee},
          nTxWeight{GetTransactionWeight(*tx)},
          nUsageSize{RecursiveDynamicUsage(tx)},
          nTime{time},
          entry_sequence{entry_sequence},
          entryHeight{entry_height},
          spendsCoinbase{spends_coinbase},
          sigOpCost{sigops_cost},
          m_modified_fee{nFee},
          lockPoints{lp} {}

    CTxMemPoolEntry& operator=(const CTxMemPoolEntry&) = delete;
    CTxMemPoolEntry(CTxMemPoolEntry&&) = default;
    CTxMemPoolEntry& operator=(CTxMemPoolEntry&&) = delete;

    const CTransaction& GetTx() const { return *this->tx; }
    CTransactionRef GetSharedTx() const { return this->tx; }
    const CAmount& GetFee() const { return nFee; }
    int32_t GetTxSize() const
    {
        return GetVirtualTransactionSize(nTxWeight, sigOpCost, ::nBytesPerSigOp);
    }
    int32_t GetAdjustedWeight() const { return GetSigOpsAdjustedWeight(nTxWeight, sigOpCost, ::nBytesPerSigOp); }
    int32_t GetTxWeight() const { return nTxWeight; }
    std::chrono::seconds GetTime() const { return std::chrono::seconds{nTime}; }
    unsigned int GetHeight() const { return entryHeight; }
    uint64_t GetSequence() const { return entry_sequence; }
    int64_t GetSigOpCost() const { return sigOpCost; }
    CAmount GetModifiedFee() const { return m_modified_fee; }
    size_t DynamicMemoryUsage() const { return nUsageSize; }
    const LockPoints& GetLockPoints() const { return lockPoints; }

    // Updates the modified fees with descendants/ancestors.
    void UpdateModifiedFee(CAmount fee_diff)
    {
        m_modified_fee = SaturatingAdd(m_modified_fee, fee_diff);
    }

    // Update the LockPoints after a reorg
    void UpdateLockPoints(const LockPoints& lp) const
    {
        lockPoints = lp;
    }

    bool GetSpendsCoinbase() const { return spendsCoinbase; }

    mutable size_t idx_randomized; //!< Index in mempool's txns_randomized
    mutable Epoch::Marker m_epoch_marker; //!< epoch when last touched, useful for graph algorithms
};

using CTxMemPoolEntryRef = CTxMemPoolEntry::CTxMemPoolEntryRef;

struct TransactionInfo {
    const CTransactionRef m_tx;
    /* The fee the transaction paid */
    const CAmount m_fee;
    /**
     * The virtual transaction size.
     *
     * This is a policy field which considers the sigop cost of the
     * transaction as well as its weight, and reinterprets it as bytes.
     *
     * It is the primary metric by which the mining algorithm selects
     * transactions.
     */
    const int64_t m_virtual_transaction_size;
    /* The block height the transaction entered the mempool */
    const unsigned int txHeight;

    TransactionInfo(const CTransactionRef& tx, const CAmount& fee, const int64_t vsize, const unsigned int height)
        : m_tx{tx},
          m_fee{fee},
          m_virtual_transaction_size{vsize},
          txHeight{height} {}
};

struct RemovedMempoolTransactionInfo {
    TransactionInfo info;
    explicit RemovedMempoolTransactionInfo(const CTxMemPoolEntry& entry)
        : info{entry.GetSharedTx(), entry.GetFee(), entry.GetTxSize(), entry.GetHeight()} {}
};

struct NewMempoolTransactionInfo {
    TransactionInfo info;
    /*
     * This boolean indicates whether the transaction was added
     * without enforcing mempool fee limits.
     */
    const bool m_mempool_limit_bypassed;
    /* This boolean indicates whether the transaction is part of a package. */
    const bool m_submitted_in_package;
    /*
     * This boolean indicates whether the blockchain is up to date when the
     * transaction is added to the mempool.
     */
    const bool m_chainstate_is_current;
    /* Indicates whether the transaction has unconfirmed parents. */
    const bool m_has_no_mempool_parents;

    explicit NewMempoolTransactionInfo(const CTransactionRef& tx, const CAmount& fee,
                                       const int64_t vsize, const unsigned int height,
                                       const bool mempool_limit_bypassed, const bool submitted_in_package,
                                       const bool chainstate_is_current,
                                       const bool has_no_mempool_parents)
        : info{tx, fee, vsize, height},
          m_mempool_limit_bypassed{mempool_limit_bypassed},
          m_submitted_in_package{submitted_in_package},
          m_chainstate_is_current{chainstate_is_current},
          m_has_no_mempool_parents{has_no_mempool_parents} {}
};

#endif // BITCOIN_KERNEL_MEMPOOL_ENTRY_H
