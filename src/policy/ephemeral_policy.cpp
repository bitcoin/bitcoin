// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/ephemeral_policy.h>
#include <policy/policy.h>

bool HasDust(const CTransaction& tx, CFeeRate dust_relay_rate)
{
    return std::ranges::any_of(tx.vout, [&](const auto& output) { return IsDust(output, dust_relay_rate); });
}

bool PreCheckEphemeralTx(const CTransaction& tx, CFeeRate dust_relay_rate, CAmount base_fee, CAmount mod_fee, TxValidationState& state)
{
    // We never want to give incentives to mine this transaction alone
    if ((base_fee != 0 || mod_fee != 0) && HasDust(tx, dust_relay_rate)) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "dust", "tx with dust output must be 0-fee");
    }

    return true;
}

std::optional<Txid> CheckEphemeralSpends(const Package& package, CFeeRate dust_relay_rate, const CTxMemPool& tx_pool)
{
    if (!Assume(std::ranges::all_of(package, [](const auto& tx){return tx != nullptr;}))) {
        // Bail out of spend checks if caller gave us an invalid package
        return std::nullopt;
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

        // Now that we have gathered parents' dust, make sure it's spent
        // by the child
        for (const auto& tx_input : tx->vin) {
            unspent_parent_dust.erase(tx_input.prevout);
        }

        if (!unspent_parent_dust.empty()) {
            return tx->GetHash();
        }
    }

    return std::nullopt;
}
