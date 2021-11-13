// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>
#include <util/system.h>
#include <util/memory.h>

#include <assert.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::DEVNET = "devnet";
const std::string CBaseChainParams::REGTEST = "regtest";

void SetupChainParamsBaseOptions()
{
    gArgs.AddArg("-budgetparams=<masternode>:<budget>:<superblock>", "Override masternode, budget and superblock start heights (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-devnet=<name>", "Use devnet chain with provided name", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-dip3params=<activation>:<enforcement>", "Override DIP3 activation and enforcement heights (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-dip8params=<activation>", "Override DIP8 activation height (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-highsubsidyblocks=<n>", "The number of blocks with a higher than normal subsidy to mine at the start of a chain (default: 0, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-highsubsidyfactor=<n>", "The factor to multiply the normal block subsidy by while in the highsubsidyblocks window of a chain (default: 1, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-llmqchainlocks=<quorum name>", "Override the default LLMQ type used for ChainLocks. Allows using ChainLocks with smaller LLMQs. (default: llmq_50_60, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-llmqdevnetparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_DEVNET quorum (default: 3:2, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-llmqinstantsend=<quorum name>", "Override the default LLMQ type used for InstantSend. Allows using InstantSend with smaller LLMQs. (default: llmq_50_60, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-llmqtestparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_TEST quorum (default: 10:6, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-minimumdifficultyblocks=<n>", "The number of blocks that can be mined with the minimum difficulty at the start of a chain (default: 0, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                                   "This is intended for regression testing tools and app development.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-testnet", "Use the test chain", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-vbparams=<deployment>:<start>:<end>(:<window>:<threshold/thresholdstart>(:<thresholdmin>:<falloffcoeff>))",
                 "Use given start/end times for specified version bits deployment (regtest-only). "
                 "Specifying window, threshold/thresholdstart, thresholdmin and falloffcoeff is optional.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);

}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return MakeUnique<CBaseChainParams>("", 9998);
    else if (chain == CBaseChainParams::TESTNET)
        return MakeUnique<CBaseChainParams>("testnet3", 19998);
    else if (chain == CBaseChainParams::DEVNET)
        return MakeUnique<CBaseChainParams>(gArgs.GetDevNetName(), 19798);
    else if (chain == CBaseChainParams::REGTEST)
        return MakeUnique<CBaseChainParams>("regtest", 19898);
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(chain);
}
