// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_MEMPOOL_ENTRY_H
#define BITCOIN_KERNEL_MEMPOOL_ENTRY_H

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/coin_age_priority.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <util/epochguard.h>
#include <util/overflow.h>

#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <set>
#include <stddef.h>
#include <stdint.h>

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

class CTxMemPoolEntry
{
public:
    typedef std::reference_wrapper<const CTxMemPoolEntry> CTxMemPoolEntryRef;
    // two aliases, should the types ever diverge
    typedef std::set<CTxMemPoolEntryRef, CompareIteratorByHash> Parents;
    typedef std::set<CTxMemPoolEntryRef, CompareIteratorByHash> Children;

private:
    CTxMemPoolEntry(const CTxMemPoolEntry&) = default;
    struct ExplicitCopyTag {
        explicit ExplicitCopyTag() = default;
    };

    const CTransactionRef tx;
    mutable Parents m_parents;
    mutable Children m_children;
    const CAmount nFee;             //!< Cached to avoid expensive parent-transaction lookups
    const int32_t nTxWeight;         //!< ... and avoid recomputing tx weight (also used for GetTxSize())
    const size_t nUsageSize;        //!< ... and total memory usage
    const int64_t nTime;            //!< Local time when entering the mempool
    const uint64_t entry_sequence;  //!< Sequence number used to determine whether this transaction is too recent for relay
    const int64_t sigOpCost;        //!< Total sigop cost
    const size_t nModSize;          //!< Cached modified size for priority
    const double entryPriority;     //!< Priority when entering the mempool
    const unsigned int entryHeight; //!< Chain height when entering the mempool
    double cachedPriority;          //!< Last calculated priority
    unsigned int cachedHeight;      //!< Height at which priority was last calculated
    CAmount inChainInputValue;      //!< Sum of all txin values that are already in blockchain
    const bool spendsCoinbase;      //!< keep track of transactions that spend a coinbase
    CAmount m_modified_fee;         //!< Used for determining the priority of the transaction for mining in a block
    mutable LockPoints lockPoints;  //!< Track the height and time at which tx was final

    // Information about descendants of this transaction that are in the
    // mempool; if we remove this transaction we must remove all of these
    // descendants as well.
    int64_t m_count_with_descendants{1}; //!< number of descendant transactions
    // Using int64_t instead of int32_t to avoid signed integer overflow issues.
    int64_t nSizeWithDescendants;      //!< ... and size
    CAmount nModFeesWithDescendants;   //!< ... and total fees (all including us)

    // Analogous statistics for ancestor transactions
    int64_t m_count_with_ancestors{1};
    // Using int64_t instead of int32_t to avoid signed integer overflow issues.
    int64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;

public:
    CTxMemPoolEntry(const CTransactionRef& tx, CAmount fee,
                    int64_t time, unsigned int entry_height, uint64_t entry_sequence,
                    double entry_tx_inputs_coin_age,
                    CAmount in_chain_input_value,
                    bool spends_coinbase,
                    int64_t sigops_cost, LockPoints lp)
        : tx{tx},
          nFee{fee},
          nTxWeight{GetTransactionWeight(*tx)},
          nUsageSize{RecursiveDynamicUsage(tx)},
          nTime{time},
          entry_sequence{entry_sequence},
          sigOpCost{sigops_cost},
          nModSize{CalculateModifiedSize(*tx, GetTxSize())},
          entryPriority{ComputePriority2(entry_tx_inputs_coin_age, nModSize)},
          entryHeight{entry_height},
          cachedPriority{entryPriority},
          // Since entries arrive *after* the tip's height, their entry priority is for the height+1
          cachedHeight{entry_height + 1},
          inChainInputValue{in_chain_input_value},
          spendsCoinbase{spends_coinbase},
          m_modified_fee{nFee},
          lockPoints{lp},
          nSizeWithDescendants{GetTxSize()},
          nModFeesWithDescendants{nFee},
          nSizeWithAncestors{GetTxSize()},
          nModFeesWithAncestors{nFee},
          nSigOpCostWithAncestors{sigOpCost} {
            CAmount nValueIn = tx->GetValueOut() + nFee;
            assert(inChainInputValue <= nValueIn);
        }

