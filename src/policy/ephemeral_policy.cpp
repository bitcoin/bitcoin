// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include<policy/ephemeral_policy.h>
#include<policy/policy.h>

bool CheckValidEphemeralTx(const CTransactionRef tx, CFeeRate dust_relay_fee, CAmount base_fee, CAmount mod_fee, TxValidationState& state)
{
    // We never want to give incentives to mine this transaction alone
    if ((base_fee != 0 || mod_fee != 0) &&
        std::any_of(tx->vout.cbegin(), tx->vout.cend(), [&](const auto& output) { return IsDust(output, dust_relay_fee); })) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "dust", "tx with dust output must be 0-fee");
    }

    return true;
}

std::optional<Txid> CheckEphemeralSpends(const Package& package, CFeeRate dust_relay_rate)
{
    // Package is topologically sorted, and PreChecks ensures that
    // there is up to one dust output per tx.
    static_assert(MAX_DUST_OUTPUTS_PER_TX == 1);

    assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx != nullptr;}));

    // If a parent tx has dust, we have to check for the spend by the child
    // Maps Txid of parent to output index of the dust.
    // Single dust per tx possible
    std::map<Txid, uint32_t> map_tx_dust;

    for (const auto& tx : package) {
        std::unordered_set<Txid, SaltedTxidHasher> child_unspent_dust;
        for (const auto& tx_input : tx->vin) {
            // Parent tx had dust, child MUST be sweeping it
            // if it's spending any output from parent.
            // Note that a child can spend multiple outputs from
            // each parent so we're only filtering by txid here.
            if (map_tx_dust.contains(tx_input.prevout.hash)) {
                child_unspent_dust.insert(tx_input.prevout.hash);
            }
        }

        // Now that we've built a list of parent txids
        // that have dust, make sure that all parent's
        // dust are swept by this same tx
        for (const auto& tx_input : tx->vin) {
            const auto& prevout = tx_input.prevout;
            // Parent tx had dust, child MUST be sweeping it
            // if it's spending any output from parent
            if (map_tx_dust.contains(prevout.hash) &&
                map_tx_dust.at(prevout.hash) == prevout.n) {
                 child_unspent_dust.erase(prevout.hash);
            }

        }

        if (!child_unspent_dust.empty()) {
            return tx->GetHash();
        }

        // Process new dust
        for (uint32_t i=0; i<tx->vout.size(); i++) {
            if (IsDust(tx->vout[i], dust_relay_rate)) {
                // CheckValidEphemeralTx disallows multiple dust outputs
                Assume(!map_tx_dust.contains(tx->GetHash()));
                map_tx_dust[tx->GetHash()] = i;
            }
        }

    }

    return std::nullopt;
}

std::optional<std::string> CheckEphemeralSpends(const CTransactionRef& ptx,
                                                const CTxMemPool::setEntries& ancestors,
                                                CFeeRate dust_relay_feerate)
{
    std::unordered_set<COutPoint, SaltedOutpointHasher> unspent_dust;

    std::unordered_set<Txid, SaltedTxidHasher> parents;
    for (const auto& tx_input : ptx->vin) {
        parents.insert(tx_input.prevout.hash);
    }

    for (const auto& entry : ancestors) {
        const auto& tx = entry->GetTx();
        // Only deal with direct parents
        if (parents.count(tx.GetHash()) == 0) continue;
        for (uint32_t i=0; i<tx.vout.size(); i++) {
            if (IsDust(tx.vout[i], dust_relay_feerate)) {
                unspent_dust.insert(COutPoint(tx.GetHash(), i));
            }
        }
    }

    for (const auto& input : ptx->vin) {
        unspent_dust.erase(input.prevout);
    }

    if (!unspent_dust.empty()) {
        return strprintf("tx does not spend parent ephemeral dust");
    }

    return std::nullopt;
}
