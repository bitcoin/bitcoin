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
RejectedTxTodo TxDownloadManager::MempoolRejectedTx(const CTransactionRef& ptx, const TxValidationState& state, NodeId nodeid, bool first_time_failure)
{
    return m_impl->MempoolRejectedTx(ptx, state, nodeid, first_time_failure);
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
    m_impl->CheckIsEmpty();
}
void TxDownloadManager::CheckIsEmpty(NodeId nodeid) const
{
    m_impl->CheckIsEmpty(nodeid);
}
std::vector<TxOrphanage::OrphanTxBase> TxDownloadManager::GetOrphanTransactions() const
{
    return m_impl->GetOrphanTransactions();
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
    // If this is a request for the parent of an orphan, we don't drop transactions that we already have. In particular,
    // we *do* want to request parents that are in m_lazy_recent_rejects_reconsiderable, since they can be CPFP'd.
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

void TxDownloadManagerImpl::ReceivedNotFound(NodeId nodeid, const std::vector<uint256>& txhashes)
{
    for (const auto& txhash : txhashes) {
        // If we receive a NOTFOUND message for a tx we requested, mark the announcement for it as
        // completed in TxRequestTracker.
        m_txrequest.ReceivedResponse(nodeid, txhash);
    }
}

std::optional<PackageToValidate> TxDownloadManagerImpl::Find1P1CPackage(const CTransactionRef& ptx, NodeId nodeid)
{
    const auto& parent_wtxid{ptx->GetWitnessHash()};

    Assume(RecentRejectsReconsiderableFilter().contains(parent_wtxid.ToUint256()));

    // Prefer children from this peer. This helps prevent censorship attempts in which an attacker
    // sends lots of fake children for the parent, and we (unluckily) keep selecting the fake
    // children instead of the real one provided by the honest peer.
    const auto cpfp_candidates_same_peer{m_orphanage.GetChildrenFromSamePeer(ptx, nodeid)};

    // These children should be sorted from newest to oldest. In the (probably uncommon) case
    // of children that replace each other, this helps us accept the highest feerate (probably the
    // most recent) one efficiently.
    for (const auto& child : cpfp_candidates_same_peer) {
        Package maybe_cpfp_package{ptx, child};
        if (!RecentRejectsReconsiderableFilter().contains(GetPackageHash(maybe_cpfp_package)) &&
            !RecentRejectsFilter().contains(child->GetHash().ToUint256())) {
            return PackageToValidate{ptx, child, nodeid, nodeid};
        }
    }

    // If no suitable candidate from the same peer is found, also try children that were provided by
    // a different peer. This is useful because sometimes multiple peers announce both transactions
    // to us, and we happen to download them from different peers (we wouldn't have known that these
    // 2 transactions are related). We still want to find 1p1c packages then.
    //
    // If we start tracking all announcers of orphans, we can restrict this logic to parent + child
    // pairs in which both were provided by the same peer, i.e. delete this step.
    const auto cpfp_candidates_different_peer{m_orphanage.GetChildrenFromDifferentPeer(ptx, nodeid)};

    // Find the first 1p1c that hasn't already been rejected. We randomize the order to not
    // create a bias that attackers can use to delay package acceptance.
    //
    // Create a random permutation of the indices.
    std::vector<size_t> tx_indices(cpfp_candidates_different_peer.size());
    std::iota(tx_indices.begin(), tx_indices.end(), 0);
    std::shuffle(tx_indices.begin(), tx_indices.end(), m_opts.m_rng);

    for (const auto index : tx_indices) {
        // If we already tried a package and failed for any reason, the combined hash was
        // cached in m_lazy_recent_rejects_reconsiderable.
        const auto [child_tx, child_sender] = cpfp_candidates_different_peer.at(index);
        Package maybe_cpfp_package{ptx, child_tx};
        if (!RecentRejectsReconsiderableFilter().contains(GetPackageHash(maybe_cpfp_package)) &&
            !RecentRejectsFilter().contains(child_tx->GetHash().ToUint256())) {
            return PackageToValidate{ptx, child_tx, nodeid, child_sender};
        }
    }
    return std::nullopt;
}

void TxDownloadManagerImpl::MempoolAcceptedTx(const CTransactionRef& tx)
{
    // As this version of the transaction was acceptable, we can forget about any requests for it.
    // No-op if the tx is not in txrequest.
    m_txrequest.ForgetTxHash(tx->GetHash());
    m_txrequest.ForgetTxHash(tx->GetWitnessHash());

    m_orphanage.AddChildrenToWorkSet(*tx);
    // If it came from the orphanage, remove it. No-op if the tx is not in txorphanage.
    m_orphanage.EraseTx(tx->GetWitnessHash());
}

node::RejectedTxTodo TxDownloadManagerImpl::MempoolRejectedTx(const CTransactionRef& ptx, const TxValidationState& state, NodeId nodeid, bool first_time_failure)
{
    const CTransaction& tx{*ptx};
    // Results returned to caller
    // Whether we should call AddToCompactExtraTransactions at the end
    bool add_extra_compact_tx{first_time_failure};
    // Hashes to pass to AddKnownTx later
    std::vector<uint256> unique_parents;
    // Populated if failure is reconsiderable and eligible package is found.
    std::optional<node::PackageToValidate> package_to_validate;

    if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS) {
        // Only process a new orphan if this is a first time failure, as otherwise it must be either
        // already in orphanage or from 1p1c processing.
        if (first_time_failure && !RecentRejectsFilter().contains(ptx->GetWitnessHash().ToUint256())) {
            bool fRejectedParents = false; // It may be the case that the orphans parents have all been rejected

            // Deduplicate parent txids, so that we don't have to loop over
            // the same parent txid more than once down below.
            unique_parents.reserve(tx.vin.size());
            for (const CTxIn& txin : tx.vin) {
                // We start with all parents, and then remove duplicates below.
                unique_parents.push_back(txin.prevout.hash);
            }
            std::sort(unique_parents.begin(), unique_parents.end());
            unique_parents.erase(std::unique(unique_parents.begin(), unique_parents.end()), unique_parents.end());

            // Distinguish between parents in m_lazy_recent_rejects and m_lazy_recent_rejects_reconsiderable.
            // We can tolerate having up to 1 parent in m_lazy_recent_rejects_reconsiderable since we
            // submit 1p1c packages. However, fail immediately if any are in m_lazy_recent_rejects.
            std::optional<uint256> rejected_parent_reconsiderable;
            for (const uint256& parent_txid : unique_parents) {
                if (RecentRejectsFilter().contains(parent_txid)) {
                    fRejectedParents = true;
                    break;
                } else if (RecentRejectsReconsiderableFilter().contains(parent_txid) &&
                           !m_opts.m_mempool.exists(GenTxid::Txid(parent_txid))) {
                    // More than 1 parent in m_lazy_recent_rejects_reconsiderable: 1p1c will not be
                    // sufficient to accept this package, so just give up here.
                    if (rejected_parent_reconsiderable.has_value()) {
                        fRejectedParents = true;
                        break;
                    }
                    rejected_parent_reconsiderable = parent_txid;
                }
            }
            if (!fRejectedParents) {
                const auto current_time{GetTime<std::chrono::microseconds>()};

                for (const uint256& parent_txid : unique_parents) {
                    // Here, we only have the txid (and not wtxid) of the
                    // inputs, so we only request in txid mode, even for
                    // wtxidrelay peers.
                    // Eventually we should replace this with an improved
                    // protocol for getting all unconfirmed parents.
                    const auto gtxid{GenTxid::Txid(parent_txid)};
                    // Exclude m_lazy_recent_rejects_reconsiderable: the missing parent may have been
                    // previously rejected for being too low feerate. This orphan might CPFP it.
                    if (!AlreadyHaveTx(gtxid, /*include_reconsiderable=*/false)) {
                        AddTxAnnouncement(nodeid, gtxid, current_time, /*p2p_inv=*/false);
                    }
                }

                // Potentially flip add_extra_compact_tx to false if AddTx returns false because the tx was already there
                add_extra_compact_tx &= m_orphanage.AddTx(ptx, nodeid);

                // Once added to the orphan pool, a tx is considered AlreadyHave, and we shouldn't request it anymore.
                m_txrequest.ForgetTxHash(tx.GetHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());

                // DoS prevention: do not allow m_orphanage to grow unbounded (see CVE-2012-3789)
                m_orphanage.LimitOrphans(m_opts.m_max_orphan_txs, m_opts.m_rng);
            } else {
                unique_parents.clear();
                LogDebug(BCLog::MEMPOOL, "not keeping orphan with rejected parents %s (wtxid=%s)\n",
                         tx.GetHash().ToString(),
                         tx.GetWitnessHash().ToString());
                // We will continue to reject this tx since it has rejected
                // parents so avoid re-requesting it from other peers.
                // Here we add both the txid and the wtxid, as we know that
                // regardless of what witness is provided, we will not accept
                // this, so we don't need to allow for redownload of this txid
                // from any of our non-wtxidrelay peers.
                RecentRejectsFilter().insert(tx.GetHash().ToUint256());
                RecentRejectsFilter().insert(tx.GetWitnessHash().ToUint256());
                m_txrequest.ForgetTxHash(tx.GetHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());
            }
        }
    } else if (state.GetResult() == TxValidationResult::TX_WITNESS_STRIPPED) {
        add_extra_compact_tx = false;
    } else {
        // We can add the wtxid of this transaction to our reject filter.
        // Do not add txids of witness transactions or witness-stripped
        // transactions to the filter, as they can have been malleated;
        // adding such txids to the reject filter would potentially
        // interfere with relay of valid transactions from peers that
        // do not support wtxid-based relay. See
        // https://github.com/bitcoin/bitcoin/issues/8279 for details.
        // We can remove this restriction (and always add wtxids to
        // the filter even for witness stripped transactions) once
        // wtxid-based relay is broadly deployed.
        // See also comments in https://github.com/bitcoin/bitcoin/pull/18044#discussion_r443419034
        // for concerns around weakening security of unupgraded nodes
        // if we start doing this too early.
        if (state.GetResult() == TxValidationResult::TX_RECONSIDERABLE) {
            // If the result is TX_RECONSIDERABLE, add it to m_lazy_recent_rejects_reconsiderable
            // because we should not download or submit this transaction by itself again, but may
            // submit it as part of a package later.
            RecentRejectsReconsiderableFilter().insert(ptx->GetWitnessHash().ToUint256());

            if (first_time_failure) {
                // When a transaction fails for TX_RECONSIDERABLE, look for a matching child in the
                // orphanage, as it is possible that they succeed as a package.
                LogDebug(BCLog::TXPACKAGES, "tx %s (wtxid=%s) failed but reconsiderable, looking for child in orphanage\n",
                         ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString());
                package_to_validate = Find1P1CPackage(ptx, nodeid);
            }
        } else {
            RecentRejectsFilter().insert(ptx->GetWitnessHash().ToUint256());
        }
        m_txrequest.ForgetTxHash(ptx->GetWitnessHash());
        // If the transaction failed for TX_INPUTS_NOT_STANDARD,
        // then we know that the witness was irrelevant to the policy
        // failure, since this check depends only on the txid
        // (the scriptPubKey being spent is covered by the txid).
        // Add the txid to the reject filter to prevent repeated
        // processing of this transaction in the event that child
        // transactions are later received (resulting in
        // parent-fetching by txid via the orphan-handling logic).
        // We only add the txid if it differs from the wtxid, to avoid wasting entries in the
        // rolling bloom filter.
        if (state.GetResult() == TxValidationResult::TX_INPUTS_NOT_STANDARD && ptx->HasWitness()) {
            RecentRejectsFilter().insert(ptx->GetHash().ToUint256());
            m_txrequest.ForgetTxHash(ptx->GetHash());
        }
    }

    // If the tx failed in ProcessOrphanTx, it should be removed from the orphanage unless the
    // tx was still missing inputs. If the tx was not in the orphanage, EraseTx does nothing and returns 0.
    if (state.GetResult() != TxValidationResult::TX_MISSING_INPUTS && m_orphanage.EraseTx(ptx->GetWitnessHash()) > 0) {
        LogDebug(BCLog::TXPACKAGES, "   removed orphan tx %s (wtxid=%s)\n", ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString());
    }

    return RejectedTxTodo{
        .m_should_add_extra_compact_tx = add_extra_compact_tx,
        .m_unique_parents = std::move(unique_parents),
        .m_package_to_validate = std::move(package_to_validate)
    };
}

