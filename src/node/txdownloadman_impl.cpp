// Copyright (c) 2024
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txdownloadman_impl.h>
#include <node/txdownloadman.h>

#include <chain.h>
#include <consensus/validation.h>
#include <validation.h>
#include <validationinterface.h>

namespace node {
// TxDownloadManager wrappers
TxDownloadManager::TxDownloadManager() :
    m_impl{std::make_unique<TxDownloadManagerImpl>()}
{}
TxDownloadManager::~TxDownloadManager() = default;

TxOrphanage& TxDownloadManager::GetOrphanageRef()
{
    return m_impl->m_orphanage;
}
TxRequestTracker& TxDownloadManager::GetTxRequestRef()
{
    return m_impl->m_txrequest;
}
CRollingBloomFilter& TxDownloadManager::RecentRejectsFilter()
{
    return m_impl->RecentRejectsFilter();
}
CRollingBloomFilter& TxDownloadManager::RecentRejectsReconsiderableFilter()
{
    return m_impl->RecentRejectsReconsiderableFilter();
}
CRollingBloomFilter& TxDownloadManager::RecentConfirmedTransactionsFilter()
{
    return m_impl->RecentConfirmedTransactionsFilter();
}
void TxDownloadManager::ActiveTipChange()
{
    m_impl->ActiveTipChange();
}
void TxDownloadManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock)
{
    m_impl->BlockConnected(pblock);
}
void TxDownloadManager::BlockDisconnected()
{
    m_impl->BlockDisconnected();
}

// TxDownloadManagerImpl
void TxDownloadManagerImpl::ActiveTipChange()
{
    RecentRejectsFilter().reset();
    RecentRejectsReconsiderableFilter().reset();
}

void TxDownloadManagerImpl::BlockConnected(const std::shared_ptr<const CBlock>& pblock)
{
    m_orphanage.EraseForBlock(*pblock);

    for (const auto& ptx : pblock->vtx) {
        RecentConfirmedTransactionsFilter().insert(ptx->GetHash().ToUint256());
        if (ptx->HasWitness()) {
            RecentConfirmedTransactionsFilter().insert(ptx->GetWitnessHash().ToUint256());
        }
        m_txrequest.ForgetTxHash(ptx->GetHash());
        m_txrequest.ForgetTxHash(ptx->GetWitnessHash());
    }
}

void TxDownloadManagerImpl::BlockDisconnected()
{
    // To avoid relay problems with transactions that were previously
    // confirmed, clear our filter of recently confirmed transactions whenever
    // there's a reorg.
    // This means that in a 1-block reorg (where 1 block is disconnected and
    // then another block reconnected), our filter will drop to having only one
    // block's worth of transactions in it, but that should be fine, since
    // presumably the most common case of relaying a confirmed transaction
    // should be just after a new block containing it is found.
    RecentConfirmedTransactionsFilter().reset();
}
} // namespace node
