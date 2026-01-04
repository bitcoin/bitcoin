// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/options.h>

#include <spork.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <deploymentstatus.h>
#include <util/ranges.h>
#include <util/system.h>
#include <util/underlying.h>
#include <validation.h>

#include <string>
#include <stdexcept>

namespace llmq
{

static bool EvalSpork(const Consensus::LLMQType llmqType, const int64_t spork_value)
{
    if (spork_value == 0) {
        return true;
    }
    if (spork_value == 1 && llmqType != Consensus::LLMQType::LLMQ_100_67 && llmqType != Consensus::LLMQType::LLMQ_400_60 && llmqType != Consensus::LLMQType::LLMQ_400_85) {
        return true;
    }
    return false;
}

bool IsAllMembersConnectedEnabled(const Consensus::LLMQType llmqType, const CSporkManager& sporkman)
{
    return EvalSpork(llmqType, sporkman.GetSporkValue(SPORK_21_QUORUM_ALL_CONNECTED));
}

bool IsQuorumPoseEnabled(const Consensus::LLMQType llmqType, const CSporkManager& sporkman)
{
    return EvalSpork(llmqType, sporkman.GetSporkValue(SPORK_23_QUORUM_POSE));
}


bool IsQuorumRotationEnabled(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindex)
{
    if (!llmqParams.useRotation) {
        return false;
    }

    int cycleQuorumBaseHeight = pindex->nHeight - (pindex->nHeight % llmqParams.dkgInterval);
    if (cycleQuorumBaseHeight < 1) {
        return false;
    }
    // It should activate at least 1 block prior to the cycle start
    return DeploymentActiveAfter(pindex->GetAncestor(cycleQuorumBaseHeight - 1), Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0024);
}

QvvecSyncModeMap GetEnabledQuorumVvecSyncEntries(const ArgsManager& args)
{
    QvvecSyncModeMap mapQuorumVvecSyncEntries;
    for (const auto& strEntry : args.GetArgs("-llmq-qvvec-sync")) {
        Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
        QvvecSyncMode mode{QvvecSyncMode::Invalid};
        std::istringstream ssEntry(strEntry);
        std::string strLLMQType, strMode, strTest;
        const bool fLLMQTypePresent = std::getline(ssEntry, strLLMQType, ':') && strLLMQType != "";
        const bool fModePresent = std::getline(ssEntry, strMode, ':') && strMode != "";
        const bool fTooManyEntries = static_cast<bool>(std::getline(ssEntry, strTest, ':'));
        if (!fLLMQTypePresent || !fModePresent || fTooManyEntries) {
            throw std::invalid_argument(strprintf("Invalid format in -llmq-qvvec-sync: %s", strEntry));
        }

        if (auto optLLMQParams = ranges::find_if_opt(Params().GetConsensus().llmqs,
                                                     [&strLLMQType](const auto& params){return params.name == strLLMQType;})) {
            llmqType = optLLMQParams->type;
        } else {
            throw std::invalid_argument(strprintf("Invalid llmqType in -llmq-qvvec-sync: %s", strEntry));
        }
        if (mapQuorumVvecSyncEntries.count(llmqType) > 0) {
            throw std::invalid_argument(strprintf("Duplicated llmqType in -llmq-qvvec-sync: %s", strEntry));
        }

        int32_t nMode;
        if (ParseInt32(strMode, &nMode)) {
            switch (nMode) {
            case (int32_t)QvvecSyncMode::Always:
                mode = QvvecSyncMode::Always;
                break;
            case (int32_t)QvvecSyncMode::OnlyIfTypeMember:
                mode = QvvecSyncMode::OnlyIfTypeMember;
                break;
            default:
                mode = QvvecSyncMode::Invalid;
                break;
            }
        }
        if (mode == QvvecSyncMode::Invalid) {
            throw std::invalid_argument(strprintf("Invalid mode in -llmq-qvvec-sync: %s", strEntry));
        }
        mapQuorumVvecSyncEntries.emplace(llmqType, mode);
    }
    return mapQuorumVvecSyncEntries;
}

std::vector<Consensus::LLMQType> GetEnabledQuorumTypes(const ChainstateManager& chainman,
                                                       gsl::not_null<const CBlockIndex*> pindex)
{
    std::vector<Consensus::LLMQType> ret;
    ret.reserve(Params().GetConsensus().llmqs.size());
    for (const auto& params : Params().GetConsensus().llmqs) {
        if (chainman.IsQuorumTypeEnabled(params.type, pindex)) {
            ret.push_back(params.type);
        }
    }
    return ret;
}

std::vector<std::reference_wrapper<const Consensus::LLMQParams>> GetEnabledQuorumParams(
    const ChainstateManager& chainman, gsl::not_null<const CBlockIndex*> pindex)
{
    std::vector<std::reference_wrapper<const Consensus::LLMQParams>> ret;
    ret.reserve(Params().GetConsensus().llmqs.size());

    std::copy_if(Params().GetConsensus().llmqs.begin(), Params().GetConsensus().llmqs.end(), std::back_inserter(ret),
                 [&pindex, &chainman](const auto& params) { return chainman.IsQuorumTypeEnabled(params.type, pindex); });

    return ret;
}
} // namespace llmq
