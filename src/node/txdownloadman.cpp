// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txdownloadman.h>
#include <node/txdownload_impl.h>

namespace node {

TxDownloadManager::TxDownloadManager(const TxDownloadOptions& options) :
    m_impl{std::make_unique<TxDownloadImpl>(options)}
{}
TxDownloadManager::~TxDownloadManager() = default;

void TxDownloadManager::UpdatedBlockTipSync()
{
    m_impl->UpdatedBlockTipSync();
}
void TxDownloadManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock)
{
    m_impl->BlockConnected(pblock);
}
void TxDownloadManager::BlockDisconnected()
{
    m_impl->BlockDisconnected();
}

void TxDownloadManager::ConnectedPeer(NodeId nodeid, const TxDownloadConnectionInfo& info)
{
    m_impl->ConnectedPeer(nodeid, info);
}
void TxDownloadManager::DisconnectedPeer(NodeId nodeid)
{
    m_impl->DisconnectedPeer(nodeid);
}
bool TxDownloadManager::AddTxAnnouncement(NodeId peer, const GenTxid& gtxid, std::chrono::microseconds now, bool p2p_inv)
{
    return m_impl->AddTxAnnouncement(peer, gtxid, now, p2p_inv);
}
std::vector<GenTxid> TxDownloadManager::GetRequestsToSend(NodeId nodeid, std::chrono::microseconds current_time)
{
    return m_impl->GetRequestsToSend(nodeid, current_time);
}

void TxDownloadManager::ReceivedNotFound(NodeId nodeid, const std::vector<uint256>& txhashes)
{
    m_impl->ReceivedNotFound(nodeid, txhashes);
}
void TxDownloadManager::MempoolAcceptedTx(const CTransactionRef& tx)
{
    m_impl->MempoolAcceptedTx(tx);
}
RejectedTxTodo TxDownloadManager::MempoolRejectedTx(const CTransactionRef& ptx, const TxValidationState& state, NodeId nodeid, bool maybe_add_new_orphan)
{
        return m_impl->MempoolRejectedTx(ptx, state, nodeid, maybe_add_new_orphan);
}
void TxDownloadManager::MempoolRejectedPackage(const Package& package)
{
    m_impl->MempoolRejectedPackage(package);
}
std::pair<bool, std::optional<PackageToValidate>> TxDownloadManager::ReceivedTx(NodeId nodeid, const CTransactionRef& ptx)
{
    return m_impl->ReceivedTx(nodeid, ptx);
}
bool TxDownloadManager::HaveMoreWork(NodeId nodeid) const
{
    return m_impl->HaveMoreWork(nodeid);
}
CTransactionRef TxDownloadManager::GetTxToReconsider(NodeId nodeid)
{
    return m_impl->GetTxToReconsider(nodeid);
}
void TxDownloadManager::CheckIsEmpty() const
{
    return m_impl->CheckIsEmpty();
}
void TxDownloadManager::CheckIsEmpty(NodeId nodeid) const
{
    return m_impl->CheckIsEmpty(nodeid);
}
} // namespace node
