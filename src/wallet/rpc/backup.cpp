// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <clientversion.h>
#include <core_io.h>
#include <hash.h>
#include <interfaces/chain.h>
#include <key_io.h>
#include <merkleblock.h>
#include <node/types.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/solver.h>
#include <sync.h>
#include <uint256.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/imports.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>

#include <cstdint>
#include <fstream>
#include <tuple>
#include <string>

#include <univalue.h>



using interfaces::FoundBlock;

namespace wallet {
RPCMethod importprunedfunds()
{
    return RPCMethod{
        "importprunedfunds",
        "Imports funds without rescan. Corresponding address or script must previously be included in wallet. Aimed towards pruned wallets. The end-user is responsible to import additional transactions that subsequently spend the imported outputs or rescan after the point in the blockchain the transaction is included.\n",
                {
                    {"rawtransaction", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A raw transaction in hex funding an already-existing address in wallet"},
                    {"txoutproof", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex output from gettxoutproof that contains the transaction"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{""},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
    }

    CMerkleBlock merkleBlock;
    SpanReader{ParseHexV(request.params[1], "proof")} >> merkleBlock;

    //Search partial merkle tree in proof for our transaction and index in valid block
    std::vector<Txid> vMatch;
    std::vector<unsigned int> vIndex;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Something wrong with merkleblock");
    }

    LOCK(pwallet->cs_wallet);
    int height;
    if (!pwallet->chain().findAncestorByHash(pwallet->GetLastBlockHash(), merkleBlock.header.GetHash(), FoundBlock().height(height))) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");
    }

    std::vector<Txid>::const_iterator it;
    if ((it = std::find(vMatch.begin(), vMatch.end(), tx.GetHash())) == vMatch.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction given doesn't exist in proof");
    }

    unsigned int txnIndex = vIndex[it - vMatch.begin()];

    CTransactionRef tx_ref = MakeTransactionRef(tx);
    if (pwallet->IsMine(*tx_ref)) {
        pwallet->AddToWallet(std::move(tx_ref), TxStateConfirmed{merkleBlock.header.GetHash(), height, static_cast<int>(txnIndex)});
        return UniValue::VNULL;
    }

    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No addresses in wallet correspond to included transaction");
},
    };
}

RPCMethod removeprunedfunds()
{
    return RPCMethod{
        "removeprunedfunds",
        "Deletes the specified transaction from the wallet. Meant for use with pruned wallets and as a companion to importprunedfunds. This will affect wallet balances.\n",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex-encoded id of the transaction you are deleting"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166eae7628734ea0a5\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166eae7628734ea0a5\"")
                },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);

    Txid hash{Txid::FromUint256(ParseHashV(request.params[0], "txid"))};
    std::vector<Txid> vHash;
    vHash.push_back(hash);
    if (auto res = pwallet->RemoveTxs(vHash); !res) {
        throw JSONRPCError(RPC_WALLET_ERROR, util::ErrorString(res).original);
    }

    return UniValue::VNULL;
},
    };
}

static std::optional<int64_t> GetImportTimestamp(const UniValue& data)
{
    if (data.exists("timestamp")) {
        const UniValue& timestamp = data["timestamp"];
        if (timestamp.isNum()) {
            return timestamp.getInt<int64_t>();
        } else if (timestamp.isStr() && timestamp.get_str() == "now") {
            return std::nullopt;
        }
        throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Expected number or \"now\" timestamp value for key. got type %s", uvTypeName(timestamp.type())));
    }
    throw JSONRPCError(RPC_TYPE_ERROR, "Missing required timestamp field for key");
}

static wallet::ImportDescriptorRequest ProcessUniValueDescriptor(const UniValue& data)
{
    wallet::ImportDescriptorRequest request;
    if (!data.exists("desc")) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Descriptor not found.");
    }

    request.descriptor = data["desc"].get_str();
    if (data.exists("label")) request.label = LabelFromValue(data["label"]);
    request.timestamp = GetImportTimestamp(data);
    request.active = data.exists("active") ? data["active"].get_bool() : false;
    if (data.exists("internal")) request.internal = data["internal"].get_bool();
    if (data.exists("range")) {
        auto r = ParseDescriptorRange(data["range"]);
        request.range = {r.first, r.second};
    }
    if (data.exists("next_index")) request.next_index = data["next_index"].getInt<int64_t>();
    return request;
}

