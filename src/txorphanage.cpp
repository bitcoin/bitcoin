// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txorphanage.h>

#include <consensus/validation.h>
#include <logging.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <util/hasher.h>
#include <util/time.h>

#include <cassert>

void TxOrphanage::RemoveIterAt(NodeId peer, size_t list_pos)
{
    auto it_peer = m_peer_orphanage_info.find(peer);
    if (!Assume(it_peer != m_peer_orphanage_info.end())) return;

    auto& orphan_list = it_peer->second.m_iter_list;
    if (list_pos + 1 != orphan_list.size()) {
        // Unless we're deleting the last entry in orphan_list, move the last entry to the position we're deleting.
        auto it_last = orphan_list.back();
        orphan_list[list_pos] = it_last;
        // Update the list_pos of this swapped element.
        it_last->second.announcers.at(peer) = list_pos;
    }
    orphan_list.pop_back();
}

bool TxOrphanage::AddTx(const CTransactionRef& tx, NodeId peer)
{
    const Txid& hash = tx->GetHash();
    const Wtxid& wtxid = tx->GetWitnessHash();
    if (auto it{m_orphans.find(wtxid)}; it != m_orphans.end()) {
        AddAnnouncer(wtxid, peer);
        // No new orphan entry was created. An announcer may have been added.
        return false;
    }

    // Ignore big transactions, to avoid a
    // send-big-orphans memory exhaustion attack. If a peer has a legitimate
    // large transaction with a missing parent then we assume
    // it will rebroadcast it later, after the parent transaction(s)
    // have been mined or received.
    // 100 orphans, each of which is at most 100,000 bytes big is
    // at most 10 megabytes of orphans and somewhat more byprev index (in the worst case):
    unsigned int sz = GetTransactionWeight(*tx);
    if (sz > MAX_STANDARD_TX_WEIGHT)
    {
        LogDebug(BCLog::TXPACKAGES, "ignoring large orphan tx (size: %u, txid: %s, wtxid: %s)\n", sz, hash.ToString(), wtxid.ToString());
        return false;
    }

    auto& orphan_list = m_peer_orphanage_info.try_emplace(peer).first->second.m_iter_list;
    std::map<NodeId, size_t> announcer_list_pos{{peer, orphan_list.size()}};
    auto ret = m_orphans.emplace(wtxid, OrphanTx{{tx, std::move(announcer_list_pos), Now<NodeSeconds>() + ORPHAN_TX_EXPIRE_TIME}});
    assert(ret.second);
    orphan_list.push_back(ret.first);
    for (const CTxIn& txin : tx->vin) {
        m_outpoint_to_orphan_it[txin.prevout].insert(ret.first);
    }
    m_total_orphan_usage += sz;
    m_total_announcements += 1;
    auto& peer_info = m_peer_orphanage_info.try_emplace(peer).first->second;
    peer_info.m_total_usage += sz;

    LogDebug(BCLog::TXPACKAGES, "stored orphan tx %s (wtxid=%s), weight: %u (mapsz %u outsz %u)\n", hash.ToString(), wtxid.ToString(), sz,
             m_orphans.size(), m_outpoint_to_orphan_it.size());
    return true;
}

bool TxOrphanage::AddAnnouncer(const Wtxid& wtxid, NodeId peer)
{
    const auto it = m_orphans.find(wtxid);
    if (it != m_orphans.end()) {
        Assume(!it->second.announcers.empty());
        auto& peer_info = m_peer_orphanage_info.try_emplace(peer).first->second;
        const auto ret = it->second.announcers.emplace(peer, peer_info.m_iter_list.size());
        if (ret.second) {
            peer_info.m_total_usage += it->second.GetUsage();
            peer_info.m_iter_list.push_back(it);
            m_total_announcements += 1;
            LogDebug(BCLog::TXPACKAGES, "added peer=%d as announcer of orphan tx %s\n", peer, wtxid.ToString());
            return true;
        }
    }
    return false;
}

