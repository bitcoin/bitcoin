// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_RPCWALLET_H
#define SYSCOIN_WALLET_RPCWALLET_H

#include <span.h>

class CRPCCommand;

Span<const CRPCCommand> GetWalletRPCCommands();
// SYSCOIN
Span<const CRPCCommand> GetAssetWalletRPCCommands();
Span<const CRPCCommand> GetEvoWalletRPCCommands();
Span<const CRPCCommand> GetGovernanceWalletRPCCommands();
Span<const CRPCCommand> GetMasternodeWalletRPCCommands();
RPCHelpMan getaddressinfo();
RPCHelpMan signrawtransactionwithwallet();
#endif // SYSCOIN_WALLET_RPCWALLET_H