ImportDescriptorResult ImportDescriptor(CWallet& wallet, const std::string& descriptor,
    bool active,
    const std::optional<bool>& internal,
    const std::optional<std::string>& label,
    int64_t timestamp,
    const std::optional<std::pair<int64_t, int64_t>>& range,
    const std::optional<int64_t>& next_index_arg) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
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

std::vector<ImportDescriptorResult> ProcessDescriptorsImport(CWallet& wallet,
    std::vector<ImportDescriptorRequest> requests)
{
    std::vector<ImportDescriptorResult> response;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    WalletRescanReserver reserver(wallet);
    if (!reserver.reserve(/*with_passphrase=*/true)) {
        ImportDescriptorResult result;
        result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
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
            ImportDescriptorResult result;
            result.Error(ImportDescriptorResult::FailureReason::WALLET_ERROR,
                "Error: Please enter the wallet passphrase with walletpassphrase first.");
            // Fill response with one error for each request
            for (size_t i = 0; i < requests.size(); ++i) {
                response.push_back(result);
            }
            return response;
        }

        CHECK_NONFATAL(wallet.chain().findBlock(wallet.GetLastBlockHash(), interfaces::FoundBlock().time(lowest_timestamp).mtpTime(now)));

        for (ImportDescriptorRequest& request : requests) {
            const int64_t timestamp = std::max(request.timestamp.value_or(now), minimum_timestamp);
            ImportDescriptorResult result = ImportDescriptor(
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
            for (ImportDescriptorResult& r : response) {
                if (r.success) {
                    r.success = false;
                    r.Error(ImportDescriptorResult::FailureReason::MISC_ERROR,
                        "Rescan aborted by user.");
                }
            }
            return response;
        }

        if (scanned_time > lowest_timestamp) {
            for (size_t i = 0; i < requests.size(); ++i) {
                ImportDescriptorResult& result = response.at(i);

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
                result.Error(ImportDescriptorResult::FailureReason::MISC_ERROR,
                    error_msg);
            }
        }
    }
    return response;
}

