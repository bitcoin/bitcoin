// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txorphanage.h>

#include <consensus/validation.h>
#include <logging.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <util/feefrac.h>
#include <util/time.h>

#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>

#include <cassert>

namespace node {
class TxOrphanageImpl final : public TxOrphanage {
    // Type alias for sequence numbers
    using SequenceNumber = uint64_t;

    /** Global sequence number, increment each time an announcement is added. */
    SequenceNumber m_current_sequence{0};

    /** One orphan announcement. Each announcement (i.e. combination of wtxid, nodeid) is unique. There may be multiple
     * announcements for the same tx, and multiple transactions with the same txid but different wtxid are possible. */
    struct Announcement
    {
        const CTransactionRef m_tx;
        /** Which peer announced this tx */
        const NodeId m_announcer;
        /** What order this transaction entered the orphanage. */
        const SequenceNumber m_entry_sequence;
        /** Whether this tx should be reconsidered. Always starts out false. A peer's workset is the collection of all
         * announcements with m_reconsider=true. */
        bool m_reconsider{false};

        Announcement(const CTransactionRef& tx, NodeId peer, SequenceNumber seq) :
            m_tx{tx}, m_announcer{peer}, m_entry_sequence{seq}
        { }

        /** Get the weight of the transaction, our approximation for "memory usage". */
        TxOrphanage::Usage GetUsage()  const {
            return GetTransactionWeight(*m_tx);
        }
    };

    // Index by wtxid, then peer
    struct ByWtxid {};
    using ByWtxidView = std::tuple<Wtxid, NodeId>;
    struct WtxidExtractor
    {
        using result_type = ByWtxidView;
        result_type operator()(const Announcement& ann) const
        {
            return ByWtxidView{ann.m_tx->GetWitnessHash(), ann.m_announcer};
        }
    };

    // Sort by peer, then by whether it is ready to reconsider, then by recency.
    struct ByPeer {};
    using ByPeerView = std::tuple<NodeId, bool, SequenceNumber>;
    struct ByPeerViewExtractor {
        using result_type = ByPeerView;
        result_type operator()(const Announcement& ann) const
        {
            return ByPeerView{ann.m_announcer, ann.m_reconsider, ann.m_entry_sequence};
        }
    };

    struct OrphanIndices final : boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::tag<ByWtxid>, WtxidExtractor>,
        boost::multi_index::ordered_unique<boost::multi_index::tag<ByPeer>, ByPeerViewExtractor>
    >{};

    using AnnouncementMap = boost::multi_index::multi_index_container<Announcement, OrphanIndices>;
    template<typename Tag>
    using Iter = typename AnnouncementMap::index<Tag>::type::iterator;
    AnnouncementMap m_orphans;

    const TxOrphanage::Count m_max_global_announcements{DEFAULT_MAX_ORPHAN_ANNOUNCEMENTS};
    const TxOrphanage::Usage m_reserved_usage_per_peer{DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER};

    /** Number of unique orphans by wtxid. Less than or equal to the number of entries in m_orphans. */
    TxOrphanage::Count m_unique_orphans{0};

    /** Memory used by orphans (see Announcement::GetUsage()), deduplicated by wtxid. */
    TxOrphanage::Usage m_unique_orphan_bytes{0};

    /** Index from the parents' outputs to wtxids that exist in m_orphans. Used to find children of
     * a transaction that can be reconsidered and to remove entries that conflict with a block.*/
    std::map<COutPoint, std::set<Wtxid>> m_outpoint_to_orphan_it;

