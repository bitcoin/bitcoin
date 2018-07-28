// Copyright (c) 2014-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMSBASE_H
#define BITCOIN_CHAINPARAMSBASE_H

#include <string>

/**
 * CBaseChainParams defines the base parameters (shared between bitcoin-cli and bitcoind)
 * of a given instance of the Bitcoin system.
 */
class CBaseChainParams
{
public:
    /** BIP70 chain name strings (main, test or regtest) */
    static const std::string MAIN;
    static const std::string TESTNET;
    static const std::string REGTEST;
};

enum class Chain {
    MAIN,
    TESTNET,
    REGTEST
};

/** @return the name for the chain */
std::string GetChainName(const Chain chain);

/** @return the data dir for the chain */
std::string GetDataDir(const Chain chain);

/** @return the rpc port for the chain */
int GetRPCPort(const Chain chain);

#endif // BITCOIN_CHAINPARAMSBASE_H
