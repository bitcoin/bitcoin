// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCSIGNER_H
#define BITCOIN_WALLET_RPCSIGNER_H

#include <util/system.h>
#include <vector>

#ifdef HAVE_BOOST_PROCESS

class CRPCTable;

namespace interfaces {
class Chain;
class Handler;
}

void RegisterSignerRPCCommands(interfaces::Chain& chain, std::vector<std::unique_ptr<interfaces::Handler>>& handlers);

#endif // BOOST_VERSION

#endif //BITCOIN_WALLET_RPCSIGNER_H