int TxOrphanage::EraseTx(const Wtxid& wtxid)
{
    std::map<Wtxid, OrphanTx>::iterator it = m_orphans.find(wtxid);
    if (it == m_orphans.end())
        return 0;
    for (const CTxIn& txin : it->second.tx->vin)
    {
        auto itPrev = m_outpoint_to_orphan_it.find(txin.prevout);
        if (itPrev == m_outpoint_to_orphan_it.end())
            continue;
        itPrev->second.erase(it);
        if (itPrev->second.empty())
            m_outpoint_to_orphan_it.erase(itPrev);
    }

    const auto tx_size{it->second.GetUsage()};
    m_total_orphan_usage -= tx_size;
    m_total_announcements -= it->second.announcers.size();
    // Decrement each announcer's m_total_usage and remove orphan from all m_iter_list.
    for (const auto& [peer, old_pos] : it->second.announcers) {
        auto peer_it = m_peer_orphanage_info.find(peer);
        if (Assume(peer_it != m_peer_orphanage_info.end())) {
            peer_it->second.m_total_usage -= tx_size;
            RemoveIterAt(peer, old_pos);
            // Clean up workset so it does not contain any wtxids of transactions no longer in m_orphans.
            peer_it->second.m_work_set.erase(wtxid);
        }
    }

    const auto& txid = it->second.tx->GetHash();
    // Time spent in orphanage = difference between current and entry time.
    // Entry time is equal to ORPHAN_TX_EXPIRE_TIME earlier than entry's expiry.
    LogDebug(BCLog::TXPACKAGES, "   removed orphan tx %s (wtxid=%s) after %ds\n", txid.ToString(), wtxid.ToString(),
             Ticks<std::chrono::seconds>(NodeClock::now() + ORPHAN_TX_EXPIRE_TIME - it->second.nTimeExpire));

    m_orphans.erase(it);
    return 1;
}

void TxOrphanage::EraseForPeer(NodeId peer)
{
    // Zeroes out this peer's m_total_usage.
    m_peer_orphanage_info.erase(peer);

    int nErased = 0;
    std::map<Wtxid, OrphanTx>::iterator iter = m_orphans.begin();
    while (iter != m_orphans.end())
    {
        // increment to avoid iterator becoming invalid after erasure
        auto& [wtxid, orphan] = *iter++;
        auto orphan_it = orphan.announcers.find(peer);
        if (orphan_it != orphan.announcers.end()) {
            orphan.announcers.erase(peer);
            m_total_announcements -= 1;

            // No remaining announcers: clean up entry
            if (orphan.announcers.empty()) {
                nErased += EraseTx(orphan.tx->GetWitnessHash());
            }
        }
    }
    if (nErased > 0) LogDebug(BCLog::TXPACKAGES, "Erased %d orphan transaction(s) from peer=%d\n", nErased, peer);
}

unsigned int TxOrphanage::MaybeExpireOrphans()
{
    unsigned int nErased = 0;
    auto nNow{Now<NodeSeconds>()};
    if (m_next_sweep <= nNow) {
        // Sweep out expired orphan pool entries:
        auto nMinExpTime{nNow + ORPHAN_TX_EXPIRE_TIME - ORPHAN_TX_EXPIRE_INTERVAL};
        std::map<Wtxid, OrphanTx>::iterator iter = m_orphans.begin();
        while (iter != m_orphans.end())
        {
            std::map<Wtxid, OrphanTx>::iterator maybeErase = iter++;
            if (maybeErase->second.nTimeExpire <= nNow) {
                nErased += EraseTx(maybeErase->first);
            } else {
                nMinExpTime = std::min(maybeErase->second.nTimeExpire, nMinExpTime);
            }
        }
        // Sweep again 5 minutes after the next entry that expires in order to batch the linear scan.
        m_next_sweep = nMinExpTime + ORPHAN_TX_EXPIRE_INTERVAL;
    }
    return nErased;
}

