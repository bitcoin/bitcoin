// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <util/check.h>
#include <util/hasher.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <numeric>

bool IsSorted(const Package& txns)
{
    std::unordered_set<uint256, SaltedTxidHasher> later_txids;
    std::transform(txns.cbegin(), txns.cend(), std::inserter(later_txids, later_txids.end()),
                   [](const auto& tx) { return tx->GetHash(); });
    for (const auto& tx : txns) {
        for (const auto& input : tx->vin) {
            if (later_txids.find(input.prevout.hash) != later_txids.end()) {
                // The parent is a subsequent transaction in the package.
                return false;
            }
        }
        later_txids.erase(tx->GetHash());
    }
    return true;
}

bool IsConsistent(const Package& txns)
{
    // Don't allow any conflicting transactions, i.e. spending the same inputs, in a package.
    std::unordered_set<COutPoint, SaltedOutpointHasher> inputs_seen;
    for (const auto& tx : txns) {
        for (const auto& input : tx->vin) {
            if (inputs_seen.find(input.prevout) != inputs_seen.end()) {
                // This input is also present in another tx in the package.
                return false;
            }
        }
        // Batch-add all the inputs for a tx at a time. If we added them 1 at a time, we could
        // catch duplicate inputs within a single tx.  This is a more severe, consensus error,
        // and we want to report that from CheckTransaction instead.
        std::transform(tx->vin.cbegin(), tx->vin.cend(), std::inserter(inputs_seen, inputs_seen.end()),
                       [](const auto& input) { return input.prevout; });
    }
    return true;
}

bool IsPackageWellFormed(const Package& txns, PackageValidationState& state)
{
    const unsigned int package_count = txns.size();

    if (package_count > MAX_PACKAGE_COUNT) {
        return state.Invalid(PackageValidationResult::PCKG_POLICY, "package-too-many-transactions");
    }

    const int64_t total_size = std::accumulate(txns.cbegin(), txns.cend(), int64_t{0},
                               [](int64_t sum, const auto& tx) { return sum + GetVirtualTransactionSize(*tx); });
    // If the package only contains 1 tx, it's better to report the policy violation on individual tx size.
    if (package_count > 1 && total_size > MAX_PACKAGE_SIZE * 1000) {
        return state.Invalid(PackageValidationResult::PCKG_POLICY, "package-too-large");
    }

    // Require the package to be sorted in order of dependency, i.e. parents appear before children.
    // An unsorted package will fail anyway on missing-inputs, but it's better to quit earlier and
    // fail on something less ambiguous (missing-inputs could also be an orphan or trying to
    // spend nonexistent coins).
    if (!IsSorted(txns)) return state.Invalid(PackageValidationResult::PCKG_POLICY, "package-not-sorted");
    if (!IsConsistent(txns)) return state.Invalid(PackageValidationResult::PCKG_POLICY, "conflict-in-package");
    return true;
}

bool IsChildWithParents(const Package& package)
{
    assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx != nullptr;}));
    if (package.size() < 2) return false;

    // The package is expected to be sorted, so the last transaction is the child.
    const auto& child = package.back();
    std::unordered_set<uint256, SaltedTxidHasher> input_txids;
    std::transform(child->vin.cbegin(), child->vin.cend(),
                   std::inserter(input_txids, input_txids.end()),
                   [](const auto& input) { return input.prevout.hash; });

    // Every transaction must be a parent of the last transaction in the package.
    return std::all_of(package.cbegin(), package.cend() - 1,
                       [&input_txids](const auto& ptx) { return input_txids.count(ptx->GetHash()) > 0; });
}

