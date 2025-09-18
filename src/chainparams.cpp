// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsbase.h>
#include <common/args.h>
#include <consensus/params.h>
#include <deploymentinfo.h>
#include <logging.h>
#include <script/script.h>
#include <tinyformat.h>
#include <util/chaintype.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>


using util::SplitString;

void ParseWrappedSignetChallenge(const std::vector<uint8_t>& wrappedChallenge, std::vector<uint8_t>& outParams, std::vector<uint8_t>& outChallenge) {
    if (wrappedChallenge.empty() || wrappedChallenge[0] != OP_RETURN) {
        // Not a wrapped challenge.
        outChallenge = wrappedChallenge;
        return;
    }

    std::vector<uint8_t> params;
    std::vector<uint8_t> challenge;

    const CScript script(wrappedChallenge.begin(), wrappedChallenge.end());
    CScript::const_iterator it = script.begin(), itend = script.end();
    int i;
    for (i = 0; it != itend; i++) {
        if (i > 2) {
            throw std::runtime_error("too many operations in wrapped challenge, must be 3.");
        }
        std::vector<unsigned char> push_data;
        opcodetype opcode;
        if (!script.GetOp(it, opcode, push_data)) {
            throw std::runtime_error(strprintf("failed to parse operation %d in wrapped challenge script.", i));
        }
        if (i == 0) {
            // OP_RETURN.
            continue;
        }
        if (opcode != OP_PUSHDATA1 && opcode != OP_PUSHDATA2 && opcode != OP_PUSHDATA4) {
                throw std::runtime_error(strprintf("operation %d of wrapped challenge script must be a PUSHDATA opcode, got 0x%02x.", i, opcode));
        }
        if (i == 1) {
            params.swap(push_data);
        } else if (i == 2) {
            challenge.swap(push_data);
        }
    }
    if (i != 3) {
        throw std::runtime_error(strprintf("too few operations in wrapped challenge, must be 3, got %d.", i));
    }

    outParams.swap(params);
    outChallenge.swap(challenge);
}

void ParseSignetParams(const std::vector<uint8_t>& params, CChainParams::SigNetOptions& options) {
    if (params.empty()) {
        return;
    }

    // The format of params is extendable in case more fields are added in the future.
    // It is encoded as a concatenation of (field_id, value) tuples, protobuf style.
    // Currently there is only one field defined: pow_target_spacing, whose field_id
    // is 0x01 and the length of encoding is 8 (int64_t). So valid values of params are:
    //  - empty string (checked in if block above),
    //  - 0x01 followed by 8 bytes of pow_target_spacing (9 bytes in total).
    // If length is not 0 and not 9, the value can not be parsed.

    if (params.size() != 1 + 8) {
        throw std::runtime_error(strprintf("signet params must have length %d, got %d.", 1+8, params.size()));
    }
    if (params[0] != 0x01) {
        throw std::runtime_error(strprintf("signet params[0] must be 0x01, got 0x%02x.", params[0]));
    }
    // Parse little-endian 64 bit number to uint64_t.
    const uint8_t* bytes = &params[1];
    const uint64_t value = uint64_t(bytes[0]) | uint64_t(bytes[1])<<8 | uint64_t(bytes[2])<<16 | uint64_t(bytes[3])<<24 |
            uint64_t(bytes[4])<<32 | uint64_t(bytes[5])<<40 | uint64_t(bytes[6])<<48 | uint64_t(bytes[7])<<56;
    auto pow_target_spacing = int64_t(value);
    if (pow_target_spacing <= 0) {
        throw std::runtime_error("signet param pow_target_spacing <= 0.");
    }

    options.pow_target_spacing = pow_target_spacing;
}

void ReadSigNetArgs(const ArgsManager& args, CChainParams::SigNetOptions& options)
{
    if (!args.GetArgs("-signetseednode").empty()) {
        options.seeds.emplace(args.GetArgs("-signetseednode"));
    }
    if (!args.GetArgs("-signetchallenge").empty()) {
        const auto signet_challenge = args.GetArgs("-signetchallenge");
        if (signet_challenge.size() != 1) {
            throw std::runtime_error("-signetchallenge cannot be multiple values.");
        }
        const auto val{TryParseHex<uint8_t>(signet_challenge[0])};
        if (!val) {
            throw std::runtime_error(strprintf("-signetchallenge must be hex, not '%s'.", signet_challenge[0]));
        }
        std::vector<unsigned char> params;
        std::vector<unsigned char> challenge;
        ParseWrappedSignetChallenge(*val, params, challenge);
        ParseSignetParams(params, options);
        options.challenge.emplace(challenge);
    }
}

void ReadRegTestArgs(const ArgsManager& args, CChainParams::RegTestOptions& options)
{
    if (auto value = args.GetBoolArg("-fastprune")) options.fastprune = *value;
    if (HasTestOption(args, "bip94")) options.enforce_bip94 = true;

    for (const std::string& arg : args.GetArgs("-testactivationheight")) {
        const auto found{arg.find('@')};
        if (found == std::string::npos) {
            throw std::runtime_error(strprintf("Invalid format (%s) for -testactivationheight=name@height.", arg));
        }

        const auto value{arg.substr(found + 1)};
        const auto height{ToIntegral<int32_t>(value)};
        if (!height || *height < 0 || *height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Invalid height value (%s) for -testactivationheight=name@height.", arg));
        }

        const auto deployment_name{arg.substr(0, found)};
        if (const auto buried_deployment = GetBuriedDeployment(deployment_name)) {
            options.activation_heights[*buried_deployment] = *height;
        } else {
            throw std::runtime_error(strprintf("Invalid name (%s) for -testactivationheight=name@height.", arg));
        }
    }

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams = SplitString(strDeployment, ':');
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        CChainParams::VersionBitsParameters vbparams{};
        const auto start_time{ToIntegral<int64_t>(vDeploymentParams[1])};
        if (!start_time) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        vbparams.start_time = *start_time;
        const auto timeout{ToIntegral<int64_t>(vDeploymentParams[2])};
        if (!timeout) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        vbparams.timeout = *timeout;
        if (vDeploymentParams.size() >= 4) {
            const auto min_activation_height{ToIntegral<int64_t>(vDeploymentParams[3])};
            if (!min_activation_height) {
                throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
            }
            vbparams.min_activation_height = *min_activation_height;
        } else {
            vbparams.min_activation_height = 0;
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                options.version_bits_parameters[Consensus::DeploymentPos(j)] = vbparams;
                found = true;
                LogInfo("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d",
                        vDeploymentParams[0], vbparams.start_time, vbparams.timeout, vbparams.min_activation_height);
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

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return CChainParams::Main();
    case ChainType::TESTNET:
        return CChainParams::TestNet();
    case ChainType::TESTNET4:
        return CChainParams::TestNet4();
    case ChainType::SIGNET: {
        auto opts = CChainParams::SigNetOptions{};
        ReadSigNetArgs(args, opts);
        return CChainParams::SigNet(opts);
    }
    case ChainType::REGTEST: {
        auto opts = CChainParams::RegTestOptions{};
        ReadRegTestArgs(args, opts);
        return CChainParams::RegTest(opts);
    }
    }
    assert(false);
}

void SelectParams(const ChainType chain)
{
    SelectBaseParams(chain);
    globalChainParams = CreateChainParams(gArgs, chain);
}
