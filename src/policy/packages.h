// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_PACKAGES_H
#define BITCOIN_POLICY_PACKAGES_H

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <util/hasher.h>

#include <cstdint>
#include <unordered_set>
#include <vector>

/** Default maximum number of transactions in a package. */
static constexpr uint32_t MAX_PACKAGE_COUNT{25};
/** Default maximum total virtual size of transactions in a package in KvB. */
static constexpr uint32_t MAX_PACKAGE_SIZE{101};
static_assert(MAX_PACKAGE_SIZE * WITNESS_SCALE_FACTOR * 1000 >= MAX_STANDARD_TX_WEIGHT);

// If a package is submitted, it must be within the mempool's ancestor/descendant limits. Since a
// submitted package must be child-with-unconfirmed-parents (all of the transactions are an ancestor
// of the child), package limits are ultimately bounded by mempool package limits. Ensure that the
// defaults reflect this constraint.
static_assert(DEFAULT_DESCENDANT_LIMIT >= MAX_PACKAGE_COUNT);
static_assert(DEFAULT_ANCESTOR_LIMIT >= MAX_PACKAGE_COUNT);
static_assert(DEFAULT_ANCESTOR_SIZE_LIMIT_KVB >= MAX_PACKAGE_SIZE);
static_assert(DEFAULT_DESCENDANT_SIZE_LIMIT_KVB >= MAX_PACKAGE_SIZE);

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
 * This function cannot detect indirect dependencies (e.g. a transaction's grandparent if its parent
 * is not present).
 * @returns true if sorted. False if any tx spends the output of a tx that appears later in txns.
 */
bool IsSorted(const Package& txns);

/** Checks that none of the transactions conflict, i.e., spend the same prevout. Consequently also
 * checks that there are no duplicate transactions.
 * @returns true if there are no conflicts. False if any two transactions spend the same prevout.
 * */
bool IsConsistent(const Package& txns);

/** Context-free package policy checks:
 * 1. The number of transactions cannot exceed MAX_PACKAGE_COUNT.
 * 2. The total virtual size cannot exceed MAX_PACKAGE_SIZE.
 * 3. If any dependencies exist between transactions, parents must appear before children.
 * 4. Transactions cannot conflict, i.e., spend the same inputs.
 */
bool IsPackageWellFormed(const Package& txns, PackageValidationState& state, bool require_sorted);

/** Context-free check that a package is exactly one child and its parents; not all parents need to
 * be present, but the package must not contain any transactions that are not the child's parents.
 * It is expected to be sorted, which means the last transaction must be the child.
 */
bool IsChildWithParents(const Package& package);

/** A potential BIP331 Ancestor Package, i.e. one transaction with its set of ancestors.
 * This class does not have any knowledge of chainstate, so it cannot determine whether all
 * unconfirmed ancestors are present. Its constructor accepts any list of transactions that
 * IsConsistent(), sorts them topologically (accessible through Txns()), and determines whether it
 * IsAncestorPackage(). GetAncestorSet() can be used to get a transaction's "subpackage," i.e.
 * ancestor set within the package. Exclude() should be called when a transaction is in
 * the mempool so that it can be excluded from other transactions' subpackages. Ban() should be
 * called when a transaction is invalid and all of its descendants should be considered invalid as
 * well; GetAncestorSet() will then return std::nullopt for those descendants.
 * */
class AncestorPackage
{
    /** Whether txns contains a connected package in which all transactions are ancestors of the
     * last transaction. This object is not aware of chainstate. So if txns only includes a
     * grandparent and not the "connecting" parent, this will (incorrectly) determine that the
     * grandparent is not an ancestor.
     * */
    bool is_ancestor_package{false};
    /** Transactions sorted topologically (see IsSorted()). */
    Package txns;
    /** Map from txid to transaction for quick lookup. */
    std::map<uint256, CTransactionRef> txid_to_tx;
    /** Cache of the in-package ancestors for each transaction, indexed by txid. */
    std::map<uint256, std::set<uint256>> ancestor_subsets;
    /** Txids of transactions to exclude when returning ancestor subsets.*/
    std::unordered_set<uint256, SaltedTxidHasher> excluded_txns;
    /** Txids of transactions that are banned. Return nullopt from GetAncestorSet() if it contains
     * any of these transactions.*/
    std::unordered_set<uint256, SaltedTxidHasher> banned_txns;

    /** Helper function for recursively constructing ancestor caches in ctor. */
    void visit(const CTransactionRef&);
public:
    /** Constructs ancestor package, sorting the transactions topologically and constructing the
     * txid_to_tx and ancestor_subsets maps. It is ok if the input txns is not sorted.
     * Expects:
     * - No duplicate transactions.
     * - No conflicts between transactions.
     * - txns is of reasonable size (e.g. below MAX_PACKAGE_COUNT) to limit recursion depth
     */
    AncestorPackage(const Package& txns);

    bool IsAncestorPackage() const { return is_ancestor_package; }
    /** Returns the transactions, in ascending order of number of in-package ancestors. */
    Package Txns() const { return txns; }
    /** Get the ancestor subpackage for a tx, sorted so that ancestors appear before descendants.
     * The list includes the tx. If this transaction is not part of this package or it depends on a
     * Banned tx, returns std::nullopt. If one or more ancestors have been Excluded, they will not
     * appear in the result. */
    std::optional<std::vector<CTransactionRef>> GetAncestorSet(const CTransactionRef& tx);
    /** From now on, exclude this tx from any result in GetAncestorSet(). Does not affect Txns(). */
    void Exclude(const CTransactionRef& transaction);
    /** Mark a transaction as "banned." From now on, if this transaction is present in the ancestor
     * set, GetAncestorSet() should return std::nullopt for that tx. Does not affect Txns(). */
    void Ban(const CTransactionRef& transaction);
};
#endif // BITCOIN_POLICY_PACKAGES_H
