// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mini_miner.h>

#include <boost/multi_index/detail/hash_index_iterator.hpp>
#include <boost/operators.hpp>
#include <consensus/amount.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>

#include <algorithm>
#include <numeric>
#include <utility>

namespace node {

MiniMiner::MiniMiner(const CTxMemPool& mempool, const std::vector<COutPoint>& outpoints)
{
    LOCK(mempool.cs);
    // Find which outpoints to calculate bump fees for.
    // Anything that's spent by the mempool is to-be-replaced
    // Anything otherwise unavailable just has a bump fee of 0
    for (const auto& outpoint : outpoints) {
        if (!mempool.exists(GenTxid::Txid(outpoint.hash))) {
            // This UTXO is either confirmed or not yet submitted to mempool.
            // If it's confirmed, no bump fee is required.
            // If it's not yet submitted, we have no information, so return 0.
            m_bump_fees.emplace(outpoint, 0);
            continue;
        }

        // UXTO is created by transaction in mempool, add to map.
        // Note: This will either create a missing entry or add the outpoint to an existing entry
        m_requested_outpoints_by_txid[outpoint.hash].push_back(outpoint);

        if (const auto ptx{mempool.GetConflictTx(outpoint)}) {
            // This outpoint is already being spent by another transaction in the mempool. We
            // assume that the caller wants to replace this transaction and its descendants. It
            // would be unusual for the transaction to have descendants as the wallet won’t normally
            // attempt to replace transactions with descendants. If the outpoint is from a mempool
            // transaction, we still need to calculate its ancestors bump fees (added to
            // m_requested_outpoints_by_txid below), but after removing the to-be-replaced entries.
            //
            // Note that the descendants of a transaction include the transaction itself. Also note,
            // that this is only calculating bump fees. RBF fee rules should be handled separately.
            CTxMemPool::setEntries descendants;
            mempool.CalculateDescendants(mempool.GetIter(ptx->GetHash()).value(), descendants);
            for (const auto& desc_txiter : descendants) {
                m_to_be_replaced.insert(desc_txiter->GetTx().GetHash());
            }
        }
    }

    // No unconfirmed UTXOs, so nothing mempool-related needs to be calculated.
    if (m_requested_outpoints_by_txid.empty()) return;

    // Calculate the cluster and construct the entry map.
    std::vector<uint256> txids_needed;
    txids_needed.reserve(m_requested_outpoints_by_txid.size());
    for (const auto& [txid, _]: m_requested_outpoints_by_txid) {
        txids_needed.push_back(txid);
    }
    const auto cluster = mempool.GatherClusters(txids_needed);
    if (cluster.empty()) {
        // An empty cluster means that at least one of the transactions is missing from the mempool
        // (should not be possible given processing above) or DoS limit was hit.
        m_ready_to_calculate = false;
        return;
    }

    // Add every entry to m_entries_by_txid and m_entries, except the ones that will be replaced.
    for (const auto& txiter : cluster) {
        if (!m_to_be_replaced.count(txiter->GetTx().GetHash())) {
            size_t ancestor_count{0};
            size_t ancestor_size{0};
            CAmount ancestor_fee{0};
            mempool.CalculateAncestorData(*txiter, ancestor_count, ancestor_size, ancestor_fee);
            auto [mapiter, success] = m_entries_by_txid.emplace(txiter->GetTx().GetHash(), MiniMinerMempoolEntry(txiter->GetSharedTx(), txiter->GetTxSize(), int64_t(ancestor_size), txiter->GetModifiedFee(), ancestor_fee));
            m_entries.push_back(mapiter);
        } else {
            auto outpoints_it = m_requested_outpoints_by_txid.find(txiter->GetTx().GetHash());
            if (outpoints_it != m_requested_outpoints_by_txid.end()) {
                // This UTXO is the output of a to-be-replaced transaction. Bump fee is 0; spending
                // this UTXO is impossible as it will no longer exist after the replacement.
                for (const auto& outpoint : outpoints_it->second) {
                    m_bump_fees.emplace(outpoint, 0);
                }
                m_requested_outpoints_by_txid.erase(outpoints_it);
            }
        }
    }

    // Build the m_descendant_set_by_txid cache.
    for (const auto& txiter : cluster) {
        const auto& txid = txiter->GetTx().GetHash();
        // Cache descendants for future use. Unlike the real mempool, a descendant MiniMinerMempoolEntry
        // will not exist without its ancestor MiniMinerMempoolEntry, so these sets won't be invalidated.
        std::vector<MockEntryMap::iterator> cached_descendants;
        const bool remove{m_to_be_replaced.count(txid) > 0};
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(txiter, descendants);
        Assume(descendants.count(txiter) > 0);
        for (const auto& desc_txiter : descendants) {
            const auto txid_desc = desc_txiter->GetTx().GetHash();
            const bool remove_desc{m_to_be_replaced.count(txid_desc) > 0};
            auto desc_it{m_entries_by_txid.find(txid_desc)};
            Assume((desc_it == m_entries_by_txid.end()) == remove_desc);
            if (remove) Assume(remove_desc);
            // It's possible that remove=false but remove_desc=true.
            if (!remove && !remove_desc) {
                cached_descendants.push_back(desc_it);
            }
        }
        if (remove) {
            Assume(cached_descendants.empty());
        } else {
            m_descendant_set_by_txid.emplace(txid, cached_descendants);
        }
    }

    // Release the mempool lock; we now have all the information we need for a subset of the entries
    // we care about. We will solely operate on the MiniMinerMempoolEntry map from now on.
    Assume(m_in_block.empty());
    Assume(m_requested_outpoints_by_txid.size() <= outpoints.size());
    SanityCheck();
}

MiniMiner::MiniMiner(const std::vector<MiniMinerMempoolEntry>& manual_entries,
                     const std::map<Txid, std::set<Txid>>& descendant_caches)
{
    for (const auto& entry : manual_entries) {
        const auto& txid = entry.GetTx().GetHash();
        // We need to know the descendant set of every transaction.
        if (!Assume(descendant_caches.count(txid) > 0)) {
            m_ready_to_calculate = false;
            return;
        }
        // Just forward these args onto MiniMinerMempoolEntry
        auto [mapiter, success] = m_entries_by_txid.emplace(txid, entry);
        // Txids must be unique; this txid shouldn't already be an entry in m_entries_by_txid
        if (Assume(success)) m_entries.push_back(mapiter);
    }
    // Descendant cache is already built, but we need to translate them to m_entries_by_txid iters.
    for (const auto& [txid, desc_txids] : descendant_caches) {
        // Descendant cache should include at least the tx itself.
        if (!Assume(!desc_txids.empty())) {
            m_ready_to_calculate = false;
            return;
        }
        std::vector<MockEntryMap::iterator> descendants;
        for (const auto& desc_txid : desc_txids) {
            auto desc_it{m_entries_by_txid.find(desc_txid)};
            // Descendants should only include transactions with corresponding entries.
            if (!Assume(desc_it != m_entries_by_txid.end())) {
                m_ready_to_calculate = false;
                return;
            } else {
                descendants.emplace_back(desc_it);
            }
        }
        m_descendant_set_by_txid.emplace(txid, descendants);
    }
    Assume(m_to_be_replaced.empty());
    Assume(m_requested_outpoints_by_txid.empty());
    Assume(m_bump_fees.empty());
    Assume(m_inclusion_order.empty());
    SanityCheck();
}

// Compare by min(ancestor feerate, individual feerate), then txid
//
// Under the ancestor-based mining approach, high-feerate children can pay for parents, but high-feerate
// parents do not incentive inclusion of their children. Therefore the mining algorithm only considers
// transactions for inclusion on basis of the minimum of their own feerate or their ancestor feerate.
struct AncestorFeerateComparator
{
    template<typename I>
    bool operator()(const I& a, const I& b) const {
        auto min_feerate = [](const MiniMinerMempoolEntry& e) -> FeeFrac {
            FeeFrac self_feerate(e.GetModifiedFee(), e.GetTxSize());
            FeeFrac ancestor_feerate(e.GetModFeesWithAncestors(), e.GetSizeWithAncestors());
            return std::min(ancestor_feerate, self_feerate);
        };
        FeeFrac a_feerate{min_feerate(a->second)};
        FeeFrac b_feerate{min_feerate(b->second)};
        if (a_feerate != b_feerate) {
            return a_feerate > b_feerate;
        }
        // Use txid as tiebreaker for stable sorting
        return a->first < b->first;
    }
};

void MiniMiner::DeleteAncestorPackage(const std::set<MockEntryMap::iterator, IteratorComparator>& ancestors)
{
    Assume(ancestors.size() >= 1);
    // "Mine" all transactions in this ancestor set.
    for (auto& anc : ancestors) {
        Assume(m_in_block.count(anc->first) == 0);
        m_in_block.insert(anc->first);
        m_total_fees += anc->second.GetModifiedFee();
        m_total_vsize += anc->second.GetTxSize();
        auto it = m_descendant_set_by_txid.find(anc->first);
        // Each entry’s descendant set includes itself
        Assume(it != m_descendant_set_by_txid.end());
        for (auto& descendant : it->second) {
            // If these fail, we must be double-deducting.
            Assume(descendant->second.GetModFeesWithAncestors() >= anc->second.GetModifiedFee());
            Assume(descendant->second.GetSizeWithAncestors() >= anc->second.GetTxSize());
            descendant->second.UpdateAncestorState(-anc->second.GetTxSize(), -anc->second.GetModifiedFee());
        }
    }
    // Delete these entries.
    for (const auto& anc : ancestors) {
        m_descendant_set_by_txid.erase(anc->first);
        // The above loop should have deducted each ancestor's size and fees from each of their
        // respective descendants exactly once.
        Assume(anc->second.GetModFeesWithAncestors() == 0);
        Assume(anc->second.GetSizeWithAncestors() == 0);
        auto vec_it = std::find(m_entries.begin(), m_entries.end(), anc);
        Assume(vec_it != m_entries.end());
        m_entries.erase(vec_it);
        m_entries_by_txid.erase(anc);
    }
}

void MiniMiner::SanityCheck() const
{
    // m_entries, m_entries_by_txid, and m_descendant_set_by_txid all same size
    Assume(m_entries.size() == m_entries_by_txid.size());
    Assume(m_entries.size() == m_descendant_set_by_txid.size());
    // Cached ancestor values should be at least as large as the transaction's own fee and size
    Assume(std::all_of(m_entries.begin(), m_entries.end(), [](const auto& entry) {
        return entry->second.GetSizeWithAncestors() >= entry->second.GetTxSize() &&
               entry->second.GetModFeesWithAncestors() >= entry->second.GetModifiedFee();}));
    // None of the entries should be to-be-replaced transactions
    Assume(std::all_of(m_to_be_replaced.begin(), m_to_be_replaced.end(),
        [&](const auto& txid){return m_entries_by_txid.find(txid) == m_entries_by_txid.end();}));
}

void MiniMiner::BuildMockTemplate(std::optional<CFeeRate> target_feerate)
{
    const auto num_txns{m_entries_by_txid.size()};
    uint32_t sequence_num{0};
    while (!m_entries_by_txid.empty()) {
        // Sort again, since transaction removal may change some m_entries' ancestor feerates.
        std::sort(m_entries.begin(), m_entries.end(), AncestorFeerateComparator());

        // Pick highest ancestor feerate entry.
        auto best_iter = m_entries.begin();
        Assume(best_iter != m_entries.end());
        const auto ancestor_package_size = (*best_iter)->second.GetSizeWithAncestors();
        const auto ancestor_package_fee = (*best_iter)->second.GetModFeesWithAncestors();
        // Stop here. Everything that didn't "make it into the block" has bumpfee.
        if (target_feerate.has_value() &&
            ancestor_package_fee < target_feerate->GetFee(ancestor_package_size)) {
            break;
        }

        // Calculate ancestors on the fly. This lookup should be fairly cheap, and ancestor sets
        // change at every iteration, so this is more efficient than maintaining a cache.
        std::set<MockEntryMap::iterator, IteratorComparator> ancestors;
        {
            std::set<MockEntryMap::iterator, IteratorComparator> to_process;
            to_process.insert(*best_iter);
            while (!to_process.empty()) {
                auto iter = to_process.begin();
                Assume(iter != to_process.end());
                ancestors.insert(*iter);
                for (const auto& input : (*iter)->second.GetTx().vin) {
                    if (auto parent_it{m_entries_by_txid.find(input.prevout.hash)}; parent_it != m_entries_by_txid.end()) {
                        if (ancestors.count(parent_it) == 0) {
                            to_process.insert(parent_it);
                        }
                    }
                }
                to_process.erase(iter);
            }
        }
        // Track the order in which transactions were selected.
        for (const auto& ancestor : ancestors) {
            m_inclusion_order.emplace(Txid::FromUint256(ancestor->first), sequence_num);
        }
        DeleteAncestorPackage(ancestors);
        SanityCheck();
        ++sequence_num;
    }
    if (!target_feerate.has_value()) {
        Assume(m_in_block.size() == num_txns);
    } else {
        Assume(m_in_block.empty() || m_total_fees >= target_feerate->GetFee(m_total_vsize));
    }
    Assume(m_in_block.empty() || sequence_num > 0);
    Assume(m_in_block.size() == m_inclusion_order.size());
    // Do not try to continue building the block template with a different feerate.
    m_ready_to_calculate = false;
}


std::map<Txid, uint32_t> MiniMiner::Linearize()
{
    BuildMockTemplate(std::nullopt);
    return m_inclusion_order;
}

std::map<COutPoint, CAmount> MiniMiner::CalculateBumpFees(const CFeeRate& target_feerate)
{
    if (!m_ready_to_calculate) return {};
    // Build a block template until the target feerate is hit.
    BuildMockTemplate(target_feerate);

    // Each transaction that "made it into the block" has a bumpfee of 0, i.e. they are part of an
    // ancestor package with at least the target feerate and don't need to be bumped.
    for (const auto& txid : m_in_block) {
        // Not all of the block transactions were necessarily requested.
        auto it = m_requested_outpoints_by_txid.find(txid);
        if (it != m_requested_outpoints_by_txid.end()) {
            for (const auto& outpoint : it->second) {
                m_bump_fees.emplace(outpoint, 0);
            }
            m_requested_outpoints_by_txid.erase(it);
        }
    }

    // A transactions and its ancestors will only be picked into a block when
    // both the ancestor set feerate and the individual feerate meet the target
    // feerate.
    //
    // We had to convince ourselves that after running the mini miner and
    // picking all eligible transactions into our MockBlockTemplate, there
    // could still be transactions remaining that have a lower individual
    // feerate than their ancestor feerate. So here is an example:
    //
    //               ┌─────────────────┐
    //               │                 │
    //               │   Grandparent   │
    //               │    1700 vB      │
    //               │    1700 sats    │                    Target feerate: 10    s/vB
    //               │       1 s/vB    │    GP Ancestor Set Feerate (ASFR):  1    s/vB
    //               │                 │                           P1_ASFR:  9.84 s/vB
    //               └──────▲───▲──────┘                           P2_ASFR:  2.47 s/vB
    //                      │   │                                   C_ASFR: 10.27 s/vB
    // ┌───────────────┐    │   │    ┌──────────────┐
    // │               ├────┘   └────┤              │             ⇒ C_FR < TFR < C_ASFR
    // │   Parent 1    │             │   Parent 2   │
    // │    200 vB     │             │    200 vB    │
    // │  17000 sats   │             │   3000 sats  │
    // │     85 s/vB   │             │     15 s/vB  │
    // │               │             │              │
    // └───────────▲───┘             └───▲──────────┘
    //             │                     │
    //             │    ┌───────────┐    │
    //             └────┤           ├────┘
    //                  │   Child   │
    //                  │  100 vB   │
    //                  │  900 sats │
    //                  │    9 s/vB │
    //                  │           │
    //                  └───────────┘
    //
    // We therefore calculate both the bump fee that is necessary to elevate
    // the individual transaction to the target feerate:
    //         target_feerate × tx_size - tx_fees
    // and the bump fee that is necessary to bump the entire ancestor set to
    // the target feerate:
    //         target_feerate × ancestor_set_size - ancestor_set_fees
    // By picking the maximum from the two, we ensure that a transaction meets
    // both criteria.
    for (const auto& [txid, outpoints] : m_requested_outpoints_by_txid) {
        auto it = m_entries_by_txid.find(txid);
        Assume(it != m_entries_by_txid.end());
        if (it != m_entries_by_txid.end()) {
            Assume(target_feerate.GetFee(it->second.GetSizeWithAncestors()) > std::min(it->second.GetModifiedFee(), it->second.GetModFeesWithAncestors()));
            CAmount bump_fee_with_ancestors = target_feerate.GetFee(it->second.GetSizeWithAncestors()) - it->second.GetModFeesWithAncestors();
            CAmount bump_fee_individual = target_feerate.GetFee(it->second.GetTxSize()) - it->second.GetModifiedFee();
            const CAmount bump_fee{std::max(bump_fee_with_ancestors, bump_fee_individual)};
            Assume(bump_fee >= 0);
            for (const auto& outpoint : outpoints) {
                m_bump_fees.emplace(outpoint, bump_fee);
            }
        }
    }
    return m_bump_fees;
}

std::optional<CAmount> MiniMiner::CalculateTotalBumpFees(const CFeeRate& target_feerate)
{
    if (!m_ready_to_calculate) return std::nullopt;
    // Build a block template until the target feerate is hit.
    BuildMockTemplate(target_feerate);

    // All remaining ancestors that are not part of m_in_block must be bumped, but no other relatives
    std::set<MockEntryMap::iterator, IteratorComparator> ancestors;
    std::set<MockEntryMap::iterator, IteratorComparator> to_process;
    for (const auto& [txid, outpoints] : m_requested_outpoints_by_txid) {
        // Skip any ancestors that already have a miner score higher than the target feerate
        // (already "made it" into the block)
        if (m_in_block.count(txid)) continue;
        auto iter = m_entries_by_txid.find(txid);
        if (iter == m_entries_by_txid.end()) continue;
        to_process.insert(iter);
        ancestors.insert(iter);
    }

    std::set<uint256> has_been_processed;
    while (!to_process.empty()) {
        auto iter = to_process.begin();
        const CTransaction& tx = (*iter)->second.GetTx();
        for (const auto& input : tx.vin) {
            if (auto parent_it{m_entries_by_txid.find(input.prevout.hash)}; parent_it != m_entries_by_txid.end()) {
                if (!has_been_processed.count(input.prevout.hash)) {
                    to_process.insert(parent_it);
                }
                ancestors.insert(parent_it);
            }
        }
        has_been_processed.insert(tx.GetHash());
        to_process.erase(iter);
    }
    const auto ancestor_package_size = std::accumulate(ancestors.cbegin(), ancestors.cend(), int64_t{0},
        [](int64_t sum, const auto it) {return sum + it->second.GetTxSize();});
    const auto ancestor_package_fee = std::accumulate(ancestors.cbegin(), ancestors.cend(), CAmount{0},
        [](CAmount sum, const auto it) {return sum + it->second.GetModifiedFee();});
    return target_feerate.GetFee(ancestor_package_size) - ancestor_package_fee;
}
} // namespace node
