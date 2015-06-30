// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparamsbase.h"

#include "tinyformat.h"
#include "util.h"

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";

void AppendParamsHelpMessages(std::string& strUsage, bool debugHelp)
{
    strUsage += HelpMessageGroup(_("Chain selection options:"));
    strUsage += HelpMessageOpt("-testnet", _("Use the test chain"));
    if (debugHelp) {
        strUsage += HelpMessageOpt("-regtest", _("Enter regression test mode, which uses a special chain in which blocks can be solved instantly.") + " " +
                                   _("This is intended for regression testing tools and app development.") + " " +
                                   _("In this mode -genproclimit controls how many blocks are generated immediately."));
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
        strDataDir = "testnet3";
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
        strDataDir = "regtest";
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

Container<CBaseChainParams> cGlobalChainBaseParams;

CBaseChainParams* CBaseChainParams::Factory(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return new CBaseMainParams();
    else if (chain == CBaseChainParams::TESTNET)
        return new CBaseTestNetParams();
    else if (chain == CBaseChainParams::REGTEST)
        return new CBaseRegTestParams();
    throw std::runtime_error(strprintf(_("%s: Unknown chain %s."), __func__, chain));
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
