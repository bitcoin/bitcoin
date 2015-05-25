// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparamsbase.h"

#include "templates.hpp"
#include "tinyformat.h"
#include "util.h"

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "testnet3";
const std::string CBaseChainParams::REGTEST = "regtest";
const std::string CBaseChainParams::MAX_NETWORK_TYPES = "unknown_chain";

std::string GetParamsHelpMessages()
{
    std::string strUsage = "";
    strUsage += HelpMessageOpt("-testnet", _("Use the test chain"));
    strUsage += HelpMessageOpt("-regtest", _("Enter regression test mode, which uses a special chain in which blocks can be solved instantly.") + " " +
        _("This is intended for regression testing tools and app development.") + " " +
        _("In this mode -genproclimit controls how many blocks are generated immediately."));
    return strUsage;
}

/**
 * Main network
 */
class CBaseMainParams : public CBaseChainParams
{
public:
    CBaseMainParams()
    {
        nRPCPort = 8332;
    }
};

/**
 * Testnet (v3)
 */
class CBaseTestNetParams : public CBaseMainParams
{
public:
    CBaseTestNetParams()
    {
        nRPCPort = 18332;
        strDataDir = CBaseChainParams::TESTNET;
    }
};

/*
 * Regression test
 */
class CBaseRegTestParams : public CBaseTestNetParams
{
public:
    CBaseRegTestParams()
    {
        strDataDir = CBaseChainParams::REGTEST;
    }
};

/*
 * Unit test
 */
class CBaseUnitTestParams : public CBaseMainParams
{
public:
    CBaseUnitTestParams()
    {
        strDataDir = "unittest";
    }
};
static CBaseUnitTestParams unitTestParams;

static Container<CBaseChainParams> currentBaseParams;

const CBaseChainParams& BaseParams()
{
    return currentBaseParams.Get();
}

CBaseChainParams* FactoryBaseParams(std::string chain)
{
    if (chain == CBaseChainParams::MAIN)
        return new CBaseMainParams();
    else if (chain == CBaseChainParams::TESTNET)
        return new CBaseTestNetParams();
    else if (chain == CBaseChainParams::REGTEST)
        return new CBaseRegTestParams();
    throw std::runtime_error(strprintf(_("%s: Unknown chain %s."), __func__, chain));
}

void SelectBaseParams(std::string chain)
{
    currentBaseParams.Set(FactoryBaseParams(chain));
}

std::string ChainNameFromCommandLine()
{
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest)
        throw std::runtime_error(_("Invalid combination of -regtest and -testnet."));
    if (fRegTest)
        return CBaseChainParams::REGTEST;
    if (fTestNet)
        return CBaseChainParams::TESTNET;
    return CBaseChainParams::MAIN;
}

void SelectBaseParamsFromCommandLine()
{
    SelectBaseParams(ChainNameFromCommandLine());
}

bool AreBaseParamsConfigured()
{
    return !currentBaseParams.IsNull();
}
