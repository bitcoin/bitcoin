// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_ANCESTOR_PACKAGES_H
#define BITCOIN_POLICY_ANCESTOR_PACKAGES_H

#include <policy/feerate.h>
#include <policy/packages.h>

#include <vector>

/** A potential ancestor package, i.e. one transaction with its set of unconfirmed ancestors.
 * This class does not have any knowledge of chainstate, so it cannot determine whether all
 * unconfirmed ancestors are present. Its constructor accepts any list of transactions that
 * IsConsistentPackage(), linearizes them topologically, and determines whether it IsAncestorPackage().
 * If fee and vsizes are given for each transaction, it can also linearize the transactions using
 * the ancestor score-based mining algorithm via MiniMiner.
 *
 * IsIsInMempool() and IsDanglingWithDescendants() can be used to omit transactions.
 * Txns() and FilteredTxns() return the linearized transactions; FilteredTxns excludes non-PACKAGE ones.
 * FilteredAncestorSet() can be used to get a transaction's "subpackage," i.e. ancestor set within the
 * package. Also excludes non-PACKAGE ones.
 * */
class AncestorPackage
{
    /** Whether m_txns contains a connected package in which all transactions are ancestors of the
     * last transaction. This object is not aware of chainstate. So if m_txns only includes a
     * grandparent and not the "connecting" parent, this will (incorrectly) determine that the
     * grandparent is not an ancestor.
     * Set in the constructor and then does not change, even if transactions are skipped.
     * */
    bool m_ancestor_package_shaped{false};

    struct PackageEntry {
        enum class State : uint8_t {
            /** Default value.
             * If calling LinearizeWithFees(), this tx must have a fee and vsize. */
            PACKAGE,
            /** Excluded from FilteredAncestor{Set,FeeAndVsize}.
             * Should not appear in any transaction's ancestor_subset.
             * Excluded in LinearizeWithFees().
             * Set by calling IsIsInMempool() on this tx. */
            MEMPOOL,
            /** This and all descendant's FilteredAncestor{Set,FeeAndVsize} will be std::nullopt.
             * Should not appear in any transaction's ancestor_subset.
             * Excluded in LinearizeWithFees().
             * Set by calling IsDanglingWithDescendants() on this tx or its ancestor. */
            DANGLING,
        // If a parent is DANGLING, all descendants must also be DANGLING.
        // If a child is MEMPOOL, all ancestors should also be MEMPOOL.
        };

        CTransactionRef tx;

        State m_state{State::PACKAGE};

        /** This value starts as std::nullopt when we don't have any fee information yet. It can be
         * updated by calling LinearizeWithFees() if this entry isn't MEMPOOL or DANGLING. */
        std::optional<uint32_t> mining_sequence;

        /** Fees of this transaction. Starts as std::nullopt, can be updated using AddFeeAndVsize(). */
        std::optional<CAmount> fee;

        /** Virtual size of this transaction (depends on policy, calculated externally). Starts as
         * std::nullopt. Can be updated using AddFeeAndVsize(). */
        std::optional<int64_t> vsize;

        /** Txids of all PACKAGE ancestors. Populated in AncestorPackage ctor. If an ancestor
         * becomes MEMPOOL or DANGLING, it is erased from this set. Use FilteredAncestorSet to get
         * filtered ancestor sets. */
        std::set<Txid> ancestor_subset;

        /** Txids of all in-package descendant. Populated in AncestorPackage ctor and does not
         * change within AncestorPackage lifetime. */
        std::set<Txid> descendant_subset;

        explicit PackageEntry(CTransactionRef tx_in) : tx(tx_in) {}

