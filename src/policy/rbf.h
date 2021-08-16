// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_RBF_H
#define BITCOIN_POLICY_RBF_H

#include <txmempool.h>

/** Maximum number of transactions that can be replaced by BIP125 RBF (Rule #5). This includes all
 * mempool conflicts and their descendants. */
static constexpr uint32_t MAX_BIP125_REPLACEMENT_CANDIDATES{100};

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

/** Get all descendants of iters_conflicting. Also enforce BIP125 Rule #5, "The number of original
 * transactions to be replaced and their descendant transactions which will be evicted from the
 * mempool must not exceed a total of 100 transactions." Quit as early as possible. There cannot be
 * more than MAX_BIP125_REPLACEMENT_CANDIDATES potential entries.
 * @param[in]   iters_conflicting  The set of iterators to mempool entries.
 * @param[out]  all_conflicts      Populated with all the mempool entries that would be replaced,
 *                                  which includes descendants of iters_conflicting. Not cleared at
 *                                  the start; any existing mempool entries will remain in the set.
 * @returns an error message if Rule #5 is broken, otherwise a std::nullopt.
 */
std::optional<std::string> GetEntriesForConflicts(const CTransaction& tx, CTxMemPool& pool,
                                                  const CTxMemPool::setEntries& iters_conflicting,
                                                  CTxMemPool::setEntries& all_conflicts)
                                                  EXCLUSIVE_LOCKS_REQUIRED(pool.cs);

/** BIP125 Rule #2: "The replacement transaction may only include an unconfirmed input if that input
 * was included in one of the original transactions."
 * @returns error message if Rule #2 is broken, otherwise std::nullopt. */
std::optional<std::string> HasNoNewUnconfirmed(const CTransaction& tx, const CTxMemPool& pool,
                                               const CTxMemPool::setEntries& iters_conflicting)
                                               EXCLUSIVE_LOCKS_REQUIRED(pool.cs);

/** Check the intersection between two sets of transactions (a set of mempool entries and a set of
 * txids) to make sure they are disjoint.
 * @param[in]   ancestors    Set of mempool entries corresponding to ancestors of the
 *                              replacement transactions.
 * @param[in]   direct_conflicts    Set of txids corresponding to the mempool conflicts
 *                              (candidates to be replaced).
 * @param[in]   txid            Transaction ID, included in the error message if violation occurs.
 * @returns error message if the sets intersect, std::nullopt if they are disjoint.
 */
std::optional<std::string> EntriesAndTxidsDisjoint(const CTxMemPool::setEntries& ancestors,
                                                   const std::set<uint256>& direct_conflicts,
                                                   const uint256& txid);

/** Check that the feerate of the replacement transaction(s) is higher than the feerate of each
 * of the transactions in iters_conflicting.
 * @param[in]   iters_conflicting  The set of mempool entries.
 * @returns error message if fees insufficient, otherwise std::nullopt.
 */
std::optional<std::string> PaysMoreThanConflicts(const CTxMemPool::setEntries& iters_conflicting,
                                                 CFeeRate replacement_feerate, const uint256& txid);

/** Enforce BIP125 Rule #3 "The replacement transaction pays an absolute fee of at least the sum
 * paid by the original transactions." Enforce BIP125 Rule #4 "The replacement transaction must also
 * pay for its own bandwidth at or above the rate set by the node's minimum relay fee setting."
 * @param[in]   original_fees    Total modified fees of original transaction(s).
 * @param[in]   replacement_fees       Total modified fees of replacement transaction(s).
 * @param[in]   replacement_vsize               Total virtual size of replacement transaction(s).
 * @param[in]   txid                Transaction ID, included in the error message if violation occurs.
 * @returns error string if fees are insufficient, otherwise std::nullopt.
 */
std::optional<std::string> PaysForRBF(CAmount original_fees,
                                      CAmount replacement_fees,
                                      size_t replacement_vsize,
                                      const uint256& txid);

#endif // BITCOIN_POLICY_RBF_H