    struct PeerDoSInfo {
        TxOrphanage::Usage m_total_usage{0};
        TxOrphanage::Count m_count_announcements{0};
        bool operator==(const PeerDoSInfo& other) const
        {
            return m_total_usage == other.m_total_usage &&
                   m_count_announcements == other.m_count_announcements;
        }
        void Add(const Announcement& ann)
        {
            m_total_usage += ann.GetUsage();
            m_count_announcements += 1;
        }
        bool Subtract(const Announcement& ann)
        {
            m_total_usage -= ann.GetUsage();
            m_count_announcements -= 1;
            return m_count_announcements == 0;
        }
        /** There are 2 DoS scores:
        * - CPU score (ratio of num announcements / max allowed announcements)
        * - Memory score (ratio of total usage / max allowed usage).
        *
        * If the peer is using more than the allowed for either resource, its DoS score is > 1.
        * A peer having a DoS score > 1 does not necessarily mean that something is wrong, since we
        * do not trim unless the orphanage exceeds global limits, but it means that this peer will
        * be selected for trimming sooner. If the global announcement or global memory usage
        * limits are exceeded, it must be that there is a peer whose DoS score > 1. */
        FeeFrac GetDosScore(TxOrphanage::Count max_peer_count, TxOrphanage::Usage max_peer_bytes) const
        {
            assert(max_peer_count > 0);
            assert(max_peer_bytes > 0);
            const FeeFrac cpu_score(m_count_announcements, max_peer_count);
            const FeeFrac mem_score(m_total_usage, max_peer_bytes);
            return std::max<FeeFrac>(cpu_score, mem_score);
        }
    };
    /** Store per-peer statistics. Used to determine each peer's DoS score. The size of this map is used to determine the
     * number of peers and thus global {announcement, memory} limits. */
    std::unordered_map<NodeId, PeerDoSInfo> m_peer_orphanage_info;

    /** Erase from m_orphans and update m_peer_orphanage_info. */
    template<typename Tag>
    void Erase(Iter<Tag> it);

    /** Check if there is exactly one announcement with the same wtxid as it. */
    bool IsUnique(Iter<ByWtxid> it) const;

public:
    TxOrphanageImpl() = default;
    TxOrphanageImpl(Count max_global_ann, Usage reserved_peer_usage) :
        m_max_global_announcements{max_global_ann},
        m_reserved_usage_per_peer{reserved_peer_usage}
    {}
    ~TxOrphanageImpl() noexcept override = default;

    TxOrphanage::Count CountAnnouncements() const override;
    TxOrphanage::Count CountUniqueOrphans() const override;
    TxOrphanage::Count AnnouncementsFromPeer(NodeId peer) const override;
    TxOrphanage::Usage UsageFromPeer(NodeId peer) const override;
    TxOrphanage::Count MaxGlobalAnnouncements() const override;
    TxOrphanage::Usage ReservedPeerUsage() const override;
    TxOrphanage::Count MaxPeerAnnouncements() const override;
    TxOrphanage::Usage MaxGlobalUsage() const override;
    bool NeedsTrim() const override;

    bool AddTx(const CTransactionRef& tx, NodeId peer) override;
    bool AddAnnouncer(const Wtxid& wtxid, NodeId peer) override;
    CTransactionRef GetTx(const Wtxid& wtxid) const override;
    bool HaveTx(const Wtxid& wtxid) const override;
    bool HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const override;
    CTransactionRef GetTxToReconsider(NodeId peer) override;
    bool EraseTx(const Wtxid& wtxid) override;
    void EraseForPeer(NodeId peer) override;
    void EraseForBlock(const CBlock& block) override;
    void LimitOrphans() override;
    void AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng) override;
    bool HaveTxToReconsider(NodeId peer) override;
    std::vector<CTransactionRef> GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId nodeid) const override;
    std::vector<OrphanTxBase> GetOrphanTransactions() const override;
    TxOrphanage::Usage TotalOrphanUsage() const override;
    TxOrphanage::Usage UsageByPeer(NodeId peer) const override;
    void SanityCheck() const override;
};

