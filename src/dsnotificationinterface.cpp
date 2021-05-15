// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coinjoin/coinjoin.h>
#ifdef ENABLE_WALLET
#include <coinjoin/coinjoin-client.h>
#endif // ENABLE_WALLET
#include <dsnotificationinterface.h>
#include <governance/governance.h>
#include <masternode/masternode-sync.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <evo/mnauth.h>

#include <llmq/quorums.h>
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_dkgsessionmgr.h>

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    SynchronousUpdatedBlockTip(chainActive.Tip(), nullptr, IsInitialBlockDownload());
    UpdatedBlockTip(chainActive.Tip(), nullptr, IsInitialBlockDownload());
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

void CDSNotificationInterface::SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    deterministicMNManager->UpdatedBlockTip(pindexNew);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    // Update global DIP0001 activation status
    fDIP0001ActiveAtTip = pindexNew->nHeight >= Params().GetConsensus().DIP0001Height;

    if (fInitialDownload)
        return;

    CCoinJoin::UpdatedBlockTip(pindexNew);
#ifdef ENABLE_WALLET
    for (auto& pair : coinJoinClientManagers) {
        pair.second->UpdatedBlockTip(pindexNew);
    }
#endif // ENABLE_WALLET

    llmq::quorumInstantSendManager->UpdatedBlockTip(pindexNew);
    llmq::chainLocksHandler->UpdatedBlockTip(pindexNew);

    llmq::quorumManager->UpdatedBlockTip(pindexNew, fInitialDownload);
    llmq::quorumDKGSessionManager->UpdatedBlockTip(pindexNew, fInitialDownload);

    if (!fDisableGovernance) governance.UpdatedBlockTip(pindexNew, connman);
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime)
{
    llmq::quorumInstantSendManager->TransactionAddedToMempool(ptx);
    llmq::chainLocksHandler->TransactionAddedToMempool(ptx, nAcceptTime);
    CCoinJoin::TransactionAddedToMempool(ptx);
}

void CDSNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx)
{
    llmq::quorumInstantSendManager->TransactionRemovedFromMempool(ptx);
}

void CDSNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted)
{
    // TODO: Temporarily ensure that mempool removals are notified before
    // connected transactions.  This shouldn't matter, but the abandoned
    // state of transactions in our wallet is currently cleared when we
    // receive another notification and there is a race condition where
    // notification of a connected conflict might cause an outside process
    // to abandon a transaction and then have it inadvertently cleared by
    // the notification that the conflicted transaction was evicted.

    llmq::quorumInstantSendManager->BlockConnected(pblock, pindex, vtxConflicted);
    llmq::chainLocksHandler->BlockConnected(pblock, pindex, vtxConflicted);
    CCoinJoin::BlockConnected(pblock, pindex, vtxConflicted);
}

void CDSNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    llmq::quorumInstantSendManager->BlockDisconnected(pblock, pindexDisconnected);
    llmq::chainLocksHandler->BlockDisconnected(pblock, pindexDisconnected);
    CCoinJoin::BlockDisconnected(pblock, pindexDisconnected);
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff);
    governance.UpdateCachesAndClean();
}

void CDSNotificationInterface::NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    llmq::quorumInstantSendManager->NotifyChainLock(pindex);
    CCoinJoin::NotifyChainLock(pindex);
}
