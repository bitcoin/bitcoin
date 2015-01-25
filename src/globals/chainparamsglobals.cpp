// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "globals/chainparamsglobals.h"

#include "chainparams.h"
#include "globals/chainparamsbaseglobals.h"

Container<CChainParams> cGlobalChainParams;
static Container<CChainParams> cGlobalSwitchingChainParams;

const CChainParams& Params()
{
    return cGlobalChainParams.Get();
}

const CChainParams& Params(const std::string& chain)
{
    cGlobalSwitchingChainParams.Set(CChainParams::Factory(chain));
    return cGlobalSwitchingChainParams.Get();
}

void SelectParams(const std::string& network)
{
    cGlobalChainBaseParams.Set(CBaseChainParams::Factory(network));
    cGlobalChainParams.Set(CChainParams::Factory(network));
}
