// Copyright (c) 2025-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ACTIVE_CONTEXT_H
#define BITCOIN_ACTIVE_CONTEXT_H

#include <llmq/options.h>

#include <validationinterface.h>

#include <gsl/pointers.h>

#include <memory>

class CActiveMasternodeManager;
class CBLSSecretKey;
class CBLSWorker;
class ChainstateManager;
class CCoinJoinServer;
class CConnman;
class CDeterministicMNManager;
class CGovernanceManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CMNHFManager;
class CSporkManager;
class CTxMemPool;
class GovernanceSigner;
class PeerManager;
namespace chainlock {
class Chainlocks;
class ChainlockHandler;
class ChainLockSigner;
} // namespace chainlock
namespace instantsend {
class InstantSendSigner;
} // namespace instantsend
namespace llmq {
class CDKGDebugManager;
class CDKGSessionManager;
class CEHFSignalsHandler;
class CInstantSendManager;
class CQuorumBlockProcessor;
class CQuorumManager;
class CQuorumSnapshotManager;
class CSigningManager;
class CSigSharesManager;
class QuorumParticipant;
} // namespace llmq
namespace util {
struct DbWrapperParams;
} // namespace util

struct ActiveContext final : public CValidationInterface {
private:
    llmq::CInstantSendManager& m_isman;
    llmq::CQuorumManager& m_qman;

public:
    ActiveContext() = delete;
    ActiveContext(const ActiveContext&) = delete;
    ActiveContext& operator=(const ActiveContext&) = delete;
    explicit ActiveContext(CBLSWorker& bls_worker, ChainstateManager& chainman, CConnman& connman,
                           CDeterministicMNManager& dmnman, CGovernanceManager& govman, CMasternodeMetaMan& mn_metaman,
                           CSporkManager& sporkman, const chainlock::Chainlocks& chainlocks, CTxMemPool& mempool,
                           chainlock::ChainlockHandler& clhandler, llmq::CInstantSendManager& isman,
                           llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumManager& qman,
                           llmq::CQuorumSnapshotManager& qsnapman, llmq::CSigningManager& sigman, PeerManager& peerman,
                           const CMasternodeSync& mn_sync, const CBLSSecretKey& operator_sk,
                           const llmq::QvvecSyncModeMap& sync_map, const util::DbWrapperParams& db_params,
                           bool quorums_recovery, bool quorums_watch);
    ~ActiveContext();

    void Interrupt();
    void Start(CConnman& connman, PeerManager& peerman);
    void Stop();

    CCoinJoinServer& GetCJServer() const;
    void SetCJServer(gsl::not_null<CCoinJoinServer*> cj_server);

protected:
    // CValidationInterface
    void NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig, bool proactive_relay) override;
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;

public:
    /*
     * Entities that are only utilized when masternode mode is enabled
     * and are accessible in their own right
     */
    const std::unique_ptr<CActiveMasternodeManager> nodeman;
    const std::unique_ptr<llmq::CDKGDebugManager> dkgdbgman;
    const std::unique_ptr<llmq::CDKGSessionManager> qdkgsman;
    const std::unique_ptr<llmq::CSigSharesManager> shareman;

private:
    const std::unique_ptr<GovernanceSigner> gov_signer;
    const std::unique_ptr<llmq::CEHFSignalsHandler> ehf_sighandler;
    const std::unique_ptr<llmq::QuorumParticipant> qman_handler;
    const std::unique_ptr<chainlock::ChainLockSigner> cl_signer;
    const std::unique_ptr<instantsend::InstantSendSigner> is_signer;

    /** Owned by PeerManager, use GetCJServer() */
    CCoinJoinServer* m_cj_server{nullptr};
};

#endif // BITCOIN_ACTIVE_CONTEXT_H
