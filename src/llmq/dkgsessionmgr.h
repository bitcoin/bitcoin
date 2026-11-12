// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSIONMGR_H
#define BITCOIN_LLMQ_DKGSESSIONMGR_H

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <sync.h>
#include <util/check.h>
#include <util/helpers.h>

#include <map>
#include <memory>

template <class T>
class CBLSIESMultiRecipientObjects;
template <class T>
class CBLSIESEncryptedObject;

class CBlockIndex;
class CDBWrapper;
class CDeterministicMNManager;
class ChainstateManager;
class CSporkManager;
namespace util {
struct DbWrapperParams;
} // namespace util

namespace llmq
{
class CDKGComplaint;
class CDKGContribution;
class CDKGJustification;
class CDKGPrematureCommitment;
class CDKGSessionHandler;
class CQuorumSnapshotManager;

class CDKGSessionManager
{
public:
    struct SessionHandlerKey {
        Consensus::LLMQType llmq_type;
        int quorum_idx;
        auto operator<=>(const SessionHandlerKey&) const = default;
    };

    using SessionHandlerMap = std::map<SessionHandlerKey, std::unique_ptr<CDKGSessionHandler>>;

private:
    static constexpr auto MAX_CONTRIBUTION_CACHE_TIME = 60s;

private:
    CDeterministicMNManager& m_dmnman;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    const CSporkManager& m_sporkman;

private:
    std::unique_ptr<CDBWrapper> db{nullptr};

    SessionHandlerMap dkgSessionHandlers;

    mutable Mutex contributionsCacheCs;
    struct ContributionsCacheKey {
        Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
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
        SteadyClock::time_point entryTime;
        BLSVerificationVectorPtr vvec;
        CBLSSecretKey skContribution;
    };
    mutable std::map<ContributionsCacheKey, ContributionsCacheEntry> contributionsCache GUARDED_BY(contributionsCacheCs);

public:
    CDKGSessionManager() = delete;
    CDKGSessionManager(const CDKGSessionManager&) = delete;
    CDKGSessionManager& operator=(const CDKGSessionManager&) = delete;
    explicit CDKGSessionManager(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                                const ChainstateManager& chainman, const CSporkManager& sporkman,
                                const util::DbWrapperParams& db_params);
    ~CDKGSessionManager();

    template <typename HandlerFn>
    void InitializeHandlers(HandlerFn&& handler_fn)
    {
        // TODO: move it to cpp file and drop chainparams.h and helpers.h
        const Consensus::Params& consensus_params = Params().GetConsensus();
        for (const auto& params : consensus_params.llmqs) {
            auto session_count = (params.useRotation) ? params.signingActiveQuorumCount : 1;
            for (const auto i : util::irange(session_count)) {
                dkgSessionHandlers.emplace(SessionHandlerKey{params.type, i}, handler_fn(params, i));
            }
        }
    }

    /**
     * Visit every registered handler with @p fn(CDKGSessionHandler&). Used by
     * the DKG NetHandler to drive per-handler phase threads.
     */
    template <typename HandlerFn>
    void ForEachHandler(HandlerFn&& fn)
    {
        for (auto& [_, handler] : dkgSessionHandlers) {
            fn(*Assert(handler));
        }
    }

    /**
     * Invoke @p fn(CDKGSessionHandler&) for the handler registered under @p key,
     * if one exists. Returns true iff the handler was found (and the callback ran).
     * Returning the handler by reference inside the callback prevents callers from
     * keeping a dangling pointer outside the handler's lifetime.
     */
    template <typename HandlerFn>
    bool DoForHandler(const SessionHandlerKey& key, HandlerFn&& fn)
    {
        auto it = dkgSessionHandlers.find(key);
        if (it == dkgSessionHandlers.end()) return false;
        fn(*Assert(it->second));
        return true;
    }

    void UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
        EXCLUSIVE_LOCKS_REQUIRED(!contributionsCacheCs);

    bool GetContribution(const uint256& hash, CDKGContribution& ret) const;
    bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const;
    bool GetJustification(const uint256& hash, CDKGJustification& ret) const;
    bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const;

    // Contributions are written while in the DKG
    void WriteVerifiedVvecContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec)
        EXCLUSIVE_LOCKS_REQUIRED(!contributionsCacheCs);
    void WriteVerifiedSkContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSSecretKey& skContribution)
        EXCLUSIVE_LOCKS_REQUIRED(!contributionsCacheCs);
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
