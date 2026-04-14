// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <wallet/imports.h>

namespace wallet {

ImportResult ImportDescriptor(CWallet& wallet, const ImportDescriptorRequest& request) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    ImportResult result;

    auto make_error = [&](WalletErrorCode code, std::string message, bool general_error = false) {
        ImportError error{code, Untranslated(message), general_error};
        result.error = std::move(error);
        return result;
    };

    // Parse descriptor string
    FlatSigningProvider keys;
    std::string error;
    auto parsed_descs = Parse(request.descriptor, keys, error, /*require_checksum=*/true);
    if (parsed_descs.empty()) {
        return make_error(
            WalletErrorCode::INVALID_DESCRIPTOR,
            error
        );
    }
    if (request.internal.has_value() && parsed_descs.size() > 1) {
        return make_error(
            WalletErrorCode::INVALID_DESCRIPTOR,
            "Cannot have multipath descriptor while also specifying 'internal'"
        );
    }

    // Range check
    bool is_ranged{false};
    int64_t range_start = 0, range_end = 1, next_index = 0;
    if (!parsed_descs.at(0)->IsRange() && request.range.has_value()) {
        return make_error(
            WalletErrorCode::INVALID_PARAMETER,
            "Range should not be specified for an un-ranged descriptor"
        );
    } else if (parsed_descs.at(0)->IsRange()) {
        if (request.range.has_value()) {
            range_start = request.range->first;
            range_end = request.range->second + 1; // Specified range end is inclusive, but we need range end as exclusive
        } else {
            result.warnings.emplace_back("Range not given, using default keypool range");
            range_start = 0;
            range_end = wallet.m_keypool_size;
        }
        next_index = request.next_index.value_or(range_start);
        is_ranged = true;

        if (next_index < range_start || next_index >= range_end) {
            return make_error(
                WalletErrorCode::INVALID_PARAMETER,
                "next_index is out of range"
            );
        }
    }

    // Active descriptors must be ranged
    if (request.active && !parsed_descs.at(0)->IsRange()) {
        return make_error(
            WalletErrorCode::INVALID_PARAMETER,
            "Active descriptors must be ranged"
        );
    }

    // Multipath descriptors should not have a label
    if (parsed_descs.size() > 1 && !request.label.empty()) {
        return make_error(
            WalletErrorCode::INVALID_PARAMETER,
            "Multipath descriptors should not have a label"
        );
    }

    // Ranged descriptors should not have a label
    if (is_ranged && !request.label.empty()) {
        return make_error(
            WalletErrorCode::INVALID_PARAMETER,
            "Ranged descriptors should not have a label"
        );
    }

    bool desc_internal = request.internal.value_or(false);
    // Internal addresses should not have a label either
    if (desc_internal && !request.label.empty()) {
        return make_error(
            WalletErrorCode::INVALID_PARAMETER,
            "Internal addresses should not have a label"
        );
    }

    // Combo descriptor check
    if (request.active && !parsed_descs.at(0)->IsSingleType()) {
        return make_error(
            WalletErrorCode::WALLET_ERROR,
            "Combo descriptors cannot be set to active"
        );
    }

    // If the wallet disabled private keys, abort if private keys exist
    if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !keys.keys.empty()) {
        return make_error(
            WalletErrorCode::WALLET_ERROR,
            "Cannot import private keys to a wallet with private keys disabled"
        );
    }

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
            return make_error(
                WalletErrorCode::WALLET_ERROR,
                "Cannot expand descriptor. Probably because of hardened derivations without private keys provided"
            );
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
                return make_error(
                    WalletErrorCode::WALLET_ERROR,
                    "Cannot import descriptor without private keys to a wallet with private keys enabled"
                );
            }
            if (!have_all_privkeys) {
                result.warnings.emplace_back("Not all private keys provided. Some wallet functionality may return unexpected errors");
            }
        }
        // If this is an unused(KEY) descriptor, check that the wallet doesn't already have other descriptors with this key
        if (!parsed_desc->HasScripts()) {
            if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
                return make_error(
                    WalletErrorCode::WALLET_ERROR,
                    "Cannot import unused() to wallet without private keys enabled"
                );
            }
            // Unused descriptors must contain a single key.
            // Earlier checks will have enforced that this key is either a private key when private keys are enabled,
            // or that this key is a public key when private keys are disabled.
            // If we can retrieve the corresponding private key from the wallet, then this key is already in the wallet
            // and we should not import it.
            std::set<CPubKey> pubkeys;
            std::set<CExtPubKey> extpubs;
            parsed_desc->GetPubKeys(pubkeys, extpubs);
            std::transform(extpubs.begin(), extpubs.end(), std::inserter(pubkeys, pubkeys.begin()), [](const CExtPubKey& xpub) { return xpub.pubkey; });
            CHECK_NONFATAL(pubkeys.size() == 1);
            if (wallet.GetKey(pubkeys.begin()->GetID())) {
                return make_error(
                    WalletErrorCode::WALLET_ERROR,
                    "Cannot import an unused() descriptor when its private key is already in the wallet"
                );
            }
        }

        Assume(request.timestamp.has_value());
        WalletDescriptor w_desc(std::move(parsed_desc), request.timestamp.value(), range_start, range_end, next_index);

        // Add descriptor to the wallet
        auto spk_manager_res = wallet.AddWalletDescriptor(w_desc, keys, request.label, desc_internal);

        if (!spk_manager_res) {
            return make_error(
                WalletErrorCode::WALLET_ERROR,
                strprintf("Could not add descriptor '%s': %s", request.descriptor, util::ErrorString(spk_manager_res).original)
            );
        }

        auto& spk_manager = spk_manager_res.value().get();

        // Set descriptor as active if necessary
        if (request.active) {
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

    return result;
}