template<typename Tag>
void TxOrphanageImpl::Erase(Iter<Tag> it)
{
    // Update m_peer_orphanage_info and clean up entries if they point to an empty struct.
    // This means peers that are not storing any orphans do not have an entry in
    // m_peer_orphanage_info (they can be added back later if they announce another orphan) and
    // ensures disconnected peers are not tracked forever.
    auto peer_it = m_peer_orphanage_info.find(it->m_announcer);
    Assume(peer_it != m_peer_orphanage_info.end());
    if (peer_it->second.Subtract(*it)) m_peer_orphanage_info.erase(peer_it);

    if (IsUnique(m_orphans.project<ByWtxid>(it))) {
        m_unique_orphans -= 1;
        m_unique_orphan_bytes -= it->GetUsage();

        // Remove references in m_outpoint_to_orphan_it
        const auto& wtxid{it->m_tx->GetWitnessHash()};
        for (const auto& input : it->m_tx->vin) {
            auto it_prev = m_outpoint_to_orphan_it.find(input.prevout);
            if (it_prev != m_outpoint_to_orphan_it.end()) {
                it_prev->second.erase(wtxid);
                // Clean up keys if they point to an empty set.
                if (it_prev->second.empty()) {
                    m_outpoint_to_orphan_it.erase(it_prev);
                }
            }
        }
    }
    m_orphans.get<Tag>().erase(it);
}

bool TxOrphanageImpl::IsUnique(Iter<ByWtxid> it) const
{
    // Iterators ByWtxid are sorted by wtxid, so check if neighboring elements have the same wtxid.
    auto& index = m_orphans.get<ByWtxid>();
    if (it == index.end()) return false;
    if (std::next(it) != index.end() && std::next(it)->m_tx->GetWitnessHash() == it->m_tx->GetWitnessHash()) return false;
    if (it != index.begin() && std::prev(it)->m_tx->GetWitnessHash() == it->m_tx->GetWitnessHash()) return false;
    return true;
}

TxOrphanage::Usage TxOrphanageImpl::UsageByPeer(NodeId peer) const
{
    auto it = m_peer_orphanage_info.find(peer);
    return it == m_peer_orphanage_info.end() ? 0 : it->second.m_total_usage;
}

/** Number of announcements. Ones for the same wtxid are not de-duplicated, use CountUniqueOrphans() instead. */
TxOrphanage::Count TxOrphanageImpl::CountAnnouncements() const { return m_orphans.size(); }

/** Total number of bytes used by orphans, de-duplicated by wtxid. */
TxOrphanage::Usage TxOrphanageImpl::TotalOrphanUsage() const { return m_unique_orphan_bytes; }

/** Number of unique orphans. */
TxOrphanage::Count TxOrphanageImpl::CountUniqueOrphans() const { return m_unique_orphans; }

/** Number of orphans stored from this peer. */
TxOrphanage::Count TxOrphanageImpl::AnnouncementsFromPeer(NodeId peer) const {
    auto it = m_peer_orphanage_info.find(peer);
    return it == m_peer_orphanage_info.end() ? 0 : it->second.m_count_announcements;
}

/** Total usage of orphans from this peer */
TxOrphanage::Usage TxOrphanageImpl::UsageFromPeer(NodeId peer) const {
    auto it = m_peer_orphanage_info.find(peer);
    return it == m_peer_orphanage_info.end() ? 0 : it->second.m_total_usage;
}

bool TxOrphanageImpl::AddTx(const CTransactionRef& tx, NodeId peer)
{
    const auto& wtxid{tx->GetWitnessHash()};
    const auto& txid{tx->GetHash()};

    // Ignore transactions above max standard size to avoid a send-big-orphans memory exhaustion attack.
    TxOrphanage::Usage sz = GetTransactionWeight(*tx);
    if (sz > MAX_STANDARD_TX_WEIGHT) {
        LogDebug(BCLog::TXPACKAGES, "ignoring large orphan tx (size: %u, txid: %s, wtxid: %s)\n", sz, txid.ToString(), wtxid.ToString());
        return false;
    }

    // We will return false if the tx already exists under a different peer.
    const bool brand_new{!HaveTx(wtxid)};

    auto ret = m_orphans.get<ByWtxid>().emplace(tx, peer, m_current_sequence);
    // If the announcement (same wtxid, same peer) already exists, emplacement fails. Return false.
    if (!ret.second) return false;

    ++m_current_sequence;
    auto& peer_info = m_peer_orphanage_info.try_emplace(peer).first->second;
    peer_info.Add(*ret.first);

    // Add links in m_outpoint_to_orphan_it
    if (brand_new) {
        for (const auto& input : tx->vin) {
            auto& wtxids_for_prevout = m_outpoint_to_orphan_it.try_emplace(input.prevout).first->second;
            wtxids_for_prevout.emplace(wtxid);
        }

        m_unique_orphans += 1;
        m_unique_orphan_bytes += ret.first->GetUsage();

        LogDebug(BCLog::TXPACKAGES, "stored orphan tx %s (wtxid=%s), weight: %u (mapsz %u outsz %u)\n",
                    txid.ToString(), wtxid.ToString(), sz, m_orphans.size(), m_outpoint_to_orphan_it.size());
        Assume(IsUnique(ret.first));
    } else {
        LogDebug(BCLog::TXPACKAGES, "added peer=%d as announcer of orphan tx %s (wtxid=%s)\n",
                    peer, txid.ToString(), wtxid.ToString());
        Assume(!IsUnique(ret.first));
    }
    return brand_new;
}

