// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_RPCWALLET_H
#define SYSCOIN_WALLET_RPCWALLET_H

#include <span.h>

#include <any>
#include <memory>
#include <string>
#include <vector>

class CRPCCommand;
class CWallet;
class JSONRPCRequest;
class LegacyScriptPubKeyMan;
class UniValue;
class CTransaction;
struct PartiallySignedTransaction;
struct WalletContext;

Span<const CRPCCommand> GetWalletRPCCommands();
// SYSCOIN
Span<const CRPCCommand> GetAssetWalletRPCCommands();
Span<const CRPCCommand> GetEvoWalletRPCCommands();
Span<const CRPCCommand> GetGovernanceWalletRPCCommands();
Span<const CRPCCommand> GetMasternodeWalletRPCCommands();
/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request);

void EnsureWalletIsUnlocked(const CWallet&);
WalletContext& EnsureWalletContext(const std::any& context);
LegacyScriptPubKeyMan& EnsureLegacyScriptPubKeyMan(CWallet& wallet, bool also_create = false);

RPCHelpMan listunspent();
RPCHelpMan getaddressinfo();
RPCHelpMan signrawtransactionwithwallet();
RPCHelpMan sendrawtransaction();
#endif //SYSCOIN_WALLET_RPCWALLET_H
