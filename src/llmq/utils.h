// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_UTILS_H
#define BITCOIN_LLMQ_UTILS_H

#include <bls/bls.h>
#include <gsl/pointers.h>
#include <llmq/params.h>
#include <saltedhasher.h>
#include <sync.h>
#include <uint256.h>

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

class CConnman;
class CBlockIndex;
class CDeterministicMN;
class CDeterministicMNList;
class CDeterministicMNManager;
class CMasternodeMetaMan;
class CSporkManager;

using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;

namespace llmq {
class CQuorumSnapshotManager;

namespace utils {
// includes members which failed DKG
std::vector<CDeterministicMNCPtr> GetAllQuorumMembers(Consensus::LLMQType llmqType, CDeterministicMNManager& dmnman,
                                                      CQuorumSnapshotManager& qsnapman,
                                                      gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                                      bool reset_cache = false);

uint256 DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2);
Uint256HashSet GetQuorumConnections(
    const Consensus::LLMQParams& llmqParams, CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
    const CSporkManager& sporkman, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& forMember,
    bool onlyOutbound);
Uint256HashSet GetQuorumRelayMembers(
    const Consensus::LLMQParams& llmqParams, CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
    gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& forMember, bool onlyOutbound);
std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, size_t memberCount, size_t connectionCount);

bool EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman,
                             CDeterministicMNManager& dmnman, const CSporkManager& sporkman,
                             CQuorumSnapshotManager& qsnapman, const CDeterministicMNList& tip_mn_list,
                             gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& myProTxHash,
                             bool is_masternode);
void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman, CDeterministicMNManager& dmnman,
                               CMasternodeMetaMan& mn_metaman, CQuorumSnapshotManager& qsnapman,
                               const CSporkManager& sporkman, const CDeterministicMNList& tip_mn_list,
                               gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, const uint256& myProTxHash);

struct BlsCheck {
    CBLSSignature m_sig;
    std::vector<CBLSPublicKey> m_pubkeys;
    uint256 m_msg_hash;
    std::string m_id_string;

    BlsCheck() = default;

    BlsCheck(CBLSSignature sig, std::vector<CBLSPublicKey> pubkeys, uint256 msg_hash, std::string id_string) :
        m_sig(sig),
        m_pubkeys(pubkeys),
        m_msg_hash(msg_hash),
        m_id_string(id_string)
    {
    }

    void swap(BlsCheck& obj)
    {
        std::swap(m_sig, obj.m_sig);
        std::swap(m_pubkeys, obj.m_pubkeys);
        std::swap(m_msg_hash, obj.m_msg_hash);
        std::swap(m_id_string, obj.m_id_string);
    }

    bool operator()();
};

template <typename CacheType>
void InitQuorumsCache(CacheType& cache, bool limit_by_connections = true);

} // namespace utils
} // namespace llmq

#endif // BITCOIN_LLMQ_UTILS_H