RPCMethod importdescriptors()
{
    return RPCMethod{
        "importdescriptors",
        "Import descriptors. This will trigger a rescan of the blockchain based on the earliest timestamp of all descriptors being imported. Requires a new wallet backup.\n"
        "When importing descriptors with multipath key expressions, if the multipath specifier contains exactly two elements, the descriptor produced from the second element will be imported as an internal descriptor.\n"
            "\nNote: This call can take over an hour to complete if using an early timestamp; during that time, other rpc calls\n"
            "may report that the imported keys, addresses or scripts exist but related transactions are still missing.\n"
            "The rescan is significantly faster if block filters are available (using startup option \"-blockfilterindex=1\").\n",
                {
                    {"requests", RPCArg::Type::ARR, RPCArg::Optional::NO, "Data to be imported",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "Descriptor to import."},
                                    {"active", RPCArg::Type::BOOL, RPCArg::Default{false}, "Set this descriptor to be the active descriptor for the corresponding output type/externality"},
                                    {"range", RPCArg::Type::RANGE, RPCArg::Optional::OMITTED, "If a ranged descriptor is used, this specifies the end or the range (in the form [begin,end]) to import"},
                                    {"next_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "If a ranged descriptor is set to active, this specifies the next index to generate addresses from"},
                                    {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::NO, "Time from which to start rescanning the blockchain for this descriptor, in " + UNIX_EPOCH_TIME + "\n"
                                        "Use the string \"now\" to substitute the current synced blockchain time.\n"
                                        "\"now\" can be specified to bypass scanning, for outputs which are known to never have been used, and\n"
                                        "0 can be specified to scan the entire blockchain. Blocks up to 2 hours before the earliest timestamp\n"
                                        "of all descriptors being imported will be scanned as well as the mempool.",
                                        RPCArgOptions{.type_str={"timestamp | \"now\"", "integer / string"}}
                                    },
                                    {"internal", RPCArg::Type::BOOL, RPCArg::Default{false}, "Whether matching outputs should be treated as not incoming payments (e.g. change)"},
                                    {"label", RPCArg::Type::STR, RPCArg::Default{""}, "Label to assign to the address, only allowed with internal=false. Disabled for ranged descriptors"},
                                },
                            },
                        },
                        RPCArgOptions{.oneline_description="requests"}},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "Response is an array with the same size as the input that has the execution result",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "success", ""},
                            {RPCResult::Type::ARR, "warnings", /*optional=*/true, "",
                            {
                                {RPCResult::Type::STR, "", ""},
                            }},
                            {RPCResult::Type::OBJ, "error", /*optional=*/true, "",
                            {
                                {RPCResult::Type::NUM, "code", "JSONRPC error code"},
                                {RPCResult::Type::STR, "message", "JSONRPC error message"},
                            }},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("importdescriptors", "'[{ \"desc\": \"<my descriptor>\", \"timestamp\":1455191478, \"internal\": true }, "
                                          "{ \"desc\": \"<my descriptor 2>\", \"label\": \"example 2\", \"timestamp\": 1455191480 }]'") +
                    HelpExampleCli("importdescriptors", "'[{ \"desc\": \"<my descriptor>\", \"timestamp\":1455191478, \"active\": true, \"range\": [0,100], \"label\": \"<my bech32 wallet>\" }]'")
                },
        [](const RPCMethod& self, const JSONRPCRequest& main_request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(main_request);
    if (!pwallet) return UniValue::VNULL;
    CWallet& wallet{*pwallet};

    UniValue response(UniValue::VARR);
    const UniValue& univalue_requests = main_request.params[0];
    std::vector<wallet::ImportDescriptorRequest> requests;

    // Collect per-item parse errors so that malformed inputs (e.g. invalid
    // label) are returned as per-item failures.
    std::vector<std::optional<UniValue>> parse_errors(univalue_requests.size());
    for (size_t i = 0; i < univalue_requests.size(); ++i) {
        try {
            requests.push_back(ProcessUniValueDescriptor(univalue_requests[i]));
        } catch (const UniValue& e) {
            parse_errors[i] = e;
        }
    }

    std::vector<wallet::ImportDescriptorResult> import_results;
    if (!requests.empty()) {
        import_results = wallet::ProcessDescriptorsImport(wallet, requests);
    }

    size_t result_idx = 0;
    for (size_t i = 0; i < univalue_requests.size(); ++i)
    {
        UniValue result(UniValue::VOBJ);
        UniValue warnings(UniValue::VARR);

        // If parsing this request failed, return a per-item error and skip it
        // as we didnt import the descriptor in the request.
        if (parse_errors[i].has_value()) {
            result.pushKV("success", UniValue(false));
            result.pushKV("error", parse_errors[i].value());
            response.push_back(std::move(result));
            continue;
        }

        wallet::ImportDescriptorResult import_result = import_results[result_idx++];
        try {
            // Translates ImportDescriptor errors to RPC errors.
            if (!import_result.success) {
                int rpc_code;
                switch (import_result.reason) {
                    case wallet::ImportDescriptorResult::FailureReason::INVALID_DESCRIPTOR:
                        rpc_code = RPC_INVALID_ADDRESS_OR_KEY;
                        break;
                    case wallet::ImportDescriptorResult::FailureReason::INVALID_PARAMETER:
                        rpc_code = RPC_INVALID_PARAMETER;
                        break;
                    case wallet::ImportDescriptorResult::FailureReason::MISC_ERROR:
                        rpc_code = RPC_MISC_ERROR;
                        result.pushKV("error", JSONRPCError(rpc_code, import_result.error));
                        break;
                    default:
                        rpc_code = RPC_WALLET_ERROR;
                        break;
                }
                if (rpc_code != RPC_MISC_ERROR) {
                    throw JSONRPCError(rpc_code, import_result.error);
                }
            }
            result.pushKV("success", UniValue(import_result.success));
        } catch (const UniValue& e) {
            result.pushKV("success", UniValue(false));
            result.pushKV("error", e);
        }

        for (const auto& w : import_result.warnings) {
            warnings.push_back(w);
        }
        PushWarnings(warnings, result);
        response.push_back(std::move(result));
    }

    return response;
},
    };
}

