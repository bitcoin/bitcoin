// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <policy/ancestor_packages.h>

#include <node/mini_miner.h>
#include <util/check.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>


/**Comparator for sorting m_txns which contains reference wrappers.*/
struct CompareEntry {
    template<typename T>
    bool operator()(const std::reference_wrapper<T> a, const std::reference_wrapper<T> b) const
    {
        return a.get() < b.get();
    }
};
// Calculates curr_tx's in-package ancestor set. If the tx spends another tx in the package, calls
// visit() for that transaction first, since any transaction's ancestor set includes its parents'
// ancestor sets. Transaction dependency cycles are not possible without breaking sha256 and
// duplicate transactions were checked in the AncestorPackage() ctor, so this won't recurse infinitely.
// After this function returns, entry is guaranteed to contain a non-empty ancestor_subset.
void AncestorPackage::visit(const CTransactionRef& curr_tx)
{
    const Txid& curr_txid = curr_tx->GetHash();
    auto& entry = m_txid_to_entry.at(curr_txid);
    if (!entry.ancestor_subset.empty()) return;
    std::set<Txid> my_ancestors;
    my_ancestors.insert(curr_txid);
    for (const auto& input : curr_tx->vin) {
        const auto& parent_txid = Txid::FromUint256(input.prevout.hash);
        if (m_txid_to_entry.count(parent_txid) == 0) continue;
        auto parent_entry = m_txid_to_entry.at(parent_txid);
        if (parent_entry.ancestor_subset.empty()) {
            visit(parent_entry.tx);
        }
        auto parent_ancestor_set = m_txid_to_entry.at(parent_txid).ancestor_subset;
        Assume(!parent_ancestor_set.empty());
        // This recursive call should not have included ourselves; it should be impossible for this
        // tx to be both an ancestor and a descendant of us.
        Assume(m_txid_to_entry.at(curr_txid).ancestor_subset.empty());
        my_ancestors.insert(parent_ancestor_set.cbegin(), parent_ancestor_set.cend());
    }
    entry.ancestor_subset = std::move(my_ancestors);
}

AncestorPackage::AncestorPackage(const Package& txns_in)
{
    if (txns_in.empty()) return;
    // Duplicate transactions are not allowed, as they will result in infinite visit() recursion.
    if (!IsConsistentPackage(txns_in)) return;

    // Populate m_txid_to_entry for quick lookup
    for (const auto& tx : txns_in) {
        m_txid_to_entry.emplace(tx->GetHash(), PackageEntry{tx});
        m_txns.emplace_back(std::ref(m_txid_to_entry.at(tx->GetHash())));
    }

    // Every tx must have a unique txid. We are indexing by Txid to make it easy to look up parents
    // using prevouts. IsConsistentPackage() should have checked for duplicate transactions.
    if (!Assume(txns_in.size() == m_txid_to_entry.size())) return;

    // DFS-based algorithm to sort transactions by ancestor count and populate ancestor_subset.
    // Best case runtime is if the package is already sorted and no recursive calls happen.
    // An empty PackageEntry::ancestor_subset is equivalent to not yet being processed.
    for (const auto& tx : txns_in) {
        if (m_txid_to_entry.at(tx->GetHash()).ancestor_subset.empty()) visit(tx);
        Assume(!m_txid_to_entry.at(tx->GetHash()).ancestor_subset.empty());
    }
    // Sort by the number of in-package ancestors.
    std::vector<std::reference_wrapper<PackageEntry>> txns_copy(m_txns);
    std::sort(m_txns.begin(), m_txns.end(), CompareEntry());
    if (!Assume(m_txns.size() == txns_in.size() && IsTopoSortedPackage(Txns()))) {
        // If something went wrong with the sorting, just revert to the original order.
        m_txns = txns_copy;
    }
    // This package is ancestor package-shaped if every transaction is an ancestor of the last tx.
    // We just check if the number of unique txids in ancestor_subset is equal to the number of
    // transactions in the package.
    m_ancestor_package_shaped = m_txns.back().get().ancestor_subset.size() == m_txns.size();
    // Now populate the descendant caches
    for (const auto& [txid, entry] : m_txid_to_entry) {
        for (const auto& anc_txid : entry.ancestor_subset) {
            m_txid_to_entry.at(anc_txid).descendant_subset.insert(txid);
        }
    }
}

