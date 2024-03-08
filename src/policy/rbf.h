// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_RBF_H
#define BITCOIN_POLICY_RBF_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <threadsafety.h>
#include <txmempool.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>

class CFeeRate;
class uint256;

/** Maximum number of transactions that can be directly conflicted when
 * processing an RBF. This does not include descendants, which are necessarily
 * in the same cluster as a direct conflict. */
static constexpr uint32_t MAX_REPLACEMENT_CANDIDATES{100};

/** The rbf state of unconfirmed transactions */
enum class RBFTransactionState {
    /** Unconfirmed tx that does not signal rbf and is not in the mempool */
    UNKNOWN,
    /** Either this tx or a mempool ancestor signals rbf */
    REPLACEABLE_BIP125,
    /** Neither this tx nor a mempool ancestor signals rbf */
    FINAL,
};

/**
 * Determine whether an unconfirmed transaction is signaling opt-in to RBF
 * according to BIP 125
 * This involves checking sequence numbers of the transaction, as well
 * as the sequence numbers of all in-mempool ancestors.
 *
 * @param tx   The unconfirmed transaction
 * @param pool The mempool, which may contain the tx
 *
 * @return     The rbf state
 */
RBFTransactionState IsRBFOptIn(const CTransaction& tx, const CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(pool.cs);
RBFTransactionState IsRBFOptInEmptyMempool(const CTransaction& tx);

/** Get all descendants of iters_conflicting. Checks that there are no more than
 * MAX_REPLACEMENT_CANDIDATES potential entries. May overestimate if the entries in
 * iters_conflicting have overlapping descendants.
 * @param[in]   iters_conflicting   The set of iterators to mempool entries.
 * @param[out]  all_conflicts       Populated with all the mempool entries that would be replaced,
 *                                  which includes iters_conflicting and all entries' descendants.
 *                                  Not cleared at the start; any existing mempool entries will
 *                                  remain in the set.
 * @returns an error message if MAX_REPLACEMENT_CANDIDATES may be exceeded, otherwise a std::nullopt.
 */
std::optional<std::string> GetEntriesForConflicts(const CTransaction& tx, CTxMemPool& pool,
                                                  const CTxMemPool::setEntries& iters_conflicting,
                                                  CTxMemPool::setEntries& all_conflicts)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs);

/** Check the intersection between two sets of transactions (a set of mempool entries and a set of
 * txids) to make sure they are disjoint.
 * @param[in]   ancestors           Set of mempool entries corresponding to ancestors of the
 *                                  replacement transactions.
 * @param[in]   direct_conflicts    Set of txids corresponding to the mempool conflicts
 *                                  (candidates to be replaced).
 * @param[in]   txid                Transaction ID, included in the error message if violation occurs.
 * @returns error message if the sets intersect, std::nullopt if they are disjoint.
 */
std::optional<std::string> EntriesAndTxidsDisjoint(const CTxMemPool::setEntries& ancestors,
                                                   const std::set<Txid>& direct_conflicts,
                                                   const uint256& txid);

/** The replacement transaction must pay more fees than the original transactions. The additional
 * fees must pay for the replacement's bandwidth at or above the incremental relay feerate.
 * @param[in]   original_fees       Total modified fees of original transaction(s).
 * @param[in]   replacement_fees    Total modified fees of replacement transaction(s).
 * @param[in]   replacement_vsize   Total virtual size of replacement transaction(s).
 * @param[in]   relay_fee           The node's minimum feerate for transaction relay.
 * @param[in]   txid                Transaction ID, included in the error message if violation occurs.
 * @returns error string if fees are insufficient, otherwise std::nullopt.
 */
std::optional<std::string> PaysForRBF(CAmount original_fees,
                                      CAmount replacement_fees,
                                      size_t replacement_vsize,
                                      CFeeRate relay_fee,
                                      const uint256& txid);

/**
 * The replacement transaction must improve the feerate diagram of the mempool.
 * @param[in]   pool                The mempool.
 * @param[in]   direct_conflicts    Set of txids corresponding to the direct conflicts
 * @param[in]   all_conflicts       Set of mempool entries corresponding to all conflicts
 * @param[in]   entry               The replacement transaction
 * @param[in]   modified_fee        The modified fee of the replacement transaction
 * @returns error string if mempool diagram doesn't improve, otherwise std::nullopt.
 */

std::optional<std::string> ImprovesFeerateDiagram(CTxMemPool& pool,
                                                const CTxMemPool::setEntries& direct_conflicts,
                                                const CTxMemPool::setEntries& all_conflicts,
                                                CTxMemPoolEntry& entry,
                                                CAmount modified_fee)
                                                EXCLUSIVE_LOCKS_REQUIRED(pool.cs);

// returns true if the new_diagram is strictly better than the old one; false
// otherwise.
bool CompareFeeSizeDiagram(std::vector<FeeFrac> old_diagram, std::vector<FeeFrac> new_diagram);

#endif // BITCOIN_POLICY_RBF_H
