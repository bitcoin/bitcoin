// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validationinterface.h>

#include <primitives/block.h>
#include <scheduler.h>
#include <txmempool.h>

#include <future>
#include <utility>

#include <boost/signals2/signal.hpp>

struct ValidationInterfaceConnections {
    boost::signals2::scoped_connection UpdatedBlockTip;
    boost::signals2::scoped_connection SynchronousUpdatedBlockTip;
    boost::signals2::scoped_connection TransactionAddedToMempool;
    boost::signals2::scoped_connection BlockConnected;
    boost::signals2::scoped_connection BlockDisconnected;
    boost::signals2::scoped_connection TransactionRemovedFromMempool;
    boost::signals2::scoped_connection ChainStateFlushed;
    boost::signals2::scoped_connection BlockChecked;
    boost::signals2::scoped_connection NewPoWValidBlock;
    boost::signals2::scoped_connection AcceptedBlockHeader;
    boost::signals2::scoped_connection NotifyHeaderTip;
    boost::signals2::scoped_connection NotifyTransactionLock;
    boost::signals2::scoped_connection NotifyChainLock;
    boost::signals2::scoped_connection NotifyGovernanceVote;
    boost::signals2::scoped_connection NotifyGovernanceObject;
    boost::signals2::scoped_connection NotifyInstantSendDoubleSpendAttempt;
    boost::signals2::scoped_connection NotifyMasternodeListChanged;
    boost::signals2::scoped_connection NotifyRecoveredSig;

};

struct MainSignalsInstance {
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> UpdatedBlockTip;
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> SynchronousUpdatedBlockTip;
    boost::signals2::signal<void (const CTransactionRef &, int64_t)> TransactionAddedToMempool;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex, const std::vector<CTransactionRef>&)> BlockConnected;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex* pindexDisconnected)> BlockDisconnected;
    boost::signals2::signal<void (const CTransactionRef &, MemPoolRemovalReason)> TransactionRemovedFromMempool;
    boost::signals2::signal<void (const CBlockLocator &)> ChainStateFlushed;
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
    std::unordered_map<CValidationInterface*, ValidationInterfaceConnections> m_connMainSignals;

    explicit MainSignalsInstance(CScheduler *pscheduler) : m_schedulerClient(pscheduler) {}
};

static CMainSignals g_signals;

// This map has to a separate global instead of a member of MainSignalsInstance,
// because RegisterWithMempoolSignals is currently called before RegisterBackgroundSignalScheduler,
// so MainSignalsInstance hasn't been created yet.
static std::unordered_map<CTxMemPool*, boost::signals2::scoped_connection> g_connNotifyEntryRemoved;

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
    g_connNotifyEntryRemoved.emplace(std::piecewise_construct,
        std::forward_as_tuple(&pool),
        std::forward_as_tuple(pool.NotifyEntryRemoved.connect(std::bind(&CMainSignals::MempoolEntryRemoved, this, std::placeholders::_1, std::placeholders::_2)))
    );
}

void CMainSignals::UnregisterWithMempoolSignals(CTxMemPool& pool) {
    g_connNotifyEntryRemoved.erase(&pool);
}

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterValidationInterface(CValidationInterface* pwalletIn) {
    ValidationInterfaceConnections& conns = g_signals.m_internals->m_connMainSignals[pwalletIn];
    conns.AcceptedBlockHeader = g_signals.m_internals->AcceptedBlockHeader.connect(std::bind(&CValidationInterface::AcceptedBlockHeader, pwalletIn, std::placeholders::_1));
    conns.NotifyHeaderTip = g_signals.m_internals->NotifyHeaderTip.connect(std::bind(&CValidationInterface::NotifyHeaderTip, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.UpdatedBlockTip = g_signals.m_internals->UpdatedBlockTip.connect(std::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    conns.SynchronousUpdatedBlockTip = g_signals.m_internals->SynchronousUpdatedBlockTip.connect(std::bind(&CValidationInterface::SynchronousUpdatedBlockTip, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    conns.TransactionAddedToMempool = g_signals.m_internals->TransactionAddedToMempool.connect(std::bind(&CValidationInterface::TransactionAddedToMempool, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.BlockConnected = g_signals.m_internals->BlockConnected.connect(std::bind(&CValidationInterface::BlockConnected, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    conns.BlockDisconnected = g_signals.m_internals->BlockDisconnected.connect(std::bind(&CValidationInterface::BlockDisconnected, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyTransactionLock = g_signals.m_internals->NotifyTransactionLock.connect(std::bind(&CValidationInterface::NotifyTransactionLock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyChainLock = g_signals.m_internals->NotifyChainLock.connect(std::bind(&CValidationInterface::NotifyChainLock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.TransactionRemovedFromMempool = g_signals.m_internals->TransactionRemovedFromMempool.connect(std::bind(&CValidationInterface::TransactionRemovedFromMempool, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.ChainStateFlushed = g_signals.m_internals->ChainStateFlushed.connect(std::bind(&CValidationInterface::ChainStateFlushed, pwalletIn, std::placeholders::_1));
    conns.BlockChecked = g_signals.m_internals->BlockChecked.connect(std::bind(&CValidationInterface::BlockChecked, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NewPoWValidBlock = g_signals.m_internals->NewPoWValidBlock.connect(std::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyGovernanceObject = g_signals.m_internals->NotifyGovernanceObject.connect(std::bind(&CValidationInterface::NotifyGovernanceObject, pwalletIn, std::placeholders::_1));
    conns.NotifyGovernanceVote = g_signals.m_internals->NotifyGovernanceVote.connect(std::bind(&CValidationInterface::NotifyGovernanceVote, pwalletIn, std::placeholders::_1));
    conns.NotifyInstantSendDoubleSpendAttempt = g_signals.m_internals->NotifyInstantSendDoubleSpendAttempt.connect(std::bind(&CValidationInterface::NotifyInstantSendDoubleSpendAttempt, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyRecoveredSig = g_signals.m_internals->NotifyRecoveredSig.connect(std::bind(&CValidationInterface::NotifyRecoveredSig, pwalletIn, std::placeholders::_1));
    conns.NotifyMasternodeListChanged = g_signals.m_internals->NotifyMasternodeListChanged.connect(std::bind(&CValidationInterface::NotifyMasternodeListChanged, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    if (g_signals.m_internals) {
        g_signals.m_internals->m_connMainSignals.erase(pwalletIn);
    }
}

void UnregisterAllValidationInterfaces() {
    if (!g_signals.m_internals) {
        return;
    }
    g_signals.m_internals->m_connMainSignals.clear();
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
    if (reason != MemPoolRemovalReason::BLOCK) {
        m_internals->m_schedulerClient.AddToProcessQueue([ptx, reason, this] {
            m_internals->TransactionRemovedFromMempool(ptx, reason);
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

void CMainSignals::ChainStateFlushed(const CBlockLocator &locator) {
    m_internals->m_schedulerClient.AddToProcessQueue([locator, this] {
        m_internals->ChainStateFlushed(locator);
    });
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
