// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAIN_PARAMS_BASE_H
#define BITCOIN_CHAIN_PARAMS_BASE_H

#include <vector>
#include <string>

/**
 * CBaseChainParams defines the base parameters (shared between bitcoin-cli and bitcoind)
 * of a given instance of the Bitcoin system.
 */
class CBaseChainParams
{
public:
    enum Network {
        MAIN,
        TESTNET,
        REGTEST,

        MAX_NETWORK_TYPES
    };

    const std::string& DataDir() const { return strDataDir; }
    int RPCPort() const { return nRPCPort; }
    Network NetworkID() const { return networkID; }
protected:
    CBaseChainParams() {}

    int nRPCPort;
    std::string strDataDir;
    Network networkID;
};

/**
 * Return the currently selected parameters. This won't change after app startup
 * outside of the unit tests.
 */
const CBaseChainParams &BaseParams();

/** Sets the params returned by Params() to those for the given network. */
void SelectBaseParams(CBaseChainParams::Network network);

/**
 * Looks for -regtest or -testnet and then calls SelectParams as appropriate.
 * Returns false if an invalid combination is given.
 */
bool SelectBaseParamsFromCommandLine();

/**
 * Return true if SelectBaseParamsFromCommandLine() has been called to select
 * a network.
 */
bool AreBaseParamsConfigured();

#endif
