// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_V3_POLICY_H
#define BITCOIN_POLICY_V3_POLICY_H

#include <consensus/amount.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <txmempool.h>
#include <util/result.h>

#include <set>
#include <string>

// This module enforces rules for BIP 431 TRUC transactions (with version=3) which help make
// RBF abilities more robust.
static constexpr decltype(CTransaction::version) TRUC_VERSION{3};

// v3 only allows 1 parent and 1 child when unconfirmed.
/** Maximum number of transactions including an unconfirmed tx and its descendants. */
static constexpr unsigned int V3_DESCENDANT_LIMIT{2};
/** Maximum number of transactions including a V3 tx and all its mempool ancestors. */
static constexpr unsigned int V3_ANCESTOR_LIMIT{2};

/** Maximum sigop-adjusted virtual size of all v3 transactions. */
static constexpr int64_t V3_MAX_VSIZE{10000};
/** Maximum sigop-adjusted virtual size of a tx which spends from an unconfirmed v3 transaction. */
static constexpr int64_t V3_CHILD_MAX_VSIZE{1000};
// These limits are within the default ancestor/descendant limits.
static_assert(V3_MAX_VSIZE + V3_CHILD_MAX_VSIZE <= DEFAULT_ANCESTOR_SIZE_LIMIT_KVB * 1000);
static_assert(V3_MAX_VSIZE + V3_CHILD_MAX_VSIZE <= DEFAULT_DESCENDANT_SIZE_LIMIT_KVB * 1000);

/** Must be called for every transaction, even if not v3. Not strictly necessary for transactions
 * accepted through AcceptMultipleTransactions.
 *
 * Checks the following rules:
 * 1. A v3 tx must only have v3 unconfirmed ancestors.
 * 2. A non-v3 tx must only have non-v3 unconfirmed ancestors.
 * 3. A v3's ancestor set, including itself, must be within V3_ANCESTOR_LIMIT.
 * 4. A v3's descendant set, including itself, must be within V3_DESCENDANT_LIMIT.
 * 5. If a v3 tx has any unconfirmed ancestors, the tx's sigop-adjusted vsize must be within
 * V3_CHILD_MAX_VSIZE.
 * 6. A v3 tx must be within V3_MAX_VSIZE.
 *
 *
 * @param[in]   pool                    A reference to the mempool.
 * @param[in]   direct_conflicts        In-mempool transactions this tx conflicts with. These conflicts
 *                                      are used to more accurately calculate the resulting descendant
 *                                      count of in-mempool ancestors.
 * @param[in]   vsize                   The sigop-adjusted virtual size of ptx.
 *
 * @returns 3 possibilities:
 * - std::nullopt if all v3 checks were applied successfully
 * - debug string + pointer to a mempool sibling if this transaction would be the second child in a
 *   1-parent-1-child cluster; the caller may consider evicting the specified sibling or return an
 *   error with the debug string.
 * - debug string + nullptr if this transaction violates some v3 rule and sibling eviction is not
 *   applicable.
 */
std::optional<std::pair<std::string, CTransactionRef>> SingleV3Checks(const CTxMemPool& pool, const CTransactionRef& ptx,
                                          const std::set<Txid>& direct_conflicts,
                                          int64_t vsize);

/** Must be called for every transaction that is submitted within a package, even if not v3.
 *
 * For each transaction in a package:
 * If it's not a v3 transaction, verify it has no direct v3 parents in the mempool or the package.

 * If it is a v3 transaction, verify that any direct parents in the mempool or the package are v3.
 * If such a parent exists, verify that parent has no other children in the package or the mempool,
 * and that the transaction itself has no children in the package.
 *
 * If any v3 violations in the package exist, this test will fail for one of them:
 * - if a v3 transaction T has a parent in the mempool and a child in the package, then PV3C(T) will fail
 * - if a v3 transaction T has a parent in the package and a child in the package, then PV3C(T) will fail
 * - if a v3 transaction T and a v3 (sibling) transaction U have some parent in the mempool,
 *   then PV3C(T) and PV3C(U) will fail
 * - if a v3 transaction T and a v3 (sibling) transaction U have some parent in the package,
 *   then PV3C(T) and PV3C(U) will fail
 * - if a v3 transaction T has a parent P and a grandparent G in the package, then
 *   PV3C(P) will fail (though PV3C(G) and PV3C(T) might succeed).
 *
 * @returns debug string if an error occurs, std::nullopt otherwise.
 * */
std::optional<std::string> PackageV3Checks(const CTxMemPool& pool, const CTransactionRef& ptx, int64_t vsize,
                                           const Package& package,
                                           const CTxMemPool::Entries& mempool_parents);

#endif // BITCOIN_POLICY_V3_POLICY_H
