// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <common/args.h>
#include <tinyformat.h>
#include <util/chaintype.h>

#include <cassert>

void SetupChainParamsBaseOptions(ArgsManager& argsman)
{
    // MAIN network only - testnet, signet, regtest DISABLED
    argsman.AddArg("-chain=<chain>", "Use the chain <chain> (default: main). Only MAIN is supported.", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return std::make_unique<CBaseChainParams>("", 8332);
    case ChainType::TESTNET:
    case ChainType::TESTNET4:
    case ChainType::SIGNET:
    case ChainType::REGTEST:
        throw std::runtime_error("ERROR: Testnet, Testnet4, Signet, and Regtest are DISABLED. Only MAIN network is supported.");
    }
    assert(false);
}

void SelectBaseParams(const ChainType chain)
{
    if (chain != ChainType::MAIN) {
        throw std::runtime_error("ERROR: Only MAIN network supported. Test networks disabled.");
    }
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(ChainTypeToString(chain));
}