    CTxMemPoolEntry(ExplicitCopyTag, const CTxMemPoolEntry& entry) : CTxMemPoolEntry(entry) {}
    CTxMemPoolEntry& operator=(const CTxMemPoolEntry&) = delete;
    CTxMemPoolEntry(CTxMemPoolEntry&&) = delete;
    CTxMemPoolEntry& operator=(CTxMemPoolEntry&&) = delete;

    static constexpr ExplicitCopyTag ExplicitCopy{};

    const CTransaction& GetTx() const { return *this->tx; }
    CTransactionRef GetSharedTx() const { return this->tx; }
    double GetStartingPriority() const {return entryPriority; }
    std::pair<double, CAmount> GetInternalCoinAgePriorityCache2() const { return {ReversePriority2(cachedPriority, nModSize), inChainInputValue}; }
    /**
     * Fast calculation of priority as update from cached value, but only valid if
     * currentHeight is greater than last height it was recalculated.
     */
    double GetPriority(unsigned int currentHeight) const;
    /**
     * Recalculate the cached priority as of currentHeight and adjust inChainInputValue by
     * valueInCurrentBlock which represents input that was just added to or removed from the blockchain.
     */
    void UpdateCachedPriority(unsigned int currentHeight, CAmount valueInCurrentBlock);
    const CAmount& GetFee() const { return nFee; }
    int32_t GetTxSize() const
    {
        return GetVirtualTransactionSize(nTxWeight, sigOpCost, ::nBytesPerSigOp);
    }
    int32_t GetTxWeight() const { return nTxWeight; }
    std::chrono::seconds GetTime() const { return std::chrono::seconds{nTime}; }
    unsigned int GetHeight() const { return entryHeight; }
    uint64_t GetSequence() const { return entry_sequence; }
    int64_t GetSigOpCost() const { return sigOpCost; }
    CAmount GetModifiedFee() const { return m_modified_fee; }
    size_t DynamicMemoryUsage() const { return nUsageSize; }
    const LockPoints& GetLockPoints() const { return lockPoints; }

    // Adjusts the descendant state.
    void UpdateDescendantState(int32_t modifySize, CAmount modifyFee, int64_t modifyCount);
    // Adjusts the ancestor state
    void UpdateAncestorState(int32_t modifySize, CAmount modifyFee, int64_t modifyCount, int64_t modifySigOps);
    // Updates the modified fees with descendants/ancestors.
    void UpdateModifiedFee(CAmount fee_diff)
    {
        nModFeesWithDescendants = SaturatingAdd(nModFeesWithDescendants, fee_diff);
        nModFeesWithAncestors = SaturatingAdd(nModFeesWithAncestors, fee_diff);
        m_modified_fee = SaturatingAdd(m_modified_fee, fee_diff);
    }

    // Update the LockPoints after a reorg
    void UpdateLockPoints(const LockPoints& lp) const
    {
        lockPoints = lp;
    }

    uint64_t GetCountWithDescendants() const { return m_count_with_descendants; }
    int64_t GetSizeWithDescendants() const { return nSizeWithDescendants; }
    CAmount GetModFeesWithDescendants() const { return nModFeesWithDescendants; }

    bool GetSpendsCoinbase() const { return spendsCoinbase; }

    uint64_t GetCountWithAncestors() const { return m_count_with_ancestors; }
    int64_t GetSizeWithAncestors() const { return nSizeWithAncestors; }
    CAmount GetModFeesWithAncestors() const { return nModFeesWithAncestors; }
    int64_t GetSigOpCostWithAncestors() const { return nSigOpCostWithAncestors; }

    const Parents& GetMemPoolParentsConst() const { return m_parents; }
    const Children& GetMemPoolChildrenConst() const { return m_children; }
    Parents& GetMemPoolParents() const { return m_parents; }
    Children& GetMemPoolChildren() const { return m_children; }

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
    const ignore_rejects_type m_ignore_rejects;
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
                                       const ignore_rejects_type& ignore_rejects, const bool submitted_in_package,
                                       const bool chainstate_is_current,
                                       const bool has_no_mempool_parents)
        : info{tx, fee, vsize, height},
          m_ignore_rejects{ignore_rejects},
          m_submitted_in_package{submitted_in_package},
          m_chainstate_is_current{chainstate_is_current},
          m_has_no_mempool_parents{has_no_mempool_parents} {}
};

#endif // BITCOIN_KERNEL_MEMPOOL_ENTRY_H
