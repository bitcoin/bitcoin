// Copyright (c) 2014-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMSBASE_H
#define BITCOIN_CHAINPARAMSBASE_H

#include <string>

enum class Chain {
    MAIN,
    TESTNET,
    REGTEST
};

/** @return the BIP70 name for the chain (main, test or regtest) */
std::string GetChainName(const Chain chain);

/** @return the data dir for the chain */
std::string GetDataDir(const Chain chain);

/** @return the rpc port for the chain */
int GetRPCPort(const Chain chain);

#endif // BITCOIN_CHAINPARAMSBASE_H
