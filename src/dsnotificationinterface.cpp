// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <dsnotificationinterface.h>
#include <governance/governance.h>
#include <masternode/masternodesync.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <evo/mnauth.h>

#include <llmq/quorums.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_chainlocks.h>
#include <shutdown.h>
#include <net_processing.h>
void CDSNotificationInterface::InitializeCurrentBlockTip(ChainstateManager& chainman)
{
    LOCK(cs_main);
    SynchronousUpdatedBlockTip(chainman.ActiveChain().Tip(), nullptr);
    UpdatedBlockTip(chainman.ActiveChain().Tip(), nullptr, chainman, chainman.ActiveChainstate().IsInitialBlockDownload());
}


void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew)
{
    if(llmq::chainLocksHandler)
        llmq::chainLocksHandler->NotifyHeaderTip(pindexNew);
    masternodeSync.NotifyHeaderTip(pindexNew);
}

void CDSNotificationInterface::SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork)
{
    if (pindexNew == pindexFork || ShutdownRequested()) // blocks were disconnected without any new ones
        return;
    if(deterministicMNManager)
        deterministicMNManager->UpdatedBlockTip(pindexNew);
}
void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, ChainstateManager& chainman, bool fInitialDownload)
{
    if (pindexNew == pindexFork || ShutdownRequested()) // blocks were disconnected without any new ones
        return;

    masternodeSync.UpdatedBlockTip(pindexNew, chainman, fInitialDownload);

    if (fInitialDownload)
        return;
    if(llmq::quorumManager)
        llmq::quorumManager->UpdatedBlockTip(pindexNew, fInitialDownload);
    if(llmq::quorumDKGSessionManager)
        llmq::quorumDKGSessionManager->UpdatedBlockTip(pindexNew, fInitialDownload);
    if(llmq::chainLocksHandler)
        llmq::chainLocksHandler->UpdatedBlockTip(pindexNew, fInitialDownload);
    if (!fDisableGovernance && governance) governance->UpdatedBlockTip(pindexNew, peerman);
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    if(ShutdownRequested())
        return;
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff, connman);
    if(governance) {
        governance->UpdateCachesAndClean();
    }
}
