// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>
#include <util.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";

void SetupChainParamsBaseOptions()
{
    gArgs.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                                   "This is intended for regression testing tools and app development.", true, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-testnet", "Use the test chain", false, OptionsCategory::CHAINPARAMS);
}

std::string GetDataDir(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return "";
    } else if (chain == CBaseChainParams::TESTNET) {
        return "testnet3";
    } else if (chain == CBaseChainParams::REGTEST) {
        return "regtest";
    } else {
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
    }
}

int GetRPCPort(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return 8332;
    } else if (chain == CBaseChainParams::TESTNET) {
        return 18332;
    } else if (chain == CBaseChainParams::REGTEST) {
        return 18443;
    } else {
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
    }
}
