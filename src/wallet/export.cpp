// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <wallet/export.h>

#include <util/expected.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>

namespace wallet {
util::Expected<std::vector<WalletDescInfo>, std::string> ExportDescriptors(const CWallet& wallet, bool export_private)
{
    AssertLockHeld(wallet.cs_wallet);
    std::vector<WalletDescInfo> wallet_descriptors;
    for (const auto& spk_man : wallet.GetAllScriptPubKeyMans()) {
        const auto desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man);
        if (!desc_spk_man) {
            return util::Unexpected{"Unexpected ScriptPubKey manager type."};
        }
        LOCK(desc_spk_man->cs_desc_man);
        const auto& wallet_descriptor = desc_spk_man->GetWalletDescriptor();
        std::string descriptor;
        if (!Assume(desc_spk_man->GetDescriptorString(descriptor, export_private))) {
            return util::Unexpected{"Can't get descriptor string."};
        }
        const bool is_range = wallet_descriptor.descriptor->IsRange();
        wallet_descriptors.emplace_back(
            descriptor,
            wallet_descriptor.creation_time,
            wallet.IsActiveScriptPubKeyMan(*desc_spk_man),
            wallet.IsInternalScriptPubKeyMan(desc_spk_man),
            is_range ? std::optional(std::make_pair(wallet_descriptor.range_start, wallet_descriptor.range_end)) : std::nullopt,
            wallet_descriptor.next_index
        );
    }
    return wallet_descriptors;
}
} // namespace wallet
