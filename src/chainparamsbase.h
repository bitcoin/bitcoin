// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMSBASE_H
#define BITCOIN_CHAINPARAMSBASE_H

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

protected:
    CBaseChainParams() {}

    int nRPCPort;
    std::string strDataDir;
};

/**
 * Creates a CBaseChainParams of the chosen chain and returns a
 * pointer to it. The caller has to delete the object.
 * Raises an error if the chain is not supported.
 */
CBaseChainParams* FactoryBaseParams(std::string chain);
/**
 * Returns a string with the help messages for the chainparams options. 
 */
std::string GetParamsHelpMessages();
/**
 * Looks for -regtest, -testnet and returns the appropriate BIP70 chain name.
 * Returns CBaseChainParams::MAIN by default.
 * Returns CBaseChainParams::MAX_NETWORK_TYPES if an invalid combination is given.
 */
std::string ChainNameFromCommandLine();

/** Functions that relay on internal state */
/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CBaseChainParams& BaseParams();

/**
 * Sets the params returned by BaseParams() to those for the appropriate chain returned by ChainNameFromCommandLine().
 * Raises a std::runtime_error if an unsupported chain name is given.
 */
void SelectBaseParams(std::string chain);

/**
 * Calls NetworkIdFromCommandLine() and then calls SelectParams as appropriate.
 * Raises a std::runtime_error if an invalid combination is given or if the chain is not supported.
 */
void SelectBaseParamsFromCommandLine();

/**
 * Return true if SelectBaseParamsFromCommandLine() has been called to select a chain.
 */
bool AreBaseParamsConfigured();

#endif // BITCOIN_CHAINPARAMSBASE_H