unsigned int TxOrphanage::MaybeTrimOrphans(unsigned int max_orphans, FastRandomContext& rng)
{
    // Exit early to avoid building the heap unnecessarily
    if (!NeedsTrim(max_orphans)) return 0;

    std::vector<PeerMap::iterator> peer_it_heap;
    peer_it_heap.reserve(m_peer_orphanage_info.size());
    for (auto it = m_peer_orphanage_info.begin(); it != m_peer_orphanage_info.end(); ++it) peer_it_heap.push_back(it);

    // Sort peers that have the highest ratio of DoSiness first
    auto compare_peer = [this](PeerMap::iterator left, PeerMap::iterator right) {
        const auto max_ann{GetPerPeerMaxAnnouncements()};
        const auto max_mem{GetPerPeerMaxUsage()};
        return left->second.GetDoSScore(max_ann, max_mem) < right->second.GetDoSScore(max_ann, max_mem);
    };

    std::make_heap(peer_it_heap.begin(), peer_it_heap.end(), compare_peer);

    unsigned int nEvicted = 0;

    // Since each iteration should remove 1 announcement, this loop runs at most m_total_announcements times.
    // Note that we don't necessarily delete an orphan on each iteration. We might only be deleting
    // a peer from its announcers list.
    while (NeedsTrim(max_orphans))
    {
        if (!Assume(!peer_it_heap.empty())) break;
        // Find the peer with the highest DoS score, which is a fraction of {usage, announcements} used
        // over the respective allowances. This metric causes us to naturally select peers who have
        // exceeded their limits (i.e. a DoS score > 1) before peers who haven't. However, no matter
        // what, we evict from the DoSiest peers first. We may choose the same peer as the last
        // iteration of this loop.
        // Note: if ratios are the same, FeeFrac tiebreaks by denominator. In practice, since
        // announcements is always lower, this means that a peer with only high CPU DoS score will
        // be targeted before a peer with only high memory DoS score, even if they have the same
        // ratios.
        std::pop_heap(peer_it_heap.begin(), peer_it_heap.end(), compare_peer);
        auto it_worst_peer = peer_it_heap.back();
        peer_it_heap.pop_back();

        // Evict a random orphan from this peer.
        size_t randompos = rng.randrange(it_worst_peer->second.m_iter_list.size());
        auto it_to_evict = it_worst_peer->second.m_iter_list.at(randompos);

        // Only erase this peer as an announcer, unless it is the only announcer. Otherwise peers
        // can selectively delete orphan transactions by announcing a lot of them.
        if (it_to_evict->second.announcers.size() > 1) {
            Assume(it_to_evict->second.announcers.erase(it_worst_peer->first));
            it_worst_peer->second.m_total_usage -= it_to_evict->second.GetUsage();
            it_worst_peer->second.m_work_set.erase(it_to_evict->second.tx->GetWitnessHash());
            m_total_announcements -= 1;
            RemoveIterAt(it_worst_peer->first, randompos);
        } else {
            EraseTx(it_to_evict->first);
            ++nEvicted;
        }

        // Unless this peer is empty, put it back in the heap so we continue to consider evicting its orphans.
        // It might still be the DoSiest peer.
        if (!it_worst_peer->second.m_iter_list.empty()) {
            peer_it_heap.push_back(it_worst_peer);
            std::push_heap(peer_it_heap.begin(), peer_it_heap.end(), compare_peer);
        }
    }
    return nEvicted;
}

void TxOrphanage::LimitOrphans(unsigned int max_orphans, FastRandomContext& rng)
{
    const auto nErased{MaybeExpireOrphans()};
    if (nErased > 0) LogDebug(BCLog::TXPACKAGES, "Erased %u orphan tx due to expiration\n", nErased);

    const auto nEvicted{MaybeTrimOrphans(max_orphans, rng)};
    if (nEvicted > 0) LogDebug(BCLog::TXPACKAGES, "orphanage overflow, removed %u tx\n", nEvicted);
}

void TxOrphanage::AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng)
{
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const auto it_by_prev = m_outpoint_to_orphan_it.find(COutPoint(tx.GetHash(), i));
        if (it_by_prev != m_outpoint_to_orphan_it.end()) {
            for (const auto& elem : it_by_prev->second) {
                // Belt and suspenders, each orphan should always have at least 1 announcer.
                if (!Assume(!elem->second.announcers.empty())) continue;

                // Select a random peer to assign orphan processing, reducing wasted work if the orphan is still missing
                // inputs. However, we don't want to create an issue in which the assigned peer can purposefully stop us
                // from processing the orphan by disconnecting.
                auto announcer_iter = std::begin(elem->second.announcers);
                std::advance(announcer_iter, rng.randrange(elem->second.announcers.size()));
                auto announcer = announcer_iter->first;

                // Get this source peer's work set, emplacing an empty set if it didn't exist
                // (note: if this peer wasn't still connected, we would have removed the orphan tx already)
                std::set<Wtxid>& orphan_work_set = m_peer_orphanage_info.try_emplace(announcer).first->second.m_work_set;
                // Add this tx to the work set
                orphan_work_set.insert(elem->first);
                LogDebug(BCLog::TXPACKAGES, "added %s (wtxid=%s) to peer %d workset\n",
                         tx.GetHash().ToString(), tx.GetWitnessHash().ToString(), announcer);
            }
        }
    }
}

