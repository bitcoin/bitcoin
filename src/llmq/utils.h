// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_UTILS_H
#define BITCOIN_LLMQ_UTILS_H

#include <consensus/params.h>

#include <random.h>
#include <set>
#include <sync.h>
#include <versionbits.h>
#include <gsl/pointers.h>

#include <optional>
#include <vector>

class CConnman;
class CBlockIndex;
class CDeterministicMN;
class CDeterministicMNList;
using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;
class CBLSPublicKey;

namespace llmq
{

class CQuorumManager;
class CQuorumSnapshot;

// A separate cache instance instead of versionbitscache has been introduced to avoid locking cs_main
// and dealing with all kinds of deadlocks.
// TODO: drop llmq_versionbitscache completely so far as VersionBitsCache do not uses anymore cs_main
extern VersionBitsCache llmq_versionbitscache;

static const bool DEFAULT_ENABLE_QUORUM_DATA_RECOVERY = true;

enum class QvvecSyncMode {
    Invalid = -1,
    Always = 0,
    OnlyIfTypeMember = 1,
};

namespace utils
{

// includes members which failed DKG
std::vector<CDeterministicMNCPtr> GetAllQuorumMembers(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, bool reset_cache = false);

uint256 GetHashModifier(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pCycleQuorumBaseBlockIndex);
uint256 BuildCommitmentHash(Consensus::LLMQType llmqType, const uint256& blockHash, const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey, const uint256& vvecHash);
uint256 BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash);

// TODO options
bool IsAllMembersConnectedEnabled(Consensus::LLMQType llmqType);
// TODO options
bool IsQuorumPoseEnabled(Consensus::LLMQType llmqType);
uint256 DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2);
std::set<uint256> GetQuorumConnections(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& forMember, bool onlyOutbound);
std::set<uint256> GetQuorumRelayMembers(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& forMember, bool onlyOutbound);
std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, size_t memberCount, size_t connectionCount);

bool EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, CConnman& connman, const uint256& myProTxHash);
void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, CConnman& connman, const uint256& myProTxHash);

bool IsQuorumActive(Consensus::LLMQType llmqType, const CQuorumManager& qman, const uint256& quorumHash);
// TODO options maybe not
bool IsQuorumTypeEnabled(Consensus::LLMQType llmqType, const CQuorumManager& qman, gsl::not_null<const CBlockIndex*> pindexPrev);
// TODO options maybe not
bool IsQuorumTypeEnabledInternal(Consensus::LLMQType llmqType, const CQuorumManager& qman, gsl::not_null<const CBlockIndex*> pindexPrev, std::optional<bool> optDIP0024IsActive, std::optional<bool> optHaveDIP0024Quorums);

// TODO options maybe not
std::vector<Consensus::LLMQType> GetEnabledQuorumTypes(gsl::not_null<const CBlockIndex*> pindex);
// TODO options maybe not
std::vector<std::reference_wrapper<const Consensus::LLMQParams>> GetEnabledQuorumParams(gsl::not_null<const CBlockIndex*> pindex);

// TODO options
bool IsQuorumRotationEnabled(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindex);
// TODO deployments
bool IsV20Active(gsl::not_null<const CBlockIndex*> pindex);

/// Returns the state of `-llmq-data-recovery`
// TODO options
bool QuorumDataRecoveryEnabled();

// TODO options
/// Returns the state of `-watchquorums`
bool IsWatchQuorumsEnabled();

/// Returns the parsed entries given by `-llmq-qvvec-sync`
std::map<Consensus::LLMQType, QvvecSyncMode> GetEnabledQuorumVvecSyncEntries();

template<typename NodesContainer, typename Continue, typename Callback>
void IterateNodesRandom(NodesContainer& nodeStates, Continue&& cont, Callback&& callback, FastRandomContext& rnd)
{
    std::vector<typename NodesContainer::iterator> rndNodes;
    rndNodes.reserve(nodeStates.size());
    for (auto it = nodeStates.begin(); it != nodeStates.end(); ++it) {
        rndNodes.emplace_back(it);
    }
    if (rndNodes.empty()) {
        return;
    }
    Shuffle(rndNodes.begin(), rndNodes.end(), rnd);

    size_t idx = 0;
    while (!rndNodes.empty() && cont()) {
        auto nodeId = rndNodes[idx]->first;
        auto& ns = rndNodes[idx]->second;

        if (callback(nodeId, ns)) {
            idx = (idx + 1) % rndNodes.size();
        } else {
            rndNodes.erase(rndNodes.begin() + idx);
            if (rndNodes.empty()) {
                break;
            }
            idx %= rndNodes.size();
        }
    }
}

template <typename CacheType>
void InitQuorumsCache(CacheType& cache, bool limit_by_connections = true);

[[ nodiscard ]] static constexpr int max_cycles(const Consensus::LLMQParams& llmqParams, int quorums_count)
{
    return llmqParams.useRotation ? quorums_count / llmqParams.signingActiveQuorumCount : quorums_count;
}

[[ nodiscard ]] static constexpr int max_store_depth(const Consensus::LLMQParams& llmqParams)
{
    // For how many blocks recent DKG info should be kept
    return max_cycles(llmqParams, llmqParams.keepOldKeys) * llmqParams.dkgInterval;
}

} // namespace utils

[[ nodiscard ]] const std::optional<Consensus::LLMQParams> GetLLMQParams(Consensus::LLMQType llmqType);

} // namespace llmq

#endif // BITCOIN_LLMQ_UTILS_H