std::vector<ImportResult> ProcessDescriptorsImport(CWallet& wallet,
    std::vector<ImportDescriptorRequest>& requests)
{
    std::vector<ImportResult> response;

    auto make_error = [&](WalletErrorCode code, std::string message, bool is_general_error = false) {
        ImportResult result;
        ImportError error{code, Untranslated(message), is_general_error};
        result.error = std::move(error);
        response.clear();
        response.push_back(std::move(result));
        return response;
    };

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    WalletRescanReserver reserver(wallet);
    if (!reserver.reserve(/*with_passphrase=*/true)) {
        return make_error(WalletErrorCode::WALLET_ERROR,
            "Wallet is currently rescanning. Abort existing rescan or wait.",
            /*is_general_error=*/true
        );
    }
    // Ensure that the wallet is not locked for the remainder of this call,
    // as the passphrase is used to top up the keypool.
    LOCK(wallet.m_relock_mutex);
    const int64_t minimum_timestamp = 0;
    int64_t now = 0;
    int64_t lowest_timestamp = 0;
    bool rescan = false;
    {
        LOCK(wallet.cs_wallet);
        if (wallet.IsLocked()) {
            return make_error(WalletErrorCode::WALLET_UNLOCK_NEEDED,
                "Error: Please enter the wallet passphrase with walletpassphrase first.",
                /*is_general_error=*/true
            );
        }

        CHECK_NONFATAL(wallet.chain().findBlock(wallet.GetLastBlockHash(), interfaces::FoundBlock().time(lowest_timestamp).mtpTime(now)));

        for (ImportDescriptorRequest& request : requests) {
            const int64_t timestamp = std::max(request.timestamp.value_or(now), minimum_timestamp);
            request.timestamp = timestamp;
            const ImportResult& import_result = ImportDescriptor(wallet, request);

            if (lowest_timestamp > timestamp) {
                lowest_timestamp = timestamp;
            }
            if (!import_result.has_error()) {
                // At least one request succeeded, so we need to rescan
                rescan = true;
            }
            response.push_back(import_result);
        }
        wallet.ConnectScriptPubKeyManNotifiers();
        wallet.RefreshAllTXOs();
    }

    if (rescan) {
        const int64_t scanned_time = wallet.RescanFromTime(lowest_timestamp, reserver);
        wallet.ResubmitWalletTransactions(node::TxBroadcast::MEMPOOL_NO_BROADCAST, /*force=*/true);

        if (wallet.IsAbortingRescan()) {
            return make_error(WalletErrorCode::MISC_ERROR,
                "Rescan aborted by user.",
                /*is_general_error=*/true
            );
        }

        if (scanned_time > lowest_timestamp) {
            // Compose the response
            for (size_t i = 0; i < requests.size(); ++i) {
                ImportResult& result = response.at(i);

                // If the descriptor timestamp is within the successfully scanned
                // range, or if the import result already has an error set, let
                // the result stand unmodified. Otherwise replace the result
                // with an error message.
                const int64_t timestamp{requests.at(i).timestamp.value()};
                if (scanned_time > timestamp && !result.has_error()) {
                    std::string error_msg = strprintf("Rescan failed for descriptor with timestamp %d. There "
                        "was an error reading a block from time %d, which is after or within %d seconds "
                        "of key creation, and could contain transactions pertaining to the desc. As a "
                        "result, transactions and coins using this desc may not appear in the wallet.",
                        timestamp, scanned_time - TIMESTAMP_WINDOW - 1, TIMESTAMP_WINDOW);
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
                    result.error = ImportError{
                        WalletErrorCode::MISC_ERROR,
                        Untranslated(error_msg),
                        /*is_wallet_error=*/false
                    };
                }
            }
        }
    }
    return response;
}

} // namespace wallet
