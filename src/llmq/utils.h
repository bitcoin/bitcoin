// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_UTILS_H
#define BITCOIN_LLMQ_UTILS_H

#include <bls/bls.h>
#include <evo/types.h>
#include <llmq/params.h>
#include <saltedhasher.h>

#include <chainparams.h>
#include <sync.h>
#include <uint256.h>

#include <gsl/pointers.h>

#include <set>
#include <vector>

class CBlockIndex;
class CConnman;
class CDeterministicMN;
class CDeterministicMNList;
class CDeterministicMNManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CSporkManager;
namespace llmq {
class CQuorumSnapshotManager;
} // namespace llmq

namespace llmq {
namespace utils {
struct BlsCheck {
    CBLSSignature m_sig;
    std::vector<CBLSPublicKey> m_pubkeys;
    uint256 m_msg_hash;
    std::string m_id_string;

public:
    BlsCheck();
    BlsCheck(CBLSSignature sig, std::vector<CBLSPublicKey> pubkeys, uint256 msg_hash, std::string id_string);
    ~BlsCheck();

    bool operator()();
    void swap(BlsCheck& obj);
};

// includes members which failed DKG
std::vector<CDeterministicMNCPtr> GetAllQuorumMembers(Consensus::LLMQType llmqType, CDeterministicMNManager& dmnman,
                                                      CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                                                      gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                                      bool reset_cache = false);

uint256 DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2);

Uint256HashSet GetQuorumConnections(const Consensus::LLMQParams& llmqParams, CDeterministicMNManager& dmnman,
                                    CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                                    const CSporkManager& sporkman, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                    const uint256& forMember, bool onlyOutbound);

Uint256HashSet GetQuorumRelayMembers(const Consensus::LLMQParams& llmqParams, CDeterministicMNManager& dmnman,
                                     CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                                     gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& forMember,
                                     bool onlyOutbound);

std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType,
                                                   gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                                   size_t memberCount, size_t connectionCount);

bool EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman, CDeterministicMNManager& dmnman,
                             CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                             const CSporkManager& sporkman, const CDeterministicMNList& tip_mn_list,
                             gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& myProTxHash,
                             bool is_masternode, bool quorums_watch);

void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman,
                               CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                               CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                               const CSporkManager& sporkman, const CDeterministicMNList& tip_mn_list,
                               gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& myProTxHash);

template <typename CacheType>
inline void InitQuorumsCache(CacheType& cache, bool limit_by_connections = true)
{
    for (const auto& llmq : Params().GetConsensus().llmqs) {
        cache.emplace(std::piecewise_construct, std::forward_as_tuple(llmq.type),
                      std::forward_as_tuple(limit_by_connections ? llmq.keepOldConnections : llmq.keepOldKeys));
    }
}
} // namespace utils
} // namespace llmq

#endif // BITCOIN_LLMQ_UTILS_H