        // Used to sort the result of Txns, FilteredTxns, and FilteredAncestorSet. Always guarantees
        // topological sort. If LinearizeWithFees() was called, also uses mining sequence.
        //
        // If ancestor score-based linearization sequence exists for both transactions, the
        // transaction with the lower sequence number comes first.
        //    If there is a tie, the transaction with fewer in-package ancestors comes first (topological sort).
        //       If there is still a tie, the transaction with the higher base feerate comes first.
        // Otherwise, the transaction with fewer in-package ancestors comes first (topological sort).
        bool operator<(const PackageEntry& rhs) const {
            if (mining_sequence == std::nullopt || rhs.mining_sequence == std::nullopt) {
                // If mining sequence is missing for either entry, default to topological order.
                return ancestor_subset.size() < rhs.ancestor_subset.size();
            } else {
                if (mining_sequence.value() == rhs.mining_sequence.value()) {
                    // Identical mining sequence means they would be included in the same ancestor
                    // set. The one with fewer ancestors comes first.
                    if (ancestor_subset.size() == rhs.ancestor_subset.size()) {
                        // Individual feerate. This is not necessarily fee-optimal, but helps in some situations.
                        // (a.fee * b.vsize > b.fee * a.vsize) is a shortcut for (a.fee / a.vsize > b.fee / b.vsize)
                        return *fee * *rhs.vsize  > *rhs.fee * *vsize;
                    }
                    return ancestor_subset.size() < rhs.ancestor_subset.size();
                } else {
                    return mining_sequence.value() < rhs.mining_sequence.value();
                }
            }
        }
    };
    /** Map from each txid to PackageEntry */
    std::map<Txid, PackageEntry> m_txid_to_entry;

    /** Linearized transactions. Always topologically sorted (IsSorted()). If fee information is
     * provided through LinearizeWithFees(), using mining_sequence scores. */
    std::vector<std::reference_wrapper<PackageEntry>> m_txns;

    /** Helper function for recursively constructing ancestor caches in ctor. */
    void visit(const CTransactionRef&);
public:
    /** Constructs ancestor package, sorting the transactions topologically and constructing the
     * txid_to_tx and ancestor_subsets maps. It is ok if the input txns is not sorted.
     * Expects:
     * - No duplicate transactions.
     * - No conflicts between transactions.
     */
    AncestorPackage(const Package& txns_in);

    bool IsAncestorPackage() const { return m_ancestor_package_shaped; }

    /** Returns all of the transactions, linearized. */
    Package Txns() const;

    /** Returns all of the transactions, without the skipped and dangling ones, linearized. */
    Package FilteredTxns() const;

    struct SubpackageInfo
    {
        std::vector<CTransactionRef> m_subpackage_txns;
        int64_t m_self_vsize;
        int64_t m_ancestor_vsize;
        CAmount m_self_fee;
        CAmount m_ancestor_fee;
    };

    /** Get the sorted, filtered ancestor subpackage for a tx. Includes the tx. Does not include
     * ancestors that are MEMPOOL. If this tx or any ancestors are DANGLING, returns std::nullopt. */
    std::optional<std::vector<CTransactionRef>> FilteredAncestorSet(const CTransactionRef& tx) const;

    /** Get SubpackageInfo for this tx and its filtered ancestors. Does not include ancestors that
     * are MEMPOOL. If this tx or any ancestors are DANGLING, returns std::nullopt. This result is
     * always consistent with FilteredAncestorSet(). */
    std::optional<SubpackageInfo> FilteredSubpackageInfo(const CTransactionRef& tx) const;

    /** Should be called if a tx or same txid transaction is found in mempool or submitted to it.
     * From now on, skip this tx from any result in FilteredAncestorSet(). Does not affect Txns(). */
    void MarkAsInMempool(const CTransactionRef& transaction);

    /** Should be called if a tx will not be considered because it is missing inputs or is
     * conflicting with another transaction that is already in mempool.
     * Skip a transaction and all of its descendants. From now on, if this transaction is present
     * in the ancestor set, FilteredAncestorSet() returns std::nullopt for that tx. Does not affect Txns(). */
    void IsDanglingWithDescendants(const CTransactionRef& transaction);

    /** Add information about fee and vsize for a transaction. */
    void AddFeeAndVsize(const Txid& txid, CAmount fee, int64_t vsize);

    /** Re-linearize transactions using the fee and vsize information given. Updates Txns().
     * Information must have been provided for all non-skipped transactions via AddFeeAndVsize().
     * @returns true if successful, false if not enough information was provided or an error
     * occurred - in that case, the transactions are still IsTopoSortedPackage() and it is safe to
     * still validate them, but we may not have a fee-optimal result. */
    bool LinearizeWithFees();
};
#endif // BITCOIN_POLICY_ANCESTOR_PACKAGES_H
