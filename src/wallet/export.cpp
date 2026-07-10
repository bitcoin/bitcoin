// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <wallet/export.h>

#include <key_io.h>
#include <util/fs.h>
#include <util/expected.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/context.h>
#include <wallet/sqlite.h>
#include <wallet/wallet.h>

#include <fstream>

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

util::Result<std::string> ExportWatchOnlyWallet(const CWallet& wallet, const fs::path& destination, WalletContext& context)
{
    AssertLockHeld(wallet.cs_wallet);

    if (destination.empty()) {
        return util::Error{_("Error: Export destination cannot be empty")};
    }
    if (fs::exists(destination)) {
        return util::Error{strprintf(_("Error: Export destination '%s' already exists"), fs::PathToString(destination))};
    }
    if (!std::ofstream{fs::PathToString(destination)}) {
        return util::Error{strprintf(_("Error: Could not create file '%s'"), fs::PathToString(destination))};
    }
    bool success = false;
    auto cleanup_destination = interfaces::MakeCleanupHandler([&success, &destination] {
        if (!success) fs::remove(destination);
    });

    // Get the descriptors from this wallet
    util::Expected<std::vector<WalletDescInfo>, std::string> exported = ExportDescriptors(wallet, /*export_private=*/false);
    if (!exported) {
        return util::Error{Untranslated(exported.error())};
    }
    if (exported->empty()) {
        return util::Error{_("Error: Wallet has no descriptors to export")};
    }

    // Make the wallet with the same flags as this wallet, but without private keys
    const uint64_t create_flags = wallet.GetWalletFlags() | WALLET_FLAG_DISABLE_PRIVATE_KEYS;

    // Create the temporary watchonly wallet in memory to avoid leaving files on disk
    std::vector<bilingual_str> warnings;
    bilingual_str error;
    WalletContext empty_context;
    empty_context.args = context.args;
    std::shared_ptr<CWallet> watchonly_wallet = CWallet::CreateNew(empty_context, /*name=*/wallet.GetName() + "_watchonly_temp", MakeInMemoryWalletDatabase(), create_flags, /*born_encrypted=*/false, error, warnings);
    if (!watchonly_wallet) {
        return util::Error{strprintf(_("Error: Failed to create new watchonly wallet. %s"), error)};
    }

    {
        LOCK(watchonly_wallet->cs_wallet);

        // Parse the descriptors and add them to the new wallet
        for (const WalletDescInfo& desc_info : *Assert(exported)) {
            // Parse the descriptor
            FlatSigningProvider dummy_keys;
            std::string dummy_err;
            std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_info.descriptor, dummy_keys, dummy_err, /*require_checksum=*/true);
            CHECK_NONFATAL(descs.size() == 1); // All of our descriptors should be valid, and not multipath
            CHECK_NONFATAL(dummy_keys.keys.size() == 0); // No private keys should be present in our exported descriptors

            // Get the range if there is one
            int32_t range_start = 0;
            int32_t range_end = 0;
            if (desc_info.range) {
                range_start = desc_info.range->first;
                range_end = desc_info.range->second;
            }

            WalletDescriptor w_desc(std::move(descs.at(0)), desc_info.creation_time, range_start, range_end, desc_info.next_index);

            // For descriptors that cannot self expand (i.e. needs private keys or cache), retrieve the cache
            uint256 desc_id = w_desc.id;
            if (!w_desc.descriptor->CanSelfExpand()) {
                DescriptorScriptPubKeyMan* desc_spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(desc_id));
                w_desc.cache = WITH_LOCK(desc_spkm->cs_desc_man, return desc_spkm->GetWalletDescriptor().cache);
            }

            // Add to the watchonly wallet
            if (auto spkm_res = watchonly_wallet->AddWalletDescriptor(w_desc, dummy_keys, /*label=*/"", /*internal=*/false); !spkm_res) {
                return util::Error{util::ErrorString(spkm_res)};
            }

            // Set active spkms as active
            if (desc_info.active) {
                // Determine whether this descriptor is internal
                // This is only set for active spkms
                bool internal = false;
                if (desc_info.internal) {
                    internal = *desc_info.internal;
                }
                watchonly_wallet->AddActiveScriptPubKeyMan(desc_id, *Assert(w_desc.descriptor->GetOutputType()), internal);
            }
        }

        // Copy locked coins that are persisted
        for (const auto& [coin, persisted] : wallet.m_locked_coins) {
            if (!persisted) continue;
            watchonly_wallet->LockCoin(coin, persisted);
        }

        {
            // Make a WalletBatch for the watchonly wallet so that everything else can be written atomically
            WalletBatch watchonly_batch(watchonly_wallet->GetDatabase());
            if (!watchonly_batch.TxnBegin()) {
                return util::Error{strprintf(_("Error: database transaction cannot be executed for new watchonly wallet %s"), watchonly_wallet->GetName())};
            }

            // Copy orderPosNext
            watchonly_batch.WriteOrderPosNext(wallet.nOrderPosNext);

            // Write the best block locator to avoid rescanning on reload
            CBlockLocator best_block_locator;
            {
                WalletBatch local_wallet_batch(wallet.GetDatabase());
                if (!local_wallet_batch.ReadBestBlock(best_block_locator)) {
                    return util::Error{_("Error: Unable to read wallet's best block locator record")};
                }
            }
            if (!watchonly_batch.WriteBestBlock(best_block_locator)) {
                return util::Error{_("Error: Unable to write watchonly wallet best block locator record")};
            }

            // Copy the transactions
            for (const auto& [txid, wtx] : wallet.mapWallet) {
                if (!watchonly_wallet->LoadToWallet(txid, [&](CWalletTx& ins_wtx, bool new_tx) EXCLUSIVE_LOCKS_REQUIRED(watchonly_wallet->cs_wallet) {
                    if (!new_tx) return false;
                    ins_wtx.SetTx(wtx.tx);
                    ins_wtx.CopyFrom(wtx);
                    return true;
                })) {
                    return util::Error{strprintf(_("Error: Could not add tx %s to watchonly wallet"), txid.GetHex())};
                }
                watchonly_batch.WriteTx(watchonly_wallet->mapWallet.at(txid));
            }

            // Copy address book
            for (const auto& [dest, entry] : wallet.m_address_book) {
                auto address{EncodeDestination(dest)};
                if (entry.purpose) watchonly_batch.WritePurpose(address, PurposeToString(*entry.purpose));
                if (entry.label) watchonly_batch.WriteName(address, *entry.label);
                for (const auto& [id, request] : entry.receive_requests) {
                    watchonly_batch.WriteAddressReceiveRequest(dest, id, request);
                }
                if (entry.previously_spent) watchonly_batch.WriteAddressPreviouslySpent(dest, true);
            }

            if (!watchonly_batch.TxnCommit()) {
                return util::Error{_("Error: cannot commit db transaction for watchonly wallet export")};
            }
        }

        // Make a backup of this wallet at the specified destination directory
        if (!watchonly_wallet->BackupWallet(fs::PathToString(destination))) {
            return util::Error{_("Error: Unable to write the exported wallet")};
        }
        success = true;
    }

    return fs::PathToString(destination);
}
} // namespace wallet
