// Copyright (c) 2014-2025 The Dash Core developers
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
#include <net_processing.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <evo/mnauth.h>
#include <instantsend/instantsend.h>
#include <llmq/chainlocks.h>
#include <llmq/context.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/quorums.h>

CDSNotificationInterface::CDSNotificationInterface(CConnman& connman,
                                                   CMasternodeSync& mn_sync,
                                                   CGovernanceManager& govman,
                                                   PeerManager& peerman,
                                                   const ChainstateManager& chainman,
                                                   const CActiveMasternodeManager* const mn_activeman,
                                                   const std::unique_ptr<CDeterministicMNManager>& dmnman,
                                                   const std::unique_ptr<LLMQContext>& llmq_ctx,
                                                   const std::unique_ptr<CJContext>& cj_ctx)
  : m_connman(connman),
    m_mn_sync(mn_sync),
    m_govman(govman),
    m_peerman(peerman),
    m_chainman(chainman),
    m_mn_activeman(mn_activeman),
    m_dmnman(dmnman),
    m_llmq_ctx(llmq_ctx),
    m_cj_ctx(cj_ctx) {}

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    SynchronousUpdatedBlockTip(m_chainman.ActiveChain().Tip(), nullptr, m_chainman.ActiveChainstate().IsInitialBlockDownload());
    UpdatedBlockTip(m_chainman.ActiveChain().Tip(), nullptr, m_chainman.ActiveChainstate().IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    assert(m_llmq_ctx);

    m_llmq_ctx->clhandler->AcceptedBlockHeader(pindexNew);
    m_mn_sync.AcceptedBlockHeader(pindexNew);
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    m_mn_sync.NotifyHeaderTip(pindexNew, fInitialDownload);
}

void CDSNotificationInterface::SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    assert(m_dmnman);

    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    m_dmnman->UpdatedBlockTip(pindexNew);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    assert(m_cj_ctx && m_llmq_ctx);

    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    m_mn_sync.UpdatedBlockTip(WITH_LOCK(::cs_main, return m_chainman.m_best_header), pindexNew, fInitialDownload);

    if (fInitialDownload)
        return;

    m_cj_ctx->dstxman->UpdatedBlockTip(pindexNew, *m_llmq_ctx->clhandler, m_mn_sync);
#ifdef ENABLE_WALLET
    m_cj_ctx->walletman->ForEachCJClientMan(
        [&pindexNew](std::unique_ptr<CCoinJoinClientManager>& clientman) { clientman->UpdatedBlockTip(pindexNew); });
#endif // ENABLE_WALLET

    m_llmq_ctx->isman->UpdatedBlockTip(pindexNew);
    m_llmq_ctx->clhandler->UpdatedBlockTip(*m_llmq_ctx->isman);

    m_llmq_ctx->qman->UpdatedBlockTip(pindexNew, m_connman, fInitialDownload);
    m_llmq_ctx->qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);
    m_llmq_ctx->ehfSignalsHandler->UpdatedBlockTip(pindexNew, /* is_masternode = */ m_mn_activeman != nullptr);

    if (m_govman.IsValid()) {
        m_govman.UpdatedBlockTip(pindexNew, m_connman, m_peerman, m_mn_activeman);
    }
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime,
                                                         uint64_t mempool_sequence)
{
    assert(m_cj_ctx && m_llmq_ctx);

    m_llmq_ctx->isman->TransactionAddedToMempool(m_peerman, ptx);
    m_llmq_ctx->clhandler->TransactionAddedToMempool(ptx, nAcceptTime);
    m_cj_ctx->dstxman->TransactionAddedToMempool(ptx);
}

void CDSNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason,
                                                             uint64_t mempool_sequence)
{
    assert(m_llmq_ctx);

    m_llmq_ctx->isman->TransactionRemovedFromMempool(ptx);
}

void CDSNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    assert(m_cj_ctx && m_llmq_ctx);

    m_llmq_ctx->isman->BlockConnected(pblock, pindex);
    m_llmq_ctx->clhandler->BlockConnected(pblock, pindex);
    m_cj_ctx->dstxman->BlockConnected(pblock, pindex);
}

void CDSNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    assert(m_cj_ctx && m_llmq_ctx);

    m_llmq_ctx->isman->BlockDisconnected(pblock, pindexDisconnected);
    m_llmq_ctx->clhandler->BlockDisconnected(pblock, pindexDisconnected);
    m_cj_ctx->dstxman->BlockDisconnected(pblock, pindexDisconnected);
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff, m_connman);
    if (m_govman.IsValid()) {
        m_govman.CheckAndRemove();
    }
}

void CDSNotificationInterface::NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    assert(m_cj_ctx && m_llmq_ctx);

    m_llmq_ctx->isman->NotifyChainLock(pindex);
    m_cj_ctx->dstxman->NotifyChainLock(pindex, *m_llmq_ctx->clhandler, m_mn_sync);
}

std::unique_ptr<CDSNotificationInterface> g_ds_notification_interface;
