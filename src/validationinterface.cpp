// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
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
    boost::signals2::signal<void (const MempoolInterface::NewMempoolTransactionInfo &, const std::vector<CTransactionRef> &)> TransactionAddedToMempool;
    boost::signals2::signal<void (const std::vector<CTransactionRef> &, const std::vector<CTransactionRef> &, int)> MempoolUpdatedForBlockConnect;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex)> BlockConnected;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &)> BlockDisconnected;
    boost::signals2::signal<void (const CTransactionRef &, MemPoolRemovalReason)> TransactionRemovedFromMempool;
    boost::signals2::signal<void (const CBlockLocator &)> ChainStateFlushed;
    boost::signals2::signal<void (const uint256 &)> Inventory;
    boost::signals2::signal<void (int64_t nBestBlockTime, CConnman* connman)> Broadcast;
    boost::signals2::signal<void (const CBlock&, const CValidationState&)> BlockChecked;
    boost::signals2::signal<void (const CBlockIndex *, const std::shared_ptr<const CBlock>&)> NewPoWValidBlock;

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

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.m_internals->UpdatedBlockTip.connect(boost::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.m_internals->BlockConnected.connect(boost::bind(&CValidationInterface::BlockConnected, pwalletIn, _1, _2));
    g_signals.m_internals->BlockDisconnected.connect(boost::bind(&CValidationInterface::BlockDisconnected, pwalletIn, _1));
    g_signals.m_internals->ChainStateFlushed.connect(boost::bind(&CValidationInterface::ChainStateFlushed, pwalletIn, _1));
    g_signals.m_internals->Inventory.connect(boost::bind(&CValidationInterface::Inventory, pwalletIn, _1));
    g_signals.m_internals->Broadcast.connect(boost::bind(&CValidationInterface::ResendWalletTransactions, pwalletIn, _1, _2));
    g_signals.m_internals->BlockChecked.connect(boost::bind(&CValidationInterface::BlockChecked, pwalletIn, _1, _2));
    g_signals.m_internals->NewPoWValidBlock.connect(boost::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, _1, _2));
}

void RegisterMempoolInterface(MempoolInterface* listener) {
    g_signals.m_internals->TransactionAddedToMempool.connect(boost::bind(&MempoolInterface::TransactionAddedToMempool, listener, _1, _2));
    g_signals.m_internals->TransactionRemovedFromMempool.connect(boost::bind(&MempoolInterface::TransactionRemovedFromMempool, listener, _1, _2));
    g_signals.m_internals->MempoolUpdatedForBlockConnect.connect(boost::bind(&MempoolInterface::MempoolUpdatedForBlockConnect, listener, _1, _2, _3));
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.m_internals->BlockChecked.disconnect(boost::bind(&CValidationInterface::BlockChecked, pwalletIn, _1, _2));
    g_signals.m_internals->Broadcast.disconnect(boost::bind(&CValidationInterface::ResendWalletTransactions, pwalletIn, _1, _2));
    g_signals.m_internals->Inventory.disconnect(boost::bind(&CValidationInterface::Inventory, pwalletIn, _1));
    g_signals.m_internals->ChainStateFlushed.disconnect(boost::bind(&CValidationInterface::ChainStateFlushed, pwalletIn, _1));
    g_signals.m_internals->BlockConnected.disconnect(boost::bind(&CValidationInterface::BlockConnected, pwalletIn, _1, _2));
    g_signals.m_internals->BlockDisconnected.disconnect(boost::bind(&CValidationInterface::BlockDisconnected, pwalletIn, _1));
    g_signals.m_internals->UpdatedBlockTip.disconnect(boost::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.m_internals->NewPoWValidBlock.disconnect(boost::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, _1, _2));
}

void UnregisterMempoolInterface(MempoolInterface* listener) {
    g_signals.m_internals->TransactionAddedToMempool.disconnect(boost::bind(&MempoolInterface::TransactionAddedToMempool, listener, _1, _2));
    g_signals.m_internals->TransactionRemovedFromMempool.disconnect(boost::bind(&MempoolInterface::TransactionRemovedFromMempool, listener, _1, _2));
    g_signals.m_internals->MempoolUpdatedForBlockConnect.disconnect(boost::bind(&MempoolInterface::MempoolUpdatedForBlockConnect, listener, _1, _2, _3));
}

