// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_RPC_WALLET_H
#define SYSCOIN_WALLET_RPC_WALLET_H

#include <span.h>

class CRPCCommand;

namespace wallet {
Span<const CRPCCommand> GetWalletRPCCommands();
// SYSCOIN
Span<const CRPCCommand> GetAssetWalletRPCCommands();
Span<const CRPCCommand> GetEvoWalletRPCCommands();
Span<const CRPCCommand> GetGovernanceWalletRPCCommands();
Span<const CRPCCommand> GetMasternodeWalletRPCCommands();
} // namespace wallet
#endif // SYSCOIN_WALLET_RPC_WALLET_H