bool TxOrphanage::HaveTx(const Wtxid& wtxid) const
{
    return m_orphans.count(wtxid);
}

CTransactionRef TxOrphanage::GetTx(const Wtxid& wtxid) const
{
    auto it = m_orphans.find(wtxid);
    return it != m_orphans.end() ? it->second.tx : nullptr;
}

bool TxOrphanage::HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const
{
    auto it = m_orphans.find(wtxid);
    return (it != m_orphans.end() && it->second.announcers.contains(peer));
}

CTransactionRef TxOrphanage::GetTxToReconsider(NodeId peer)
{
    auto peer_it = m_peer_orphanage_info.find(peer);
    if (peer_it == m_peer_orphanage_info.end()) return nullptr;

    auto& work_set = peer_it->second.m_work_set;
    while (!work_set.empty()) {
        Wtxid wtxid = *work_set.begin();
        work_set.erase(work_set.begin());

        const auto orphan_it = m_orphans.find(wtxid);
        if (orphan_it != m_orphans.end()) {
            return orphan_it->second.tx;
        }
    }
    return nullptr;
}

bool TxOrphanage::HaveTxToReconsider(NodeId peer)
{
    auto peer_it = m_peer_orphanage_info.find(peer);
    if (peer_it == m_peer_orphanage_info.end()) return false;

    auto& work_set = peer_it->second.m_work_set;
    return !work_set.empty();
}

void TxOrphanage::EraseForBlock(const CBlock& block)
{
    std::unordered_set<Wtxid, SaltedTxidHasher> wtxids_to_erase;

    for (const CTransactionRef& ptx : block.vtx) {
        const CTransaction& tx = *ptx;

        // Which orphan pool entries must we evict?
        for (const auto& txin : tx.vin) {
            auto itByPrev = m_outpoint_to_orphan_it.find(txin.prevout);
            if (itByPrev == m_outpoint_to_orphan_it.end()) continue;
            for (auto mi = itByPrev->second.begin(); mi != itByPrev->second.end(); ++mi) {
                const CTransaction& orphanTx = *(*mi)->second.tx;
                wtxids_to_erase.insert(orphanTx.GetWitnessHash());
                // Stop to avoid doing too much work. If there are more orphans to erase, rely on
                // expiration and evictions to clean up everything eventually.
                if (wtxids_to_erase.size() >= 100) break;
            }
        }
    }

    // Erase orphan transactions included or precluded by this block
    if (wtxids_to_erase.size()) {
        int nErased = 0;
        for (const auto& wtxid : wtxids_to_erase) {
            nErased += EraseTx(wtxid);
        }
        LogDebug(BCLog::TXPACKAGES, "Erased %d orphan transaction(s) included or conflicted by block\n", nErased);
    }
}

std::vector<CTransactionRef> TxOrphanage::GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId nodeid) const
{
    // First construct a vector of iterators to ensure we do not return duplicates of the same tx
    // and so we can sort by nTimeExpire.
    std::vector<OrphanMap::iterator> iters;

    // For each output, get all entries spending this prevout, filtering for ones from the specified peer.
    for (unsigned int i = 0; i < parent->vout.size(); i++) {
        const auto it_by_prev = m_outpoint_to_orphan_it.find(COutPoint(parent->GetHash(), i));
        if (it_by_prev != m_outpoint_to_orphan_it.end()) {
            for (const auto& elem : it_by_prev->second) {
                if (elem->second.announcers.contains(nodeid)) {
                    iters.emplace_back(elem);
                }
            }
        }
    }

    // Sort by address so that duplicates can be deleted. At the same time, sort so that more recent
    // orphans (which expire later) come first.  Break ties based on address, as nTimeExpire is
    // quantified in seconds and it is possible for orphans to have the same expiry.
    std::sort(iters.begin(), iters.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs->second.nTimeExpire == rhs->second.nTimeExpire) {
            return &(*lhs) < &(*rhs);
        } else {
            return lhs->second.nTimeExpire > rhs->second.nTimeExpire;
        }
    });
    // Erase duplicates
    iters.erase(std::unique(iters.begin(), iters.end()), iters.end());

    // Convert to a vector of CTransactionRef
    std::vector<CTransactionRef> children_found;
    children_found.reserve(iters.size());
    for (const auto& child_iter : iters) {
        children_found.emplace_back(child_iter->second.tx);
    }
    return children_found;
}

