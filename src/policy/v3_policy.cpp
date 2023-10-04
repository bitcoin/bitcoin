// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/v3_policy.h>

#include <coins.h>
#include <consensus/amount.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/check.h>

#include <algorithm>
#include <numeric>
#include <vector>

/** Helper for PackageV3Checks: Returns a vector containing the indices of transactions (within
 * package) that are direct parents of ptx. */
std::vector<size_t> FindInPackageParents(const Package& package, const CTransactionRef& ptx)
{
    std::vector<size_t> in_package_parents;

    std::set<Txid> possible_parents;
    for (auto &input : ptx->vin) {
        possible_parents.insert(input.prevout.hash);
    }

    for (size_t i{0}; i < package.size(); ++i) {
        const auto& tx = package.at(i);
        // We assume the package is sorted, so that we don't need to continue
        // looking past the transaction itself.
        if (&(*tx) == &(*ptx)) break;
        if (possible_parents.count(tx->GetHash())) {
            in_package_parents.push_back(i);
        }
    }
    return in_package_parents;
}

/** Helper for PackageV3Checks, storing info for a mempool or package parent. */
struct ParentInfo {
    /** Txid used to identify this parent by prevout */
    const Txid& m_txid;
    /** Wtxid used for debug string */
    const Wtxid& m_wtxid;
    /** version used to check inheritance of v3 and non-v3 */
    decltype(CTransaction::version) m_version;
    /** If parent is in mempool, whether it has any descendants in mempool. */
    bool m_has_mempool_descendant;

    ParentInfo() = delete;
    ParentInfo(const Txid& txid, const Wtxid& wtxid, decltype(CTransaction::version) version, bool has_mempool_descendant) :
        m_txid{txid}, m_wtxid{wtxid}, m_version{version},
        m_has_mempool_descendant{has_mempool_descendant}
    {}
};

