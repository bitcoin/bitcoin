// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <active/context.h>

#include <active/dkgsessionhandler.h>
#include <active/masternode.h>
#include <active/quorums.h>
#include <chainlock/handler.h>
#include <chainlock/signing.h>
#include <governance/governance.h>
#include <governance/signing.h>
#include <instantsend/instantsend.h>
#include <instantsend/signing.h>
#include <llmq/context.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/quorumsman.h>
#include <llmq/signing_shares.h>
#include <util/check.h>
#include <validation.h>
#include <validationinterface.h>

ActiveContext::ActiveContext(CBLSWorker& bls_worker, ChainstateManager& chainman, CConnman& connman,
                             CDeterministicMNManager& dmnman, CGovernanceManager& govman, CMasternodeMetaMan& mn_metaman,
                             CSporkManager& sporkman, const chainlock::Chainlocks& chainlocks, CTxMemPool& mempool,
                             chainlock::ChainlockHandler& clhandler, llmq::CInstantSendManager& isman,
                             llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumManager& qman,
                             llmq::CQuorumSnapshotManager& qsnapman, llmq::CSigningManager& sigman,
                             PeerManager& peerman, const CMasternodeSync& mn_sync, const CBLSSecretKey& operator_sk,
                             const llmq::QvvecSyncModeMap& sync_map, const util::DbWrapperParams& db_params,
                             bool quorums_recovery, bool quorums_watch) :
    m_isman{isman},
    m_qman{qman},
    nodeman{std::make_unique<CActiveMasternodeManager>(connman, dmnman, operator_sk)},
    dkgdbgman{std::make_unique<llmq::CDKGDebugManager>()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(dmnman, qsnapman, chainman, sporkman, db_params, quorums_watch)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, chainman.ActiveChainstate(), sigman, peerman, *nodeman,
                                                       qman, sporkman)},
    gov_signer{std::make_unique<GovernanceSigner>(connman, dmnman, govman, *nodeman, chainman, mn_sync)},
    ehf_sighandler{std::make_unique<llmq::CEHFSignalsHandler>(chainman, sigman, *shareman, qman)},
    qman_handler{std::make_unique<llmq::QuorumParticipant>(bls_worker, connman, dmnman, qman, qsnapman, *nodeman, chainman,
                                                           mn_sync, sporkman, sync_map, quorums_recovery, quorums_watch)},
    cl_signer{std::make_unique<chainlock::ChainLockSigner>(chainman.ActiveChainstate(), chainlocks, clhandler, isman,
                                                           qman, sigman, *shareman, mn_sync)},
    is_signer{std::make_unique<instantsend::InstantSendSigner>(chainman.ActiveChainstate(), chainlocks, isman, sigman,
                                                               *shareman, qman, sporkman, mempool, mn_sync)}
{
    qdkgsman->InitializeHandlers([&](const Consensus::LLMQParams& llmq_params,
                                     int quorum_idx) -> std::unique_ptr<llmq::ActiveDKGSessionHandler> {
        return std::make_unique<llmq::ActiveDKGSessionHandler>(bls_worker, dmnman, mn_metaman, *dkgdbgman, *qdkgsman,
                                                               qblockman, qsnapman, *nodeman, chainman, sporkman,
                                                               llmq_params, quorums_watch, quorum_idx);
    });
    m_isman.ConnectSigner(is_signer.get());
    m_qman.ConnectManagers(qman_handler.get(), qdkgsman.get());
}

ActiveContext::~ActiveContext()
{
    m_qman.DisconnectManagers();
    m_isman.DisconnectSigner();
}

void ActiveContext::Interrupt()
{
    shareman->InterruptWorkerThread();
}

void ActiveContext::Start(CConnman& connman, PeerManager& peerman)
{
    qman_handler->Start();
    qdkgsman->StartThreads(connman, peerman);
    shareman->Start();
    cl_signer->Start();
    cl_signer->RegisterRecoveryInterface();
    is_signer->RegisterRecoveryInterface();
    shareman->RegisterRecoveryInterface();

    RegisterValidationInterface(cl_signer.get());
}

void ActiveContext::Stop()
{
    UnregisterValidationInterface(cl_signer.get());

    shareman->UnregisterRecoveryInterface();
    is_signer->UnregisterRecoveryInterface();
    cl_signer->UnregisterRecoveryInterface();
    cl_signer->Stop();
    shareman->Stop();
    qdkgsman->StopThreads();
    qman_handler->Stop();
}

CCoinJoinServer& ActiveContext::GetCJServer() const
{
    return *Assert(m_cj_server);
}

void ActiveContext::SetCJServer(gsl::not_null<CCoinJoinServer*> cj_server)
{
    // Prohibit double initialization
    assert(m_cj_server == nullptr);
    m_cj_server = cj_server;
}

void ActiveContext::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    nodeman->UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    ehf_sighandler->UpdatedBlockTip(pindexNew);
    gov_signer->UpdatedBlockTip(pindexNew);
    qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);
    qman_handler->UpdatedBlockTip(pindexNew, fInitialDownload);
}

void ActiveContext::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig, bool proactive_relay)
{
    shareman->NotifyRecoveredSig(sig, proactive_relay);
}
