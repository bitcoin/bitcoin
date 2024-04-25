// Copyright (c) 2024
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txdownloadman_impl.h>
#include <node/txdownloadman.h>

#include <chain.h>
#include <consensus/validation.h>
#include <logging.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>

namespace node {
// TxDownloadManager wrappers
TxDownloadManager::TxDownloadManager(const TxDownloadOptions& options) :
    m_impl{std::make_unique<TxDownloadManagerImpl>(options)}
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
bool TxDownloadManager::AlreadyHaveTx(const GenTxid& gtxid, bool include_reconsiderable)
{
    return m_impl->AlreadyHaveTx(gtxid, include_reconsiderable);
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

bool TxDownloadManagerImpl::AlreadyHaveTx(const GenTxid& gtxid, bool include_reconsiderable)
{
    const uint256& hash = gtxid.GetHash();

    if (gtxid.IsWtxid()) {
        // Normal query by wtxid.
        if (m_orphanage.HaveTx(Wtxid::FromUint256(hash))) return true;
    } else {
        // Never query by txid: it is possible that the transaction in the orphanage has the same
        // txid but a different witness, which would give us a false positive result. If we decided
        // not to request the transaction based on this result, an attacker could prevent us from
        // downloading a transaction by intentionally creating a malleated version of it.  While
        // only one (or none!) of these transactions can ultimately be confirmed, we have no way of
        // discerning which one that is, so the orphanage can store multiple transactions with the
        // same txid.
        //
        // While we won't query by txid, we can try to "guess" what the wtxid is based on the txid.
        // A non-segwit transaction's txid == wtxid. Query this txid "casted" to a wtxid. This will
        // help us find non-segwit transactions, saving bandwidth, and should have no false positives.
        if (m_orphanage.HaveTx(Wtxid::FromUint256(hash))) return true;
    }

    if (include_reconsiderable && RecentRejectsReconsiderableFilter().contains(hash)) return true;

    if (RecentConfirmedTransactionsFilter().contains(hash)) return true;

    return RecentRejectsFilter().contains(hash) || m_opts.m_mempool.exists(gtxid);
}

void TxDownloadManagerImpl::ConnectedPeer(NodeId nodeid, const TxDownloadConnectionInfo& info)
{
    // If already connected (shouldn't happen in practice), exit early.
    if (m_peer_info.contains(nodeid)) return;

    m_peer_info.try_emplace(nodeid, info);
    if (info.m_wtxid_relay) m_num_wtxid_peers += 1;
}

void TxDownloadManagerImpl::DisconnectedPeer(NodeId nodeid)
{
    m_orphanage.EraseForPeer(nodeid);
    m_txrequest.DisconnectedPeer(nodeid);

    if (auto it = m_peer_info.find(nodeid); it != m_peer_info.end()) {
        if (it->second.m_connection_info.m_wtxid_relay) m_num_wtxid_peers -= 1;
        m_peer_info.erase(it);
    }

}

bool TxDownloadManagerImpl::AddTxAnnouncement(NodeId peer, const GenTxid& gtxid, std::chrono::microseconds now, bool p2p_inv)
{
    // If this is an inv received from a peer and we already have it, we can drop it.
    if (p2p_inv && AlreadyHaveTx(gtxid, /*include_reconsiderable=*/true)) return true;

    auto it = m_peer_info.find(peer);
    if (it == m_peer_info.end()) return false;
    const auto& info = it->second.m_connection_info;
    if (!info.m_relay_permissions && m_txrequest.Count(peer) >= MAX_PEER_TX_ANNOUNCEMENTS) {
        // Too many queued announcements for this peer
        return false;
    }
    // Decide the TxRequestTracker parameters for this announcement:
    // - "preferred": if fPreferredDownload is set (= outbound, or NetPermissionFlags::NoBan permission)
    // - "reqtime": current time plus delays for:
    //   - NONPREF_PEER_TX_DELAY for announcements from non-preferred connections
    //   - TXID_RELAY_DELAY for txid announcements while wtxid peers are available
    //   - OVERLOADED_PEER_TX_DELAY for announcements from peers which have at least
    //     MAX_PEER_TX_REQUEST_IN_FLIGHT requests in flight (and don't have NetPermissionFlags::Relay).
    auto delay{0us};
    if (!info.m_preferred) delay += NONPREF_PEER_TX_DELAY;
    if (!gtxid.IsWtxid() && m_num_wtxid_peers > 0) delay += TXID_RELAY_DELAY;
    const bool overloaded = !info.m_relay_permissions && m_txrequest.CountInFlight(peer) >= MAX_PEER_TX_REQUEST_IN_FLIGHT;
    if (overloaded) delay += OVERLOADED_PEER_TX_DELAY;

    m_txrequest.ReceivedInv(peer, gtxid, info.m_preferred, now + delay);

    return false;
}

std::vector<GenTxid> TxDownloadManagerImpl::GetRequestsToSend(NodeId nodeid, std::chrono::microseconds current_time)
{
    std::vector<GenTxid> requests;
    std::vector<std::pair<NodeId, GenTxid>> expired;
    auto requestable = m_txrequest.GetRequestable(nodeid, current_time, &expired);
    for (const auto& entry : expired) {
        LogDebug(BCLog::NET, "timeout of inflight %s %s from peer=%d\n", entry.second.IsWtxid() ? "wtx" : "tx",
            entry.second.GetHash().ToString(), entry.first);
    }
    for (const GenTxid& gtxid : requestable) {
        if (!AlreadyHaveTx(gtxid, /*include_reconsiderable=*/false)) {
            LogDebug(BCLog::NET, "Requesting %s %s peer=%d\n", gtxid.IsWtxid() ? "wtx" : "tx",
                gtxid.GetHash().ToString(), nodeid);
            requests.emplace_back(gtxid);
            m_txrequest.RequestedTx(nodeid, gtxid.GetHash(), current_time + GETDATA_TX_INTERVAL);
        } else {
            // We have already seen this transaction, no need to download. This is just a belt-and-suspenders, as
            // this should already be called whenever a transaction becomes AlreadyHaveTx().
            m_txrequest.ForgetTxHash(gtxid.GetHash());
        }
    }
    return requests;
}
} // namespace node
