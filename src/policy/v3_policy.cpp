// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/v3_policy.h>

#include <coins.h>
#include <consensus/amount.h>
#include <logging.h>
#include <tinyformat.h>

#include <numeric>
#include <vector>

std::optional<std::tuple<uint256, uint256, bool>> CheckV3Inheritance(const Package& package)
{
    assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx != nullptr;}));
    // If all transactions are V3, we can stop here.
    if (std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx->nVersion == 3;})) {
        return std::nullopt;
    }
    // If all transactions are non-V3, we can stop here.
    if (std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx->nVersion != 3;})) {
        return std::nullopt;
    }
    // Look for a V3 transaction spending a non-V3 or vice versa.
    std::unordered_map<uint256, uint256, SaltedTxidHasher> v3_txid_to_wtxid;
    std::unordered_map<uint256, uint256, SaltedTxidHasher> non_v3_txid_to_wtxid;
    for (const auto& tx : package) {
        if (tx->nVersion == 3) {
            v3_txid_to_wtxid.emplace(tx->GetHash(), tx->GetWitnessHash());
        } else {
            non_v3_txid_to_wtxid.emplace(tx->GetHash(), tx->GetWitnessHash());
        }
    }
    for (const auto& tx : package) {
        if (tx->nVersion == 3) {
            for (const auto& input : tx->vin) {
                if (auto it = non_v3_txid_to_wtxid.find(input.prevout.hash); it != non_v3_txid_to_wtxid.end()) {
                    return std::make_tuple(it->second, tx->GetWitnessHash(), true);
                }
            }
        } else {
            for (const auto& input : tx->vin) {
                if (auto it = v3_txid_to_wtxid.find(input.prevout.hash); it != v3_txid_to_wtxid.end()) {
                    return std::make_tuple(it->second, tx->GetWitnessHash(), false);
                }
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string> CheckV3Inheritance(const CTransactionRef& ptx,
                                              const CTxMemPool::setEntries& ancestors)
{
    for (const auto& entry : ancestors) {
        if (ptx->nVersion != 3 && entry->GetTx().nVersion == 3) {
            return strprintf("tx that spends from %s must be nVersion=3",
                             entry->GetTx().GetWitnessHash().ToString());
        } else if (ptx->nVersion == 3 && entry->GetTx().nVersion != 3) {
            return strprintf("v3 tx cannot spend from %s which is not nVersion=3",
                             entry->GetTx().GetWitnessHash().ToString());
        }
    }
    return std::nullopt;
}

std::optional<std::string> ApplyV3Rules(const CTransactionRef& ptx,
                                        const CTxMemPool::setEntries& ancestors,
                                        const std::set<uint256>& direct_conflicts)
{
    // These rules only apply to transactions with nVersion=3.
    if (ptx->nVersion != 3) return std::nullopt;

    if (ancestors.size() + 1 > V3_ANCESTOR_LIMIT) {
        return strprintf("tx %s would have too many ancestors", ptx->GetWitnessHash().ToString());
    }
    if (ancestors.empty()) {
        return std::nullopt;
    } else {
        const auto tx_weight{GetTransactionWeight(*ptx)};
        // If this transaction spends V3 parents, it cannot be too large.
        if (tx_weight > V3_CHILD_MAX_WEIGHT) {
            return strprintf("v3 child tx is too big: %u weight units", tx_weight);
        }
        // Any ancestor of a V3 transaction must also be V3.
        const auto& parent_entry = *ancestors.begin();
        if (parent_entry->GetTx().nVersion != 3) {
            return strprintf("v3 tx cannot spend from %s which is not nVersion=3",
                             parent_entry->GetTx().GetWitnessHash().ToString());
        }
        // If there are any ancestors, this is the only child allowed. The parent cannot have any
        // other descendants.
        const auto& children = parent_entry->GetMemPoolChildrenConst();
        // Don't double-count a transaction that is going to be replaced. This logic assumes that
        // any descendant of the V3 transaction is a direct child, which makes sense because a V3
        // transaction can only have 1 descendant.
        const bool child_will_be_replaced = !children.empty() &&
            std::any_of(children.cbegin(), children.cend(),
                [&direct_conflicts](const CTxMemPoolEntry& child){return direct_conflicts.count(child.GetTx().GetHash()) > 0;});
        if (parent_entry->GetCountWithDescendants() + 1 > V3_DESCENDANT_LIMIT && !child_will_be_replaced) {
            return strprintf("tx %u would exceed descendant count limit", parent_entry->GetTx().GetHash().ToString());
        }
    }
    return std::nullopt;
}
