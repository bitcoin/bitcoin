// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coinjoin/coinjoin.h>
#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET
#include <coinjoin/context.h>
#include <dsnotificationinterface.h>
#include <governance/governance.h>
#include <masternode/sync.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <evo/mnauth.h>

#include <llmq/chainlocks.h>
#include <llmq/context.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>

CDSNotificationInterface::CDSNotificationInterface(CConnman& _connman,
                                                   CMasternodeSync& _mn_sync, const std::unique_ptr<CDeterministicMNManager>& _dmnman,
                                                   CGovernanceManager& _govman, const std::unique_ptr<LLMQContext>& _llmq_ctx,
                                                   const std::unique_ptr<CJContext>& _cj_ctx
) : connman(_connman), m_mn_sync(_mn_sync), dmnman(_dmnman), govman(_govman), llmq_ctx(_llmq_ctx), cj_ctx(_cj_ctx) {}

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    SynchronousUpdatedBlockTip(::ChainActive().Tip(), nullptr, ::ChainstateActive().IsInitialBlockDownload());
    UpdatedBlockTip(::ChainActive().Tip(), nullptr, ::ChainstateActive().IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    llmq_ctx->clhandler->AcceptedBlockHeader(pindexNew);
    m_mn_sync.AcceptedBlockHeader(pindexNew);
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    m_mn_sync.NotifyHeaderTip(pindexNew, fInitialDownload);
}

void CDSNotificationInterface::SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    dmnman->UpdatedBlockTip(pindexNew);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    m_mn_sync.UpdatedBlockTip(pindexNew, fInitialDownload);

    // Update global DIP0001 activation status
    fDIP0001ActiveAtTip = pindexNew->nHeight >= Params().GetConsensus().DIP0001Height;

    if (fInitialDownload)
        return;

    ::dstxManager->UpdatedBlockTip(pindexNew, *llmq_ctx->clhandler, m_mn_sync);
#ifdef ENABLE_WALLET
    for (auto& pair : cj_ctx->walletman->raw()) {
        pair.second->UpdatedBlockTip(pindexNew);
    }
#endif // ENABLE_WALLET

    llmq_ctx->isman->UpdatedBlockTip(pindexNew);
    llmq_ctx->clhandler->UpdatedBlockTip();

    llmq_ctx->qman->UpdatedBlockTip(pindexNew, fInitialDownload);
    llmq_ctx->qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);
    llmq_ctx->ehfSignalsHandler->UpdatedBlockTip(pindexNew);

    if (!fDisableGovernance) govman.UpdatedBlockTip(pindexNew, connman);
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime)
{
    llmq_ctx->isman->TransactionAddedToMempool(ptx);
    llmq_ctx->clhandler->TransactionAddedToMempool(ptx, nAcceptTime);
    ::dstxManager->TransactionAddedToMempool(ptx);
}

void CDSNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason)
{
    llmq_ctx->isman->TransactionRemovedFromMempool(ptx);
}

void CDSNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    llmq_ctx->isman->BlockConnected(pblock, pindex);
    llmq_ctx->clhandler->BlockConnected(pblock, pindex);
    ::dstxManager->BlockConnected(pblock, pindex);
}

void CDSNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    llmq_ctx->isman->BlockDisconnected(pblock, pindexDisconnected);
    llmq_ctx->clhandler->BlockDisconnected(pblock, pindexDisconnected);
    ::dstxManager->BlockDisconnected(pblock, pindexDisconnected);
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff, connman);
    govman.UpdateCachesAndClean();
}

void CDSNotificationInterface::NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    llmq_ctx->isman->NotifyChainLock(pindex);
    ::dstxManager->NotifyChainLock(pindex, *llmq_ctx->clhandler, m_mn_sync);
}