std::optional<std::string> PackageV3Checks(const CTransactionRef& ptx, int64_t vsize,
                                           const Package& package,
                                           const CTxMemPool::setEntries& mempool_ancestors)
{
    // This function is specialized for these limits, and must be reimplemented if they ever change.
    static_assert(V3_ANCESTOR_LIMIT == 2);
    static_assert(V3_DESCENDANT_LIMIT == 2);

    const auto in_package_parents{FindInPackageParents(package, ptx)};

    // Now we have all ancestors, so we can start checking v3 rules.
    if (ptx->version == TRUC_VERSION) {
        // SingleV3Checks should have checked this already.
        if (!Assume(vsize <= V3_MAX_VSIZE)) {
            return strprintf("v3 tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(), vsize, V3_MAX_VSIZE);
        }

        if (mempool_ancestors.size() + in_package_parents.size() + 1 > V3_ANCESTOR_LIMIT) {
            return strprintf("tx %s (wtxid=%s) would have too many ancestors",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString());
        }

        const bool has_parent{mempool_ancestors.size() + in_package_parents.size() > 0};
        if (has_parent) {
            // A v3 child cannot be too large.
            if (vsize > V3_CHILD_MAX_VSIZE) {
                return strprintf("v3 child tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
                                 ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                                 vsize, V3_CHILD_MAX_VSIZE);
            }

            // Exactly 1 parent exists, either in mempool or package. Find it.
            const auto parent_info = [&] {
                if (mempool_ancestors.size() > 0) {
                    auto& mempool_parent = *mempool_ancestors.begin();
                    return ParentInfo{mempool_parent->GetTx().GetHash(),
                                      mempool_parent->GetTx().GetWitnessHash(),
                                      mempool_parent->GetTx().version,
                                      /*has_mempool_descendant=*/mempool_parent->GetNumChildren() > 0};
                } else {
                    auto& parent_index = in_package_parents.front();
                    auto& package_parent = package.at(parent_index);
                    return ParentInfo{package_parent->GetHash(),
                                      package_parent->GetWitnessHash(),
                                      package_parent->version,
                                      /*has_mempool_descendant=*/false};
                }
            }();

            // If there is a parent, it must have the right version.
            if (parent_info.m_version != TRUC_VERSION) {
                return strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3 tx %s (wtxid=%s)",
                                 ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                                 parent_info.m_txid.ToString(), parent_info.m_wtxid.ToString());
            }

            for (const auto& package_tx : package) {
                // Skip same tx.
                if (&(*package_tx) == &(*ptx)) continue;

                for (auto& input : package_tx->vin) {
                    // Fail if we find another tx with the same parent. We don't check whether the
                    // sibling is to-be-replaced (done in SingleV3Checks) because these transactions
                    // are within the same package.
                    if (input.prevout.hash == parent_info.m_txid) {
                        return strprintf("tx %s (wtxid=%s) would exceed descendant count limit",
                                         parent_info.m_txid.ToString(),
                                         parent_info.m_wtxid.ToString());
                    }

                    // This tx can't have both a parent and an in-package child.
                    if (input.prevout.hash == ptx->GetHash()) {
                        return strprintf("tx %s (wtxid=%s) would have too many ancestors",
                                         package_tx->GetHash().ToString(), package_tx->GetWitnessHash().ToString());
                    }
                }
            }

            if (parent_info.m_has_mempool_descendant) {
                return strprintf("tx %s (wtxid=%s) would exceed descendant count limit",
                                parent_info.m_txid.ToString(), parent_info.m_wtxid.ToString());
            }
        }
    } else {
        // Non-v3 transactions cannot have v3 parents.
        for (auto it : mempool_ancestors) {
            if (it->GetTx().version == TRUC_VERSION) {
                return strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
                                 ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                                 it->GetSharedTx()->GetHash().ToString(), it->GetSharedTx()->GetWitnessHash().ToString());
            }
        }
        for (const auto& index: in_package_parents) {
            if (package.at(index)->version == TRUC_VERSION) {
                return strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
                                 ptx->GetHash().ToString(),
                                 ptx->GetWitnessHash().ToString(),
                                 package.at(index)->GetHash().ToString(),
                                 package.at(index)->GetWitnessHash().ToString());
            }
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, CTransactionRef>> SingleV3Checks(const CTransactionRef& ptx,
                                          const CTxMemPool::setEntries& mempool_ancestors,
                                          const std::set<Txid>& direct_conflicts,
                                          int64_t vsize)
{
    // Check v3 and non-v3 inheritance.
    for (const auto& entry : mempool_ancestors) {
        if (ptx->version != TRUC_VERSION && entry->GetTx().version == TRUC_VERSION) {
            return std::make_pair(strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                             entry->GetSharedTx()->GetHash().ToString(), entry->GetSharedTx()->GetWitnessHash().ToString()),
                nullptr);
        } else if (ptx->version == TRUC_VERSION && entry->GetTx().version != TRUC_VERSION) {
            return std::make_pair(strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3 tx %s (wtxid=%s)",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                             entry->GetSharedTx()->GetHash().ToString(), entry->GetSharedTx()->GetWitnessHash().ToString()),
                nullptr);
        }
    }

    // This function is specialized for these limits, and must be reimplemented if they ever change.
    static_assert(V3_ANCESTOR_LIMIT == 2);
    static_assert(V3_DESCENDANT_LIMIT == 2);

    // The rest of the rules only apply to transactions with version=3.
    if (ptx->version != TRUC_VERSION) return std::nullopt;

    if (vsize > V3_MAX_VSIZE) {
        return std::make_pair(strprintf("v3 tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
                         ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(), vsize, V3_MAX_VSIZE),
            nullptr);
    }

    // Check that V3_ANCESTOR_LIMIT would not be violated.
    if (mempool_ancestors.size() + 1 > V3_ANCESTOR_LIMIT) {
        return std::make_pair(strprintf("tx %s (wtxid=%s) would have too many ancestors",
                         ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString()),
            nullptr);
    }

    // Remaining checks only pertain to transactions with unconfirmed ancestors.
    if (mempool_ancestors.size() > 0) {
        // If this transaction spends V3 parents, it cannot be too large.
        if (vsize > V3_CHILD_MAX_VSIZE) {
            return std::make_pair(strprintf("v3 child tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(), vsize, V3_CHILD_MAX_VSIZE),
                nullptr);
        }

        // Check the descendant counts of in-mempool ancestors.
        const auto& parent_entry = *mempool_ancestors.begin();
        // If there are any ancestors, this is the only child allowed. The parent cannot have any
        // other descendants. We handle the possibility of multiple children as that case is
        // possible through a reorg.
        const auto& children = parent_entry->GetMemPoolChildrenConst();
        // Don't double-count a transaction that is going to be replaced. This logic assumes that
        // any descendant of the V3 transaction is a direct child, which makes sense because a V3
        // transaction can only have 1 descendant.
        const bool child_will_be_replaced = !children.empty() &&
            std::any_of(children.cbegin(), children.cend(),
                [&direct_conflicts](const CTxMemPoolEntry& child){return direct_conflicts.count(child.GetTx().GetHash()) > 0;});
        if (parent_entry->GetNumChildren() + 2 > V3_DESCENDANT_LIMIT && !child_will_be_replaced) {
            // Allow sibling eviction for v3 transaction: if another child already exists, even if
            // we don't conflict inputs with it, consider evicting it under RBF rules. We rely on v3 rules
            // only permitting 1 descendant, as otherwise we would need to have logic for deciding
            // which descendant to evict. Skip if this isn't true, e.g. if the transaction has
            // multiple children or the sibling also has descendants due to a reorg.
            const bool consider_sibling_eviction{parent_entry->GetNumChildren() == 1 &&
                children.begin()->get().GetNumChildren() == 0};

            // Return the sibling if its eviction can be considered. Provide the "descendant count
            // limit" string either way, as the caller may decide not to do sibling eviction.
            return std::make_pair(strprintf("tx %u (wtxid=%s) would exceed descendant count limit",
                                            parent_entry->GetSharedTx()->GetHash().ToString(),
                                            parent_entry->GetSharedTx()->GetWitnessHash().ToString()),
                                  consider_sibling_eviction ?  children.begin()->get().GetSharedTx() : nullptr);
        }
    }
    return std::nullopt;
}
