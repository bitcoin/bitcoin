// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_PACKAGES_H
#define BITCOIN_POLICY_PACKAGES_H

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <util/hasher.h>

#include <cstdint>
#include <unordered_set>
#include <vector>

/** Default maximum number of transactions in a package. */
static constexpr uint32_t MAX_PACKAGE_COUNT{25};
/** Default maximum total weight of transactions in a package in weight
    to allow for context-less checks. This must allow a superset of sigops
    weighted vsize limited transactions to not disallow transactions we would
    have otherwise accepted individually. */
static constexpr uint32_t MAX_PACKAGE_WEIGHT = 404'000;
static_assert(MAX_PACKAGE_WEIGHT >= MAX_STANDARD_TX_WEIGHT);

// If a package is to be evaluated, it must be at least as large as the mempool's ancestor/descendant limits,
// otherwise transactions that would be individually accepted may be rejected in a package erroneously.
// Since a submitted package must be child-with-parents (all of the transactions are a parent
// of the child), package limits are ultimately bounded by mempool package limits. Ensure that the
// defaults reflect this constraint.
static_assert(DEFAULT_DESCENDANT_LIMIT >= MAX_PACKAGE_COUNT);
static_assert(DEFAULT_ANCESTOR_LIMIT >= MAX_PACKAGE_COUNT);
static_assert(MAX_PACKAGE_WEIGHT >= DEFAULT_ANCESTOR_SIZE_LIMIT_KVB * WITNESS_SCALE_FACTOR * 1000);
static_assert(MAX_PACKAGE_WEIGHT >= DEFAULT_DESCENDANT_SIZE_LIMIT_KVB * WITNESS_SCALE_FACTOR * 1000);

/** A "reason" why a package was invalid. It may be that one or more of the included
 * transactions is invalid or the package itself violates our rules.
 * We don't distinguish between consensus and policy violations right now.
 */
enum class PackageValidationResult {
    PCKG_RESULT_UNSET = 0,        //!< Initial value. The package has not yet been rejected.
    PCKG_POLICY,                  //!< The package itself is invalid (e.g. too many transactions).
    PCKG_TX,                      //!< At least one tx is invalid.
    PCKG_MEMPOOL_ERROR,           //!< Mempool logic error.
};

/** A package is an ordered list of transactions. The transactions cannot conflict with (spend the
 * same inputs as) one another. */
using Package = std::vector<CTransactionRef>;

class PackageValidationState : public ValidationState<PackageValidationResult> {};

/** If any direct dependencies exist between transactions (i.e. a child spending the output of a
 * parent), checks that all parents appear somewhere in the list before their respective children.
 * No other ordering is enforced. This function cannot detect indirect dependencies (e.g. a
 * transaction's grandparent if its parent is not present).
 * @returns true if sorted. False if any tx spends the output of a tx that appears later in txns.
 */
bool IsTopoSortedPackage(const Package& txns);

/** Checks that these transactions don't conflict, i.e., spend the same prevout. This includes
 * checking that there are no duplicate transactions. Since these checks require looking at the inputs
 * of a transaction, returns false immediately if any transactions have empty vin.
 *
 * Does not check consistency of a transaction with oneself; does not check if a transaction spends
 * the same prevout multiple times (see bad-txns-inputs-duplicate in CheckTransaction()).
 *
 * @returns true if there are no conflicts. False if any two transactions spend the same prevout.
 * */
bool IsConsistentPackage(const Package& txns);

/** Context-free package policy checks:
 * 1. The number of transactions cannot exceed MAX_PACKAGE_COUNT.
 * 2. The total weight cannot exceed MAX_PACKAGE_WEIGHT.
 * 3. If any dependencies exist between transactions, parents must appear before children.
 * 4. Transactions cannot conflict, i.e., spend the same inputs.
 */
bool IsWellFormedPackage(const Package& txns, PackageValidationState& state, bool require_sorted);

/** Context-free check that a package is exactly one child and its parents; not all parents need to
 * be present, but the package must not contain any transactions that are not the child's parents.
 * It is expected to be sorted, which means the last transaction must be the child.
 */
bool IsChildWithParents(const Package& package);

/** Context-free check that a package IsChildWithParents() and none of the parents depend on each
 * other (the package is a "tree").
 */
bool IsChildWithParentsTree(const Package& package);

/** Get the hash of the concatenated wtxids of transactions, with wtxids
 * treated as a little-endian numbers and sorted in ascending numeric order.
 */
uint256 GetPackageHash(const std::vector<CTransactionRef>& transactions);

#endif // BITCOIN_POLICY_PACKAGES_H
