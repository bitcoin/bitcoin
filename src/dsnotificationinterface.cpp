// Copyright (c) 2014-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coinjoin/coinjoin.h>
#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET
#include <dsnotificationinterface.h>
#include <governance/governance.h>
#include <masternode/sync.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <evo/mnauth.h>

#include <llmq/quorums.h>
#include <llmq/chainlocks.h>
#include <llmq/instantsend.h>
#include <llmq/dkgsessionmgr.h>

CDSNotificationInterface::CDSNotificationInterface(CConnman& _connman,
    std::unique_ptr<CMasternodeSync>& _mnsync, std::unique_ptr<CDeterministicMNManager>& _dmnman,
    std::unique_ptr<CGovernanceManager>& _govman, std::unique_ptr<llmq::CChainLocksHandler>& _clhandler,
    std::unique_ptr<llmq::CInstantSendManager>& _isman, std::unique_ptr<llmq::CQuorumManager>& _qman,
    std::unique_ptr<llmq::CDKGSessionManager>& _qdkgsman
) : connman(_connman), mnsync(_mnsync), dmnman(_dmnman), govman(_govman), clhandler(_clhandler), isman(_isman), qman(_qman), qdkgsman(_qdkgsman) {}

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    SynchronousUpdatedBlockTip(::ChainActive().Tip(), nullptr, ::ChainstateActive().IsInitialBlockDownload());
    UpdatedBlockTip(::ChainActive().Tip(), nullptr, ::ChainstateActive().IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    clhandler->AcceptedBlockHeader(pindexNew);
    if (mnsync != nullptr) {
        mnsync->AcceptedBlockHeader(pindexNew);
    }
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    mnsync->NotifyHeaderTip(pindexNew, fInitialDownload);
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

    mnsync->UpdatedBlockTip(pindexNew, fInitialDownload);

    // Update global DIP0001 activation status
    fDIP0001ActiveAtTip = pindexNew->nHeight >= Params().GetConsensus().DIP0001Height;

    if (fInitialDownload)
        return;

    CCoinJoin::UpdatedBlockTip(pindexNew, *clhandler);
#ifdef ENABLE_WALLET
    for (auto& pair : coinJoinClientManagers) {
        pair.second->UpdatedBlockTip(pindexNew);
    }
#endif // ENABLE_WALLET

    isman->UpdatedBlockTip(pindexNew);
    clhandler->UpdatedBlockTip();

    qman->UpdatedBlockTip(pindexNew, fInitialDownload);
    qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);

    if (!fDisableGovernance) govman->UpdatedBlockTip(pindexNew, connman);
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime)
{
    isman->TransactionAddedToMempool(ptx);
    clhandler->TransactionAddedToMempool(ptx, nAcceptTime);
    CCoinJoin::TransactionAddedToMempool(ptx);
}

void CDSNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason)
{
    isman->TransactionRemovedFromMempool(ptx);
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

    isman->BlockConnected(pblock, pindex, vtxConflicted);
    clhandler->BlockConnected(pblock, pindex, vtxConflicted);
    CCoinJoin::BlockConnected(pblock, pindex, vtxConflicted);
}

void CDSNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    isman->BlockDisconnected(pblock, pindexDisconnected);
    clhandler->BlockDisconnected(pblock, pindexDisconnected);
    CCoinJoin::BlockDisconnected(pblock, pindexDisconnected);
}

void CDSNotificationInterface::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff, CConnman& connman)
{
    CMNAuth::NotifyMasternodeListChanged(undo, oldMNList, diff, connman);
    govman->UpdateCachesAndClean();
}

void CDSNotificationInterface::NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    isman->NotifyChainLock(pindex);
    CCoinJoin::NotifyChainLock(pindex, *clhandler);
}
