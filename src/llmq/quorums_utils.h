// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_UTILS_H
#define BITCOIN_LLMQ_QUORUMS_UTILS_H

#include <consensus/params.h>

#include <vector>
#include <set>
#include <random.h>
#include <sync.h>
#include <dbwrapper.h>
#include <chainparams.h>
#include <versionbits.h>

class CBlockIndex;
class CDeterministicMN;
typedef std::shared_ptr<const CDeterministicMN> CDeterministicMNCPtr;
class CBLSPublicKey;

namespace llmq
{

// Use a separate cache instance instead of versionbitscache to avoid locking cs_main
// and dealing with all kinds of deadlocks.
extern CCriticalSection cs_llmq_vbc;
extern VersionBitsCache llmq_versionbitscache;

static const bool DEFAULT_ENABLE_QUORUM_DATA_RECOVERY = true;

enum class QvvecSyncMode {
    Invalid = -1,
    Always = 0,
    OnlyIfTypeMember = 1,
};

class CLLMQUtils
{
public:
    // includes members which failed DKG
    static std::vector<CDeterministicMNCPtr> GetAllQuorumMembers(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum);

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
    static std::set<uint256> GetQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, const uint256& forMember, bool onlyOutbound);
    static std::set<uint256> GetQuorumRelayMembers(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, const uint256& forMember, bool onlyOutbound);
    static std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, size_t memberCount, size_t connectionCount);

    static bool EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, const uint256& myProTxHash);
    static void AddQuorumProbeConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, const uint256& myProTxHash);

    static bool IsQuorumActive(Consensus::LLMQType llmqType, const uint256& quorumHash);
    static bool IsQuorumTypeEnabled(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
    static std::vector<Consensus::LLMQType> GetEnabledQuorumTypes(const CBlockIndex* pindex);

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
    static void InitQuorumsCache(CacheType& cache)
    {
        for (auto& llmq : Params().GetConsensus().llmqs) {
            cache.emplace(std::piecewise_construct, std::forward_as_tuple(llmq.first),
                                                    std::forward_as_tuple(llmq.second.signingActiveQuorumCount + 1));
        }
    }
};

const Consensus::LLMQParams& GetLLMQParams(const Consensus::LLMQType llmqType);

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_UTILS_H