bool TxOrphanageImpl::AddAnnouncer(const Wtxid& wtxid, NodeId peer)
{
    auto it = m_orphans.get<ByWtxid>().lower_bound(ByWtxidView{wtxid, MIN_PEER});

    // Do nothing if this transaction isn't already present. We can't create an entry if we don't
    // have the tx data.
    if (it == m_orphans.end()) return false;
    if (it->m_tx->GetWitnessHash() != wtxid) return false;

    // Add another announcement, copying the CTransactionRef from one that already exists.
    const auto& ptx = it->m_tx;
    auto ret = m_orphans.get<ByWtxid>().emplace(ptx, peer, m_current_sequence);
    // If the announcement (same wtxid, same peer) already exists, emplacement fails. Return false.
    if (!ret.second) return false;

    ++m_current_sequence;
    auto& peer_info = m_peer_orphanage_info.try_emplace(peer).first->second;
    peer_info.Add(*ret.first);

    const auto& txid = ptx->GetHash();
    LogDebug(BCLog::TXPACKAGES, "added peer=%d as announcer of orphan tx %s (wtxid=%s)\n",
                peer, txid.ToString(), wtxid.ToString());

    Assume(!IsUnique(ret.first));
    return true;
}

bool TxOrphanageImpl::EraseTx(const Wtxid& wtxid)
{
    auto& index_by_wtxid = m_orphans.get<ByWtxid>();

    auto it = index_by_wtxid.lower_bound(ByWtxidView{wtxid, MIN_PEER});
    if (it == index_by_wtxid.end() || it->m_tx->GetWitnessHash() != wtxid) return false;

    auto it_end = index_by_wtxid.upper_bound(ByWtxidView{wtxid, MAX_PEER});
    unsigned int num_ann{0};
    const auto& txid = it->m_tx->GetHash();
    while (it != it_end) {
        Assume(it->m_tx->GetWitnessHash() == wtxid);
        Erase<ByWtxid>(it++);
        num_ann += 1;
    }

    LogDebug(BCLog::TXPACKAGES, "removed orphan tx %s (wtxid=%s) (%u announcements)\n", txid.ToString(), wtxid.ToString(), num_ann);

    return true;
}

/** Erase all entries by this peer. */
void TxOrphanageImpl::EraseForPeer(NodeId peer)
{
    auto& index_by_peer = m_orphans.get<ByPeer>();
    auto it = index_by_peer.lower_bound(ByPeerView{peer, false, 0});
    if (it == index_by_peer.end() || it->m_announcer != peer) return;

    unsigned int num_ann{0};
    while (it != index_by_peer.end() && it->m_announcer == peer) {
        // Delete item, cleaning up m_outpoint_to_orphan_it iff this entry is unique by wtxid.
        Erase<ByPeer>(it++);
        num_ann += 1;
    }
    Assume(!m_peer_orphanage_info.contains(peer));

    if (num_ann > 0) LogDebug(BCLog::TXPACKAGES, "Erased %d orphan transaction(s) from peer=%d\n", num_ann, peer);
}

