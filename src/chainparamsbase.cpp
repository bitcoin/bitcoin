// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>
#include <util.h>
#include <utilmemory.h>

#include <assert.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";

const std::string& FormatChainType(const ChainType& chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return CBaseChainParams::MAIN;
    case ChainType::TESTNET:
        return CBaseChainParams::TESTNET;
    case ChainType::REGTEST:
        return CBaseChainParams::REGTEST;
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

void SetupChainParamsBaseOptions()
{
    gArgs.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                                   "This is intended for regression testing tools and app development.", true, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-testnet", "Use the test chain", false, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-vbparams=deployment:start:end", "Use given start/end times for specified version bits deployment (regtest-only)", true, OptionsCategory::CHAINPARAMS);
}

static std::unique_ptr<const CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::unique_ptr<const CBaseChainParams> CreateBaseChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return MakeUnique<CBaseChainParams>("", 8332);
    else if (chain == CBaseChainParams::TESTNET)
        return MakeUnique<CBaseChainParams>("testnet3", 18332);
    else if (chain == CBaseChainParams::REGTEST)
        return MakeUnique<CBaseChainParams>("regtest", 18443);
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

std::unique_ptr<const CBaseChainParams> CreateBaseChainParams(const ChainType& chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return MakeUnique<CBaseChainParams>("", 8332);
    case ChainType::TESTNET:
        return MakeUnique<CBaseChainParams>("testnet3", 18332);
    case ChainType::REGTEST:
        return MakeUnique<CBaseChainParams>("regtest", 18443);
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(chain);
}

void SelectBaseParams(const ChainType& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(chain);
}
