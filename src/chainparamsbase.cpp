// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>
#include <util/system.h>

#include <assert.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::DEVNET = "devnet";
const std::string CBaseChainParams::REGTEST = "regtest";

void SetupChainParamsBaseOptions(ArgsManager& argsman)
{
    argsman.AddArg("-budgetparams=<masternode>:<budget>:<superblock>", "Override masternode, budget and superblock start heights (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-devnet=<name>", "Use devnet chain with provided name", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-dip3params=<activation>:<enforcement>", "Override DIP3 activation and enforcement heights (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-dip8params=<activation>", "Override DIP8 activation height (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-highsubsidyblocks=<n>", "The number of blocks with a higher than normal subsidy to mine at the start of a chain (default: 0, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-highsubsidyfactor=<n>", "The factor to multiply the normal block subsidy by while in the highsubsidyblocks window of a chain (default: 1, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqchainlocks=<quorum name>", "Override the default LLMQ type used for ChainLocks. Allows using ChainLocks with smaller LLMQs. (default: llmq_50_60, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqdevnetparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_DEVNET quorum (default: 3:2, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqinstantsend=<quorum name>", "Override the default LLMQ type used for InstantSend. Allows using InstantSend with smaller LLMQs. (default: llmq_50_60, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqinstantsenddip0024=<quorum name>", "Override the default LLMQ type used for InstantSendDIP0024. (default: llmq_60_75, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqplatform=<quorum name>", "Override the default LLMQ type used for Platform. (default: llmq_100_67, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqtestparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_TEST quorum (default: 3:2, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqtestinstantsendparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_TEST_INSTANTSEND quorums (default: 3:2, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-powtargetspacing=<n>", "Override the default PowTargetSpacing value in seconds (default: 2.5 minutes, devnet-only)", ArgsManager::ALLOW_INT, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-minimumdifficultyblocks=<n>", "The number of blocks that can be mined with the minimum difficulty at the start of a chain (default: 0, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-chain=<chain>", "Use the chain <chain> (default: main). Allowed values: main, test, regtest", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                   "This is intended for regression testing tools and app development. Equivalent to -chain=regtest", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-testnet", "Use the test chain. Equivalent to -chain=test", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-vbparams=<deployment>:<start>:<end>(:<window>:<threshold/thresholdstart>(:<thresholdmin>:<falloffcoeff>:<mnactivation>))",
                 "Use given start/end times for specified version bits deployment (regtest-only). "
                 "Specifying window, threshold/thresholdstart, thresholdmin, falloffcoeff and mnactivation is optional.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);

}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

/**
 * Port numbers for incoming Tor connections (9996, 19996, 19796, 19896) have
 * been chosen arbitrarily to keep ranges of used ports tight.
 */
std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::make_unique<CBaseChainParams>("", 9998, 9996);
    else if (chain == CBaseChainParams::TESTNET)
        return std::make_unique<CBaseChainParams>("testnet3", 19998, 19996);
    else if (chain == CBaseChainParams::DEVNET)
        return std::make_unique<CBaseChainParams>(gArgs.GetDevNetName(), 19798, 19796);
    else if (chain == CBaseChainParams::REGTEST)
        return std::make_unique<CBaseChainParams>("regtest", 19898, 19896);
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(chain);
}
