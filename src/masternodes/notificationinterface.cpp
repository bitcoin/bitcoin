// Copyright (c) 2014-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <masternodes/notificationinterface.h>
#include <masternodes/payments.h>
#include <masternodes/sync.h>
#include <validation.h>

#include <special/deterministicmns.h>
#include <special/mnauth.h>

#include <llmq/quorums.h>
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_dkgsessionmgr.h>

#include <util/init.h>

void CMNNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(ChainActive().Tip(), nullptr, ::ChainstateActive().IsInitialBlockDownload());
}

void CMNNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    llmq::chainLocksHandler->AcceptedBlockHeader(pindexNew);
    masternodeSync.AcceptedBlockHeader(pindexNew);
}

void CMNNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    masternodeSync.NotifyHeaderTip(pindexNew, fInitialDownload, connman);
}

void CMNNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    deterministicMNManager->UpdatedBlockTip(pindexNew);

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    if (fInitialDownload)
        return;

    if (fLiteMode)
        return;

    llmq::quorumInstantSendManager->UpdatedBlockTip(pindexNew);
    llmq::chainLocksHandler->UpdatedBlockTip(pindexNew);

    // governance.UpdatedBlockTip(pindexNew, connman);
    llmq::quorumManager->UpdatedBlockTip(pindexNew, fInitialDownload);
    llmq::quorumDKGSessionManager->UpdatedBlockTip(pindexNew, fInitialDownload);
}

void CMNNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock)
{
    llmq::quorumInstantSendManager->SyncTransaction(tx, pindex, posInBlock);
    llmq::chainLocksHandler->SyncTransaction(tx, pindex, posInBlock);
}

void CMNNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff);
    // governance.CheckMasternodeOrphanObjects(connman);
    // governance.CheckMasternodeOrphanVotes(connman);
    // governance.UpdateCachesAndClean();
}

void CMNNotificationInterface::NotifyChainLock(const CBlockIndex* pindex)
{
    llmq::quorumInstantSendManager->NotifyChainLock(pindex);
}

CMNNotificationInterface* g_mn_notification_interface = nullptr;
