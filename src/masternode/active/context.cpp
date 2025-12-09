// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/active/context.h>

#include <chainlock/chainlock.h>
#include <chainlock/signing.h>
#include <coinjoin/server.h>
#include <governance/governance.h>
#include <governance/signing.h>
#include <instantsend/instantsend.h>
#include <instantsend/signing.h>
#include <llmq/context.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/signing_shares.h>
#include <validation.h>

ActiveContext::ActiveContext(ChainstateManager& chainman, CConnman& connman, CDeterministicMNManager& dmnman,
                             CDSTXManager& dstxman, CGovernanceManager& govman, CMasternodeMetaMan& mn_metaman,
                             CMNHFManager& mnhfman, CSporkManager& sporkman, CTxMemPool& mempool, LLMQContext& llmq_ctx,
                             PeerManager& peerman, const CActiveMasternodeManager& mn_activeman,
                             const CMasternodeSync& mn_sync) :
    m_llmq_ctx{llmq_ctx},
    cj_server{std::make_unique<CCoinJoinServer>(chainman, connman, dmnman, dstxman, mn_metaman, mempool, peerman,
                                                mn_activeman, mn_sync, *llmq_ctx.isman)},
    gov_signer{std::make_unique<GovernanceSigner>(connman, dmnman, govman, mn_activeman, chainman, mn_sync)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, chainman.ActiveChainstate(), *llmq_ctx.sigman, peerman,
                                                       mn_activeman, *llmq_ctx.qman, sporkman)},
    ehf_sighandler{
        std::make_unique<llmq::CEHFSignalsHandler>(chainman, mnhfman, *llmq_ctx.sigman, *shareman, *llmq_ctx.qman)},
    cl_signer{std::make_unique<chainlock::ChainLockSigner>(chainman.ActiveChainstate(), *llmq_ctx.clhandler,
                                                           *llmq_ctx.sigman, *shareman, sporkman, mn_sync)},
    is_signer{std::make_unique<instantsend::InstantSendSigner>(chainman.ActiveChainstate(), *llmq_ctx.clhandler,
                                                               *llmq_ctx.isman, *llmq_ctx.sigman, *shareman,
                                                               *llmq_ctx.qman, sporkman, mempool, mn_sync)}
{
    m_llmq_ctx.clhandler->ConnectSigner(cl_signer.get());
    m_llmq_ctx.isman->ConnectSigner(is_signer.get());
}

ActiveContext::~ActiveContext()
{
    m_llmq_ctx.clhandler->DisconnectSigner();
    m_llmq_ctx.isman->DisconnectSigner();
}

void ActiveContext::Interrupt()
{
    shareman->InterruptWorkerThread();
}

void ActiveContext::Start(CConnman& connman, PeerManager& peerman)
{
    m_llmq_ctx.qdkgsman->StartThreads(connman, peerman);
    shareman->RegisterAsRecoveredSigsListener();
    shareman->Start();
}

void ActiveContext::Stop()
{
    shareman->Stop();
    shareman->UnregisterAsRecoveredSigsListener();
    m_llmq_ctx.qdkgsman->StopThreads();
}
