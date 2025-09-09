// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/active/context.h>

#include <chainlock/chainlock.h>
#include <chainlock/signing.h>
#include <coinjoin/server.h>
#include <instantsend/instantsend.h>
#include <instantsend/signing.h>
#include <llmq/context.h>
#include <validation.h>

ActiveContext::ActiveContext(ChainstateManager& chainman, CConnman& connman, CDeterministicMNManager& dmnman,
                             CDSTXManager& dstxman, CMasternodeMetaMan& mn_metaman, LLMQContext& llmq_ctx,
                             CSporkManager& sporkman, CTxMemPool& mempool, const CActiveMasternodeManager& mn_activeman,
                             const CMasternodeSync& mn_sync, std::unique_ptr<PeerManager>& peerman) :
    m_llmq_ctx{llmq_ctx},
    cl_signer{std::make_unique<chainlock::ChainLockSigner>(chainman.ActiveChainstate(), *llmq_ctx.clhandler,
                                                           *llmq_ctx.sigman, *llmq_ctx.shareman, sporkman, mn_sync)},
    is_signer{std::make_unique<instantsend::InstantSendSigner>(chainman.ActiveChainstate(), *llmq_ctx.clhandler,
                                                               *llmq_ctx.isman, *llmq_ctx.sigman, *llmq_ctx.shareman,
                                                               *llmq_ctx.qman, sporkman, mempool, mn_sync)},
    cj_server{std::make_unique<CCoinJoinServer>(chainman, connman, dmnman, dstxman, mn_metaman, mempool, &mn_activeman,
                                                mn_sync, *llmq_ctx.isman, peerman)}
{
    m_llmq_ctx.clhandler->ConnectSigner(cl_signer.get());
    m_llmq_ctx.isman->ConnectSigner(is_signer.get());
}

ActiveContext::~ActiveContext()
{
    m_llmq_ctx.clhandler->DisconnectSigner();
    m_llmq_ctx.isman->DisconnectSigner();
}
