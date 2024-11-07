// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINI_MINER_H
#define BITCOIN_NODE_MINI_MINER_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <vector>

class CFeeRate;
class CTxMemPool;

namespace node {

// Container for tracking updates to ancestor feerate as we include ancestors in the "block"
class MiniMinerMempoolEntry
{
    const CTransactionRef tx;
    const int64_t vsize_individual;
    int64_t vsize_with_ancestors;
    const CAmount fee_individual;
    CAmount fee_with_ancestors;

// This class must be constructed while holding mempool.cs. After construction, the object's
// methods can be called without holding that lock.

public:
    explicit MiniMinerMempoolEntry(const CTransactionRef& tx_in,
                                   int64_t vsize_self,
                                   int64_t vsize_ancestor,
                                   CAmount fee_self,
                                   CAmount fee_ancestor):
        tx{tx_in},
        vsize_individual{vsize_self},
        vsize_with_ancestors{vsize_ancestor},
        fee_individual{fee_self},
        fee_with_ancestors{fee_ancestor}
    { }

    CAmount GetModifiedFee() const { return fee_individual; }
    CAmount GetModFeesWithAncestors() const { return fee_with_ancestors; }
    int64_t GetTxSize() const { return vsize_individual; }
    int64_t GetSizeWithAncestors() const { return vsize_with_ancestors; }
    const CTransaction& GetTx() const LIFETIMEBOUND { return *tx; }
    void UpdateAncestorState(int64_t vsize_change, CAmount fee_change) {
        vsize_with_ancestors += vsize_change;
        fee_with_ancestors += fee_change;
    }
};

// Comparator needed for std::set<MockEntryMap::iterator>
struct IteratorComparator
{
    template<typename I>
    bool operator()(const I& a, const I& b) const
    {
        return a->first < b->first;
    }
};

/** A minimal version of BlockAssembler, using the same ancestor set scoring algorithm. Allows us to
 * run this algorithm on a limited set of transactions (e.g. subset of mempool or transactions that
 * are not yet in mempool) instead of the entire mempool, ignoring consensus rules.
 * Callers may use this to:
 * - Calculate the "bump fee" needed to spend an unconfirmed UTXO at a given feerate
 * - "Linearize" a list of transactions to see the order in which they would be selected for
 *   inclusion in a block
 */
class MiniMiner
{
    // When true, a caller may use CalculateBumpFees(). Becomes false if we failed to retrieve
    // mempool entries (i.e. cluster size too large) or bump fees have already been calculated.
    bool m_ready_to_calculate{true};

    // Set once per lifetime, fill in during initialization.
    // txids of to-be-replaced transactions
    std::set<uint256> m_to_be_replaced;

    // If multiple argument outpoints correspond to the same transaction, cache them together in
    // a single entry indexed by txid. Then we can just work with txids since all outpoints from
    // the same tx will have the same bumpfee. Excludes non-mempool transactions.
    std::map<uint256, std::vector<COutPoint>> m_requested_outpoints_by_txid;

    // Txid to a number representing the order in which this transaction was included (smaller
    // number = included earlier).  Transactions included in an ancestor set together have the same
    // sequence number.
    std::map<Txid, uint32_t> m_inclusion_order;
    // What we're trying to calculate. Outpoint to the fee needed to bring the transaction to the target feerate.
    std::map<COutPoint, CAmount> m_bump_fees;

    // The constructed block template
    std::set<uint256> m_in_block;

    // Information on the current status of the block
    CAmount m_total_fees{0};
    int32_t m_total_vsize{0};

    /** Main data structure holding the entries, can be indexed by txid */
    std::map<uint256, MiniMinerMempoolEntry> m_entries_by_txid;
    using MockEntryMap = decltype(m_entries_by_txid);

    /** Vector of entries, can be sorted by ancestor feerate. */
    std::vector<MockEntryMap::iterator> m_entries;

    /** Map of txid to its descendants. Should be inclusive. */
    std::map<uint256, std::vector<MockEntryMap::iterator>> m_descendant_set_by_txid;

    /** Consider this ancestor package "mined" so remove all these entries from our data structures. */
    void DeleteAncestorPackage(const std::set<MockEntryMap::iterator, IteratorComparator>& ancestors);

    /** Perform some checks. */
    void SanityCheck() const;

public:
    /** Returns true if CalculateBumpFees may be called, false if not. */
    bool IsReadyToCalculate() const { return m_ready_to_calculate; }

    /** Build a block template until the target feerate is hit. If target_feerate is not given,
     * builds a block template until all transactions have been selected. */
    void BuildMockTemplate(std::optional<CFeeRate> target_feerate);

    /** Returns set of txids in the block template if one has been constructed. */
    std::set<uint256> GetMockTemplateTxids() const { return m_in_block; }

    /** Constructor that takes a list of outpoints that may or may not belong to transactions in the
     * mempool. Copies out information about the relevant transactions in the mempool into
     * MiniMinerMempoolEntrys.
    */
    MiniMiner(const CTxMemPool& mempool, const std::vector<COutPoint>& outpoints);

    /** Constructor in which the MiniMinerMempoolEntry entries have been constructed manually.
     * It is assumed that all entries are unique and their values are correct, otherwise results
     * computed by MiniMiner may be incorrect. Callers should check IsReadyToCalculate() after
     * construction.
     * @param[in] descendant_caches A map from each transaction to the set of txids of this
     *                              transaction's descendant set, including itself. Each tx in
     *                              manual_entries must have a corresponding entry in this map, and
     *                              all of the txids in a descendant set must correspond to a tx in
     *                              manual_entries.
     */
    MiniMiner(const std::vector<MiniMinerMempoolEntry>& manual_entries,
              const std::map<Txid, std::set<Txid>>& descendant_caches);

    /** Construct a new block template and, for each outpoint corresponding to a transaction that
     * did not make it into the block, calculate the cost of bumping those transactions (and their
     * ancestors) to the minimum feerate. Returns a map from outpoint to bump fee, or an empty map
     * if they cannot be calculated. */
    std::map<COutPoint, CAmount> CalculateBumpFees(const CFeeRate& target_feerate);

    /** Construct a new block template and, calculate the cost of bumping all transactions that did
     * not make it into the block to the target feerate. Returns the total bump fee, or std::nullopt
     * if it cannot be calculated. */
    std::optional<CAmount> CalculateTotalBumpFees(const CFeeRate& target_feerate);

    /** Construct a new block template with all of the transactions and calculate the order in which
     * they are selected. Returns the sequence number (lower = selected earlier) with which each
     * transaction was selected, indexed by txid, or an empty map if it cannot be calculated.
     */
    std::map<Txid, uint32_t> Linearize();
};
} // namespace node

#endif // BITCOIN_NODE_MINI_MINER_H