// Calculates curr_tx's in-package ancestor set. If the tx spends another tx in the package, calls
// visit() for that transaction first, since any transaction's ancestor set includes its parents'
// ancestor sets. Transaction dependency cycles are not possible without breaking sha256 and
// duplicate transactions were checked in the AncestorPackage() ctor, so this won't recurse infinitely.
// After this function returns, curr_tx is guaranteed to be in the ancestor_subsets map.
void AncestorPackage::visit(const CTransactionRef& curr_tx)
{
    const uint256& curr_txid = curr_tx->GetHash();
    if (ancestor_subsets.count(curr_txid) > 0) return;
    std::set<uint256> my_ancestors;
    my_ancestors.insert(curr_txid);
    for (const auto& input : curr_tx->vin) {
        auto parent_tx = txid_to_tx.find(input.prevout.hash);
        if (parent_tx == txid_to_tx.end()) continue;
        if (ancestor_subsets.count(parent_tx->first) == 0) {
            visit(parent_tx->second);
        }
        auto parent_ancestor_set = ancestor_subsets.find(parent_tx->first);
        my_ancestors.insert(parent_ancestor_set->second.cbegin(), parent_ancestor_set->second.cend());
    }
    ancestor_subsets.insert(std::make_pair(curr_txid, my_ancestors));
}

AncestorPackage::AncestorPackage(const Package& txns_in)
{
    // Duplicate transactions are not allowed, as they will result in infinite visit() recursion.
    Assume(IsConsistent(txns_in));
    if (txns_in.empty() || !IsConsistent(txns_in)) return;
    // Populate txid_to_tx for quick lookup
    std::transform(txns_in.cbegin(), txns_in.cend(), std::inserter(txid_to_tx, txid_to_tx.end()),
            [](const auto& tx) { return std::make_pair(tx->GetHash(), tx); });
    // DFS-based algorithm to sort transactions by ancestor count and populate ancestor_subsets cache.
    // Best case runtime is if the package is already sorted and no recursive calls happen.
    // Exclusion from ancestor_subsets is equivalent to not yet being fully processed.
    size_t i{0};
    while (ancestor_subsets.size() < txns_in.size() && i < txns_in.size()) {
        const auto& tx = txns_in[i];
        if (ancestor_subsets.count(tx->GetHash()) == 0) visit(tx);
        Assume(ancestor_subsets.count(tx->GetHash()) == 1);
        ++i;
    }
    txns = txns_in;
    // Sort by the number of in-package ancestors.
    std::sort(txns.begin(), txns.end(), [&](const CTransactionRef& a, const CTransactionRef& b) -> bool {
        auto a_ancestors = ancestor_subsets.find(a->GetHash());
        auto b_ancestors = ancestor_subsets.find(b->GetHash());
        return a_ancestors->second.size() < b_ancestors->second.size();
    });
    Assume(IsSorted(txns));
    Assume(ancestor_subsets.find(txns.back()->GetHash()) != ancestor_subsets.end());
    is_ancestor_package = ancestor_subsets.find(txns.back()->GetHash())->second.size() == txns.size();
}

std::optional<std::vector<CTransactionRef>> AncestorPackage::GetAncestorSet(const CTransactionRef& tx)
{
    auto ancestor_set = ancestor_subsets.find(tx->GetHash());
    if (ancestor_set == ancestor_subsets.end()) return std::nullopt;
    std::vector<CTransactionRef> result;
    for (const auto& txid : ancestor_set->second) {
        if (banned_txns.find(txid) != banned_txns.end()) {
            return std::nullopt;
        }
    }
    result.reserve(ancestor_set->second.size());
    for (const auto& txid : ancestor_set->second) {
        auto it = txid_to_tx.find(txid);
        if (excluded_txns.find(txid) == excluded_txns.end()) {
            result.push_back(it->second);
        }
    }
    std::sort(result.begin(), result.end(), [&](const CTransactionRef& a, const CTransactionRef& b) -> bool {
        auto a_ancestors = ancestor_subsets.find(a->GetHash());
        auto b_ancestors = ancestor_subsets.find(b->GetHash());
        return a_ancestors->second.size() < b_ancestors->second.size();
    });
    return result;
}

void AncestorPackage::Exclude(const CTransactionRef& transaction)
{
    if (ancestor_subsets.count(transaction->GetHash()) == 0) return;
    excluded_txns.insert(transaction->GetHash());
}
void AncestorPackage::Ban(const CTransactionRef& transaction)
{
    if (ancestor_subsets.count(transaction->GetHash()) == 0) return;
    banned_txns.insert(transaction->GetHash());
}
