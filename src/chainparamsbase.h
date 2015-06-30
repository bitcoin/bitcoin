// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMSBASE_H
#define BITCOIN_CHAINPARAMSBASE_H

#include "templates.hpp"

#include <string>
#include <vector>

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
    static const std::string MAX_NETWORK_TYPES;

    const std::string& DataDir() const { return strDataDir; }
    int RPCPort() const { return nRPCPort; }
    /**
     * Creates and returns a CBaseChainParams* of the chosen chain. The caller has to delete the object.
     * @returns A CBaseChainParams* of the chosen chain.
     * @throws a std::runtime_error if the chain is not supported.
     */
    static CBaseChainParams* Factory(const std::string& chain);
protected:
    CBaseChainParams() {}

    int nRPCPort;
    std::string strDataDir;
};

/**
 * Append the help messages for the chainparams options to the
 * parameter string.
 */
void AppendParamsHelpMessages(std::string& strUsage, bool debugHelp=true);

/**
 * Looks for -regtest, -testnet and returns the appropriate BIP70 chain name.
 * @return CBaseChainParams::MAX_NETWORK_TYPES if an invalid combination is given. CBaseChainParams::MAIN by default.
 */
std::string ChainNameFromCommandLine();

extern Container<CBaseChainParams> cGlobalChainBaseParams;

#endif // BITCOIN_CHAINPARAMSBASE_H
