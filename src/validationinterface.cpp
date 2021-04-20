// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validationinterface.h>

#include <init.h>
#include <primitives/block.h>
#include <scheduler.h>
#include <sync.h>
#include <txmempool.h>
#include <util.h>
#include <validation.h>

#include <list>
#include <atomic>
#include <future>

#include <boost/signals2/signal.hpp>

struct MainSignalsInstance {
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> UpdatedBlockTip;
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> SynchronousUpdatedBlockTip;
    boost::signals2::signal<void (const CTransactionRef &, int64_t)> TransactionAddedToMempool;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex, const std::vector<CTransactionRef>&)> BlockConnected;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex* pindexDisconnected)> BlockDisconnected;
    boost::signals2::signal<void (const CTransactionRef &)> TransactionRemovedFromMempool;
    boost::signals2::signal<void (const CBlockLocator &)> SetBestChain;
    boost::signals2::signal<void (int64_t nBestBlockTime, CConnman* connman)> Broadcast;
    boost::signals2::signal<void (const CBlock&, const CValidationState&)> BlockChecked;
    boost::signals2::signal<void (const CBlockIndex *, const std::shared_ptr<const CBlock>&)> NewPoWValidBlock;
    boost::signals2::signal<void (const CBlockIndex *)>AcceptedBlockHeader;
    boost::signals2::signal<void (const CBlockIndex *, bool)>NotifyHeaderTip;
    boost::signals2::signal<void (const CTransactionRef& tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock)>NotifyTransactionLock;
    boost::signals2::signal<void (const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)>NotifyChainLock;
    boost::signals2::signal<void (const std::shared_ptr<const CGovernanceVote>& vote)>NotifyGovernanceVote;
    boost::signals2::signal<void (const std::shared_ptr<const CGovernanceObject>& object)>NotifyGovernanceObject;
    boost::signals2::signal<void (const CTransactionRef& currentTx, const CTransactionRef& previousTx)>NotifyInstantSendDoubleSpendAttempt;
    boost::signals2::signal<void (bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)>NotifyMasternodeListChanged;
    boost::signals2::signal<void (const std::shared_ptr<const llmq::CRecoveredSig>& sig)>NotifyRecoveredSig;
    // We are not allowed to assume the scheduler only runs in one thread,
    // but must ensure all callbacks happen in-order, so we end up creating
    // our own queue here :(
    SingleThreadedSchedulerClient m_schedulerClient;

    explicit MainSignalsInstance(CScheduler *pscheduler) : m_schedulerClient(pscheduler) {}
};

static CMainSignals g_signals;

void CMainSignals::RegisterBackgroundSignalScheduler(CScheduler& scheduler) {
    assert(!m_internals);
    m_internals.reset(new MainSignalsInstance(&scheduler));
}

void CMainSignals::UnregisterBackgroundSignalScheduler() {
    m_internals.reset(nullptr);
}

void CMainSignals::FlushBackgroundCallbacks() {
    if (m_internals) {
        m_internals->m_schedulerClient.EmptyQueue();
    }
}

size_t CMainSignals::CallbacksPending() {
    if (!m_internals) return 0;
    return m_internals->m_schedulerClient.CallbacksPending();
}

void CMainSignals::RegisterWithMempoolSignals(CTxMemPool& pool) {
    pool.NotifyEntryRemoved.connect(boost::bind(&CMainSignals::MempoolEntryRemoved, this, _1, _2));
}