/** If the data structure needs trimming, evicts announcements by selecting the DoSiest peer and evicting its oldest
 * announcement (sorting non-reconsiderable orphans first, to give reconsiderable orphans a greater chance of being
 * processed). Does nothing if no global limits are exceeded.  This eviction strategy effectively "reserves" an
 * amount of announcements and space for each peer. The reserved amount is protected from eviction even if there
 * are peers spamming the orphanage.
 */
void TxOrphanageImpl::LimitOrphans()
{
    if (!NeedsTrim()) return;

    const auto original_unique_txns{CountUniqueOrphans()};

    // Even though it's possible for MaxPeerAnnouncements to increase within this call to LimitOrphans
    // (e.g. if a peer's orphans are removed entirely, changing the number of peers), use consistent limits throughout.
    const auto max_ann{MaxPeerAnnouncements()};
    const auto max_mem{ReservedPeerUsage()};

    // We have exceeded the global limit(s). Now, identify who is using too much and evict their orphans.
    // Create a heap of pairs (NodeId, DoS score), sorted by descending DoS score.
    std::vector<std::pair<NodeId, FeeFrac>> heap_peer_dos;
    heap_peer_dos.reserve(m_peer_orphanage_info.size());
    for (const auto& [nodeid, entry] : m_peer_orphanage_info) {
        // Performance optimization: only consider peers with a DoS score >= 1.
        if (entry.GetDosScore(max_ann, max_mem) >= FeeFrac{1, 1}) {
            heap_peer_dos.emplace_back(nodeid, entry.GetDosScore(max_ann, max_mem));
        }
    }
    static constexpr auto compare_score = [](const auto& left, const auto& right) { return left.second < right.second; };
    std::make_heap(heap_peer_dos.begin(), heap_peer_dos.end(), compare_score);

    unsigned int num_erased{0};
    // This outer loop finds the peer with the highest DoS score, which is a fraction of {usage, announcements} used
    // over the respective allowances. We continue until the orphanage is within global limits. That means some peers
    // might still have a DoS score > 1 at the end.
    // Note: if ratios are the same, FeeFrac tiebreaks by denominator. In practice, since the CPU denominator (number of
    // announcements) is always lower, this means that a peer with only high number of announcements will be targeted
    // before a peer using a lot of memory, even if they have the same ratios.
    do {
        Assume(!heap_peer_dos.empty());
        // This is a max-heap, so the worst peer is at the front. pop_heap()
        // moves it to the back, and the next worst peer is moved to the front.
        std::pop_heap(heap_peer_dos.begin(), heap_peer_dos.end(), compare_score);
        const auto [worst_peer, dos_score] = std::move(heap_peer_dos.back());
        heap_peer_dos.pop_back();

        // If needs trim, then at least one peer has a DoS score higher than 1.
        Assume(dos_score.fee > dos_score.size);

        auto it_worst_peer = m_peer_orphanage_info.find(worst_peer);

        // This inner loop trims until this peer is no longer the DoSiest one or has a score within 1. The score 1 is
        // just a conservative fallback: once the last peer goes below ratio 1, NeedsTrim() will return false anyway.
        // We evict the oldest announcement(s) from this peer, sorting non-reconsiderable before reconsiderable.
        // The number of inner loop iterations is bounded by the total number of announcements.
        const auto& dos_threshold = heap_peer_dos.empty() ? FeeFrac{1, 1} : heap_peer_dos.front().second;
        auto it_ann = m_orphans.get<ByPeer>().lower_bound(ByPeerView{worst_peer, false, 0});
        while (NeedsTrim()) {
            if (!Assume(it_ann->m_announcer == worst_peer)) break;
            Erase<ByPeer>(it_ann++);
            num_erased += 1;

            // If we erased the last orphan from this peer, it_worst_peer will be invalidated.
            it_worst_peer = m_peer_orphanage_info.find(worst_peer);
            if (it_worst_peer == m_peer_orphanage_info.end() || it_worst_peer->second.GetDosScore(max_ann, max_mem) < dos_threshold) break;
        }

        if (!NeedsTrim()) break;

        // Unless this peer is empty, put it back in the heap so we continue to consider evicting its orphans.
        // We may select this peer for evictions again if there are multiple DoSy peers.
        if (it_worst_peer != m_peer_orphanage_info.end() && it_worst_peer->second.m_count_announcements > 0) {
            heap_peer_dos.emplace_back(worst_peer, it_worst_peer->second.GetDosScore(max_ann, max_mem));
            std::push_heap(heap_peer_dos.begin(), heap_peer_dos.end(), compare_score);
        }
    } while (true);

    const auto remaining_unique_orphans{CountUniqueOrphans()};
    LogDebug(BCLog::TXPACKAGES, "orphanage overflow, removed %u tx (%u announcements)\n", original_unique_txns - remaining_unique_orphans, num_erased);
}

