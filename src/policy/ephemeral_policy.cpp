// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <policy/ephemeral_policy.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/hasher.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

bool PreCheckEphemeralTx(const CTransaction& tx, CFeeRate dust_relay_rate, CAmount base_fee, CAmount mod_fee, TxValidationState& state)
{
    // We never want to give incentives to mine this transaction alone
    if ((base_fee != 0 || mod_fee != 0) && !GetDust(tx, dust_relay_rate).empty()) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "dust", "tx with dust output must be 0-fee");
    }

    return true;
}

bool CheckEphemeralSpends(const Package& package, CFeeRate dust_relay_rate, const CTxMemPool& tx_pool, TxValidationState& out_child_state, Wtxid& out_child_wtxid)
{
    if (!Assume(std::ranges::all_of(package, [](const auto& tx){return tx != nullptr;}))) {
        // Bail out of spend checks if caller gave us an invalid package
        return true;
    }

    std::map<Txid, CTransactionRef> map_txid_ref;
    for (const auto& tx : package) {
        map_txid_ref[tx->GetHash()] = tx;
    }

    for (const auto& tx : package) {
        std::unordered_set<Txid, SaltedTxidHasher> processed_parent_set;
        std::unordered_set<COutPoint, SaltedOutpointHasher> unspent_parent_dust;

        for (const auto& tx_input : tx->vin) {
            const Txid& parent_txid{tx_input.prevout.hash};
            // Skip parents we've already checked dust for
            if (processed_parent_set.contains(parent_txid)) continue;

            // We look for an in-package or in-mempool dependency
            CTransactionRef parent_ref = nullptr;
            if (auto it = map_txid_ref.find(parent_txid); it != map_txid_ref.end()) {
                parent_ref = it->second;
            } else {
                parent_ref = tx_pool.get(parent_txid);
            }

            // Check for dust on parents
            if (parent_ref) {
                for (uint32_t out_index = 0; out_index < parent_ref->vout.size(); out_index++) {
                    const auto& tx_output = parent_ref->vout[out_index];
                    if (IsDust(tx_output, dust_relay_rate)) {
                        unspent_parent_dust.insert(COutPoint(parent_txid, out_index));
                    }
                }
            }

            processed_parent_set.insert(parent_txid);
        }

        if (unspent_parent_dust.empty()) {
            continue;
        }

        // Now that we have gathered parents' dust, make sure it's spent
        // by the child
        for (const auto& tx_input : tx->vin) {
            unspent_parent_dust.erase(tx_input.prevout);
        }

        if (!unspent_parent_dust.empty()) {
            const Txid& out_child_txid = tx->GetHash();
            out_child_wtxid = tx->GetWitnessHash();
            out_child_state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "missing-ephemeral-spends",
                                strprintf("tx %s (wtxid=%s) did not spend parent's ephemeral dust", out_child_txid.ToString(), out_child_wtxid.ToString()));
            return false;
        }
    }

    return true;
}
