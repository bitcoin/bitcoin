// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/v3_policy.h>

#include <coins.h>
#include <consensus/amount.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/check.h>
#include <script/solver.h>

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
    /** nVersion used to check inheritance of v3 and non-v3 */
    decltype(CTransaction::nVersion) m_version;
    /** If parent is in mempool, whether it has any descendants in mempool. */
    bool m_has_mempool_descendant;

    ParentInfo() = delete;
    ParentInfo(const Txid& txid, const Wtxid& wtxid, decltype(CTransaction::nVersion) version, bool has_mempool_descendant) :
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
    if (ptx->nVersion == 3) {
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

            const auto parent_info = [&] {
                if (mempool_ancestors.size() > 0) {
                    // There's a parent in the mempool.
                    auto& mempool_parent = *mempool_ancestors.begin();
                    Assume(mempool_parent->GetCountWithDescendants() == 1);
                    return ParentInfo{mempool_parent->GetTx().GetHash(),
                                      mempool_parent->GetTx().GetWitnessHash(),
                                      mempool_parent->GetTx().nVersion,
                                      /*has_mempool_descendant=*/mempool_parent->GetCountWithDescendants() > 1};
                } else {
                    // Ancestor must be in the package. Find it.
                    auto& parent_index = in_package_parents.front();
                    auto& package_parent = package.at(parent_index);
                    return ParentInfo{package_parent->GetHash(),
                                      package_parent->GetWitnessHash(),
                                      package_parent->nVersion,
                                      /*has_mempool_descendant=*/false};
                }
            }();

            // If there is a parent, it must have the right version.
            if (parent_info.m_version != 3) {
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

            // It shouldn't be possible to have any mempool siblings at this point. SingleV3Checks
            // catches mempool siblings. Also, if the package consists of connected transactions,
            // any tx having a mempool ancestor would mean the package exceeds ancestor limits.
            if (!Assume(!parent_info.m_has_mempool_descendant)) {
                return strprintf("tx %u would exceed descendant count limit", parent_info.m_wtxid.ToString());
            }
        }
    } else {
        // Non-v3 transactions cannot have v3 parents.
        for (auto it : mempool_ancestors) {
            if (it->GetTx().nVersion == 3) {
                return strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
                                 ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                                 it->GetSharedTx()->GetHash().ToString(), it->GetSharedTx()->GetWitnessHash().ToString());
            }
        }
        for (const auto& index: in_package_parents) {
            if (package.at(index)->nVersion == 3) {
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

// Given the parent of a transaction that is being considered for the mempool,
// check whether we should imbue the parent with v3-semantics. If it is so
// imbued, then the child will be required to comply with the v3-child
// semantics.
bool ImbueV3Parent(const CTxMemPool::txiter it)
{
    const CTransaction& tx = it->GetTx();

    // If it's labeled v3, then it's obviously v3.
    if (tx.nVersion == 3) return true;

    // We imbue v3-ness for a transaction if:
    // 1) no in-mempool ancestors
    if (it->GetCountWithAncestors() > 1) return false;

    // 2) must be version 2
    if (tx.nVersion != 2) return false;

    // 3) must have 1 input
    if (tx.vin.size() != 1) return false;

    // 4) must have upper 8 bits of locktime == 0x20
    if (tx.nLockTime >> 8*3 != 0x20) return false;

    // 5) must have upper 8 bits of sequence == 0x80
    if (tx.vin[0].nSequence >> 8*3 != 0x80) return false;

    // 6) must have exactly 2 330-satoshi outputs (only case where carveout
    // matters), and all outputs should be p2wsh, and outputs are sorted in
    // increasing value order.
    int num_330_sat_outputs{0};
    for (size_t index=0; index<tx.vout.size(); ++index) {
        const auto& output = tx.vout[index];

        // Outputs are sorted in increasing nValue order.
        if (index > 0 && output.nValue < tx.vout[index-1].nValue) return false;

        // Check that the output is P2WSH.
        int witnessversion;
        std::vector<unsigned char> witnessprogram;
        if (!output.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram) ||
                witnessversion != 0 ||
                witnessprogram.size() != WITNESS_V0_SCRIPTHASH_SIZE) {
            return false;
        }

        // Track the number of 330-sat outputs.
        if (output.nValue == 330) ++num_330_sat_outputs;
    }
    if (num_330_sat_outputs != 2) return false;

    return true;
}

std::optional<std::string> SingleV3Checks(const CTransactionRef& ptx,
                                          const CTxMemPool::setEntries& mempool_ancestors,
                                          const std::set<Txid>& direct_conflicts,
                                          int64_t vsize)
{
    // Check v3 and non-v3 inheritance.
    for (const auto& entry : mempool_ancestors) {
        if (ptx->nVersion != 3 && entry->GetTx().nVersion == 3) {
            return strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                             entry->GetSharedTx()->GetHash().ToString(), entry->GetSharedTx()->GetWitnessHash().ToString());
        } else if (ptx->nVersion == 3 && entry->GetTx().nVersion != 3) {
            return strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3 tx %s (wtxid=%s)",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                             entry->GetSharedTx()->GetHash().ToString(), entry->GetSharedTx()->GetWitnessHash().ToString());
        }
    }

    bool imbue_v3_semantics{false};
    for (const auto& entry : mempool_ancestors) {
        imbue_v3_semantics |= ImbueV3Parent(entry);
    }

    if (!imbue_v3_semantics && ptx->nVersion != 3) {
        // If this tx is not v3, and no ancestor matches the v3 template, then we are done.
        return std::nullopt;
    }

    // This function is specialized for these limits, and must be reimplemented if they ever change.
    static_assert(V3_ANCESTOR_LIMIT == 2);
    static_assert(V3_DESCENDANT_LIMIT == 2);

    // The rest of the rules only apply to transactions with nVersion=3, or that have imbued v3 semantics.

    // Check that V3_ANCESTOR_LIMIT would not be violated, including both in-package and in-mempool.
    if (mempool_ancestors.size() + 1 > V3_ANCESTOR_LIMIT) {
        return strprintf("tx %s (wtxid=%s) would have too many ancestors",
                         ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString());
    }

    // Remaining checks only pertain to transactions with unconfirmed ancestors.
    if (mempool_ancestors.size() > 0) {
        // If this transaction spends V3 parents, it cannot be too large.
        if (vsize > V3_CHILD_MAX_VSIZE) {
            return strprintf("v3 child tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(), vsize, V3_CHILD_MAX_VSIZE);
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
        if (parent_entry->GetCountWithDescendants() + 1 > V3_DESCENDANT_LIMIT && !child_will_be_replaced) {
            return strprintf("tx %u (wtxid=%s) would exceed descendant count limit",
                             parent_entry->GetSharedTx()->GetHash().ToString(),
                             parent_entry->GetSharedTx()->GetWitnessHash().ToString());
        }
    }
    return std::nullopt;
}
