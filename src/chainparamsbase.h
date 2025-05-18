// Copyright (c) 2014-2020 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_CHAINPARAMSBASE_H
#define TORTOISECOIN_CHAINPARAMSBASE_H

#include <util/chaintype.h>

#include <cstdint>
#include <memory>
#include <string>

class ArgsManager;

/**
 * CBaseChainParams defines the base parameters (shared between tortoisecoin-cli and tortoisecoind)
 * of a given instance of the Tortoisecoin system.
 */
class CBaseChainParams
{
public:
    const std::string& DataDir() const { return strDataDir; }
    uint16_t RPCPort() const { return m_rpc_port; }

    CBaseChainParams() = delete;
    CBaseChainParams(const std::string& data_dir, uint16_t rpc_port)
        : m_rpc_port(rpc_port), strDataDir(data_dir) {}

private:
    const uint16_t m_rpc_port;
    std::string strDataDir;
};

/**
 * Creates and returns a std::unique_ptr<CBaseChainParams> of the chosen chain.
 */
std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const ChainType chain);

/**
 *Set the arguments for chainparams
 */
void SetupChainParamsBaseOptions(ArgsManager& argsman);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CBaseChainParams& BaseParams();

/** Sets the params returned by Params() to those for the given chain. */
void SelectBaseParams(const ChainType chain);

/** List of possible chain / network names  */
#define LIST_CHAIN_NAMES "main, test, testnet4, signet, regtest"

#endif // TORTOISECOIN_CHAINPARAMSBASE_H