void TxOrphanageImpl::AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng)
{
    auto& index_by_wtxid = m_orphans.get<ByWtxid>();
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const auto it_by_prev = m_outpoint_to_orphan_it.find(COutPoint(tx.GetHash(), i));
        if (it_by_prev != m_outpoint_to_orphan_it.end()) {
            for (const auto& wtxid : it_by_prev->second) {
                // Belt and suspenders, each entry in m_outpoint_to_orphan_it should always have at least 1 announcement.
                auto it = index_by_wtxid.lower_bound(ByWtxidView{wtxid, MIN_PEER});
                if (!Assume(it != index_by_wtxid.end())) continue;

                // Select a random peer to assign orphan processing, reducing wasted work if the orphan is still missing
                // inputs. However, we don't want to create an issue in which the assigned peer can purposefully stop us
                // from processing the orphan by disconnecting.
                auto it_end = index_by_wtxid.upper_bound(ByWtxidView{wtxid, MAX_PEER});
                const auto num_announcers{std::distance(it, it_end)};
                if (!Assume(num_announcers > 0)) continue;
                std::advance(it, rng.randrange(num_announcers));

                if (!Assume(it->m_tx->GetWitnessHash() == wtxid)) break;

                // Mark this orphan as ready to be reconsidered.
                static constexpr auto mark_reconsidered_modifier = [](auto& ann) { ann.m_reconsider = true; };
                m_orphans.get<ByWtxid>().modify(it, mark_reconsidered_modifier);

                LogDebug(BCLog::TXPACKAGES, "added %s (wtxid=%s) to peer %d workset\n",
                            it->m_tx->GetHash().ToString(), it->m_tx->GetWitnessHash().ToString(), it->m_announcer);
            }
        }
    }
}

bool TxOrphanageImpl::HaveTx(const Wtxid& wtxid) const
{
    auto it_lower = m_orphans.get<ByWtxid>().lower_bound(ByWtxidView{wtxid, MIN_PEER});
    return it_lower != m_orphans.end() && it_lower->m_tx->GetWitnessHash() == wtxid;
}

CTransactionRef TxOrphanageImpl::GetTx(const Wtxid& wtxid) const
{
    auto it_lower = m_orphans.get<ByWtxid>().lower_bound(ByWtxidView{wtxid, MIN_PEER});
    if (it_lower != m_orphans.end() && it_lower->m_tx->GetWitnessHash() == wtxid) return it_lower->m_tx;
    return nullptr;
}

bool TxOrphanageImpl::HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const
{
    return m_orphans.get<ByWtxid>().count(ByWtxidView{wtxid, peer}) > 0;
}

/** If there is a tx that can be reconsidered, return it and set it back to
 * non-reconsiderable. Otherwise, return a nullptr. */
CTransactionRef TxOrphanageImpl::GetTxToReconsider(NodeId peer)
{
    auto it = m_orphans.get<ByPeer>().lower_bound(ByPeerView{peer, true, 0});
    if (it != m_orphans.get<ByPeer>().end() && it->m_announcer == peer && it->m_reconsider) {
        // Flip m_reconsider. Even if this transaction stays in orphanage, it shouldn't be
        // reconsidered again until there is a new reason to do so.
        static constexpr auto mark_reconsidered_modifier = [](auto& ann) { ann.m_reconsider = false; };
        m_orphans.get<ByPeer>().modify(it, mark_reconsidered_modifier);
        return it->m_tx;
    }
    return nullptr;
}