Package AncestorPackage::Txns() const
{
    Package result;
    std::transform(m_txns.cbegin(), m_txns.cend(), std::back_inserter(result),
                   [](const auto refentry){ return refentry.get().tx; });
    return result;
}

Package AncestorPackage::FilteredTxns() const
{
    Package result;
    for (const auto entryref : m_txns) {
        if (entryref.get().m_state == PackageEntry::State::PACKAGE) result.emplace_back(entryref.get().tx);
    }
    return result;
}

std::optional<std::vector<CTransactionRef>> AncestorPackage::FilteredAncestorSet(const CTransactionRef& tx) const
{
    const auto& entry_it = m_txid_to_entry.find(tx->GetHash());
    if (entry_it == m_txid_to_entry.end()) return std::nullopt;
    const auto& entry = entry_it->second;
    if (entry.m_state == PackageEntry::State::DANGLING) return std::nullopt;

    std::vector<CTransactionRef> result;
    result.reserve(entry.ancestor_subset.size());
    for (const auto entryref : m_txns) {
        if (entry.ancestor_subset.count(entryref.get().tx->GetHash()) > 0) {
            // All non-PACKAGE ancestor_subset transactions should have been removed when the
            // transaction was first skipped.
            if (Assume(entryref.get().m_state == PackageEntry::State::PACKAGE)) {
                result.emplace_back(entryref.get().tx);
            }
        }
    }
    return result;
}

std::optional<AncestorPackage::SubpackageInfo> AncestorPackage::FilteredSubpackageInfo(const CTransactionRef& tx) const
{
    const auto& entry_it = m_txid_to_entry.find(tx->GetHash());
    if (entry_it == m_txid_to_entry.end()) return std::nullopt;
    const auto& entry = entry_it->second;
    if (entry.m_state == PackageEntry::State::DANGLING || !entry.fee.has_value() || !entry.vsize.has_value()) return std::nullopt;

    CAmount total_fee{0};
    int64_t total_vsize{0};
    for (const auto& txid : entry.ancestor_subset) {
        const auto& anc_entry = m_txid_to_entry.at(txid);
        // All non-PACKAGE ancestor_subset transactions should have been removed when the
        // transaction was first skipped.
        if (Assume(anc_entry.m_state == PackageEntry::State::PACKAGE)) {
            if (anc_entry.fee.has_value() && anc_entry.vsize.has_value()) {
                total_fee += anc_entry.fee.value();
                total_vsize += anc_entry.vsize.value();
            } else {
                // If any fee or vsize information is missing, we can't return an accurate result.
                return std::nullopt;
            }
        }
    }

    return SubpackageInfo{
        .m_self_vsize = entry.vsize.value(),
        .m_ancestor_vsize = total_vsize,
        .m_self_fee = entry.fee.value(),
        .m_ancestor_fee = total_fee
    };
}

void AncestorPackage::MarkAsInMempool(const CTransactionRef& transaction)
{
    const auto& txid{transaction->GetHash()};
    if (m_txid_to_entry.count(txid) == 0) return;

    Assume(m_txid_to_entry.at(txid).m_state != PackageEntry::State::DANGLING);
    m_txid_to_entry.at(txid).m_state = PackageEntry::State::MEMPOOL;

    // Delete this tx from the descendant's ancestor_subset.
    for (const auto& descendant_txid : m_txid_to_entry.at(transaction->GetHash()).descendant_subset) {
        m_txid_to_entry.at(descendant_txid).ancestor_subset.erase(txid);
    }
}
void AncestorPackage::IsDanglingWithDescendants(const CTransactionRef& transaction)
{
    const auto& txid{transaction->GetHash()};
    if (m_txid_to_entry.count(txid) == 0) return;
    m_txid_to_entry.at(txid).m_state = PackageEntry::State::DANGLING;

    // Set all descendants to DANGLING, as they depend on something that is being discarded.
    // Delete this tx from the descendant's ancestor_subset.
    for (const auto& descendant_txid : m_txid_to_entry.at(txid).descendant_subset) {
        Assume(m_txid_to_entry.at(descendant_txid).m_state != PackageEntry::State::MEMPOOL);
        m_txid_to_entry.at(descendant_txid).m_state = PackageEntry::State::DANGLING;
        m_txid_to_entry.at(descendant_txid).ancestor_subset.erase(txid);
    }
}

