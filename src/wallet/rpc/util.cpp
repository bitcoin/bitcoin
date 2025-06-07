// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/rpc/util.h>

#include <common/url.h>
#include <rpc/util.h>
#include <util/any.h>
#include <util/translation.h>
#include <wallet/context.h>
#include <wallet/wallet.h>

#include <string_view>
#include <univalue.h>

namespace wallet {
static const std::string WALLET_ENDPOINT_BASE = "/wallet/";
const std::string HELP_REQUIRING_PASSPHRASE{"\nRequires wallet passphrase to be set with walletpassphrase call if wallet is encrypted.\n"};

bool GetAvoidReuseFlag(const CWallet& wallet, const UniValue& param) {
    bool can_avoid_reuse = wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);
    bool avoid_reuse = param.isNull() ? can_avoid_reuse : param.get_bool();

    if (avoid_reuse && !can_avoid_reuse) {
        throw JSONRPCError(RPC_WALLET_ERROR, "wallet does not have the \"avoid reuse\" feature enabled");
    }

    return avoid_reuse;
}

bool GetWalletNameFromJSONRPCRequest(const JSONRPCRequest& request, std::string& wallet_name)
{
    if (request.URI.starts_with(WALLET_ENDPOINT_BASE)) {
        // wallet endpoint was used
        wallet_name = UrlDecode(std::string_view{request.URI}.substr(WALLET_ENDPOINT_BASE.size()));
        return true;
    }
    return false;
}

std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request)
{
    CHECK_NONFATAL(request.mode == JSONRPCRequest::EXECUTE);
    WalletContext& context = EnsureWalletContext(request.context);

    std::string wallet_name;
    if (GetWalletNameFromJSONRPCRequest(request, wallet_name)) {
        std::shared_ptr<CWallet> pwallet = GetWallet(context, wallet_name);
        if (!pwallet) throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Requested wallet does not exist or is not loaded");
        return pwallet;
    }

    size_t count{0};
    auto wallet = GetDefaultWallet(context, count);
    if (wallet) return wallet;

    if (count == 0) {
        throw JSONRPCError(
            RPC_WALLET_NOT_FOUND, "No wallet is loaded. Load a wallet using loadwallet or create a new one with createwallet. (Note: A default wallet is no longer automatically created)");
    }
    throw JSONRPCError(RPC_WALLET_NOT_SPECIFIED,
        "Multiple wallets are loaded. Please select which wallet to use by requesting the RPC through the /wallet/<walletname> URI path.");
}

void EnsureWalletIsUnlocked(const CWallet& wallet)
{
    if (wallet.IsLocked()) {
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    }
}

WalletContext& EnsureWalletContext(const std::any& context)
{
    auto wallet_context = util::AnyPtr<WalletContext>(context);
    if (!wallet_context) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Wallet context not found");
    }
    return *wallet_context;
}

std::string LabelFromValue(const UniValue& value)
{
    static const std::string empty_string;
    if (value.isNull()) return empty_string;

    const std::string& label{value.get_str()};
    if (label == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_LABEL_NAME, "Invalid label name");
    return label;
}

void PushParentDescriptors(const CWallet& wallet, const CScript& script_pubkey, UniValue& entry)
{
    UniValue parent_descs(UniValue::VARR);
    for (const auto& desc: wallet.GetWalletDescriptors(script_pubkey)) {
        std::string desc_str;
        FlatSigningProvider dummy_provider;
        if (!CHECK_NONFATAL(desc.descriptor->ToNormalizedString(dummy_provider, desc_str, &desc.cache))) continue;
        parent_descs.push_back(desc_str);
    }
    entry.pushKV("parent_descs", std::move(parent_descs));
}

void HandleWalletError(const std::shared_ptr<CWallet> wallet, DatabaseStatus& status, bilingual_str& error)
{
    if (!wallet) {
        // Map bad format to not found, since bad format is returned when the
        // wallet directory exists, but doesn't contain a data file.
        RPCErrorCode code = RPC_WALLET_ERROR;
        switch (status) {
            case DatabaseStatus::FAILED_NOT_FOUND:
            case DatabaseStatus::FAILED_BAD_FORMAT:
            case DatabaseStatus::FAILED_LEGACY_DISABLED:
                code = RPC_WALLET_NOT_FOUND;
                break;
            case DatabaseStatus::FAILED_ALREADY_LOADED:
                code = RPC_WALLET_ALREADY_LOADED;
                break;
            case DatabaseStatus::FAILED_ALREADY_EXISTS:
                code = RPC_WALLET_ALREADY_EXISTS;
                break;
            case DatabaseStatus::FAILED_INVALID_BACKUP_FILE:
                code = RPC_INVALID_PARAMETER;
                break;
            default: // RPC_WALLET_ERROR is returned for all other cases.
                break;
        }
        throw JSONRPCError(code, error.original);
    }
}

void AppendLastProcessedBlock(UniValue& entry, const CWallet& wallet)
{
    AssertLockHeld(wallet.cs_wallet);
    UniValue lastprocessedblock{UniValue::VOBJ};
    lastprocessedblock.pushKV("hash", wallet.GetLastBlockHash().GetHex());
    lastprocessedblock.pushKV("height", wallet.GetLastBlockHeight());
    entry.pushKV("lastprocessedblock", std::move(lastprocessedblock));
}

} // namespace wallet
