// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCHDWALLET_H
#define BITCOIN_WALLET_RPCHDWALLET_H

class CRPCTable;
class CHDWallet;
class JSONRPCRequest;

void RegisterHDWalletRPCCommands(CRPCTable &t);

#endif //BITCOIN_WALLET_RPCHDWALLET_H