void UnregisterAllValidationAndMempoolInterfaces() {
    if (!g_signals.m_internals) {
        return;
    }
    g_signals.m_internals->BlockChecked.disconnect_all_slots();
    g_signals.m_internals->Broadcast.disconnect_all_slots();
    g_signals.m_internals->Inventory.disconnect_all_slots();
    g_signals.m_internals->ChainStateFlushed.disconnect_all_slots();
    g_signals.m_internals->TransactionAddedToMempool.disconnect_all_slots();
    g_signals.m_internals->BlockConnected.disconnect_all_slots();
    g_signals.m_internals->BlockDisconnected.disconnect_all_slots();
    g_signals.m_internals->TransactionRemovedFromMempool.disconnect_all_slots();
    g_signals.m_internals->MempoolUpdatedForBlockConnect.disconnect_all_slots();
    g_signals.m_internals->UpdatedBlockTip.disconnect_all_slots();
    g_signals.m_internals->NewPoWValidBlock.disconnect_all_slots();
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
    m_internals->m_schedulerClient.AddToProcessQueue([ptx, reason, this] {
        m_internals->TransactionRemovedFromMempool(ptx, reason);
    });
}

void CMainSignals::MempoolUpdatedForBlockConnect(std::vector<CTransactionRef>&& tx_removed_in_block, std::vector<CTransactionRef>&& tx_removed_conflicted, int block_height) {
    auto tx_removed_in_block_ptr = std::make_shared<std::vector<CTransactionRef>>(std::move(tx_removed_in_block));
    auto tx_removed_conflicted_ptr = std::make_shared<std::vector<CTransactionRef>>(std::move(tx_removed_conflicted));
    m_internals->m_schedulerClient.AddToProcessQueue([tx_removed_in_block_ptr, tx_removed_conflicted_ptr, block_height, this] {
        m_internals->MempoolUpdatedForBlockConnect(*tx_removed_in_block_ptr, *tx_removed_conflicted_ptr, block_height);
    });
}

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    // Dependencies exist that require UpdatedBlockTip events to be delivered in the order in which
    // the chain actually updates. One way to ensure this is for the caller to invoke this signal
    // in the same critical section where the chain is updated

    m_internals->m_schedulerClient.AddToProcessQueue([pindexNew, pindexFork, fInitialDownload, this] {
        m_internals->UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    });
}

void CMainSignals::TransactionAddedToMempool(const MempoolInterface::NewMempoolTransactionInfo &info, const std::shared_ptr<std::vector<CTransactionRef>>& txn_replaced) {
    m_internals->m_schedulerClient.AddToProcessQueue([info, txn_replaced, this] {
        m_internals->TransactionAddedToMempool(info, *txn_replaced);
    });
}

void CMainSignals::BlockConnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex) {
    //TODO: To fix unbounded memory usage during reorg we really need to only
    // have a limited set of blocks in memory, be willing to throw them away
    // here, and read them back off disk when we find time to call the signal
    // here.
    m_internals->m_schedulerClient.AddToProcessQueue([pblock, pindex, this] {
        m_internals->BlockConnected(pblock, pindex);
    });
}

void CMainSignals::BlockDisconnected(const std::shared_ptr<const CBlock> &pblock) {
    //TODO: To fix unbounded memory usage during reorg we really need to only
    // have a limited set of blocks in memory, be willing to throw them away
    // here, and read them back off disk when we find time to call the signal
    // here.
    m_internals->m_schedulerClient.AddToProcessQueue([pblock, this] {
        m_internals->BlockDisconnected(pblock);
    });
}

void CMainSignals::ChainStateFlushed(const CBlockLocator &locator) {
    m_internals->m_schedulerClient.AddToProcessQueue([locator, this] {
        m_internals->ChainStateFlushed(locator);
    });
}

void CMainSignals::Inventory(const uint256 &hash) {
    m_internals->m_schedulerClient.AddToProcessQueue([hash, this] {
        m_internals->Inventory(hash);
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
