// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_TRUC_POLICY_H
#define BITCOIN_POLICY_TRUC_POLICY_H

#include <consensus/amount.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <txmempool.h>
#include <util/result.h>

#include <set>
#include <string>

// This module enforces rules for BIP 431 TRUC transactions which help make
// RBF abilities more robust. A transaction with version=3 is treated as TRUC.
static constexpr decltype(CTransaction::version) TRUC_VERSION{3};

// TRUC only allows 1 parent and 1 child when unconfirmed. This translates to a descendant set size
// of 2 and ancestor set size of 2.
/** Maximum number of transactions including an unconfirmed tx and its descendants. */
static constexpr unsigned int TRUC_DESCENDANT_LIMIT{2};
/** Maximum number of transactions including a TRUC tx and all its mempool ancestors. */
static constexpr unsigned int TRUC_ANCESTOR_LIMIT{2};

/** Maximum sigop-adjusted virtual size of all v3 transactions. */
static constexpr int64_t TRUC_MAX_VSIZE{10000};
static constexpr int64_t TRUC_MAX_WEIGHT{TRUC_MAX_VSIZE * WITNESS_SCALE_FACTOR};
/** Maximum sigop-adjusted virtual size of a tx which spends from an unconfirmed TRUC transaction. */
static constexpr int64_t TRUC_CHILD_MAX_VSIZE{1000};
static constexpr int64_t TRUC_CHILD_MAX_WEIGHT{TRUC_CHILD_MAX_VSIZE * WITNESS_SCALE_FACTOR};
// These limits are within the default ancestor/descendant limits.
static_assert(TRUC_MAX_VSIZE + TRUC_CHILD_MAX_VSIZE <= DEFAULT_ANCESTOR_SIZE_LIMIT_KVB * 1000);
static_assert(TRUC_MAX_VSIZE + TRUC_CHILD_MAX_VSIZE <= DEFAULT_DESCENDANT_SIZE_LIMIT_KVB * 1000);

/** Must be called for every transaction, even if not TRUC. Not strictly necessary for transactions
 * accepted through AcceptMultipleTransactions.
 *
 * Checks the following rules:
 * 1. A TRUC tx must only have TRUC unconfirmed ancestors.
 * 2. A non-TRUC tx must only have non-TRUC unconfirmed ancestors.
 * 3. A TRUC's ancestor set, including itself, must be within TRUC_ANCESTOR_LIMIT.
 * 4. A TRUC's descendant set, including itself, must be within TRUC_DESCENDANT_LIMIT.
 * 5. If a TRUC tx has any unconfirmed ancestors, the tx's sigop-adjusted vsize must be within
 * TRUC_CHILD_MAX_VSIZE.
 * 6. A TRUC tx must be within TRUC_MAX_VSIZE.
 *
 *
 * @param[in]   pool                    A reference to the mempool.
 * @param[in]   mempool_parents         The in-mempool ancestors of ptx.
 * @param[in]   direct_conflicts        In-mempool transactions this tx conflicts with. These conflicts
 *                                      are used to more accurately calculate the resulting descendant
 *                                      count of in-mempool ancestors.
 * @param[in]   vsize                   The sigop-adjusted virtual size of ptx.
 *
 * @returns 3 possibilities:
 * - std::nullopt if all TRUC checks were applied successfully
 * - debug string + pointer to a mempool sibling if this transaction would be the second child in a
 *   1-parent-1-child cluster; the caller may consider evicting the specified sibling or return an
 *   error with the debug string.
 * - debug string + nullptr if this transaction violates some TRUC rule and sibling eviction is not
 *   applicable.
 */
std::optional<std::pair<std::string, CTransactionRef>> SingleTRUCChecks(const CTxMemPool& pool, const CTransactionRef& ptx,
                                          const std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef>& mempool_parents,
                                          const std::set<Txid>& direct_conflicts,
                                          int64_t vsize);

/** Must be called for every transaction that is submitted within a package, even if not TRUC.
 *
 * For each transaction in a package:
 * If it's not a TRUC transaction, verify it has no direct TRUC parents in the mempool or the package.

 * If it is a TRUC transaction, verify that any direct parents in the mempool or the package are TRUC.
 * If such a parent exists, verify that parent has no other children in the package or the mempool,
 * and that the transaction itself has no children in the package.
 *
 * If any TRUC violations in the package exist, this test will fail for one of them:
 * - if a TRUC transaction T has a parent in the mempool and a child in the package, then PTRUCC(T) will fail
 * - if a TRUC transaction T has a parent in the package and a child in the package, then PTRUCC(T) will fail
 * - if a TRUC transaction T and a TRUC (sibling) transaction U have some parent in the mempool,
 *   then PTRUCC(T) and PTRUCC(U) will fail
 * - if a TRUC transaction T and a TRUC (sibling) transaction U have some parent in the package,
 *   then PTRUCC(T) and PTRUCC(U) will fail
 * - if a TRUC transaction T has a parent P and a grandparent G in the package, then
 *   PTRUCC(P) will fail (though PTRUCC(G) and PTRUCC(T) might succeed).
 *
 * @returns debug string if an error occurs, std::nullopt otherwise.
 * */
std::optional<std::string> PackageTRUCChecks(const CTxMemPool& pool, const CTransactionRef& ptx, int64_t vsize,
                                           const Package& package,
                                           const std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef>& mempool_parents);

#endif // BITCOIN_POLICY_TRUC_POLICY_H
