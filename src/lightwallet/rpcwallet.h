// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LIGHTWALLET_RPCWALLET_H
#define BITCOIN_LIGHTWALLET_RPCWALLET_H

class CRPCTable;
namespace Lightwallet {
void RegisterWalletRPCCommands(CRPCTable &tableRPC);
}
#endif //BITCOIN_LIGHTWALLET_RPCWALLET_H
