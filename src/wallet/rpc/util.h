// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_RPC_UTIL_H
#define SYSCOIN_WALLET_RPC_UTIL_H

#include <any>
#include <memory>
#include <string>
#include <vector>
namespace node {
class JSONRPCRequest;
}
class UniValue;
struct bilingual_str;

namespace wallet {
class CWallet;
class LegacyScriptPubKeyMan;
enum class DatabaseStatus;
struct WalletContext;

extern const std::string HELP_REQUIRING_PASSPHRASE;

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const node::JSONRPCRequest& request);
bool GetWalletNameFromJSONRPCRequest(const node::JSONRPCRequest& request, std::string& wallet_name);

void EnsureWalletIsUnlocked(const CWallet&);
WalletContext& EnsureWalletContext(const std::any& context);
LegacyScriptPubKeyMan& EnsureLegacyScriptPubKeyMan(CWallet& wallet, bool also_create = false);
const LegacyScriptPubKeyMan& EnsureConstLegacyScriptPubKeyMan(const CWallet& wallet);

bool GetAvoidReuseFlag(const CWallet& wallet, const UniValue& param);
bool ParseIncludeWatchonly(const UniValue& include_watchonly, const CWallet& wallet);
std::string LabelFromValue(const UniValue& value);

void HandleWalletError(const std::shared_ptr<CWallet> wallet, DatabaseStatus& status, bilingual_str& error);
} //  namespace wallet

#endif // SYSCOIN_WALLET_RPC_UTIL_H
