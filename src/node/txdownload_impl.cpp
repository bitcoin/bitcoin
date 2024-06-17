// Copyright (c) 2023
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txdownload_impl.h>
#include <node/txdownloadman.h>

#include <chain.h>
#include <consensus/validation.h>
#include <logging.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>

namespace node {
void TxDownloadImpl::UpdatedBlockTipSync()
{
    // If the chain tip has changed, previously rejected transactions might now be invalid, e.g. due
    // to a timelock. Reset the rejection filters to give those transactions another chance if we
    // see them again.
    m_recent_rejects.reset();
    m_recent_rejects_reconsiderable.reset();
}

void TxDownloadImpl::BlockConnected(const std::shared_ptr<const CBlock>& pblock)
{
    m_orphanage.EraseForBlock(*pblock);

    for (const auto& ptx : pblock->vtx) {
        m_recent_confirmed_transactions.insert(ptx->GetHash().ToUint256());
        if (ptx->HasWitness()) {
            m_recent_confirmed_transactions.insert(ptx->GetWitnessHash().ToUint256());
        }
    }
    for (const auto& ptx : pblock->vtx) {
        m_txrequest.ForgetTxHash(ptx->GetHash());
        m_txrequest.ForgetTxHash(ptx->GetWitnessHash());
    }
}

void TxDownloadImpl::BlockDisconnected()
{
    // To avoid relay problems with transactions that were previously
    // confirmed, clear our filter of recently confirmed transactions whenever
    // there's a reorg.
    // This means that in a 1-block reorg (where 1 block is disconnected and
    // then another block reconnected), our filter will drop to having only one
    // block's worth of transactions in it, but that should be fine, since
    // presumably the most common case of relaying a confirmed transaction
    // should be just after a new block containing it is found.
    m_recent_confirmed_transactions.reset();
}

bool TxDownloadImpl::AlreadyHaveTx(const GenTxid& gtxid, bool include_reconsiderable)
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

    if (include_reconsiderable && m_recent_rejects_reconsiderable.contains(hash)) return true;

    if (m_recent_confirmed_transactions.contains(hash)) return true;

    return m_recent_rejects.contains(hash) || m_opts.m_mempool.exists(gtxid);
}

void TxDownloadImpl::ConnectedPeer(NodeId nodeid, const TxDownloadConnectionInfo& info)
{
    // If already connected (shouldn't happen in practice), exit early.
    if (m_peer_info.count(nodeid) > 0) return;

    m_peer_info.emplace(nodeid, PeerInfo(info));
    if (info.m_wtxid_relay) m_num_wtxid_peers += 1;
}
void TxDownloadImpl::DisconnectedPeer(NodeId nodeid)
{
    m_orphanage.EraseForPeer(nodeid);
    m_txrequest.DisconnectedPeer(nodeid);

    if (m_peer_info.count(nodeid) > 0) {
        if (m_peer_info.at(nodeid).m_connection_info.m_wtxid_relay) m_num_wtxid_peers -= 1;
        m_peer_info.erase(nodeid);
    }
}

