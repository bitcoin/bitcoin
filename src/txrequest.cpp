// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txrequest.h>

#include <crypto/siphash.h>
#include <net.h>
#include <random.h>
#include <uint256.h>

#include <chrono>
#include <utility>

#include <assert.h>

PriorityComputer::PriorityComputer(bool deterministic) :
    m_k0{deterministic ? 0 : GetRand(0xFFFFFFFFFFFFFFFF)},
    m_k1{deterministic ? 0 : GetRand(0xFFFFFFFFFFFFFFFF)} {}

TxRequestTracker::TxRequestTracker(std::chrono::microseconds timeout, bool deterministic) :
    m_timeout(timeout),
    m_computer(deterministic),
    // Explicitly initialize m_index as we need to pass a reference to m_computer to
    // EntryTxidExtractor.
    m_index(boost::make_tuple(
        boost::make_tuple(
            boost::multi_index::const_mem_fun<Entry, EntryPeer, &Entry::ExtractPeer>(),
            std::less<EntryPeer>()
        ),
        boost::make_tuple(
            EntryTxidExtractor(m_computer),
            std::less<EntryTxid>()
        ),
        boost::make_tuple(
            boost::multi_index::const_mem_fun<Entry, EntryTime, &Entry::ExtractTime>(),
            std::less<EntryTime>()
        )
    )) {}

void TxRequestTracker::PromoteCandidateNew(typename TxRequestTracker::Index::index<ByTxid>::type::iterator it)
{
    assert(it->GetState() == State::CANDIDATE_DELAYED);
    // Convert CANDIDATE_DELAYED to CANDIDATE_READY first.
    Modify<ByTxid>(it, [](Entry& entry){ entry.SetState(State::CANDIDATE_READY); });
    // The following code relies on the fact that the ByTxid is sorted by txid, and then by state (first _DELAYED,
    // then _BEST/REQUESTED, then _READY). Within the _READY entries, the best one (lowest priority) comes first.
    // Thus, if an existing _BEST exists for the same txid that this entry may be preferred over, it must immediately
    // precede the newly created _READY.
    if (it == m_index.get<ByTxid>().begin() || std::prev(it)->m_txid != it->m_txid ||
        std::prev(it)->GetState() == State::CANDIDATE_DELAYED) {
        // This is the new best CANDIDATE_READY, and there is no IsSelected() entry for this txid already.
        Modify<ByTxid>(it, [](Entry& entry){ entry.SetState(State::CANDIDATE_BEST); });
    } else if (std::prev(it)->GetState() == State::CANDIDATE_BEST) {
        uint64_t priority_old = std::prev(it)->ComputePriority(m_computer);
        uint64_t priority_new = it->ComputePriority(m_computer);
        if (priority_new < priority_old) {
            // There is a CANDIDATE_BEST entry already, but this one is better.
            Modify<ByTxid>(std::prev(it), [](Entry& entry){ entry.SetState(State::CANDIDATE_READY); });
            Modify<ByTxid>(it, [](Entry& entry){ entry.SetState(State::CANDIDATE_BEST); });
        }
    }
}

void TxRequestTracker::ChangeAndReselect(typename TxRequestTracker::Index::index<ByTxid>::type::iterator it,
    TxRequestTracker::State new_state)
{
    if (it->IsSelected()) {
        auto it_next = std::next(it);
        // The next best CANDIDATE_READY, if any, immediately follows the REQUESTED or CANDIDATE_BEST entry in the
        // ByTxid index.
        if (it_next != m_index.get<ByTxid>().end() && it_next->m_txid == it->m_txid &&
            it_next->GetState() == State::CANDIDATE_READY) {
            // If one such CANDIDATE_READY exists (for this txid), convert it to CANDIDATE_BEST.
            Modify<ByTxid>(it_next, [](Entry& entry){ entry.SetState(State::CANDIDATE_BEST); });
        }
    }
    Modify<ByTxid>(it, [new_state](Entry& entry){ entry.SetState(new_state); });
    assert(!it->IsSelected());
}

bool TxRequestTracker::MakeCompleted(typename TxRequestTracker::Index::index<ByTxid>::type::iterator it)
{
    // Nothing to be done if it's already COMPLETED.
    if (it->GetState() == State::COMPLETED) return true;

    if ((it == m_index.get<ByTxid>().begin() || std::prev(it)->m_txid != it->m_txid) &&
        (std::next(it) == m_index.get<ByTxid>().end() || std::next(it)->m_txid != it->m_txid ||
        std::next(it)->GetState() == State::COMPLETED)) {
        // This is the first entry for this txid, and the last non-COMPLETED one. There are only COMPLETED ones left.
        // Delete them all.
        uint256 txid = it->m_txid;
        do {
            it = Erase<ByTxid>(it);
        } while (it != m_index.get<ByTxid>().end() && it->m_txid == txid);
        return false;
    }

    // Mark the entry COMPLETED, and select the best best entry if needed.
    ChangeAndReselect(it, State::COMPLETED);

    return true;
}

void TxRequestTracker::SetTimePoint(std::chrono::microseconds now)
{
    // Iterate over all CANDIDATE_DELAYED and REQUESTED from old to new, as long as they're in the past,
    // and convert them to CANDIDATE_READY and COMPLETED respectively.
    while (!m_index.empty()) {
        auto it = m_index.get<ByTime>().begin();
        if (it->GetState() == State::CANDIDATE_DELAYED && it->m_time <= now) {
            PromoteCandidateNew(m_index.project<ByTxid>(it));
        } else if (it->GetState() == State::REQUESTED && it->m_time <= now) {
            MakeCompleted(m_index.project<ByTxid>(it));
        } else {
            break;
        }
    }

    while (!m_index.empty()) {
        // If time went backwards, we may need to demote CANDIDATE_BEST and CANDIDATE_READY entries back
        // to CANDIDATE_DELAYED. This is an unusual edge case, and unlikely to matter in production. However,
        // it makes it much easier to specify and test TxRequestTracker's behaviour.
        auto it = std::prev(m_index.get<ByTime>().end());
        if (it->IsSelectable() && it->m_time > now) {
            ChangeAndReselect(m_index.project<ByTxid>(it), State::CANDIDATE_DELAYED);
        } else {
            break;
        }
    }
}

