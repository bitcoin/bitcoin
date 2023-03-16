// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <hash.h> // for signet block challenge hash
#include <script/interpreter.h>
#include <util/string.h>
#include <util/system.h>

#include <assert.h>

void ReadSigNetArgs(const ArgsManager& args, CChainParams::SigNetOptions& options)
{
    if (args.IsArgSet("-signetseednode")) {
        options.seeds.emplace(args.GetArgs("-signetseednode"));
    }
    if (args.IsArgSet("-signetchallenge")) {
        const auto signet_challenge = args.GetArgs("-signetchallenge");
        if (signet_challenge.size() != 1) {
            throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
        }
        options.challenge.emplace(ParseHex(signet_challenge[0]));
    }
}
void ReadRegTestArgs(const ArgsManager& args, CChainParams::RegTestOptions& options)
{
    if (auto value = args.GetBoolArg("-fastprune")) options.fastprune = *value;

    for (const std::string& arg : args.GetArgs("-testactivationheight")) {
        const auto found{arg.find('@')};
        if (found == std::string::npos) {
            throw std::runtime_error(strprintf("Invalid format (%s) for -testactivationheight=name@height.", arg));
        }

        const auto value{arg.substr(found + 1)};
        int32_t height;
        if (!ParseInt32(value, &height) || height < 0 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Invalid height value (%s) for -testactivationheight=name@height.", arg));
        }

        const auto deployment_name{arg.substr(0, found)};
        if (const auto buried_deployment = GetBuriedDeployment(deployment_name)) {
            options.activation_heights[*buried_deployment] = height;
        } else {
            throw std::runtime_error(strprintf("Invalid name (%s) for -testactivationheight=name@height.", arg));
        }
    }
    // SYSCOIN
    if (args.IsArgSet("-mncollateral")) {
        uint32_t collateral = args.GetIntArg("-mncollateral", DEFAULT_MN_COLLATERAL_REQUIRED);
        nMNCollateralRequired = collateral*COIN;
    }
    if (args.IsArgSet("-dip3params")) {
        std::string strDIP3Params = args.GetArg("-dip3params", "");
        std::vector<std::string> vDIP3Params = SplitString(strDIP3Params, ':');
        if (vDIP3Params.size() != 2) {
            throw std::runtime_error("DIP3 parameters malformed, expecting DIP3ActivationHeight:DIP3EnforcementHeight");
        }
        if (!ParseInt32(vDIP3Params[0], &options.dip3startblock)) {
            throw std::runtime_error(strprintf("Invalid nDIP3ActivationHeight (%s)", vDIP3Params[0]));
        }
        if (!ParseInt32(vDIP3Params[1], &options.dip3enforcement)) {
            throw std::runtime_error(strprintf("Invalid nDIP3EnforcementHeight (%s)", vDIP3Params[1]));
        }
    }
    else if (args.IsArgSet("-dip19params")) {
        std::string strDIP19Params = args.GetArg("-dip19params", "");
        if (!ParseInt32(strDIP19Params, &options.v19startblock)) {
            throw std::runtime_error(strprintf("Invalid nDIP19ActivationHeight (%s)", strDIP19Params));
        }
    }
    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams = SplitString(strDeployment, ':');
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        CChainParams::VersionBitsParameters vbparams{};
        if (!ParseInt64(vDeploymentParams[1], &vbparams.start_time)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &vbparams.timeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4) {
            if (!ParseInt32(vDeploymentParams[3], &vbparams.min_activation_height)) {
                throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
            }
        } else {
            vbparams.min_activation_height = 0;
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                options.version_bits_parameters[Consensus::DeploymentPos(j)] = vbparams;
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], vbparams.start_time, vbparams.timeout, vbparams.min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return CChainParams::Main();
    } else if (chain == CBaseChainParams::TESTNET) {
        return CChainParams::TestNet();
    } else if (chain == CBaseChainParams::SIGNET) {
        auto opts = CChainParams::SigNetOptions{};
        ReadSigNetArgs(args, opts);
        return CChainParams::SigNet(opts);
    } else if (chain == CBaseChainParams::REGTEST) {
        auto opts = CChainParams::RegTestOptions{};
        ReadRegTestArgs(args, opts);
        return CChainParams::RegTest(opts);
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}
void UpdateLLMQTestParams(int size, int threshold)
{
    auto* params = const_cast<CChainParams*> (globalChainParams.get ());
    params->UpdateLLMQTestParams(size, threshold);
}