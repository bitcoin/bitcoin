// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/active/context.h>

#include <active/dkgsessionhandler.h>
#include <active/quorums.h>
#include <chainlock/chainlock.h>
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
#include <validation.h>

ActiveContext::ActiveContext(CCoinJoinServer& cj_server, CConnman& connman, CDeterministicMNManager& dmnman,
                             CGovernanceManager& govman, ChainstateManager& chainman, CMasternodeMetaMan& mn_metaman,
                             CMNHFManager& mnhfman, CSporkManager& sporkman, CTxMemPool& mempool, LLMQContext& llmq_ctx,
                             PeerManager& peerman, const CActiveMasternodeManager& mn_activeman,
                             const CMasternodeSync& mn_sync, const llmq::QvvecSyncModeMap& sync_map,
                             const util::DbWrapperParams& db_params, bool quorums_recovery, bool quorums_watch) :
    m_llmq_ctx{llmq_ctx},
    m_cj_server(cj_server),
    gov_signer{std::make_unique<GovernanceSigner>(connman, dmnman, govman, mn_activeman, chainman, mn_sync)},
    dkgdbgman{std::make_unique<llmq::CDKGDebugManager>()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(dmnman, *llmq_ctx.qsnapman, chainman, sporkman, db_params,
                                                        quorums_watch)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, chainman.ActiveChainstate(), *llmq_ctx.sigman, peerman,
                                                       mn_activeman, *llmq_ctx.qman, sporkman)},
    ehf_sighandler{
        std::make_unique<llmq::CEHFSignalsHandler>(chainman, mnhfman, *llmq_ctx.sigman, *shareman, *llmq_ctx.qman)},
    qman_handler{std::make_unique<llmq::QuorumParticipant>(*llmq_ctx.bls_worker, connman, dmnman, *llmq_ctx.qman,
                                                           *llmq_ctx.qsnapman, mn_activeman, chainman, mn_sync,
                                                           sporkman, sync_map, quorums_recovery, quorums_watch)},
    cl_signer{std::make_unique<chainlock::ChainLockSigner>(chainman.ActiveChainstate(), *llmq_ctx.clhandler,
                                                           *llmq_ctx.sigman, *shareman, sporkman, mn_sync)},
    is_signer{std::make_unique<instantsend::InstantSendSigner>(chainman.ActiveChainstate(), *llmq_ctx.clhandler,
                                                               *llmq_ctx.isman, *llmq_ctx.sigman, *shareman,
                                                               *llmq_ctx.qman, sporkman, mempool, mn_sync)}
{
    qdkgsman->InitializeHandlers([&](const Consensus::LLMQParams& llmq_params,
                                     int quorum_idx) -> std::unique_ptr<llmq::ActiveDKGSessionHandler> {
        return std::make_unique<llmq::ActiveDKGSessionHandler>(*llmq_ctx.bls_worker, dmnman, mn_metaman, *dkgdbgman,
                                                               *qdkgsman, *llmq_ctx.quorum_block_processor,
                                                               *llmq_ctx.qsnapman, mn_activeman, chainman, sporkman,
                                                               llmq_params, quorums_watch, quorum_idx);
    });
    m_llmq_ctx.clhandler->ConnectSigner(cl_signer.get());
    m_llmq_ctx.isman->ConnectSigner(is_signer.get());
    m_llmq_ctx.qman->ConnectManagers(qman_handler.get(), qdkgsman.get());
}

ActiveContext::~ActiveContext()
{
    m_llmq_ctx.qman->DisconnectManagers();
    m_llmq_ctx.isman->DisconnectSigner();
    m_llmq_ctx.clhandler->DisconnectSigner();
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
    cl_signer->RegisterRecoveryInterface();
    is_signer->RegisterRecoveryInterface();
    shareman->RegisterRecoveryInterface();
}

void ActiveContext::Stop()
{
    shareman->UnregisterRecoveryInterface();
    is_signer->UnregisterRecoveryInterface();
    cl_signer->UnregisterRecoveryInterface();
    shareman->Stop();
    qdkgsman->StopThreads();
    qman_handler->Stop();
}
