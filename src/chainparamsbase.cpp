// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparamsbase.h"

#include "tinyformat.h"
#include "util.h"

#include <assert.h>
#include <algorithm>

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
// Network type and each name
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
static std::map<NetworkType, std::string> _NetworkTypeNames = {
    { NETWORK_MAIN, "main" },
    { NETWORK_TESTNET, "test" },
    { NETWORK_REGTEST, "regtest" },
};
std::string NetworkType2String(NetworkType networkType)
{
    return _NetworkTypeNames[networkType];
}
NetworkType String2NetworkType(const std::string& networkTypeName)
{
    auto itr = find_if(_NetworkTypeNames.begin(), _NetworkTypeNames.end(),
        [networkTypeName](const std::pair<NetworkType, std::string>& s) { return s.second == networkTypeName; });
    return itr->first;
}
/*
const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";
*/

void AppendParamsHelpMessages(std::string& strUsage, bool debugHelp)
{
    strUsage += HelpMessageGroup(_("Chain selection options:"));
    strUsage += HelpMessageOpt("-testnet", _("Use the test chain"));
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
        nRPCPort = 8332;
    }
};
static CBaseMainParams g_baseMainParams;

/**
 * Testnet (v3)
 */
class CBaseTestNetParams : public CBaseChainParams
{
public:
    CBaseTestNetParams()
    {
        nRPCPort = 18332;
        strDataDir = "testnet3";
    }
};
static CBaseTestNetParams g_baseTestNetParams;

/*
 * Regression test
 */
class CBaseRegTestParams : public CBaseChainParams
{
public:
    CBaseRegTestParams()
    {
        nRPCPort = 18332;
        strDataDir = "regtest";
    }
};
static CBaseRegTestParams g_baseRegTestParams;

static CBaseChainParams* pCurrentBaseParams = 0;

const CBaseChainParams& BaseParams()
{
    assert(pCurrentBaseParams);
    return *pCurrentBaseParams;
}

CBaseChainParams& BaseParams(NetworkType chain)
{
    if (chain == NETWORK_MAIN)
        return g_baseMainParams;
    else if (chain == NETWORK_TESTNET)
        return g_baseTestNetParams;
    else if (chain == NETWORK_REGTEST)
        return g_baseRegTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectBaseParams(NetworkType chain)
{
    pCurrentBaseParams = &BaseParams(chain);
}

NetworkType ChainNameFromCommandLine()
{
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest)
        throw std::runtime_error("Invalid combination of -regtest and -testnet.");
    if (fRegTest)
        return NETWORK_REGTEST;
    if (fTestNet)
        return NETWORK_TESTNET;
    return NETWORK_MAIN;
}

bool AreBaseParamsConfigured()
{
    return pCurrentBaseParams != NULL;
}