/** Return whether there is a tx that can be reconsidered. */
bool TxOrphanageImpl::HaveTxToReconsider(NodeId peer)
{
    auto it = m_orphans.get<ByPeer>().lower_bound(ByPeerView{peer, true, 0});
    return it != m_orphans.get<ByPeer>().end() && it->m_announcer == peer && it->m_reconsider;
}
void TxOrphanageImpl::EraseForBlock(const CBlock& block)
{
    std::set<Wtxid> wtxids_to_erase;
    for (const CTransactionRef& ptx : block.vtx) {
        const CTransaction& block_tx = *ptx;

        // Which orphan pool entries must we evict?
        for (const auto& input : block_tx.vin) {
            auto it_prev = m_outpoint_to_orphan_it.find(input.prevout);
            if (it_prev != m_outpoint_to_orphan_it.end()) {
                // Copy all wtxids to wtxids_to_erase.
                std::copy(it_prev->second.cbegin(), it_prev->second.cend(), std::inserter(wtxids_to_erase, wtxids_to_erase.end()));
            }
        }
    }

    unsigned int num_erased{0};
    for (const auto& wtxid : wtxids_to_erase) {
        num_erased += EraseTx(wtxid);
    }

    if (num_erased != 0) {
        LogDebug(BCLog::TXPACKAGES, "Erased %d orphan transaction(s) included or conflicted by block\n", num_erased);
    }
    Assume(wtxids_to_erase.size() == num_erased);
}

/** Get all children that spend from this tx and were received from nodeid. Sorted from most
 * recent to least recent. */
std::vector<CTransactionRef> TxOrphanageImpl::GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId peer) const
{
    std::vector<CTransactionRef> children_found;
    const auto& parent_txid{parent->GetHash()};

    // Iterate through all orphans from this peer, in reverse order, so that more recent
    // transactions are added first. Doing so helps avoid work when one of the orphans replaced
    // an earlier one. Since we require the NodeId to match, one peer's announcement order does
    // not bias how we process other peer's orphans.
    auto& index_by_peer = m_orphans.get<ByPeer>();
    auto it_upper = index_by_peer.upper_bound(ByPeerView{peer, true, std::numeric_limits<uint64_t>::max()});
    auto it_lower = index_by_peer.lower_bound(ByPeerView{peer, false, 0});

    if (it_upper != index_by_peer.begin()) {
        for (auto rit = std::make_reverse_iterator(it_upper); rit != std::make_reverse_iterator(it_lower); ++rit) {
            if (!Assume(rit->m_announcer == peer)) break;
            // Check if this tx spends from parent.
            for (const auto& input : rit->m_tx->vin) {
                if (input.prevout.hash == parent_txid) {
                    children_found.emplace_back(rit->m_tx);
                    break;
                }
            }
        }
    }
    return children_found;
}

std::vector<TxOrphanage::OrphanTxBase> TxOrphanageImpl::GetOrphanTransactions() const
{
    std::vector<TxOrphanage::OrphanTxBase> result;
    result.reserve(m_unique_orphans);

    auto& index_by_wtxid = m_orphans.get<ByWtxid>();
    auto it = index_by_wtxid.begin();
    std::set<NodeId> this_orphan_announcers;
    while (it != index_by_wtxid.end()) {
        this_orphan_announcers.insert(it->m_announcer);
        // If this is the last entry, or the next entry has a different wtxid, build a OrphanTxBase.
        if (std::next(it) == index_by_wtxid.end() || std::next(it)->m_tx->GetWitnessHash() != it->m_tx->GetWitnessHash()) {
            result.emplace_back(it->m_tx, std::move(this_orphan_announcers));
            this_orphan_announcers.clear();
        }

        ++it;
    }
    Assume(m_unique_orphans == result.size());

    return result;
}