void TxDownloadManagerImpl::MempoolRejectedPackage(const Package& package)
{
    RecentRejectsReconsiderableFilter().insert(GetPackageHash(package));
}

std::pair<bool, std::optional<PackageToValidate>> TxDownloadManagerImpl::ReceivedTx(NodeId nodeid, const CTransactionRef& ptx)
{
    const uint256& txid = ptx->GetHash();
    const uint256& wtxid = ptx->GetWitnessHash();

    // Mark that we have received a response
    m_txrequest.ReceivedResponse(nodeid, txid);
    if (ptx->HasWitness()) m_txrequest.ReceivedResponse(nodeid, wtxid);

    // First check if we should drop this tx.
    // We do the AlreadyHaveTx() check using wtxid, rather than txid - in the
    // absence of witness malleation, this is strictly better, because the
    // recent rejects filter may contain the wtxid but rarely contains
    // the txid of a segwit transaction that has been rejected.
    // In the presence of witness malleation, it's possible that by only
    // doing the check with wtxid, we could overlook a transaction which
    // was confirmed with a different witness, or exists in our mempool
    // with a different witness, but this has limited downside:
    // mempool validation does its own lookup of whether we have the txid
    // already; and an adversary can already relay us old transactions
    // (older than our recency filter) if trying to DoS us, without any need
    // for witness malleation.
    if (AlreadyHaveTx(GenTxid::Wtxid(wtxid), /*include_reconsiderable=*/false)) {
        // If a tx is detected by m_lazy_recent_rejects it is ignored. Because we haven't
        // submitted the tx to our mempool, we won't have computed a DoS
        // score for it or determined exactly why we consider it invalid.
        //
        // This means we won't penalize any peer subsequently relaying a DoSy
        // tx (even if we penalized the first peer who gave it to us) because
        // we have to account for m_lazy_recent_rejects showing false positives. In
        // other words, we shouldn't penalize a peer if we aren't *sure* they
        // submitted a DoSy tx.
        //
        // Note that m_lazy_recent_rejects doesn't just record DoSy or invalid
        // transactions, but any tx not accepted by the mempool, which may be
        // due to node policy (vs. consensus). So we can't blanket penalize a
        // peer simply for relaying a tx that our m_lazy_recent_rejects has caught,
        // regardless of false positives.
        return {false, std::nullopt};
    } else if (RecentRejectsReconsiderableFilter().contains(wtxid)) {
        // When a transaction is already in m_lazy_recent_rejects_reconsiderable, we shouldn't submit
        // it by itself again. However, look for a matching child in the orphanage, as it is
        // possible that they succeed as a package.
        LogDebug(BCLog::TXPACKAGES, "found tx %s (wtxid=%s) in reconsiderable rejects, looking for child in orphanage\n",
                 txid.ToString(), wtxid.ToString());
        return {false, Find1P1CPackage(ptx, nodeid)};
    }


    return {true, std::nullopt};
}

bool TxDownloadManagerImpl::HaveMoreWork(NodeId nodeid)
{
    return m_orphanage.HaveTxToReconsider(nodeid);
}

CTransactionRef TxDownloadManagerImpl::GetTxToReconsider(NodeId nodeid)
{
    return m_orphanage.GetTxToReconsider(nodeid);
}

void TxDownloadManagerImpl::CheckIsEmpty(NodeId nodeid)
{
    assert(m_txrequest.Count(nodeid) == 0);
}
void TxDownloadManagerImpl::CheckIsEmpty()
{
    assert(m_orphanage.Size() == 0);
    assert(m_txrequest.Size() == 0);
    assert(m_num_wtxid_peers == 0);
}
std::vector<TxOrphanage::OrphanTxBase> TxDownloadManagerImpl::GetOrphanTransactions() const
{
    return m_orphanage.GetOrphanTransactions();
}
} // namespace node
