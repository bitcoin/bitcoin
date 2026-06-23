// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsbase.h>
#include <common/args.h>
#include <consensus/params.h>
#include <deploymentinfo.h>
#include <tinyformat.h>
#include <util/chaintype.h>
#include <util/log.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

using util::SplitString;

static void HandleDeploymentArgs(const ArgsManager& args, CChainParams::DeploymentOptions& options)
{
    // MAIN network only - simplified, no test deployments
}

void ReadMainNetArgs(const ArgsManager& args, CChainParams::MainNetOptions& options)
{
    HandleDeploymentArgs(args, options.dep_opts);
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN: {
        auto opts = CChainParams::MainNetOptions{};
        ReadMainNetArgs(args, opts);
        return CChainParams::Main(opts);
    }
    case ChainType::TESTNET:
    case ChainType::TESTNET4:
    case ChainType::SIGNET:
    case ChainType::REGTEST:
        throw std::runtime_error("ERROR: Test networks (testnet, testnet4, signet, regtest) are DISABLED in this fork.");
    }
    assert(false);
}

void SelectParams(const ChainType chain)
{
    if (chain != ChainType::MAIN) {
        throw std::runtime_error("ERROR: Only MAIN network is supported. Test networks are disabled.");
    }
    SelectBaseParams(chain);
    globalChainParams = CreateChainParams(gArgs, chain);
}
