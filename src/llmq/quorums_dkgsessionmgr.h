// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_DKGSESSIONMGR_H
#define SYSCOIN_LLMQ_QUORUMS_DKGSESSIONMGR_H

#include <llmq/quorums_dkgsessionhandler.h>
#include <llmq/quorums_dkgsession.h>
#include <bls/bls.h>
#include <bls/bls_worker.h>
class CConnman;
class UniValue;
class PeerManager;
class CBlockIndex;
class ChainstateManager;
namespace llmq
{

class CDKGSessionManager
{
    static constexpr int64_t MAX_CONTRIBUTION_CACHE_TIME = 60 * 1000;

private:
    std::unique_ptr<CDBWrapper> db{nullptr};
    std::map<uint8_t, CDKGSessionHandler> dkgSessionHandlers;

    mutable RecursiveMutex contributionsCacheCs;
    struct ContributionsCacheKey {
        uint8_t llmqType;
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
    CConnman& connman;
    PeerManager& peerman;
    CDKGSessionManager(CBLSWorker& _blsWorker, CConnman &connman, PeerManager& peerman, ChainstateManager& chainman, bool unitTests, bool fWipe);
   ~CDKGSessionManager() = default;

    void StartThreads();
    void StopThreads();

    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);
    bool AlreadyHave(const uint256& hash) const;
    bool GetContribution(const uint256& hash, CDKGContribution& ret) const;
    bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const;
    bool GetJustification(const uint256& hash, CDKGJustification& ret) const;
    bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const;

    // Verified contributions are written while in the DKG
    void WriteVerifiedVvecContribution(uint8_t llmqType, const uint256& hashQuorum, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec);
    void WriteVerifiedSkContribution(uint8_t llmqType, const uint256& hashQuorum, const uint256& proTxHash, const CBLSSecretKey& skContribution);
    bool GetVerifiedContributions(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, std::vector<uint16_t>& memberIndexesRet, std::vector<BLSVerificationVectorPtr>& vvecsRet, BLSSecretKeyVector& skContributionsRet) const;
    /// Write encrypted (unverified) DKG contributions for the member with the given proTxHash to the llmqDb
    void WriteEncryptedContributions(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSIESMultiRecipientObjects<CBLSSecretKey>& contributions);
    /// Read encrypted (unverified) DKG contributions for the member with the given proTxHash from the llmqDb
    bool GetEncryptedContributions(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, const uint256& proTxHash, std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vecRet) const;

private:
    void MigrateDKG();
    void CleanupCache() const;
};

bool IsQuorumDKGEnabled();

extern CDKGSessionManager* quorumDKGSessionManager;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_DKGSESSIONMGR_H
