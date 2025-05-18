// Copyright (c) 2016-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_WALLET_RPC_WALLET_H
#define TORTOISECOIN_WALLET_RPC_WALLET_H

#include <span.h>

class CRPCCommand;

namespace wallet {
Span<const CRPCCommand> GetWalletRPCCommands();
} // namespace wallet

#endif // TORTOISECOIN_WALLET_RPC_WALLET_H
