// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_PACKAGES_H
#define BITCOIN_POLICY_PACKAGES_H

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>

#include <cstdint>
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
// Since a submitted package must be child-with-unconfirmed-parents (all of the transactions are an ancestor
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

/** Context-free package policy checks:
 * 1. The number of transactions cannot exceed MAX_PACKAGE_COUNT.
 * 2. The total weight cannot exceed MAX_PACKAGE_WEIGHT.
 * 3. If any dependencies exist between transactions, parents must appear before children.
 * 4. Transactions cannot conflict, i.e., spend the same inputs.
 */
bool CheckPackage(const Package& txns, PackageValidationState& state);

/** Context-free check that a package is exactly one child and its parents; not all parents need to
 * be present, but the package must not contain any transactions that are not the child's parents.
 * It is expected to be sorted, which means the last transaction must be the child.
 */
bool IsChildWithParents(const Package& package);

/** Context-free check that a package IsChildWithParents() and none of the parents depend on each
 * other (the package is a "tree").
 */
bool IsChildWithParentsTree(const Package& package);
#endif // BITCOIN_POLICY_PACKAGES_H
