// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <wallet/imports.h>

namespace wallet{
    ImportDescriptorResult ImportDescriptor(CWallet& wallet, const std::string& descriptor,
        bool active,
        const std::optional<bool>& internal,
        const std::optional<std::string>& label,
        int64_t timestamp,
        const std::optional<std::pair<int64_t, int64_t>>& range,
        const std::optional<int64_t>& next_index_arg)
    {
        AssertLockHeld(wallet.cs_wallet);

        ImportDescriptorResult result;

        // Parse descriptor string
        FlatSigningProvider keys;
        std::string error;
        auto parsed_descs = Parse(descriptor, keys, error, /*require_checksum=*/true);
        if (parsed_descs.empty()) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_DESCRIPTOR,
                error);
        }
        if (internal.has_value() && parsed_descs.size() > 1) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_DESCRIPTOR,
                "Cannot have multipath descriptor while also specifying 'internal'");
        }

        // Range check
        bool is_ranged{false};
        int64_t range_start = 0, range_end = 1, next_index = 0;
        if (!parsed_descs.at(0)->IsRange() && range.has_value()) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_PARAMETER,
                "Range should not be specified for an un-ranged descriptor");
        } else if (parsed_descs.at(0)->IsRange()) {
            if (range.has_value()) {
                range_start = range->first;
                range_end = range->second + 1; // Specified range end is inclusive, but we need range end as exclusive
            } else {
                result.warnings.emplace_back("Range not given, using default keypool range");
                result.used_default_range = true;
                range_start = 0;
                range_end = wallet.m_keypool_size;
            }
            next_index = next_index_arg.value_or(range_start);
            is_ranged = true;
            if (next_index < range_start || next_index >= range_end) {
                return result.Error(ImportDescriptorResult::FailureReason::INVALID_PARAMETER,
                    "next_index is out of range");
            }
        }

        // Active descriptors must be ranged
        if (active && !parsed_descs.at(0)->IsRange()) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_PARAMETER,
                "Active descriptors must be ranged");
        }

        // Multipath descriptors should not have a label
        if (parsed_descs.size() > 1 && label.has_value()) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_PARAMETER,
                "Multipath descriptors should not have a label");
        }

        // Ranged descrptors should not have a label
        if (is_ranged && label.has_value()) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_PARAMETER,
                "Ranged descriptors should not have a label");
        }

        // Internal descriptors should not have a label
        bool desc_internal = internal.value_or(false);
        if (desc_internal && label.has_value()) {
            return result.Error(ImportDescriptorResult::FailureReason::INVALID_PARAMETER,
                "Internal addresses should not have a label");
        }

        //Combo descriptor check
        if (active && !parsed_descs.at(0)->IsSingleType()) {
            return result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
                "Combo descriptors cannot be set to active");
        }

        // If the wallet disabled private keys, abort if private keys exist
        if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !keys.keys.empty()) {
            return result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
                "Cannot import private keys to a wallet with private keys disabled");
        }

        const std::string label_str = label.value_or(std::string{});

        for (size_t j = 0; j < parsed_descs.size(); ++j) {
            auto parsed_desc = std::move(parsed_descs[j]);
            if (parsed_descs.size() == 2) {
                desc_internal = j == 1;
            } else if (parsed_descs.size() > 2) {
                CHECK_NONFATAL(!desc_internal);
            }
            // Need to ExpandPrivate to check if private keys are available for all pubkeys
            FlatSigningProvider expand_keys;
            std::vector<CScript> scripts;
            if (!parsed_desc->Expand(0, keys, scripts, expand_keys)) {
                return result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
                    "Cannot expand descriptor. Probably because of hardened derivations without private keys provided");
            }
            parsed_desc->ExpandPrivate(0, keys, expand_keys);

            for (const auto& w : parsed_desc->Warnings()) {
                result.warnings.push_back(w);
            }

            // Check if all private keys are provided
            bool have_all_privkeys = !expand_keys.keys.empty();
            for (const auto& entry : expand_keys.origins) {
                const CKeyID& key_id = entry.first;
                CKey key;
                if (!expand_keys.GetKey(key_id, key)) {
                    have_all_privkeys = false;
                    break;
                }
            }

            // If private keys are enabled, check some things.
            if (!wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
                if (keys.keys.empty()) {
                    return result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
                        "Cannot import descriptor without private keys to a wallet with private keys enabled");
                }
                if (!have_all_privkeys) {
                    result.warnings.emplace_back("Not all private keys provided. Some wallet functionality may return unexpected errors");
                }
            }

            WalletDescriptor w_desc(std::move(parsed_desc), timestamp, range_start, range_end, next_index);

            // Add descriptor to wallet
            auto spk_manager_res = wallet.AddWalletDescriptor(w_desc, keys, label_str, desc_internal);

            if (!spk_manager_res) {
                return result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
                    strprintf("Could not add descriptor '%s': %s", descriptor, util::ErrorString(spk_manager_res).original));
            }

            auto& spk_manager = spk_manager_res.value().get();

            // Set descriptor as active if necessary
            if (active) {
                if (!w_desc.descriptor->GetOutputType()) {
                    result.warnings.emplace_back("Unknown output type, cannot set descriptor to active.");
                } else {
                    wallet.AddActiveScriptPubKeyMan(spk_manager.GetID(), *w_desc.descriptor->GetOutputType(), desc_internal);
                }
            } else {
                if (w_desc.descriptor->GetOutputType()) {
                    wallet.DeactivateScriptPubKeyMan(spk_manager.GetID(), *w_desc.descriptor->GetOutputType(), desc_internal);
                }
            }
        }

        result.success = true;
        return result;
    }

    std::vector<wallet::ImportDescriptorResult> ProcessDescriptorsImport(CWallet& wallet,
        std::vector<wallet::ImportDescriptorRequest> requests)
    {
        std::vector<wallet::ImportDescriptorResult> response;

        // Make sure the results are valid at least up to the most recent block
        // the user could have gotten from another RPC command prior to now
        wallet.BlockUntilSyncedToCurrentChain();

        WalletRescanReserver reserver(wallet);
        if (!reserver.reserve(/*with_passphrase=*/true)) {
            wallet::ImportDescriptorResult result;
            result.Error(wallet::ImportDescriptorResult::FailureReason::WALLET_ERROR,
                "Wallet is currently rescanning. Abort existing rescan or wait.");
            // Fill response with one error for each request
            for (size_t i = 0; i < requests.size(); ++i) {
                response.push_back(result);
            }
            return response;
        }

        // Ensure that the wallet is not locked for the remainder of this call,
        // as the passphrase is used to top up the keypool.
        LOCK(wallet.m_relock_mutex);

        const int64_t minimum_timestamp = 1;
        int64_t now = 0;
        int64_t lowest_timestamp = 0;
        bool rescan = false;
        {
            LOCK(wallet.cs_wallet);

            if (wallet.IsLocked()) {
                wallet::ImportDescriptorResult result;
                result.Error(wallet::ImportDescriptorResult::FailureReason::WALLET_ERROR,
                    "Error: Please enter the wallet passphrase with walletpassphrase first.");
                // Fill response with one error for each request
                for (size_t i = 0; i < requests.size(); ++i) {
                    response.push_back(result);
                }
                return response;
            }

            CHECK_NONFATAL(wallet.chain().findBlock(wallet.GetLastBlockHash(), interfaces::FoundBlock().time(lowest_timestamp).mtpTime(now)));

            for (wallet::ImportDescriptorRequest& request : requests) {
                const int64_t timestamp = std::max(request.timestamp.value_or(now), minimum_timestamp);
                wallet::ImportDescriptorResult result = wallet::ImportDescriptor(
                    wallet,
                    request.descriptor,
                    request.active.value_or(false),
                    request.internal,
                    request.label,
                    timestamp,
                    request.range,
                    request.next_index);
                response.push_back(result);

                if (result.success) {
                    // At least one request succeeded, so we need to rescan
                    rescan = true;
                    if (lowest_timestamp > timestamp) {
                        lowest_timestamp = timestamp;
                    }
                }
            }
            wallet.ConnectScriptPubKeyManNotifiers();
            wallet.RefreshAllTXOs();
        }

        if (rescan) {
            const int64_t scanned_time = wallet.RescanFromTime(lowest_timestamp, reserver, /*update=*/true);
            wallet.ResubmitWalletTransactions(node::TxBroadcast::MEMPOOL_NO_BROADCAST, /*force=*/true);

            if (wallet.IsAbortingRescan()) {
                // Mark all previously successful imports as aborted
                for (wallet::ImportDescriptorResult& r : response) {
                    if (r.success) {
                        r.success = false;
                        r.Error(wallet::ImportDescriptorResult::FailureReason::MISC_ERROR,
                            "Rescan aborted by user.");
                    }
                }
                return response;
            }

            if (scanned_time > lowest_timestamp) {
                for (size_t i = 0; i < requests.size(); ++i) {
                    wallet::ImportDescriptorResult& result = response.at(i);

                    // If the descriptor timestamp is within the successfully scanned
                    // range, or if the import result already has an error set, let
                    // the result stand unmodified. Otherwise replace the result
                    // with an error message.
                    if (scanned_time <= requests.at(i).timestamp.value() || !result.success) continue;

                    result.success = false;
                    std::string error_msg = strprintf("Rescan failed for descriptor with timestamp %d. There "
                        "was an error reading a block from time %d, which is after or within %d seconds "
                        "of key creation, and could contain transactions pertaining to the desc. As a "
                        "result, transactions and coins using this desc may not appear in the wallet.",
                        requests.at(i).timestamp.value(), scanned_time - TIMESTAMP_WINDOW - 1, TIMESTAMP_WINDOW);
                    if (wallet.chain().havePruned()) {
                        error_msg += strprintf(" This error could be caused by pruning or data corruption "
                            "(see bitcoind log for details) and could be dealt with by downloading and "
                            "rescanning the relevant blocks (see -reindex option and rescanblockchain RPC).");
                    } else if (wallet.chain().hasAssumedValidChain()) {
                        error_msg += strprintf(" This error is likely caused by an in-progress assumeutxo "
                            "background sync. Check logs or getchainstates RPC for assumeutxo background "
                            "sync progress and try again later.");
                    } else {
                        error_msg += strprintf(" This error could potentially caused by data corruption. If "
                            "the issue persists you may want to reindex (see -reindex option).");
                    }
                    result.Error(wallet::ImportDescriptorResult::FailureReason::MISC_ERROR,
                        error_msg);
                }
            }
        }
        return response;
    }
}