bool TxDownloadImpl::AddTxAnnouncement(NodeId peer, const GenTxid& gtxid, std::chrono::microseconds now, bool p2p_inv)
{
    const bool already_had{AlreadyHaveTx(gtxid, /*include_reconsiderable=*/true)};

    // If this is an inv received from a peer and we already have it, we can drop it.
    // Ignore when this is a scheduled parent request for an orphan.
    if (p2p_inv && already_had) return already_had;

    if (m_peer_info.count(peer) == 0) return already_had;
    const auto& info = m_peer_info.at(peer).m_connection_info;
    if (!info.m_relay_permissions && m_txrequest.Count(peer) >= MAX_PEER_TX_ANNOUNCEMENTS) {
        // Too many queued announcements for this peer
        return already_had;
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

    return already_had;
}

std::vector<GenTxid> TxDownloadImpl::GetRequestsToSend(NodeId nodeid, std::chrono::microseconds current_time)
{
    std::vector<GenTxid> requests;
    std::vector<std::pair<NodeId, GenTxid>> expired;
    auto requestable = m_txrequest.GetRequestable(nodeid, current_time, &expired);
    for (const auto& entry : expired) {
        LogPrint(BCLog::NET, "timeout of inflight %s %s from peer=%d\n", entry.second.IsWtxid() ? "wtx" : "tx",
            entry.second.GetHash().ToString(), entry.first);
    }
    for (const GenTxid& gtxid : requestable) {
        if (!AlreadyHaveTx(gtxid, /*include_reconsiderable=*/false)) {
            LogPrint(BCLog::NET, "Requesting %s %s peer=%d\n", gtxid.IsWtxid() ? "wtx" : "tx",
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

void TxDownloadImpl::ReceivedNotFound(NodeId nodeid, const std::vector<uint256>& txhashes)
{
    for (const auto& txhash: txhashes) {
        // If we receive a NOTFOUND message for a tx we requested, mark the announcement for it as
        // completed in TxRequestTracker.
        m_txrequest.ReceivedResponse(nodeid, txhash);
    }
}

std::optional<PackageToValidate> TxDownloadImpl::Find1P1CPackage(const CTransactionRef& ptx, NodeId nodeid)
{
    const auto& parent_wtxid{ptx->GetWitnessHash()};

    Assume(m_recent_rejects_reconsiderable.contains(parent_wtxid.ToUint256()));

    // Prefer children from this peer. This helps prevent censorship attempts in which an attacker
    // sends lots of fake children for the parent, and we (unluckily) keep selecting the fake
    // children instead of the real one provided by the honest peer.
    const auto cpfp_candidates_same_peer{m_orphanage.GetChildrenFromSamePeer(ptx, nodeid)};

    // These children should be sorted from most newest to oldest. In the (probably uncommon) case
    // of children that replace each other, this helps us accept the highest feerate (probably the
    // most recent) one efficiently.
    for (const auto& child : cpfp_candidates_same_peer) {
        Package maybe_cpfp_package{ptx, child};
        if (!m_recent_rejects_reconsiderable.contains(GetPackageHash(maybe_cpfp_package))) {
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
    Shuffle(tx_indices.begin(), tx_indices.end(), m_opts.m_rng);

    for (const auto index : tx_indices) {
        // If we already tried a package and failed for any reason, the combined hash was
        // cached in m_recent_rejects_reconsiderable.
        const auto [child_tx, child_sender] = cpfp_candidates_different_peer.at(index);
        Package maybe_cpfp_package{ptx, child_tx};
        if (!m_recent_rejects_reconsiderable.contains(GetPackageHash(maybe_cpfp_package))) {
            return PackageToValidate{ptx, child_tx, nodeid, child_sender};
        }
    }
    return std::nullopt;
}

void TxDownloadImpl::MempoolAcceptedTx(const CTransactionRef& tx)
{
    // As this version of the transaction was acceptable, we can forget about any requests for it.
    // No-op if the tx is not in txrequest.
    m_txrequest.ForgetTxHash(tx->GetHash());
    m_txrequest.ForgetTxHash(tx->GetWitnessHash());

    m_orphanage.AddChildrenToWorkSet(*tx);
    // If it came from the orphanage, remove it. No-op if the tx is not in txorphanage.
    m_orphanage.EraseTx(tx->GetWitnessHash());
}

node::RejectedTxTodo TxDownloadImpl::MempoolRejectedTx(const CTransactionRef& ptx, const TxValidationState& state, NodeId nodeid, bool not_from_orphanage)
{
    const CTransaction& tx{*ptx};
    // Whether we should call AddToCompactExtraTransactions at the end
    bool add_extra_compact_tx{not_from_orphanage};
    // Hashes to pass to AddKnownTx later
    std::vector<uint256> unique_parents;
    // Populated if failure is reconsiderable and eligible package is found.
    std::optional<node::PackageToValidate> package_to_validate;

    // Only process a new orphan if not_from_orphanage, as otherwise it must be either
    // already in orphanage or from 1p1c processing.
    const bool maybe_add_new_orphan{not_from_orphanage};

    if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS && maybe_add_new_orphan) {
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

        // Distinguish between parents in m_recent_rejects and m_recent_rejects_reconsiderable.
        // We can tolerate having up to 1 parent in m_recent_rejects_reconsiderable since we
        // submit 1p1c packages. However, fail immediately if any are in m_recent_rejects.
        std::optional<uint256> rejected_parent_reconsiderable;
        for (const uint256& parent_txid : unique_parents) {
            if (m_recent_rejects.contains(parent_txid)) {
                fRejectedParents = true;
                break;
            } else if (m_recent_rejects_reconsiderable.contains(parent_txid) && !m_opts.m_mempool.exists(GenTxid::Txid(parent_txid))) {
                // More than 1 parent in m_recent_rejects_reconsiderable: 1p1c will not be
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

                // Exclude m_recent_rejects_reconsiderable: the missing parent may have been
                // previously rejected for being too low feerate. This orphan might CPFP it.
                if (!AlreadyHaveTx(gtxid, /*include_reconsiderable=*/false)) {
                    AddTxAnnouncement(nodeid, gtxid, current_time, /*p2p_inv=*/false);
                }
            }

            add_extra_compact_tx &= m_orphanage.AddTx(ptx, nodeid);

            // Once added to the orphan pool, a tx is considered AlreadyHave, and we shouldn't request it anymore.
            m_txrequest.ForgetTxHash(tx.GetHash());
            m_txrequest.ForgetTxHash(tx.GetWitnessHash());

            // DoS prevention: do not allow m_orphanage to grow unbounded (see CVE-2012-3789)
            m_orphanage.LimitOrphans(m_opts.m_max_orphan_txs, m_opts.m_rng);

        } else {
            LogPrint(BCLog::MEMPOOL, "not keeping orphan with rejected parents %s (wtxid=%s)\n",
                     tx.GetHash().ToString(),
                     tx.GetWitnessHash().ToString());
            // We will continue to reject this tx since it has rejected
            // parents so avoid re-requesting it from other peers.
            // Here we add both the txid and the wtxid, as we know that
            // regardless of what witness is provided, we will not accept
            // this, so we don't need to allow for redownload of this txid
            // from any of our non-wtxidrelay peers.
            m_recent_rejects.insert(tx.GetHash().ToUint256());
            m_recent_rejects.insert(tx.GetWitnessHash().ToUint256());
            m_txrequest.ForgetTxHash(tx.GetHash());
            m_txrequest.ForgetTxHash(tx.GetWitnessHash());
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
            // If the result is TX_RECONSIDERABLE, add it to m_recent_rejects_reconsiderable
            // because we should not download or submit this transaction by itself again, but may
            // submit it as part of a package later.
            m_recent_rejects_reconsiderable.insert(ptx->GetWitnessHash().ToUint256());

            if (not_from_orphanage) {
                // When a transaction fails for TX_RECONSIDERABLE, look for a matching child in the
                // orphanage, as it is possible that they succeed as a package.
                LogPrint(BCLog::TXPACKAGES, "tx %s (wtxid=%s) failed but reconsiderable, looking for child in orphanage\n",
                         ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString());
                package_to_validate = Find1P1CPackage(ptx, nodeid);
            }
        } else {
            m_recent_rejects.insert(ptx->GetWitnessHash().ToUint256());
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
            m_recent_rejects.insert(ptx->GetHash().ToUint256());
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

void TxDownloadImpl::MempoolRejectedPackage(const Package& package)
{
    m_recent_rejects_reconsiderable.insert(GetPackageHash(package));
}

std::pair<bool, std::optional<PackageToValidate>> TxDownloadImpl::ReceivedTx(NodeId nodeid, const CTransactionRef& ptx)
{
    const uint256& txid = ptx->GetHash();
    const uint256& wtxid = ptx->GetWitnessHash();

    // Mark that we have received a response
    m_txrequest.ReceivedResponse(nodeid, ptx->GetHash());
    if (ptx->HasWitness()) m_txrequest.ReceivedResponse(nodeid, ptx->GetWitnessHash());

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
    if (AlreadyHaveTx(GenTxid::Wtxid(wtxid), /*include_reconsiderable=*/true)) {

        if (m_recent_rejects_reconsiderable.contains(wtxid)) {
            // When a transaction is already in m_recent_rejects_reconsiderable, we shouldn't submit
            // it by itself again. However, look for a matching child in the orphanage, as it is
            // possible that they succeed as a package.
            LogPrint(BCLog::TXPACKAGES, "found tx %s (wtxid=%s) in reconsiderable rejects, looking for child in orphanage\n",
                     txid.ToString(), wtxid.ToString());
            return std::make_pair(false, Find1P1CPackage(ptx, nodeid));
        }

        // If a tx is detected by m_recent_rejects it is ignored. Because we haven't
        // submitted the tx to our mempool, we won't have computed a DoS
        // score for it or determined exactly why we consider it invalid.
        //
        // This means we won't penalize any peer subsequently relaying a DoSy
        // tx (even if we penalized the first peer who gave it to us) because
        // we have to account for m_recent_rejects showing false positives. In
        // other words, we shouldn't penalize a peer if we aren't *sure* they
        // submitted a DoSy tx.
        //
        // Note that m_recent_rejects doesn't just record DoSy or invalid
        // transactions, but any tx not accepted by the mempool, which may be
        // due to node policy (vs. consensus). So we can't blanket penalize a
        // peer simply for relaying a tx that our m_recent_rejects has caught,
        // regardless of false positives.
        return std::make_pair(false, std::nullopt);
    }

    return std::make_pair(true, std::nullopt);
}

bool TxDownloadImpl::HaveMoreWork(NodeId nodeid)
{
    return m_orphanage.HaveTxToReconsider(nodeid);
}

CTransactionRef TxDownloadImpl::GetTxToReconsider(NodeId nodeid)
{
    return m_orphanage.GetTxToReconsider(nodeid);
}

void TxDownloadImpl::CheckIsEmpty(NodeId nodeid)
{
    assert(m_txrequest.Count(nodeid) == 0);
}
void TxDownloadImpl::CheckIsEmpty()
{
    assert(m_orphanage.Size() == 0);
    assert(m_txrequest.Size() == 0);
}
} // namespace node