RPCMethod listdescriptors()
{
    return RPCMethod{
        "listdescriptors",
        "List all descriptors present in a wallet.\n",
        {
            {"private", RPCArg::Type::BOOL, RPCArg::Default{false}, "Show private descriptors."}
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::STR, "wallet_name", "Name of wallet this operation was performed on"},
            {RPCResult::Type::ARR, "descriptors", "Array of descriptor objects (sorted by descriptor string representation)",
            {
                {RPCResult::Type::OBJ, "", "", {
                    {RPCResult::Type::STR, "desc", "Descriptor string representation"},
                    {RPCResult::Type::NUM, "timestamp", "The creation time of the descriptor"},
                    {RPCResult::Type::BOOL, "active", "Whether this descriptor is currently used to generate new addresses"},
                    {RPCResult::Type::BOOL, "internal", /*optional=*/true, "True if this descriptor is used to generate change addresses. False if this descriptor is used to generate receiving addresses; defined only for active descriptors"},
                    {RPCResult::Type::ARR_FIXED, "range", /*optional=*/true, "Defined only for ranged descriptors", {
                        {RPCResult::Type::NUM, "", "Range start inclusive"},
                        {RPCResult::Type::NUM, "", "Range end inclusive"},
                    }},
                    {RPCResult::Type::NUM, "next", /*optional=*/true, "Same as next_index field. Kept for compatibility reason."},
                    {RPCResult::Type::NUM, "next_index", /*optional=*/true, "The next index to generate addresses from; defined only for ranged descriptors"},
                }},
            }}
        }},
        RPCExamples{
            HelpExampleCli("listdescriptors", "") + HelpExampleRpc("listdescriptors", "")
            + HelpExampleCli("listdescriptors", "true") + HelpExampleRpc("listdescriptors", "true")
        },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return UniValue::VNULL;

    const bool priv = !request.params[0].isNull() && request.params[0].get_bool();
    if (wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && priv) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Can't get private descriptor string for watch-only wallets");
    }
    if (priv) {
        EnsureWalletIsUnlocked(*wallet);
    }

    LOCK(wallet->cs_wallet);

    const auto active_spk_mans = wallet->GetActiveScriptPubKeyMans();

    struct WalletDescInfo {
        std::string descriptor;
        uint64_t creation_time;
        bool active;
        std::optional<bool> internal;
        std::optional<std::pair<int64_t,int64_t>> range;
        int64_t next_index;
    };

    std::vector<WalletDescInfo> wallet_descriptors;
    for (const auto& spk_man : wallet->GetAllScriptPubKeyMans()) {
        const auto desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man);
        if (!desc_spk_man) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Unexpected ScriptPubKey manager type.");
        }
        LOCK(desc_spk_man->cs_desc_man);
        const auto& wallet_descriptor = desc_spk_man->GetWalletDescriptor();
        std::string descriptor;
        CHECK_NONFATAL(desc_spk_man->GetDescriptorString(descriptor, priv));
        const bool is_range = wallet_descriptor.descriptor->IsRange();
        wallet_descriptors.push_back({
            descriptor,
            wallet_descriptor.creation_time,
            active_spk_mans.contains(desc_spk_man),
            wallet->IsInternalScriptPubKeyMan(desc_spk_man),
            is_range ? std::optional(std::make_pair(wallet_descriptor.range_start, wallet_descriptor.range_end)) : std::nullopt,
            wallet_descriptor.next_index
        });
    }

    std::sort(wallet_descriptors.begin(), wallet_descriptors.end(), [](const auto& a, const auto& b) {
        return a.descriptor < b.descriptor;
    });

    UniValue descriptors(UniValue::VARR);
    for (const WalletDescInfo& info : wallet_descriptors) {
        UniValue spk(UniValue::VOBJ);
        spk.pushKV("desc", info.descriptor);
        spk.pushKV("timestamp", info.creation_time);
        spk.pushKV("active", info.active);
        if (info.internal.has_value()) {
            spk.pushKV("internal", info.internal.value());
        }
        if (info.range.has_value()) {
            UniValue range(UniValue::VARR);
            range.push_back(info.range->first);
            range.push_back(info.range->second - 1);
            spk.pushKV("range", std::move(range));
            spk.pushKV("next", info.next_index);
            spk.pushKV("next_index", info.next_index);
        }
        descriptors.push_back(std::move(spk));
    }

    UniValue response(UniValue::VOBJ);
    response.pushKV("wallet_name", wallet->GetName());
    response.pushKV("descriptors", std::move(descriptors));

    return response;
},
    };
}

