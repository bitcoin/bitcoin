// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

#include <span.h>

#include <memory>
#include <string>
#include <vector>

class CRPCCommand;
class CWallet;
struct WalletContext;

Span<const CRPCCommand> GetWalletRPCCommands();

std::tuple<std::shared_ptr<CWallet>, std::vector<bilingual_str>> LoadWalletHelper(WalletContext& context, UniValue load_on_start_param, const std::string wallet_name);

RPCHelpMan getaddressinfo();
RPCHelpMan signrawtransactionwithwallet();
#endif // BITCOIN_WALLET_RPCWALLET_H
