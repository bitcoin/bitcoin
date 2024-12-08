// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <chainparamsbase_settings.h>
#include <common/args.h>
#include <tinyformat.h>
#include <util/chaintype.h>

#include <assert.h>

void SetupChainParamsBaseOptions(ArgsManager& argsman)
{
    ChainSetting::Register(argsman);
    RegtestSetting::Register(argsman);
    TestactivationheightSetting::Register(argsman);
    TestnetSetting::Register(argsman);
    Testnet4Setting::Register(argsman);
    VbparamsSetting::Register(argsman);
    SignetSetting::Register(argsman);
    SignetchallengeSetting::Register(argsman);
    SignetseednodeSetting::Register(argsman);
}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

/**
 * Port numbers for incoming Tor connections (8334, 18334, 38334, 48334, 18445) have
 * been chosen arbitrarily to keep ranges of used ports tight.
 */
std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return std::make_unique<CBaseChainParams>("", 8332, 8334);
    case ChainType::TESTNET:
        return std::make_unique<CBaseChainParams>("testnet3", 18332, 18334);
    case ChainType::TESTNET4:
        return std::make_unique<CBaseChainParams>("testnet4", 48332, 48334);
    case ChainType::SIGNET:
        return std::make_unique<CBaseChainParams>("signet", 38332, 38334);
    case ChainType::REGTEST:
        return std::make_unique<CBaseChainParams>("regtest", 18443, 18445);
    }
    assert(false);
}

void SelectBaseParams(const ChainType chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(ChainTypeToString(chain));
}
