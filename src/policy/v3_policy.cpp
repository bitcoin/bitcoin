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

std::optional<std::tuple<Wtxid, Wtxid, bool>> CheckV3Inheritance(const Package& package)
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
    std::unordered_map<Txid, Wtxid, SaltedTxidHasher> v3_txid_to_wtxid;
    std::unordered_map<Txid, Wtxid, SaltedTxidHasher> non_v3_txid_to_wtxid;
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

bool CheckValidEphemeralTx(const CTransaction& tx, TxValidationState& state, CAmount& txfee, bool package_context)
{
    // No anchor; it's ok
    if (!HasPayToAnchor(tx)) {
        return true;
    }

    /* Only allow in package context; in single transaction context the anchor must be unspent */
    if (!package_context) {
        /* Allows re-evaluation in package context */
        return state.Invalid(TxValidationResult::TX_RECONSIDERABLE, "missing-ephemeral-spends");
    }

    if (tx.nVersion != 3) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "wrong-ephemeral-nversion");
    } else if (txfee != 0) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "invalid-ephemeral-fee");
    }

    return true;
}

std::optional<uint256> CheckEphemeralSpends(const Package& package)
{
    assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx != nullptr;}));

    // Only size one or two are possible to violate ephemeral spend checks
    if (package.empty() || package.size() > V3_ANCESTOR_LIMIT) {
        return std::nullopt;
    }
    static_assert(V3_ANCESTOR_LIMIT == 2, "value of V3_ANCESTOR_LIMIT changed");

    const auto& parent = package.front();

    for (uint32_t i=0; i<parent->vout.size(); i++) {
        // Once we find an anchor, it must be spent by child
        if (parent->vout[i].scriptPubKey.IsPayToAnchor()) {
            if (package.size() == V3_ANCESTOR_LIMIT) {
                for (const auto& child_input : package.back()->vin) {
                    if (child_input.prevout == COutPoint(parent->GetHash(), i)) {
                        return std::nullopt;
                    }
                }
            }
            return parent->GetWitnessHash();
        }
    }

    return std::nullopt;
}

std::optional<std::string> CheckEphemeralSpends(const CTransactionRef& ptx,
                                                const CTxMemPool::setEntries& ancestors)
{
    /* Ephemeral anchors are disallowed already, no need to check */
    if (ptx->nVersion != 3) {
        return std::nullopt;
    }

    std::unordered_set<COutPoint, SaltedOutpointHasher> unspent_ephemeral_outputs;

    // Note that all ancestors are direct parents due to V3
    for (const auto& entry : ancestors) {
        const auto& tx = entry->GetTx();
        for (uint32_t i=0; i<tx.vout.size(); i++) {
            if (tx.vout[i].scriptPubKey.IsPayToAnchor()) {
                unspent_ephemeral_outputs.insert(COutPoint(tx.GetHash(), i));
            }
        }
    }

    for (const auto& input : ptx->vin) {
        unspent_ephemeral_outputs.erase(input.prevout);
    }

    if (!unspent_ephemeral_outputs.empty()) {
        return strprintf("tx does not spend all parent ephemeral anchors");
    }

    return std::nullopt;
}

std::optional<std::string> ApplyV3Rules(const CTransactionRef& ptx,
                                        const CTxMemPool::setEntries& ancestors,
                                        const std::set<Txid>& direct_conflicts,
                                        int64_t vsize)
{
    // This function is specialized for these limits, and must be reimplemented if they ever change.
    static_assert(V3_ANCESTOR_LIMIT == 2);
    static_assert(V3_DESCENDANT_LIMIT == 2);

    // These rules only apply to transactions with nVersion=3.
    if (ptx->nVersion != 3) return std::nullopt;

    if (ancestors.size() + 1 > V3_ANCESTOR_LIMIT) {
        return strprintf("tx %s would have too many ancestors", ptx->GetWitnessHash().ToString());
    }
    if (ancestors.empty()) {
        return std::nullopt;
    }
    // If this transaction spends V3 parents, it cannot be too large.
    if (vsize > V3_CHILD_MAX_VSIZE) {
        return strprintf("v3 child tx is too big: %u > %u virtual bytes", vsize, V3_CHILD_MAX_VSIZE);
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
    return std::nullopt;
}