RPCMethod backupwallet()
{
    return RPCMethod{
        "backupwallet",
        "Safely copies the current wallet file to the specified destination, which can either be a directory or a path with a filename.\n",
                {
                    {"destination", RPCArg::Type::STR, RPCArg::Optional::NO, "The destination directory or file"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("backupwallet", "\"backup.dat\"")
            + HelpExampleRpc("backupwallet", "\"backup.dat\"")
                },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    std::string strDest = request.params[0].get_str();
    if (!pwallet->BackupWallet(strDest)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");
    }

    return UniValue::VNULL;
},
    };
}


RPCMethod restorewallet()
{
    return RPCMethod{
        "restorewallet",
        "Restores and loads a wallet from backup.\n"
        "\nThe rescan is significantly faster if block filters are available"
        "\n(using startup option \"-blockfilterindex=1\").\n",
        {
            {"wallet_name", RPCArg::Type::STR, RPCArg::Optional::NO, "The name that will be applied to the restored wallet"},
            {"backup_file", RPCArg::Type::STR, RPCArg::Optional::NO, "The backup file that will be used to restore the wallet."},
            {"load_on_startup", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Save wallet name to persistent settings and load on startup. True to add wallet to startup list, false to remove, null to leave unchanged."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "name", "The wallet name if restored successfully."},
                {RPCResult::Type::ARR, "warnings", /*optional=*/true, "Warning messages, if any, related to restoring and loading the wallet.",
                {
                    {RPCResult::Type::STR, "", ""},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("restorewallet", "\"testwallet\" \"home\\backups\\backup-file.bak\"")
            + HelpExampleRpc("restorewallet", "\"testwallet\" \"home\\backups\\backup-file.bak\"")
            + HelpExampleCliNamed("restorewallet", {{"wallet_name", "testwallet"}, {"backup_file", "home\\backups\\backup-file.bak\""}, {"load_on_startup", true}})
            + HelpExampleRpcNamed("restorewallet", {{"wallet_name", "testwallet"}, {"backup_file", "home\\backups\\backup-file.bak\""}, {"load_on_startup", true}})
        },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{

    WalletContext& context = EnsureWalletContext(request.context);

    auto backup_file = fs::u8path(request.params[1].get_str());

    std::string wallet_name = request.params[0].get_str();

    std::optional<bool> load_on_start = request.params[2].isNull() ? std::nullopt : std::optional<bool>(request.params[2].get_bool());

    DatabaseStatus status;
    bilingual_str error;
    std::vector<bilingual_str> warnings;

    const std::shared_ptr<CWallet> wallet = RestoreWallet(context, backup_file, wallet_name, load_on_start, status, error, warnings);

    HandleWalletError(wallet, status, error);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("name", wallet->GetName());
    PushWarnings(warnings, obj);

    return obj;

},
    };
}
} // namespace wallet