std::vector<TxOrphanage::OrphanTxBase> TxOrphanage::GetOrphanTransactions() const
{
    std::vector<OrphanTxBase> ret;
    ret.reserve(m_orphans.size());
    for (auto const& o : m_orphans) {
        ret.push_back({o.second.tx, o.second.announcers, o.second.nTimeExpire});
    }
    return ret;
}

void TxOrphanage::SanityCheck() const
{
    // Check that cached m_total_announcements is correct. First count when iterating through m_orphans (counting number
    // of announcers each), then count when iterating through peers (counting number of orphans per peer).
    unsigned int counted_total_announcements{0};
    // Check that m_total_orphan_usage is correct
    int64_t counted_total_usage{0};

    // Check that cached PeerOrphanInfo::m_total_size is correct
    std::map<NodeId, int64_t> counted_size_per_peer;

    for (const auto& [wtxid, orphan] : m_orphans) {
        counted_total_announcements += orphan.announcers.size();
        counted_total_usage += orphan.GetUsage();

        Assert(!orphan.announcers.empty());
        for (const auto& [peer, list_pos] : orphan.announcers) {
            auto& count_peer_entry = counted_size_per_peer.try_emplace(peer).first->second;
            count_peer_entry += orphan.GetUsage();

            // Check that list_pos is accurate
            auto peer_it = m_peer_orphanage_info.find(peer);
            Assert(peer_it != m_peer_orphanage_info.end());
            Assert(peer_it->second.m_iter_list.at(list_pos)->first == wtxid);
        }
    }

    Assert(m_total_announcements >= m_orphans.size());
    Assert(counted_total_announcements == m_total_announcements);
    Assert(counted_total_usage == m_total_orphan_usage);

    // There must be an entry in m_peer_orphanage_info for each peer
    // However, there may be m_peer_orphanage_info entries corresponding to peers for whom we
    // previously had orphans but no longer do.
    Assert(counted_size_per_peer.size() <= m_peer_orphanage_info.size());

    // Collect set of unique wtxids found in all peers' m_iter_list to ensure nothing is missing.
    std::set<Wtxid> wtxids_in_peer_map;
    for (const auto& [peerid, info] : m_peer_orphanage_info) {
        auto it_counted = counted_size_per_peer.find(peerid);
        if (it_counted == counted_size_per_peer.end()) {
            Assert(info.m_total_usage == 0);
        } else {
            Assert(it_counted->second == info.m_total_usage);
        }

        counted_total_announcements -= info.m_iter_list.size();
        for (const auto& orphan_it : info.m_iter_list) {
            Assert(orphan_it->second.announcers.contains(peerid));
            wtxids_in_peer_map.insert(orphan_it->second.tx->GetWitnessHash());
        }
        // The work set is always a subset of the announcements.
        for (const auto& wtxid : info.m_work_set) {
            Assert(std::find_if(info.m_iter_list.begin(), info.m_iter_list.end(), [&](const auto& orphan_it) {
                return orphan_it->second.tx->GetWitnessHash() == wtxid;
            }) != info.m_iter_list.end());
        }
    }

    Assert(wtxids_in_peer_map.size() == m_orphans.size());
    Assert(counted_total_announcements == 0);
}

bool TxOrphanage::NeedsTrim(unsigned int max_orphans) const
{
    return (m_orphans.size() > max_orphans) ||
           (m_total_announcements > GetGlobalMaxAnnouncements()) ||
           (m_total_orphan_usage > GetGlobalMaxUsage());
}