void AncestorPackage::AddFeeAndVsize(const Txid& txid, CAmount fee, int64_t vsize)
{
    if (auto it = m_txid_to_entry.find(txid); it != m_txid_to_entry.end()) {
        it->second.fee = fee;
        it->second.vsize = vsize;
    }
}

bool AncestorPackage::LinearizeWithFees()
{
    if (!m_ancestor_package_shaped) return false;
    // All fee and vsize information for PACKAGE txns must be available to do linearization.
    if (!std::all_of(m_txid_to_entry.cbegin(), m_txid_to_entry.cend(),
        [](const auto& entry) { return entry.second.m_state != PackageEntry::State::PACKAGE ||
                                 (entry.second.fee.has_value() && entry.second.vsize.has_value()); })) {
        return false;
    }
    // Clear any previously-calculated mining sequences for all transactions.
    for (auto& [_, entry] : m_txid_to_entry) entry.mining_sequence = std::nullopt;

    // Construct mempool entries for MiniMiner
    std::vector<node::MiniMinerMempoolEntry> miniminer_info;
    std::map<Txid, std::set<Txid>> descendant_caches;
    for (const auto& [txid, entry] : m_txid_to_entry) {
        // Skip transactions that are not being considered for submission.
        if (entry.m_state != PackageEntry::State::PACKAGE) continue;

        auto fee_and_vsize_info = FilteredSubpackageInfo(entry.tx);
        if (!Assume(fee_and_vsize_info.has_value())) continue;
        miniminer_info.emplace_back(/*tx_in=*/entry.tx,
                                    /*vsize_self=*/fee_and_vsize_info->m_self_vsize,
                                    /*vsize_ancestor=*/fee_and_vsize_info->m_ancestor_vsize,
                                    /*fee_self=*/fee_and_vsize_info->m_self_fee,
                                    /*fee_ancestor=*/fee_and_vsize_info->m_ancestor_fee);

        // Provide descendant cache, again skipping transactions that are MEMPOOL or DANGLING.
        std::set<Txid>& descendant_cache_to_populate = descendant_caches.try_emplace(txid).first->second;
        for (const auto& txid : entry.descendant_subset) {
            if (!Assume(m_txid_to_entry.count(txid) > 0)) continue;

            const auto& entry = m_txid_to_entry.at(txid);
            if (entry.m_state == PackageEntry::State::PACKAGE) {
                descendant_cache_to_populate.insert(txid);
            } else {
                // Descendant shouldn't be MEMPOOL as that would mean submitting a tx without its ancestor.
                Assume(entry.m_state == PackageEntry::State::DANGLING);
            }
        }
    }

    // Use MiniMiner to calculate the order in which these transactions would be selected for mining.
    node::MiniMiner miniminer(miniminer_info, descendant_caches);
    if (!miniminer.IsReadyToCalculate()) return false;
    for (const auto& [txid, mining_sequence] : miniminer.Linearize()) {
        m_txid_to_entry.at(txid).mining_sequence = mining_sequence;
    }
    // Sort again, this time including mining scores and individual feerates.
    std::vector<std::reference_wrapper<PackageEntry>> txns_copy(m_txns);
    std::sort(m_txns.begin(), m_txns.end(), CompareEntry());
    Assume(std::all_of(m_txid_to_entry.cbegin(), m_txid_to_entry.cend(), [](const auto& entry) {
        bool should_have_sequence = (entry.second.m_state == PackageEntry::State::PACKAGE);
        return entry.second.mining_sequence.has_value() == should_have_sequence;
    }));

    // If something went wrong with the linearization, just revert to the original order and reset
    // mining_sequences.
    if (!Assume(IsTopoSortedPackage(Txns()))) {
        m_txns = txns_copy;
        for (auto& [txid, entry] : m_txid_to_entry) {
            entry.mining_sequence = std::nullopt;
        }
        Assume(IsTopoSortedPackage(Txns()));
        return false;
    }
    return true;
}
