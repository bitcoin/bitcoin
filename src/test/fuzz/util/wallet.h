// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_WALLET_H
#define BITCOIN_TEST_FUZZ_UTIL_WALLET_H

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <policy/policy.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {

/**
 * Wraps a descriptor wallet for fuzzing.
 */
struct FuzzedWallet {
    std::shared_ptr<CWallet> wallet;
    FuzzedWallet(interfaces::Chain* chain, const std::string& name, const std::string& seed_insecure)
    {
        wallet = std::make_shared<CWallet>(chain, name, CreateMockableWalletDatabase());
        {
            LOCK(wallet->cs_wallet);
            wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            auto height{*Assert(chain->getHeight())};
            wallet->SetLastBlockProcessed(height, chain->getBlockHash(height));
        }
        wallet->m_keypool_size = 1; // Avoid timeout in TopUp()
        assert(wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
        ImportDescriptors(seed_insecure);
    }
    void ImportDescriptors(const std::string& seed_insecure)
    {
        const std::vector<std::string> DESCS{
            "pkh(%s/%s/*)",
            "sh(wpkh(%s/%s/*))",
            "tr(%s/%s/*)",
            "wpkh(%s/%s/*)",
        };

        for (const std::string& desc_fmt : DESCS) {
            for (bool internal : {true, false}) {
                const auto descriptor{(strprintf)(desc_fmt, "[5aa9973a/66h/4h/2h]" + seed_insecure, int{internal})};

                FlatSigningProvider keys;
                std::string error;
                auto parsed_desc = Parse(descriptor, keys, error, /*require_checksum=*/false);
                assert(parsed_desc);
                assert(error.empty());
                assert(parsed_desc->IsRange());
                assert(parsed_desc->IsSingleType());
                assert(!keys.keys.empty());
                WalletDescriptor w_desc{std::move(parsed_desc), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0};
                assert(!wallet->GetDescriptorScriptPubKeyMan(w_desc));
                LOCK(wallet->cs_wallet);
                auto spk_manager{wallet->AddWalletDescriptor(w_desc, keys, /*label=*/"", internal)};
                assert(spk_manager);
                wallet->AddActiveScriptPubKeyMan(spk_manager->GetID(), *Assert(w_desc.descriptor->GetOutputType()), internal);
            }
        }
    }
    CTxDestination GetDestination(FuzzedDataProvider& fuzzed_data_provider)
    {
        auto type{fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)};
        if (fuzzed_data_provider.ConsumeBool()) {
            return *Assert(wallet->GetNewDestination(type, ""));
        } else {
            return *Assert(wallet->GetNewChangeDestination(type));
        }
    }
    CScript GetScriptPubKey(FuzzedDataProvider& fuzzed_data_provider) { return GetScriptForDestination(GetDestination(fuzzed_data_provider)); }
    void FundTx(FuzzedDataProvider& fuzzed_data_provider, CMutableTransaction tx)
    {
        // The fee of "tx" is 0, so this is the total input and output amount
        const CAmount total_amt{
            std::accumulate(tx.vout.begin(), tx.vout.end(), CAmount{}, [](CAmount t, const CTxOut& out) { return t + out.nValue; })};
        const uint32_t tx_size(GetVirtualTransactionSize(CTransaction{tx}));
        std::set<int> subtract_fee_from_outputs;
        if (fuzzed_data_provider.ConsumeBool()) {
            for (size_t i{}; i < tx.vout.size(); ++i) {
                if (fuzzed_data_provider.ConsumeBool()) {
                    subtract_fee_from_outputs.insert(i);
                }
            }
        }
        std::vector<CRecipient> recipients;
        for (size_t idx = 0; idx < tx.vout.size(); idx++) {
            const CTxOut& tx_out = tx.vout[idx];
            CTxDestination dest;
            ExtractDestination(tx_out.scriptPubKey, dest);
            CRecipient recipient = {dest, tx_out.nValue, subtract_fee_from_outputs.count(idx) == 1};
            recipients.push_back(recipient);
        }
        CCoinControl coin_control;
        coin_control.m_allow_other_inputs = fuzzed_data_provider.ConsumeBool();
        CallOneOf(
            fuzzed_data_provider, [&] { coin_control.destChange = GetDestination(fuzzed_data_provider); },
            [&] { coin_control.m_change_type.emplace(fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)); },
            [&] { /* no op (leave uninitialized) */ });
        coin_control.fAllowWatchOnly = fuzzed_data_provider.ConsumeBool();
        coin_control.m_include_unsafe_inputs = fuzzed_data_provider.ConsumeBool();
        {
            auto& r{coin_control.m_signal_bip125_rbf};
            CallOneOf(
                fuzzed_data_provider, [&] { r = true; }, [&] { r = false; }, [&] { r = std::nullopt; });
        }
        coin_control.m_feerate = CFeeRate{
            // A fee of this range should cover all cases
            fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, 2 * total_amt),
            tx_size,
        };
        if (fuzzed_data_provider.ConsumeBool()) {
            *coin_control.m_feerate += GetMinimumFeeRate(*wallet, coin_control, nullptr);
        }
        coin_control.fOverrideFeeRate = fuzzed_data_provider.ConsumeBool();
        // Add solving data (m_external_provider and SelectExternal)?

        int change_position{fuzzed_data_provider.ConsumeIntegralInRange<int>(-1, tx.vout.size() - 1)};
        bilingual_str error;
        // Clear tx.vout since it is not meant to be used now that we are passing outputs directly.
        // This sets us up for a future PR to completely remove tx from the function signature in favor of passing inputs directly
        tx.vout.clear();
        (void)FundTransaction(*wallet, tx, recipients, change_position, /*lockUnspents=*/false, coin_control);
    }
};
}

#endif // BITCOIN_TEST_FUZZ_UTIL_WALLET_H
