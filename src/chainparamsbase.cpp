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
    argsman.AddArg("-chain=<chain>", "Use the chain <chain> (default: main). Allowed values: " LIST_CHAIN_NAMES, ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                 "This is intended for regression testing tools and app development. Equivalent to -chain=regtest.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-testactivationheight=name@height.", "Set the activation height of 'name' (segwit, bip34, dersig, cltv, csv). (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-testnet", "Use the testnet3 chain. Equivalent to -chain=test. Support for testnet3 is deprecated and will be removed in an upcoming release. Consider moving to testnet4 now by using -testnet4.", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-testnet4", "Use the testnet4 chain. Equivalent to -chain=testnet4.", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-vbparams=deployment:start:end[:min_activation_height]", "Use given start/end times and min_activation_height for specified version bits deployment (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-signet", "Use the signet chain. Equivalent to -chain=signet. Note that the network is defined by the -signetchallenge parameter", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-signetchallenge", "Blocks must satisfy the given script to be considered valid (only for signet networks; defaults to the global default signet test network challenge)", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-signetseednode", "Specify a seed node for the signet network, in the hostname[:port] format, e.g. sig.net:1234 (may be used multiple times to specify multiple seed nodes; defaults to the global default signet test network seed node(s))", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::CHAINPARAMS);
}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::string GetSignetDataDir()
{
    std::string base_data_dir = "signet";
    const std::string signet_challenge = gArgs.GetArg("-signetchallenge", "");
    const std::string default_signet_challenge = "512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae";
    if (!signet_challenge.empty() && signet_challenge != default_signet_challenge) {
        const std::string data_dir_suffix = signet_challenge.substr(0, 16);
        return base_data_dir + "_" + data_dir_suffix;
    }
    return base_data_dir;
}

/**
 * Port numbers for incoming Tor connections (8334, 18334, 38334, 48334, 18445) have
 * been chosen arbitrarily to keep ranges of used ports tight.
 */
std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return std::make_unique<CBaseChainParams>("", 8332);
    case ChainType::TESTNET:
        return std::make_unique<CBaseChainParams>("testnet3", 18332);
    case ChainType::TESTNET4:
        return std::make_unique<CBaseChainParams>("testnet4", 48332);
    case ChainType::SIGNET:
        return std::make_unique<CBaseChainParams>(GetSignetDataDir(), 38332);
    case ChainType::REGTEST:
        return std::make_unique<CBaseChainParams>("regtest", 18443);
    }
    assert(false);
}

void SelectBaseParams(const ChainType chain)
{
    // We need to call SelectConfigNetwork before CreateBaseChainParams since we
    // check -signetchallenge in CreateBaseChainParams to determine the signet
    // datadir.
    gArgs.SelectConfigNetwork(ChainTypeToString(chain));
    globalChainBaseParams = CreateBaseChainParams(chain);
}
