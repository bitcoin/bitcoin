// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <dsnotificationinterface.h>

#include <util/check.h>
#include <validation.h>

#include <coinjoin/coinjoin.h>
#include <evo/deterministicmns.h>
#include <evo/mnauth.h>
#include <governance/governance.h>
#include <instantsend/instantsend.h>
#include <llmq/context.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/quorumsman.h>
#include <masternode/sync.h>

CDSNotificationInterface::CDSNotificationInterface(CConnman& connman,
                                                   CDSTXManager& dstxman,
                                                   CMasternodeSync& mn_sync,
                                                   CGovernanceManager& govman,
                                                   const ChainstateManager& chainman,
                                                   const std::unique_ptr<CDeterministicMNManager>& dmnman,
                                                   const std::unique_ptr<LLMQContext>& llmq_ctx) :
    m_connman{connman},
    m_dstxman{dstxman},
    m_mn_sync{mn_sync},
    m_govman{govman},
    m_chainman{chainman},
    m_dmnman{dmnman},
    m_llmq_ctx{llmq_ctx}
{
}

CDSNotificationInterface::~CDSNotificationInterface() = default;

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    SynchronousUpdatedBlockTip(m_chainman.ActiveChain().Tip(), nullptr, m_chainman.ActiveChainstate().IsInitialBlockDownload());
    UpdatedBlockTip(m_chainman.ActiveChain().Tip(), nullptr, m_chainman.ActiveChainstate().IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
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

    Assert(m_dmnman)->UpdatedBlockTip(pindexNew);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    m_mn_sync.UpdatedBlockTip(WITH_LOCK(::cs_main, return m_chainman.m_best_header), pindexNew, fInitialDownload);

    if (fInitialDownload)
        return;

    if (m_mn_sync.IsBlockchainSynced()) {
        m_dstxman.UpdatedBlockTip(pindexNew);
    }

    m_llmq_ctx->isman->UpdatedBlockTip(pindexNew);
    if (m_govman.IsValid()) {
        m_govman.UpdatedBlockTip(pindexNew);
    }
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime,
                                                         uint64_t mempool_sequence)
{
    Assert(m_llmq_ctx)->isman->TransactionAddedToMempool(ptx);
    m_dstxman.TransactionAddedToMempool(ptx);
}

void CDSNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason,
                                                             uint64_t mempool_sequence)
{
    Assert(m_llmq_ctx)->isman->TransactionRemovedFromMempool(ptx);
}

void CDSNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    Assert(m_llmq_ctx)->isman->BlockConnected(pblock, pindex);
    m_dstxman.BlockConnected(pblock, pindex);
}

void CDSNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    Assert(m_llmq_ctx)->isman->BlockDisconnected(pblock, pindexDisconnected);
    m_dstxman.BlockDisconnected(pblock, pindexDisconnected);
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff, m_connman);
    if (m_govman.IsValid()) {
        m_govman.CheckAndRemove();
    }
}

void CDSNotificationInterface::NotifyChainLock(const CBlockIndex* pindex,
                                               const std::shared_ptr<const chainlock::ChainLockSig>& clsig)
{
    Assert(m_llmq_ctx)->isman->NotifyChainLock(pindex);
    if (m_mn_sync.IsBlockchainSynced()) {
        m_dstxman.NotifyChainLock(pindex);
    }
}

std::unique_ptr<CDSNotificationInterface> g_ds_notification_interface;
