// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/options.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <deploymentstatus.h>
#include <spork.h>
#include <util/ranges.h>
#include <util/system.h>
#include <util/underlying.h>

#include <string>
#include <stdexcept>

static constexpr int TESTNET_LLMQ_25_67_ACTIVATION_HEIGHT = 847000;

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

bool QuorumDataRecoveryEnabled()
{
    return gArgs.GetBoolArg("-llmq-data-recovery", DEFAULT_ENABLE_QUORUM_DATA_RECOVERY);
}

bool IsWatchQuorumsEnabled()
{
    static bool fIsWatchQuroumsEnabled = gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS);
    return fIsWatchQuroumsEnabled;
}

std::map<Consensus::LLMQType, QvvecSyncMode> GetEnabledQuorumVvecSyncEntries()
{
    std::map<Consensus::LLMQType, QvvecSyncMode> mapQuorumVvecSyncEntries;
    for (const auto& strEntry : gArgs.GetArgs("-llmq-qvvec-sync")) {
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

bool IsQuorumTypeEnabled(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindexPrev)
{
    return IsQuorumTypeEnabledInternal(llmqType, pindexPrev, std::nullopt, std::nullopt);
}

bool IsQuorumTypeEnabledInternal(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindexPrev,
                                std::optional<bool> optDIP0024IsActive, std::optional<bool> optHaveDIP0024Quorums)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();

    const bool fDIP0024IsActive{optDIP0024IsActive.value_or(DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_DIP0024))};
    const bool fHaveDIP0024Quorums{optHaveDIP0024Quorums.value_or(pindexPrev->nHeight >= consensusParams.DIP0024QuorumsHeight)};
    switch (llmqType)
    {
        case Consensus::LLMQType::LLMQ_DEVNET:
            return true;
        case Consensus::LLMQType::LLMQ_50_60:
            return !fDIP0024IsActive || !fHaveDIP0024Quorums ||
                    Params().NetworkIDString() == CBaseChainParams::TESTNET;
        case Consensus::LLMQType::LLMQ_TEST_INSTANTSEND:
            return !fDIP0024IsActive || !fHaveDIP0024Quorums ||
                    consensusParams.llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_TEST_INSTANTSEND;
        case Consensus::LLMQType::LLMQ_TEST:
        case Consensus::LLMQType::LLMQ_TEST_PLATFORM:
        case Consensus::LLMQType::LLMQ_400_60:
        case Consensus::LLMQType::LLMQ_400_85:
        case Consensus::LLMQType::LLMQ_DEVNET_PLATFORM:
            return true;

        case Consensus::LLMQType::LLMQ_TEST_V17: {
            return DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_TESTDUMMY);
        }
        case Consensus::LLMQType::LLMQ_100_67:
            return DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_DIP0020);

        case Consensus::LLMQType::LLMQ_60_75:
        case Consensus::LLMQType::LLMQ_DEVNET_DIP0024:
        case Consensus::LLMQType::LLMQ_TEST_DIP0024: {
            return fDIP0024IsActive;
        }
        case Consensus::LLMQType::LLMQ_25_67:
            return pindexPrev->nHeight >= TESTNET_LLMQ_25_67_ACTIVATION_HEIGHT;

        default:
            throw std::runtime_error(strprintf("%s: Unknown LLMQ type %d", __func__, ToUnderlying(llmqType)));
    }

    // Something wrong with conditions above, they are not consistent
    assert(false);
}

std::vector<Consensus::LLMQType> GetEnabledQuorumTypes(gsl::not_null<const CBlockIndex*> pindex)
{
    std::vector<Consensus::LLMQType> ret;
    ret.reserve(Params().GetConsensus().llmqs.size());
    for (const auto& params : Params().GetConsensus().llmqs) {
        if (IsQuorumTypeEnabled(params.type, pindex)) {
            ret.push_back(params.type);
        }
    }
    return ret;
}

std::vector<std::reference_wrapper<const Consensus::LLMQParams>> GetEnabledQuorumParams(gsl::not_null<const CBlockIndex*> pindex)
{
    std::vector<std::reference_wrapper<const Consensus::LLMQParams>> ret;
    ret.reserve(Params().GetConsensus().llmqs.size());

    std::copy_if(Params().GetConsensus().llmqs.begin(), Params().GetConsensus().llmqs.end(), std::back_inserter(ret),
                 [&pindex](const auto& params){return IsQuorumTypeEnabled(params.type, pindex);});

    return ret;
}
} // namespace llmq