void TxOrphanageImpl::SanityCheck() const
{
    std::unordered_map<NodeId, PeerDoSInfo> reconstructed_peer_info;
    std::map<Wtxid, TxOrphanage::Usage> unique_wtxids_to_usage;
    std::set<COutPoint> all_outpoints;

    for (auto it = m_orphans.begin(); it != m_orphans.end(); ++it) {
        for (const auto& input : it->m_tx->vin) {
            all_outpoints.insert(input.prevout);
        }
        unique_wtxids_to_usage.emplace(it->m_tx->GetWitnessHash(), it->GetUsage());

        auto& peer_info = reconstructed_peer_info[it->m_announcer];
        peer_info.m_total_usage += it->GetUsage();
        peer_info.m_count_announcements += 1;
    }

    // Recalculated per-peer stats are identical to m_peer_orphanage_info
    assert(reconstructed_peer_info == m_peer_orphanage_info);

    // All outpoints exist in m_outpoint_to_orphan_it, all keys in m_outpoint_to_orphan_it correspond to some
    // orphan, and all wtxids referenced in m_outpoint_to_orphan_it are also in m_orphans.
    // This ensures m_outpoint_to_orphan_it is cleaned up.
    assert(all_outpoints.size() == m_outpoint_to_orphan_it.size());
    for (const auto& [outpoint, wtxid_set] : m_outpoint_to_orphan_it) {
        assert(all_outpoints.contains(outpoint));
        for (const auto& wtxid : wtxid_set) {
            assert(unique_wtxids_to_usage.contains(wtxid));
        }
    }

    // Cached m_unique_orphans value is correct.
    assert(m_orphans.size() >= m_unique_orphans);
    assert(unique_wtxids_to_usage.size() == m_unique_orphans);

    const auto calculated_dedup_usage = std::accumulate(unique_wtxids_to_usage.begin(), unique_wtxids_to_usage.end(),
        TxOrphanage::Usage{0}, [](TxOrphanage::Usage sum, const auto pair) { return sum + pair.second; });
    assert(calculated_dedup_usage == m_unique_orphan_bytes);
}

/** Constant, decided on initialization. */
TxOrphanage::Count TxOrphanageImpl::MaxGlobalAnnouncements() const { return m_max_global_announcements; }
/** Constant, decided on initialization. */
TxOrphanage::Usage TxOrphanageImpl::ReservedPeerUsage() const { return m_reserved_usage_per_peer; }
/** Dynamic based on number of peers. Each peer has an equal amount, but the global maximum number of announcements
 * stays constant. The number of peers times MaxPeerAnnouncements() (rounded) adds up to MaxGlobalAnnouncements().
 * As long as every peer's m_count_announcements / MaxPeerAnnouncements() < 1, MaxGlobalAnnouncements() is not
 * exceeded. */
TxOrphanage::Count TxOrphanageImpl::MaxPeerAnnouncements() const { return m_max_global_announcements / std::max<unsigned int>(m_peer_orphanage_info.size(), 1); }
/** Dynamic based on number of peers. More peers means more allowed memory usage. The number of peers times
 * ReservedPeerUsage() adds up to MaxGlobalUsage(). As long as every peer's m_total_usage / ReservedPeerUsage() < 1,
 * MaxGlobalUsage() is not exceeded. */
TxOrphanage::Usage TxOrphanageImpl::MaxGlobalUsage() const { return m_reserved_usage_per_peer * std::max<int64_t>(m_peer_orphanage_info.size(), 1); }

/** Returns whether global announcement or usage limits have been exceeded. */
bool TxOrphanageImpl::NeedsTrim() const
{
    return CountAnnouncements() > MaxGlobalAnnouncements() || TotalOrphanUsage() > MaxGlobalUsage();
}
std::unique_ptr<TxOrphanage> MakeTxOrphanage() noexcept
{
    return std::make_unique<TxOrphanageImpl>();
}
std::unique_ptr<TxOrphanage> MakeTxOrphanage(TxOrphanage::Count max_global_ann, TxOrphanage::Usage reserved_peer_usage) noexcept
{
    return std::make_unique<TxOrphanageImpl>(max_global_ann, reserved_peer_usage);
}
} // namespace node
