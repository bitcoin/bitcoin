// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_UTILS_H
#define BITCOIN_LLMQ_UTILS_H

#include <consensus/params.h>

#include <dbwrapper.h>
#include <random.h>
#include <set>
#include <sync.h>
#include <optional>
#include <vector>
#include <versionbits.h>

class CBlockIndex;
class CDeterministicMN;
class CDeterministicMNList;
using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;
class CBLSPublicKey;

namespace llmq
{

class CQuorumSnapshot;

// Use a separate cache instance instead of versionbitscache to avoid locking cs_main
// and dealing with all kinds of deadlocks.
extern CCriticalSection cs_llmq_vbc;
extern VersionBitsCache llmq_versionbitscache GUARDED_BY(cs_llmq_vbc);

static const bool DEFAULT_ENABLE_QUORUM_DATA_RECOVERY = true;

enum class QvvecSyncMode {
    Invalid = -1,
    Always = 0,
    OnlyIfTypeMember = 1,
};

//QuorumMembers per quorumIndex at heights H-Cycle, H-2Cycles, H-3Cycles
struct PreviousQuorumQuarters {
    std::vector<std::vector<CDeterministicMNCPtr>> quarterHMinusC;
    std::vector<std::vector<CDeterministicMNCPtr>> quarterHMinus2C;
    std::vector<std::vector<CDeterministicMNCPtr>> quarterHMinus3C;
    explicit PreviousQuorumQuarters(size_t s) :
        quarterHMinusC(s), quarterHMinus2C(s), quarterHMinus3C(s) {}
};

class CLLMQUtils
{
public:
    // includes members which failed DKG
    static std::vector<CDeterministicMNCPtr> GetAllQuorumMembers(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex);

    static void PreComputeQuorumMembers(const CBlockIndex* pQuorumBaseBlockIndex);
    static std::vector<CDeterministicMNCPtr> ComputeQuorumMembers(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex);
    static std::vector<std::vector<CDeterministicMNCPtr>> ComputeQuorumMembersByQuarterRotation(Consensus::LLMQType llmqType, const CBlockIndex* pCycleQuorumBaseBlockIndex);

    static std::vector<std::vector<CDeterministicMNCPtr>> BuildNewQuorumQuarterMembers(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pQuorumBaseBlockIndex, const PreviousQuorumQuarters& quarters);

    static PreviousQuorumQuarters GetPreviousQuorumQuarterMembers(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pBlockHMinusCIndex, const CBlockIndex* pBlockHMinus2CIndex, const CBlockIndex* pBlockHMinus3CIndex, int nHeight);
    static std::vector<std::vector<CDeterministicMNCPtr>> GetQuorumQuarterMembersBySnapshot(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pQuorumBaseBlockIndex, const llmq::CQuorumSnapshot& snapshot, int nHeights);
    static std::pair<CDeterministicMNList, CDeterministicMNList> GetMNUsageBySnapshot(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const llmq::CQuorumSnapshot& snapshot, int nHeight);

    static void BuildQuorumSnapshot(const Consensus::LLMQParams& llmqParams, const CDeterministicMNList& mnAtH, const CDeterministicMNList& mnUsedAtH, std::vector<CDeterministicMNCPtr>& sortedCombinedMns, CQuorumSnapshot& quorumSnapshot, int nHeight, std::vector<int>& skipList, const CBlockIndex* pQuorumBaseBlockIndex);

    static uint256 BuildCommitmentHash(Consensus::LLMQType llmqType, const uint256& blockHash, const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey, const uint256& vvecHash);
    static uint256 BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash);

    // works for sig shares and recovered sigs
    template<typename T>
    static uint256 BuildSignHash(const T& s)
    {
        return BuildSignHash((Consensus::LLMQType)s.llmqType, s.quorumHash, s.id, s.msgHash);
    }

    static bool IsAllMembersConnectedEnabled(Consensus::LLMQType llmqType);
    static bool IsQuorumPoseEnabled(Consensus::LLMQType llmqType);
    static uint256 DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2);
    static std::set<uint256> GetQuorumConnections(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& forMember, bool onlyOutbound);
    static std::set<uint256> GetQuorumRelayMembers(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& forMember, bool onlyOutbound);
    static std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, size_t memberCount, size_t connectionCount);

    static bool EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& myProTxHash);
    static void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& myProTxHash);

    static bool IsQuorumActive(Consensus::LLMQType llmqType, const uint256& quorumHash);
    static bool IsQuorumTypeEnabled(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
    static bool IsQuorumTypeEnabledInternal(Consensus::LLMQType llmqType, const CBlockIndex* pindex, std::optional<bool> optDIP0024IsActive, std::optional<bool> optHaveDIP0024Quorums);

    static std::vector<Consensus::LLMQType> GetEnabledQuorumTypes(const CBlockIndex* pindex);
    static std::vector<std::reference_wrapper<const Consensus::LLMQParams>> GetEnabledQuorumParams(const CBlockIndex* pindex);

    static bool IsQuorumRotationEnabled(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
    static Consensus::LLMQType GetInstantSendLLMQType(const CBlockIndex* pindex);
    static Consensus::LLMQType GetInstantSendLLMQType(bool deterministic);
    static bool IsDIP0024Active(const CBlockIndex* pindex);

    /// Returns the state of `-llmq-data-recovery`
    static bool QuorumDataRecoveryEnabled();

    /// Returns the state of `-watchquorums`
    static bool IsWatchQuorumsEnabled();

    /// Returns the parsed entries given by `-llmq-qvvec-sync`
    static std::map<Consensus::LLMQType, QvvecSyncMode> GetEnabledQuorumVvecSyncEntries();

    template<typename NodesContainer, typename Continue, typename Callback>
    static void IterateNodesRandom(NodesContainer& nodeStates, Continue&& cont, Callback&& callback, FastRandomContext& rnd)
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
    static std::string ToHexStr(const std::vector<bool>& vBits)
    {
        std::vector<uint8_t> vBytes((vBits.size() + 7) / 8);
        for (size_t i = 0; i < vBits.size(); i++) {
            vBytes[i / 8] |= vBits[i] << (i % 8);
        }
        return HexStr(vBytes);
    }
    template <typename CacheType>
    static void InitQuorumsCache(CacheType& cache);
};

const Consensus::LLMQParams& GetLLMQParams(Consensus::LLMQType llmqType);

} // namespace llmq

#endif // BITCOIN_LLMQ_UTILS_H
