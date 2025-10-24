// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETTOOL_H
#define BITCOIN_WALLET_WALLETTOOL_H

#include <string>

class ArgsManager;

namespace interfaces {
class Chain;
} // namespace interfaces

namespace wallet {
namespace WalletTool {

bool ExecuteWalletToolFunc(const ArgsManager& args, interfaces::Chain* chain, const std::string& command);

} // namespace WalletTool
} // namespace wallet

#endif // BITCOIN_WALLET_WALLETTOOL_H
