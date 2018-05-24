// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QUORUMS_DKGSESSIONMGR_H
#define DASH_QUORUMS_DKGSESSIONMGR_H

#include "llmq/quorums_dkgsessionhandler.h"

#include "validation.h"

#include "ctpl.h"

class UniValue;

namespace llmq
{

class CDKGSessionManager
{
    static const int64_t MAX_CONTRIBUTION_CACHE_TIME = 60 * 1000;

private:
    CEvoDB& evoDb;
    CBLSWorker& blsWorker;
    ctpl::thread_pool messageHandlerPool;

    std::map<Consensus::LLMQType, CDKGSessionHandler> dkgSessionHandlers;

    CCriticalSection contributionsCacheCs;
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
    std::map<ContributionsCacheKey, ContributionsCacheEntry> contributionsCache;

public:
    CDKGSessionManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker);
    ~CDKGSessionManager();

    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    bool AlreadyHave(const CInv& inv) const;
    bool GetContribution(const uint256& hash, CDKGContribution& ret) const;
    bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const;
    bool GetJustification(const uint256& hash, CDKGJustification& ret) const;
    bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const;

    // Verified contributions are written while in the DKG
    void WriteVerifiedVvecContribution(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec);
    void WriteVerifiedSkContribution(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& proTxHash, const CBLSSecretKey& skContribution);
    bool GetVerifiedContributions(Consensus::LLMQType llmqType, const uint256& quorumHash, const std::vector<bool>& validMembers, std::vector<uint16_t>& memberIndexesRet, std::vector<BLSVerificationVectorPtr>& vvecsRet, BLSSecretKeyVector& skContributionsRet);
    bool GetVerifiedContribution(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& proTxHash, BLSVerificationVectorPtr& vvecRet, CBLSSecretKey& skContributionRet);

private:
    void CleanupCache();
};

extern CDKGSessionManager* quorumDKGSessionManager;

}

#endif //DASH_QUORUMS_DKGSESSIONMGR_H