void CMainSignals::UnregisterWithMempoolSignals(CTxMemPool& pool) {
    pool.NotifyEntryRemoved.disconnect(boost::bind(&CMainSignals::MempoolEntryRemoved, this, _1, _2));
}

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.m_internals->AcceptedBlockHeader.connect(boost::bind(&CValidationInterface::AcceptedBlockHeader, pwalletIn, _1));
    g_signals.m_internals->NotifyHeaderTip.connect(boost::bind(&CValidationInterface::NotifyHeaderTip, pwalletIn, _1, _2));
    g_signals.m_internals->UpdatedBlockTip.connect(boost::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.m_internals->SynchronousUpdatedBlockTip.connect(boost::bind(&CValidationInterface::SynchronousUpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.m_internals->TransactionAddedToMempool.connect(boost::bind(&CValidationInterface::TransactionAddedToMempool, pwalletIn, _1, _2));
    g_signals.m_internals->BlockConnected.connect(boost::bind(&CValidationInterface::BlockConnected, pwalletIn, _1, _2, _3));
    g_signals.m_internals->BlockDisconnected.connect(boost::bind(&CValidationInterface::BlockDisconnected, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyTransactionLock.connect(boost::bind(&CValidationInterface::NotifyTransactionLock, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyChainLock.connect(boost::bind(&CValidationInterface::NotifyChainLock, pwalletIn, _1, _2));
    g_signals.m_internals->TransactionRemovedFromMempool.connect(boost::bind(&CValidationInterface::TransactionRemovedFromMempool, pwalletIn, _1));
    g_signals.m_internals->SetBestChain.connect(boost::bind(&CValidationInterface::SetBestChain, pwalletIn, _1));
    g_signals.m_internals->Broadcast.connect(boost::bind(&CValidationInterface::ResendWalletTransactions, pwalletIn, _1, _2));
    g_signals.m_internals->BlockChecked.connect(boost::bind(&CValidationInterface::BlockChecked, pwalletIn, _1, _2));
    g_signals.m_internals->NewPoWValidBlock.connect(boost::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyGovernanceObject.connect(boost::bind(&CValidationInterface::NotifyGovernanceObject, pwalletIn, _1));
    g_signals.m_internals->NotifyGovernanceVote.connect(boost::bind(&CValidationInterface::NotifyGovernanceVote, pwalletIn, _1));
    g_signals.m_internals->NotifyInstantSendDoubleSpendAttempt.connect(boost::bind(&CValidationInterface::NotifyInstantSendDoubleSpendAttempt, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyRecoveredSig.connect(boost::bind(&CValidationInterface::NotifyRecoveredSig, pwalletIn, _1));
    g_signals.m_internals->NotifyMasternodeListChanged.connect(boost::bind(&CValidationInterface::NotifyMasternodeListChanged, pwalletIn, _1, _2, _3));
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.m_internals->BlockChecked.disconnect(boost::bind(&CValidationInterface::BlockChecked, pwalletIn, _1, _2));
    g_signals.m_internals->Broadcast.disconnect(boost::bind(&CValidationInterface::ResendWalletTransactions, pwalletIn, _1, _2));
    g_signals.m_internals->SetBestChain.disconnect(boost::bind(&CValidationInterface::SetBestChain, pwalletIn, _1));
    g_signals.m_internals->NotifyChainLock.disconnect(boost::bind(&CValidationInterface::NotifyChainLock, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyTransactionLock.disconnect(boost::bind(&CValidationInterface::NotifyTransactionLock, pwalletIn, _1, _2));
    g_signals.m_internals->TransactionAddedToMempool.disconnect(boost::bind(&CValidationInterface::TransactionAddedToMempool, pwalletIn, _1, _2));
    g_signals.m_internals->BlockConnected.disconnect(boost::bind(&CValidationInterface::BlockConnected, pwalletIn, _1, _2, _3));
    g_signals.m_internals->BlockDisconnected.disconnect(boost::bind(&CValidationInterface::BlockDisconnected, pwalletIn, _1, _2));
    g_signals.m_internals->TransactionRemovedFromMempool.disconnect(boost::bind(&CValidationInterface::TransactionRemovedFromMempool, pwalletIn, _1));
    g_signals.m_internals->UpdatedBlockTip.disconnect(boost::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.m_internals->SynchronousUpdatedBlockTip.disconnect(boost::bind(&CValidationInterface::SynchronousUpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.m_internals->NewPoWValidBlock.disconnect(boost::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyHeaderTip.disconnect(boost::bind(&CValidationInterface::NotifyHeaderTip, pwalletIn, _1, _2));
    g_signals.m_internals->AcceptedBlockHeader.disconnect(boost::bind(&CValidationInterface::AcceptedBlockHeader, pwalletIn, _1));
    g_signals.m_internals->NotifyGovernanceObject.disconnect(boost::bind(&CValidationInterface::NotifyGovernanceObject, pwalletIn, _1));
    g_signals.m_internals->NotifyGovernanceVote.disconnect(boost::bind(&CValidationInterface::NotifyGovernanceVote, pwalletIn, _1));
    g_signals.m_internals->NotifyInstantSendDoubleSpendAttempt.disconnect(boost::bind(&CValidationInterface::NotifyInstantSendDoubleSpendAttempt, pwalletIn, _1, _2));
    g_signals.m_internals->NotifyRecoveredSig.disconnect(boost::bind(&CValidationInterface::NotifyRecoveredSig, pwalletIn, _1));
    g_signals.m_internals->NotifyMasternodeListChanged.disconnect(boost::bind(&CValidationInterface::NotifyMasternodeListChanged, pwalletIn, _1, _2, _3));
}

void UnregisterAllValidationInterfaces() {
    if (!g_signals.m_internals) {
        return;
    }
    g_signals.m_internals->BlockChecked.disconnect_all_slots();
    g_signals.m_internals->Broadcast.disconnect_all_slots();
    g_signals.m_internals->SetBestChain.disconnect_all_slots();
    g_signals.m_internals->NotifyTransactionLock.disconnect_all_slots();
    g_signals.m_internals->NotifyChainLock.disconnect_all_slots();
    g_signals.m_internals->TransactionAddedToMempool.disconnect_all_slots();
    g_signals.m_internals->BlockConnected.disconnect_all_slots();
    g_signals.m_internals->BlockDisconnected.disconnect_all_slots();
    g_signals.m_internals->TransactionRemovedFromMempool.disconnect_all_slots();
    g_signals.m_internals->UpdatedBlockTip.disconnect_all_slots();
    g_signals.m_internals->SynchronousUpdatedBlockTip.disconnect_all_slots();
    g_signals.m_internals->NewPoWValidBlock.disconnect_all_slots();
    g_signals.m_internals->NotifyHeaderTip.disconnect_all_slots();
    g_signals.m_internals->AcceptedBlockHeader.disconnect_all_slots();
    g_signals.m_internals->NotifyGovernanceObject.disconnect_all_slots();
    g_signals.m_internals->NotifyGovernanceVote.disconnect_all_slots();
    g_signals.m_internals->NotifyInstantSendDoubleSpendAttempt.disconnect_all_slots();
    g_signals.m_internals->NotifyRecoveredSig.disconnect_all_slots();
    g_signals.m_internals->NotifyMasternodeListChanged.disconnect_all_slots();
}

void CallFunctionInValidationInterfaceQueue(std::function<void ()> func) {
    g_signals.m_internals->m_schedulerClient.AddToProcessQueue(std::move(func));
}

void SyncWithValidationInterfaceQueue() {
    AssertLockNotHeld(cs_main);
    // Block until the validation queue drains
    std::promise<void> promise;
    CallFunctionInValidationInterfaceQueue([&promise] {
        promise.set_value();
    });
    promise.get_future().wait();
}

void CMainSignals::MempoolEntryRemoved(CTransactionRef ptx, MemPoolRemovalReason reason) {
    if (reason != MemPoolRemovalReason::BLOCK && reason != MemPoolRemovalReason::CONFLICT) {
        m_internals->m_schedulerClient.AddToProcessQueue([ptx, this] {
            m_internals->TransactionRemovedFromMempool(ptx);
        });
    }
}

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    // Dependencies exist that require UpdatedBlockTip events to be delivered in the order in which
    // the chain actually updates. One way to ensure this is for the caller to invoke this signal
    // in the same critical section where the chain is updated

    m_internals->m_schedulerClient.AddToProcessQueue([pindexNew, pindexFork, fInitialDownload, this] {
        m_internals->UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    });
}

void CMainSignals::SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    m_internals->SynchronousUpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
}

void CMainSignals::TransactionAddedToMempool(const CTransactionRef &ptx, int64_t nAcceptTime) {
    m_internals->m_schedulerClient.AddToProcessQueue([ptx, nAcceptTime, this] {
        m_internals->TransactionAddedToMempool(ptx, nAcceptTime);
    });
}

void CMainSignals::BlockConnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex, const std::shared_ptr<const std::vector<CTransactionRef>>& pvtxConflicted) {
    m_internals->m_schedulerClient.AddToProcessQueue([pblock, pindex, pvtxConflicted, this] {
        m_internals->BlockConnected(pblock, pindex, *pvtxConflicted);
    });
}

void CMainSignals::BlockDisconnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex* pindexDisconnected) {
    m_internals->m_schedulerClient.AddToProcessQueue([pblock, pindexDisconnected, this] {
        m_internals->BlockDisconnected(pblock, pindexDisconnected);
    });
}

void CMainSignals::SetBestChain(const CBlockLocator &locator) {
    m_internals->m_schedulerClient.AddToProcessQueue([locator, this] {
        m_internals->SetBestChain(locator);
    });
}

void CMainSignals::Broadcast(int64_t nBestBlockTime, CConnman* connman) {
    m_internals->Broadcast(nBestBlockTime, connman);
}

void CMainSignals::BlockChecked(const CBlock& block, const CValidationState& state) {
    m_internals->BlockChecked(block, state);
}

void CMainSignals::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) {
    m_internals->NewPoWValidBlock(pindex, block);
}

void CMainSignals::AcceptedBlockHeader(const CBlockIndex *pindexNew) {
    m_internals->AcceptedBlockHeader(pindexNew);
}

void CMainSignals::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload) {
    m_internals->NotifyHeaderTip(pindexNew, fInitialDownload);
}

void CMainSignals::NotifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock) {
    m_internals->m_schedulerClient.AddToProcessQueue([tx, islock, this] {
        m_internals->NotifyTransactionLock(tx, islock);
    });
}

void CMainSignals::NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig) {
    m_internals->m_schedulerClient.AddToProcessQueue([pindex, clsig, this] {
        m_internals->NotifyChainLock(pindex, clsig);
    });
}

void CMainSignals::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote) {
    m_internals->m_schedulerClient.AddToProcessQueue([vote, this] {
        m_internals->NotifyGovernanceVote(vote);
    });
}

void CMainSignals::NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& object) {
    m_internals->m_schedulerClient.AddToProcessQueue([object, this] {
        m_internals->NotifyGovernanceObject(object);
    });
}

void CMainSignals::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx) {
    m_internals->m_schedulerClient.AddToProcessQueue([currentTx, previousTx, this] {
        m_internals->NotifyInstantSendDoubleSpendAttempt(currentTx, previousTx);
    });
}

void CMainSignals::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig) {
    m_internals->m_schedulerClient.AddToProcessQueue([sig, this] {
        m_internals->NotifyRecoveredSig(sig);
    });
}

void CMainSignals::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff) {
    m_internals->NotifyMasternodeListChanged(undo, oldMNList, diff);
}