void TxRequestTracker::AlreadyHaveTx(const uint256& txid)
{
    auto it = m_index.get<ByTxid>().lower_bound(EntryTxid{txid, State::CANDIDATE_DELAYED, 0});
    while (it != m_index.get<ByTxid>().end() && it->m_txid == txid) {
        it = Erase<ByTxid>(it);
    }
}

static const uint256 UINT256_ZERO;

void TxRequestTracker::DeletedPeer(uint64_t peer)
{
    auto& index = m_index.get<ByPeer>();
    auto it = index.lower_bound(EntryPeer{peer, false, UINT256_ZERO});
    while (it != index.end() && it->m_peer == peer) {
        // Check what to continue with after this iteration. Note that 'it' may change position, and std::next(it)
        // may be deleted in the process, so this needs to be decided beforehand.
        auto it_next = (std::next(it) == index.end() || std::next(it)->m_peer != peer) ? index.end() : std::next(it);
        // If the entry isn't already COMPLETED, first make it COMPLETED (which will mark other CANDIDATEs as
        // CANDIDATE_BEST, or delete all of a txid's entries if no non-COMPLETED ones are left).
        if (MakeCompleted(m_index.project<ByTxid>(it))) {
            // Then actually delete the entry (unless it was already deleted by MakeCompleted).
            Erase<ByPeer>(it);
        }
        it = it_next;
    }
}

void TxRequestTracker::ReceivedInv(uint64_t peer, const uint256& txid, std::chrono::microseconds reqtime)
{
    // Bail out if we already have a CANDIDATE_BEST entry for this (txid, peer) combination. The case where there is
    // a non-CANDIDATE_BEST entry already will be caught by the uniqueness property of the ByPeer index
    // automatically.
    if (m_index.get<ByPeer>().count(EntryPeer{peer, true, txid})) return;

    auto ret = m_index.get<ByPeer>().emplace(txid, peer, reqtime, m_sequence);
    if (ret.second) {
        auto it = m_index.project<ByTxid>(ret.first);
        ++m_peerinfo[peer].m_total;
        ++m_sequence;
        if ((it == m_index.get<ByTxid>().begin() || std::prev(it)->m_txid != txid) &&
            (std::next(it) == m_index.get<ByTxid>().end() || std::next(it)->m_txid != txid)) {
            // This is both the first and the last entry for a given txid; set its m_first.
            ret.first->m_first = true;
        }
    }
}

void TxRequestTracker::RequestedTx(uint64_t peer, const uint256& txid, std::chrono::microseconds now)
{
    auto it = m_index.get<ByPeer>().find(EntryPeer{peer, true, txid});
    // RequestedTx can only be called on CANDIDATE_BEST entries (this is implied by its condition that it can only be
    // called on txids returned by GetRequestable (and only AlreadyHave and RequestedTx can be called in between,
    // which preserve the state of other txids).
    assert(it != m_index.get<ByPeer>().end());
    assert(it->GetState() == State::CANDIDATE_BEST);
    Modify<ByPeer>(it, [now,this](Entry& entry) {
        entry.SetState(State::REQUESTED);
        entry.m_time = now + m_timeout;
    });
}

void TxRequestTracker::ReceivedResponse(uint64_t peer, const uint256& txid)
{
    // We need to search the ByPeer index for both (peer, false, txid) and (peer, true, txid).
    auto it = m_index.get<ByPeer>().find(EntryPeer{peer, false, txid});
    if (it == m_index.get<ByPeer>().end()) it = m_index.get<ByPeer>().find(EntryPeer{peer, true, txid});
    if (it != m_index.get<ByPeer>().end()) MakeCompleted(m_index.project<ByTxid>(it));
}

size_t TxRequestTracker::CountInFlight(uint64_t peer) const
{
    auto it = m_peerinfo.find(peer);
    if (it != m_peerinfo.end()) return it->second.m_requested;
    return 0;
}

size_t TxRequestTracker::CountTracked(uint64_t peer) const
{
    auto it = m_peerinfo.find(peer);
    if (it != m_peerinfo.end()) return it->second.m_total;
    return 0;
}

std::vector<uint256> TxRequestTracker::GetRequestable(uint64_t peer, std::chrono::microseconds now)
{
    // Move time.
    SetTimePoint(now);

    // Find all CANDIDATE_BEST entries for this peer.
    std::vector<std::pair<uint64_t, const uint256*>> selected;
    auto it_peer = m_index.get<ByPeer>().lower_bound(EntryPeer{peer, true, UINT256_ZERO});
    while (it_peer != m_index.get<ByPeer>().end() && it_peer->m_peer == peer &&
        it_peer->GetState() == State::CANDIDATE_BEST) {
        selected.emplace_back(it_peer->m_sequence, &it_peer->m_txid);
        ++it_peer;
    }

    // Return them, sorted by sequence number.
    std::sort(selected.begin(), selected.end());
    std::vector<uint256> ret;
    for (auto& item : selected) {
        ret.push_back(*item.second);
    }
    return ret;
}
