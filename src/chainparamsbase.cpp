// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparamsbase.h"

#include "tinyformat.h"
#include "util.h"

#include <boost/scoped_ptr.hpp>

const std::string CBaseChainParams::MAIN = CHAINPARAMS_MAIN;
const std::string CBaseChainParams::TESTNET = CHAINPARAMS_TESTNET;

void AppendParamsHelpMessages(std::string& strUsage, bool debugHelp)
{
    strUsage += HelpMessageGroup(_("Chain selection options:"));
    strUsage += HelpMessageOpt("-chain=<chain>", _("Use the chain <chain> (default: main). Allowed values: main, testnet, regtest, sizetest"));
    strUsage += HelpMessageOpt("-testnet", _("Use the test chain"));
    if (debugHelp) {
        strUsage += HelpMessageOpt("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                                   "This is intended for regression testing tools and app development.");
        strUsage += HelpMessageOpt("-blocksize=<n>", strprintf("Max block size to be used with a sizetest chain (default: %u)", 1000000));
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
        strDataDir = "";
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
        nRPCPort = 18332;
        strDataDir = CHAINPARAMS_TESTNET;
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
        nRPCPort = 18332;
        strDataDir = CHAINPARAMS_REGTEST;
    }
};

class CBaseSizeTestParams : public CBaseChainParams
{
public:
    CBaseSizeTestParams()
    {
        uint64_t nMaxBlockSize = GetArg("-blocksize", 1000000);
        strDataDir = strprintf("%s%d", CHAINPARAMS_SIZETEST, nMaxBlockSize);
        nRPCPort = 28333;
    }
};

static boost::scoped_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams.get());
    return *globalChainBaseParams;
}

CBaseChainParams* CBaseChainParams::Factory(const std::string& chain)
{
    if (chain == CHAINPARAMS_MAIN)
        return new CBaseMainParams();
    else if (chain == CHAINPARAMS_TESTNET)
        return new CBaseTestNetParams();
    else if (chain == CHAINPARAMS_REGTEST)
        return new CBaseRegTestParams();
    else if (chain == CHAINPARAMS_SIZETEST)
        return new CBaseSizeTestParams();
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams.reset(CBaseChainParams::Factory(chain));
}

std::string ChainNameFromCommandLine()
{
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest)
        throw std::runtime_error("Invalid combination of -regtest and -testnet.");
    if (fRegTest)
        return CHAINPARAMS_REGTEST;
    if (fTestNet)
        return CHAINPARAMS_TESTNET;
    return GetArg("-chain", CHAINPARAMS_MAIN);
}

bool AreBaseParamsConfigured()
{
    return globalChainBaseParams.get();
}
