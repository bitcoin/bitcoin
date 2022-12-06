// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_KERNEL_MEMPOOL_ENTRY_H
#define SYSCOIN_KERNEL_MEMPOOL_ENTRY_H

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <util/epochguard.h>
#include <util/overflow.h>

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
 * (nCountWithDescendants, nSizeWithDescendants, and nModFeesWithDescendants) for
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
    const CTransactionRef tx;
    mutable Parents m_parents;
    mutable Children m_children;
    const CAmount nFee;             //!< Cached to avoid expensive parent-transaction lookups
    const size_t nTxWeight;         //!< ... and avoid recomputing tx weight (also used for GetTxSize())
    const size_t nUsageSize;        //!< ... and total memory usage
    const int64_t nTime;            //!< Local time when entering the mempool
    const unsigned int entryHeight; //!< Chain height when entering the mempool
    const bool spendsCoinbase;      //!< keep track of transactions that spend a coinbase
    const int64_t sigOpCost;        //!< Total sigop cost
    CAmount m_modified_fee;         //!< Used for determining the priority of the transaction for mining in a block
    LockPoints lockPoints;          //!< Track the height and time at which tx was final

    // Information about descendants of this transaction that are in the
    // mempool; if we remove this transaction we must remove all of these
    // descendants as well.
    uint64_t nCountWithDescendants{1}; //!< number of descendant transactions
    uint64_t nSizeWithDescendants;     //!< ... and size
    CAmount nModFeesWithDescendants;   //!< ... and total fees (all including us)

    // Analogous statistics for ancestor transactions
    uint64_t nCountWithAncestors{1};
    uint64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;

public:
    CTxMemPoolEntry(const CTransactionRef& tx, CAmount fee,
                    int64_t time, unsigned int entry_height,
                    bool spends_coinbase,
                    int64_t sigops_cost, LockPoints lp)
        : tx{tx},
          nFee{fee},
          nTxWeight(GetTransactionWeight(*tx)),
          nUsageSize{RecursiveDynamicUsage(tx)},
          nTime{time},
          entryHeight{entry_height},
          spendsCoinbase{spends_coinbase},
          sigOpCost{sigops_cost},
          m_modified_fee{nFee},
          lockPoints{lp},
          nSizeWithDescendants{GetTxSize()},
          nModFeesWithDescendants{nFee},
          nSizeWithAncestors{GetTxSize()},
          nModFeesWithAncestors{nFee},
          nSigOpCostWithAncestors{sigOpCost} {}

    const CTransaction& GetTx() const { return *this->tx; }
    CTransactionRef GetSharedTx() const { return this->tx; }
    const CAmount& GetFee() const { return nFee; }
    size_t GetTxSize() const
    {
        return GetVirtualTransactionSize(nTxWeight, sigOpCost, ::nBytesPerSigOp);
    }
    size_t GetTxWeight() const { return nTxWeight; }
    std::chrono::seconds GetTime() const { return std::chrono::seconds{nTime}; }
    unsigned int GetHeight() const { return entryHeight; }
    int64_t GetSigOpCost() const { return sigOpCost; }
    CAmount GetModifiedFee() const { return m_modified_fee; }
    size_t DynamicMemoryUsage() const { return nUsageSize; }
    const LockPoints& GetLockPoints() const { return lockPoints; }

    // Adjusts the descendant state.
    void UpdateDescendantState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount);
    // Adjusts the ancestor state
    void UpdateAncestorState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount, int64_t modifySigOps);
    // Updates the modified fees with descendants/ancestors.
    void UpdateModifiedFee(CAmount fee_diff)
    {
        nModFeesWithDescendants = SaturatingAdd(nModFeesWithDescendants, fee_diff);
        nModFeesWithAncestors = SaturatingAdd(nModFeesWithAncestors, fee_diff);
        m_modified_fee = SaturatingAdd(m_modified_fee, fee_diff);
    }

    // Update the LockPoints after a reorg
    void UpdateLockPoints(const LockPoints& lp)
    {
        lockPoints = lp;
    }

    uint64_t GetCountWithDescendants() const { return nCountWithDescendants; }
    uint64_t GetSizeWithDescendants() const { return nSizeWithDescendants; }
    CAmount GetModFeesWithDescendants() const { return nModFeesWithDescendants; }

    bool GetSpendsCoinbase() const { return spendsCoinbase; }

    uint64_t GetCountWithAncestors() const { return nCountWithAncestors; }
    uint64_t GetSizeWithAncestors() const { return nSizeWithAncestors; }
    CAmount GetModFeesWithAncestors() const { return nModFeesWithAncestors; }
    int64_t GetSigOpCostWithAncestors() const { return nSigOpCostWithAncestors; }

    const Parents& GetMemPoolParentsConst() const { return m_parents; }
    const Children& GetMemPoolChildrenConst() const { return m_children; }
    Parents& GetMemPoolParents() const { return m_parents; }
    Children& GetMemPoolChildren() const { return m_children; }

    mutable size_t vTxHashesIdx; //!< Index in mempool's vTxHashes
    mutable Epoch::Marker m_epoch_marker; //!< epoch when last touched, useful for graph algorithms
    // SYSCOIN
    // If this is a proTx, this will be the hash of the key for which this ProTx was valid
    mutable uint256 validForProTxKey;
    mutable bool isKeyChangeProTx{false};
};

#endif // SYSCOIN_KERNEL_MEMPOOL_ENTRY_H
