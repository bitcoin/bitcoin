// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparamsbase.h"

#include "tinyformat.h"
#include "util.h"

#include <assert.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::DEVNET = "dev";
const std::string CBaseChainParams::REGTEST = "regtest";

void AppendParamsHelpMessages(std::string& strUsage, bool debugHelp)
{
    strUsage += HelpMessageGroup(_("Chain selection options:"));
    strUsage += HelpMessageOpt("-testnet", _("Use the test chain"));
    strUsage += HelpMessageOpt("-devnet=<name>", _("Use devnet chain with provided name"));
    if (debugHelp) {
        strUsage += HelpMessageOpt("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                                   "This is intended for regression testing tools and app development.");
    }
}

/**
 * Main network
 */
class CBaseMainParams : public CBaseChainParams
{
public:
    CBaseMainParams()
    {
        nRPCPort = 9998;
    }
};

/**
 * Testnet (v3)
 */
class CBaseTestNetParams : public CBaseChainParams
{
public:
    CBaseTestNetParams()
    {
        nRPCPort = 19998;
        strDataDir = "testnet3";
    }
};

/**
 * Devnet
 */
class CBaseDevNetParams : public CBaseChainParams
{
public:
    CBaseDevNetParams(const std::string &dataDir)
    {
        nRPCPort = 19798;
        strDataDir = dataDir;
    }
};

/*
 * Regression test
 */
class CBaseRegTestParams : public CBaseChainParams
{
public:
    CBaseRegTestParams()
    {
        nRPCPort = 19898;
        strDataDir = "regtest";
    }
};

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CBaseChainParams>(new CBaseMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CBaseChainParams>(new CBaseTestNetParams());
    else if (chain == CBaseChainParams::DEVNET) {
        return std::unique_ptr<CBaseChainParams>(new CBaseDevNetParams(GetDevNetName()));
    } else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CBaseChainParams>(new CBaseRegTestParams());
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
}

std::string ChainNameFromCommandLine()
{
    bool fRegTest = gArgs.GetBoolArg("-regtest", false);
    bool fDevNet = gArgs.IsArgSet("-devnet");
    bool fTestNet = gArgs.GetBoolArg("-testnet", false);

    int nameParamsCount = (fRegTest ? 1 : 0) + (fDevNet ? 1 : 0) + (fTestNet ? 1 : 0);
    if (nameParamsCount > 1)
        throw std::runtime_error("Only one of -regtest, -testnet or -devnet can be used.");

    if (fDevNet)
        return CBaseChainParams::DEVNET;
    if (fRegTest)
        return CBaseChainParams::REGTEST;
    if (fTestNet)
        return CBaseChainParams::TESTNET;
    return CBaseChainParams::MAIN;
}

std::string GetDevNetName()
{
    // This function should never be called for non-devnets
    assert(gArgs.IsArgSet("-devnet"));
    std::string devNetName = gArgs.GetArg("-devnet", "");
    return "devnet" + (devNetName.empty() ? "" : "-" + devNetName);
}
