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
class CDeterministicMNList;
class CDeterministicMNManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CSporkManager;
namespace llmq {
class CQuorumSnapshotManager;
} // namespace llmq

namespace llmq {
struct UtilParameters {
    CDeterministicMNManager& m_dmnman;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    gsl::not_null<const CBlockIndex*> m_base_index;

public:
    UtilParameters replace_index(gsl::not_null<const CBlockIndex*> base_index) const
    {
        return {m_dmnman, m_qsnapman, m_chainman, base_index};
    }
};

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

uint256 DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2);

std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType,
                                                   gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                                   size_t memberCount, size_t connectionCount);

// includes members which failed DKG
std::vector<CDeterministicMNCPtr> GetAllQuorumMembers(Consensus::LLMQType llmqType, const UtilParameters& util_params,
                                                      bool reset_cache = false);

Uint256HashSet GetQuorumConnections(const Consensus::LLMQParams& llmqParams, const CSporkManager& sporkman,
                                    const UtilParameters& util_params, const uint256& forMember, bool onlyOutbound);

Uint256HashSet GetQuorumRelayMembers(const Consensus::LLMQParams& llmqParams, const UtilParameters& util_params,
                                     const uint256& forMember, bool onlyOutbound);

bool EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman, const CSporkManager& sporkman,
                             const UtilParameters& util_params, const CDeterministicMNList& tip_mn_list,
                             const uint256& myProTxHash, bool is_masternode, bool quorums_watch);

void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman, CMasternodeMetaMan& mn_metaman,
                               const CSporkManager& sporkman, const UtilParameters& util_params,
                               const CDeterministicMNList& tip_mn_list, const uint256& myProTxHash);

template <typename CacheType>
inline void InitQuorumsCache(CacheType& cache, const Consensus::Params& consensus_params, bool limit_by_connections = true)
{
    for (const auto& llmq : consensus_params.llmqs) {
        cache.emplace(std::piecewise_construct, std::forward_as_tuple(llmq.type),
                      std::forward_as_tuple(limit_by_connections ? llmq.keepOldConnections : llmq.keepOldKeys));
    }
}
} // namespace utils
} // namespace llmq

#endif // BITCOIN_LLMQ_UTILS_H
