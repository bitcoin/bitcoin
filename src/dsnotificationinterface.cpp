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

void CDSNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock)
{
    llmq::quorumInstantSendManager->SyncTransaction(tx, pindex, posInBlock);
    llmq::chainLocksHandler->SyncTransaction(tx, pindex, posInBlock);
    instantsend.SyncTransaction(tx, pindex, posInBlock);
    CPrivateSend::SyncTransaction(tx, pindex, posInBlock);
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
