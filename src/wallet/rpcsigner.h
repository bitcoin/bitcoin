// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCSIGNER_H
#define BITCOIN_WALLET_RPCSIGNER_H

#include <span.h>
#include <util/system.h>
#include <vector>

#ifdef ENABLE_EXTERNAL_SIGNER

class CRPCCommand;

namespace interfaces {
class Chain;
class Handler;
}

Span<const CRPCCommand> GetSignerRPCCommands();

#endif // ENABLE_EXTERNAL_SIGNER

#endif //BITCOIN_WALLET_RPCSIGNER_H
