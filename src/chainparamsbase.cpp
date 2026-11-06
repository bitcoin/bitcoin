// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <chainparamsbase_settings.h>
#include <common/args.h>
#include <crypto/hex_base.h>
#include <kernel/signet.h>
#include <tinyformat.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <cassert>

void SetupChainParamsBaseOptions(ArgsManager& argsman)
{
    ChainSetting::Register(argsman);
    RegTestSetting::Register(argsman);
    TestActivationHeightSetting::Register(argsman);
    TestNetSetting::Register(argsman);
    TestNet4Setting::Register(argsman);
    VbParamsSetting::Register(argsman);
    SignetSetting::Register(argsman);
    SignetChallengeSetting::Register(argsman);
    SignetSeedNodeSetting::Register(argsman);
}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::string GetSignetDataDir(const ArgsManager& args)
{
    std::string base_data_dir = "signet";
    // no default to treat no -signetchallenge (default signet) as distinct from
    // the empty -signetchallenge= (custom signet)
    const auto challenge_hex = SignetChallengeSettingStr::Get(args);
    if (!challenge_hex) {
        return base_data_dir;
    }
    // -signetchallenge can be invalid hex here (it's checked later in
    // ReadSigNetArgs), but we don't mind to keep validation in one place.
    const auto challenge_bytes = TryParseHex<uint8_t>(*challenge_hex);
    if (!challenge_bytes || *challenge_bytes == kernel::SIGNET_DEFAULT_CHALLENGE) {
        return base_data_dir;
    }
    return base_data_dir + "_" + HexStr(kernel::GetSignetMessageStart(*challenge_bytes));
}

/**
 * Port numbers for incoming Tor connections (8334, 18334, 38334, 48334, 18445) have
 * been chosen arbitrarily to keep ranges of used ports tight.
 */
std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const ArgsManager& args, const ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return std::make_unique<CBaseChainParams>("", 8332);
    case ChainType::TESTNET:
        return std::make_unique<CBaseChainParams>("testnet3", 18332);
    case ChainType::TESTNET4:
        return std::make_unique<CBaseChainParams>("testnet4", 48332);
    case ChainType::SIGNET:
        return std::make_unique<CBaseChainParams>(GetSignetDataDir(args), 38332);
    case ChainType::REGTEST:
        return std::make_unique<CBaseChainParams>("regtest", 18443);
    }
    assert(false);
}

void SelectBaseParams(const ChainType chain)
{
    gArgs.SelectConfigNetwork(ChainTypeToString(chain));
    globalChainBaseParams = CreateBaseChainParams(gArgs, chain);
}
