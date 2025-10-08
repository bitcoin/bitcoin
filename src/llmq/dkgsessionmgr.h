// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSIONMGR_H
#define BITCOIN_LLMQ_DKGSESSIONMGR_H

#include <bls/bls.h>
#include <bls/bls_worker.h>
#include <llmq/dkgsessionhandler.h>
#include <net_types.h>

#include <map>
#include <memory>
#include <string_view>

template <class T>
class CBLSIESMultiRecipientObjects;
template <class T>
class CBLSIESEncryptedObject;

class CActiveMasternodeManager;
class CBlockIndex;
class CChainState;
class CDBWrapper;
class CDeterministicMNManager;
class CDKGDebugManager;
class CMasternodeMetaMan;
class CSporkManager;
class PeerManager;
class CDKGContribution;
class CDKGComplaint;
class CDKGJustification;
class CDKGPrematureCommitment;

class UniValue;

namespace llmq
{
class CQuorumSnapshotManager;

class CDKGSessionManager
{
    static constexpr int64_t MAX_CONTRIBUTION_CACHE_TIME = 60 * 1000;

private:
    std::unique_ptr<CDBWrapper> db{nullptr};

    CBLSWorker& blsWorker;
    CChainState& m_chainstate;
    CDeterministicMNManager& m_dmnman;
    CDKGDebugManager& dkgDebugManager;
    CQuorumBlockProcessor& quorumBlockProcessor;
    CQuorumSnapshotManager& m_qsnapman;
    const CSporkManager& spork_manager;

    //TODO name struct instead of std::pair
    std::map<std::pair<Consensus::LLMQType, int>, CDKGSessionHandler> dkgSessionHandlers;

    mutable Mutex contributionsCacheCs;
    struct ContributionsCacheKey {
        Consensus::LLMQType llmqType;
        uint256 quorumHash;
        uint256 proTxHash;
        bool operator<(const ContributionsCacheKey& r) const
        {
            if (llmqType != r.llmqType) return llmqType < r.llmqType;
            if (quorumHash != r.quorumHash) return quorumHash < r.quorumHash;
            return proTxHash < r.proTxHash;
        }
    };
    struct ContributionsCacheEntry {
        int64_t entryTime;
        BLSVerificationVectorPtr vvec;
        CBLSSecretKey skContribution;
    };
    mutable std::map<ContributionsCacheKey, ContributionsCacheEntry> contributionsCache GUARDED_BY(contributionsCacheCs);

public:
    CDKGSessionManager(CBLSWorker& _blsWorker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                       CDKGDebugManager& _dkgDebugManager, CMasternodeMetaMan& mn_metaman,
                       CQuorumBlockProcessor& _quorumBlockProcessor, CQuorumSnapshotManager& qsnapman,
                       const CActiveMasternodeManager* const mn_activeman, const CSporkManager& sporkman,
                       bool unitTests, bool fWipe);
    ~CDKGSessionManager();

    void StartThreads(CConnman& connman, PeerManager& peerman);
    void StopThreads();

    void UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
        EXCLUSIVE_LOCKS_REQUIRED(!contributionsCacheCs);

    [[nodiscard]] MessageProcessingResult ProcessMessage(CNode& pfrom, bool is_masternode, std::string_view msg_type,
                                                         CDataStream& vRecv);
    bool AlreadyHave(const CInv& inv) const;
    bool GetContribution(const uint256& hash, CDKGContribution& ret) const;
    bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const;
    bool GetJustification(const uint256& hash, CDKGJustification& ret) const;
    bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const;

    // Contributions are written while in the DKG
    void WriteVerifiedVvecContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec);
    void WriteVerifiedSkContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSSecretKey& skContribution);
    bool GetVerifiedContributions(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex,
                                  const std::vector<bool>& validMembers, std::vector<uint16_t>& memberIndexesRet,
                                  std::vector<BLSVerificationVectorPtr>& vvecsRet,
                                  std::vector<CBLSSecretKey>& skContributionsRet) const
        EXCLUSIVE_LOCKS_REQUIRED(!contributionsCacheCs);
    /// Write encrypted (unverified) DKG contributions for the member with the given proTxHash to the llmqDb
    void WriteEncryptedContributions(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSIESMultiRecipientObjects<CBLSSecretKey>& contributions);
    /// Read encrypted (unverified) DKG contributions for the member with the given proTxHash from the llmqDb
    bool GetEncryptedContributions(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, const uint256& proTxHash, std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vecRet) const;

    void CleanupOldContributions() const;

private:
    void CleanupCache() const EXCLUSIVE_LOCKS_REQUIRED(!contributionsCacheCs);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_DKGSESSIONMGR_H
