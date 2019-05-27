// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "dsnotificationinterface.h"
#include "instantsend.h"
#include "governance/governance.h"
#include "masternode/masternode-payments.h"
#include "masternode/masternode-sync.h"
#include "privatesend/privatesend.h"
#ifdef ENABLE_WALLET
#include "privatesend/privatesend-client.h"
#endif // ENABLE_WALLET
#include "validation.h"

#include "evo/deterministicmns.h"
#include "evo/mnauth.h"

#include "llmq/quorums.h"
#include "llmq/quorums_chainlocks.h"
#include "llmq/quorums_instantsend.h"
#include "llmq/quorums_dkgsessionmgr.h"

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(chainActive.Tip(), NULL, IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    llmq::chainLocksHandler->AcceptedBlockHeader(pindexNew);
    masternodeSync.AcceptedBlockHeader(pindexNew);
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    masternodeSync.NotifyHeaderTip(pindexNew, fInitialDownload, connman);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    deterministicMNManager->UpdatedBlockTip(pindexNew);

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    // Update global DIP0001 activation status
    fDIP0001ActiveAtTip = pindexNew->nHeight >= Params().GetConsensus().DIP0001Height;
    // update instantsend autolock activation flag (we reuse the DIP3 deployment)
    instantsend.isAutoLockBip9Active = pindexNew->nHeight + 1 >= Params().GetConsensus().DIP0003Height;

    if (fInitialDownload)
        return;

    if (fLiteMode)
        return;

    llmq::quorumInstantSendManager->UpdatedBlockTip(pindexNew);
    llmq::chainLocksHandler->UpdatedBlockTip(pindexNew);

    CPrivateSend::UpdatedBlockTip(pindexNew);
#ifdef ENABLE_WALLET
    privateSendClient.UpdatedBlockTip(pindexNew);
#endif // ENABLE_WALLET
    instantsend.UpdatedBlockTip(pindexNew);
    governance.UpdatedBlockTip(pindexNew, connman);
    llmq::quorumManager->UpdatedBlockTip(pindexNew, fInitialDownload);
    llmq::quorumDKGSessionManager->UpdatedBlockTip(pindexNew, fInitialDownload);
}

void CDSNotificationInterface::SyncTransaction(const CTransactionRef& tx, const CBlockIndex* pindex, int posInBlock)
{
    instantsend.SyncTransaction(tx, pindex, posInBlock);
    CPrivateSend::SyncTransaction(tx, pindex, posInBlock);
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx)
{
    llmq::quorumInstantSendManager->TransactionAddedToMempool(ptx);
    llmq::chainLocksHandler->TransactionAddedToMempool(ptx);
    SyncTransaction(ptx);
}

void CDSNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted)
{
    // TODO: Tempoarily ensure that mempool removals are notified before
    // connected transactions.  This shouldn't matter, but the abandoned
    // state of transactions in our wallet is currently cleared when we
    // receive another notification and there is a race condition where
    // notification of a connected conflict might cause an outside process
    // to abandon a transaction and then have it inadvertantly cleared by
    // the notification that the conflicted transaction was evicted.

    llmq::quorumInstantSendManager->BlockConnected(pblock, pindex, vtxConflicted);
    llmq::chainLocksHandler->BlockConnected(pblock, pindex, vtxConflicted);

    for (const CTransactionRef& ptx : vtxConflicted) {
        SyncTransaction(ptx);
    }
    for (size_t i = 0; i < pblock->vtx.size(); i++) {
        SyncTransaction(pblock->vtx[i], pindex, i);
    }
}

void CDSNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    llmq::quorumInstantSendManager->BlockDisconnected(pblock, pindexDisconnected);
    llmq::chainLocksHandler->BlockDisconnected(pblock, pindexDisconnected);

    for (const CTransactionRef& ptx : pblock->vtx) {
        SyncTransaction(ptx, pindexDisconnected->pprev, -1);
    }
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff);
    governance.CheckMasternodeOrphanObjects(connman);
    governance.CheckMasternodeOrphanVotes(connman);
    governance.UpdateCachesAndClean();
}

void CDSNotificationInterface::NotifyChainLock(const CBlockIndex* pindex, const llmq::CChainLockSig& clsig)
{
    llmq::quorumInstantSendManager->NotifyChainLock(pindex);
}
