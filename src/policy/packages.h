// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_PACKAGES_H
#define BITCOIN_POLICY_PACKAGES_H

#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>

#include <vector>

/** Default maximum number of transactions in a package. */
static constexpr uint32_t MAX_PACKAGE_COUNT{25};
/** Default maximum total virtual size of transactions in a package in KvB. */
static constexpr uint32_t MAX_PACKAGE_SIZE{101};
static_assert(MAX_PACKAGE_SIZE * WITNESS_SCALE_FACTOR * 1000 >= MAX_STANDARD_TX_WEIGHT);

/** A "reason" why a package was invalid. It may be that one or more of the included
 * transactions is invalid or the package itself fails. It's possible for failures to arise from
 * rule violations or mempool policy.
 */
enum class PackageValidationResult {
    PCKG_RESULT_UNSET = 0,        //!< Initial value. The package has not yet been rejected.
    PCKG_BAD,                     //!< The package itself is invalid or malformed.
    PCKG_POLICY,                  //!< The package failed due to package-related mempool policy.
    PCKG_TX_CONSENSUS,            //!< At least one tx is invalid for consensus reasons.
    PCKG_TX_POLICY,               //!< At least one tx is invalid for non-consensus reasons.
};

/** A package is an ordered list of transactions. The transactions cannot conflict with (spend the
 * same inputs as) one another. */
using Package = std::vector<CTransactionRef>;

class PackageValidationState : public ValidationState<PackageValidationResult> {};

/** Context-free package sanitization. A package is not well-formed if it fails any of these checks:
 * 1. The number of transactions cannot exceed MAX_PACKAGE_COUNT.
 * 2. The total virtual size cannot exceed MAX_PACKAGE_SIZE.
 * 3. If any dependencies exist between transactions, parents must appear before children.
 * 4. Transactions cannot conflict, i.e., spend the same inputs.
 */
bool CheckPackage(const Package& txns, PackageValidationState& state);

/** Context-free check that a package is exactly one child and at least one of its parents. It is
 * expected to be sorted, which means the last transaction must be the child. The package cannot
 * contain any transactions that are not the child's parents.
 * @param[in]   exact   When true, return whether this package is exactly one child and all of its
 *                      parents. Otherwise, only require that non-child transactions are parents.
 */
bool IsChildWithParents(const Package& package, bool exact);

#endif // BITCOIN_POLICY_PACKAGES_H
