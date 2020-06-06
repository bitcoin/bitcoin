// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXREQUEST_H
#define BITCOIN_TXREQUEST_H

#include <crypto/siphash.h>
#include <uint256.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <chrono>
#include <unordered_map>
#include <vector>

#include <stdint.h>


/** A functor with embedded salt that computes priority of a txid/peer combination.
 *
 * The priority is used to order candidates for a given txid; lower priority values are selected first. It has the
 * following properties:
 * - Entries are called delayed if they have a reqtime different from std::chrono::microseconds::min(). This is the
 *   case for entries from inbound non-whitelisted nodes. The priority for delayed entries is always higher than
 *   non-delayed ones, so non-delayed ones are always preferred.
 * - Within the set of delayed peers and within the set of non-delayed peers:
 *   - The very first announcement for a given txid is always lowest.
 *   - Announcements after the first one are ordered uniformly randomly (using a salted hash of txid/peer).
 */
class PriorityComputer {
    const uint64_t m_k0, m_k1;
public:
    explicit PriorityComputer(bool deterministic);
    uint64_t operator()(const uint256& txid, uint64_t peer, bool delayed, bool first) const
    {
        uint64_t low_bits = 0;
        if (!first) {
            low_bits = CSipHasher(m_k0, m_k1).Write(txid.begin(), txid.size()).Write(peer).Finalize() >> 1;
        }
        return low_bits | uint64_t{delayed} << 63;
    }
};

/** Data structure to keep track of, and schedule, transaction downloads from peers.
 *
 * === High level behavior ===
 *
 * Transaction downloads are attempted in random order, first choosing among non-delayed peers, then delayed
 * ones whose delay has passed. When a NOTFOUND is received, an invalid transaction is received, or a download
 * times out (after a configurable delay), the next candidate is immediately scheduled according to the same rules
 * above. The same transaction is never re-requested from the same peer, unless the transaction was forgotten about
 * in the mean time. This happens whenever no candidates remain, or when a transaction is successfully received.
 *
 *
 * === Specification ===
 *
 * The data structure maintains a collection of entries:
 *
 * - CANDIDATE (txid, peer, reqtime) entries representing txids that were announced by peer, and become available for
 *   download after reqtime has passed. The reqtime can std::chrono::microseconds::min() for immediate entries,
 *   or some timestamp in the future for delayed ones.
 *
 * - REQUESTED (txid, peer, exptime) entries representing txids that have been requested, and we're awaiting a
 *   response for from that peer. The exptime value determines when the request times out.
 *
 * - COMPLETED (txid, peer) entries representing txids that have been requested from a peer, but they timed out, a
 *   NOTFOUND message was received for them, or an invalid response was received. They're only kept around to prevent
 *   downloading them again. If only COMPLETED entries exist for a given txid remain (so no CANDIDATE or REQUESTED
 *   ones), all of them are deleted (this is an invariant, and maintained by all operations below).
 *
 * The following operations are supported on this data structure:
 *
 * - ReceivedInv(txid, peer) adds a new CANDIDATE entry, unless one already exists for that (txid, peer) combination
 *   (whether it's CANDIDATE, REQUESTED, or COMPLETED).
 *
 * - DeletedPeer(peer) deletes all entries for a given peer. It should be called when a peer goes offline.
 *
 * - AlreadyHaveTx(txid) deletes all entries for a given txid. It should be called when a transaction is successfully
 *   added to the mempool, seen in a block, or for whatever reason we no longer care about it.
 *
 * - ReceivedResponse(peer, txid) converts any CANDIDATE or REQUESTED entry to a COMPLETED one. It should be called
 *   whenever a transaction or NOTFOUND was received from a peer. When the transaction is acceptable, AlreadyHaveTx
 *   should be called instead of (or in addition to) this operation.
 *
 * - GetRequestable(peer) does the following:
 *   - Convert all REQUESTED entries (for all txids/peers) with (exptime <= now) to COMPLETED entries.
 *   - Requestable txids are selected: txids which have no REQUESTED entry, and for which this peer has the
 *     lowest-priority CANDIDATE entry among all CANDIDATEs with (reqtime <= now) at the time of the GetRequestable
 *     call. As the high bit of the priority is whether or not the peer had a delay, immediate entries will always be
 *     picked first as long as one is available.
 *   - The selected txids are sorted in order of announcement and returned (even if multiple were added at the same
 *     time, or even when the clock went backwards while they were being added).
 *
 * - RequestedTx(peer, txid) converts the CANDIDATE entry for the provided peer and txid into a REQUESTED one, with
 *   exptime set to (now + timeout). It can ONLY be called immediately after GetRequestable was called (for the same
 *   peer), with only AlreadyHaveTx and other RequestedTx calls (both for other txids) in between. Any other
 *   non-const operation removes the ability to call RequestedTx.
 */
class TxRequestTracker {
    //! Configuration option: timeout after requesting a download.
    const std::chrono::microseconds m_timeout;

    //! The various states a (txid,node) pair can be in.
    //! Note that CANDIDATE is split up into 3 substates (DELAYED, BEST, READY), allowing more efficient implementation.
    //! Also note that the sorting order of EntryTxid relies on the specific order of values in this enum.
    enum class State : uint8_t {
        //! A CANDIDATE entry whose reqtime is in the future.
        CANDIDATE_DELAYED,
        //! The best CANDIDATE for a given txid; only if there is no REQUESTED entry already for that txid.
        //! The CANDIDATE_BEST is the lowest-priority entry among all CANDIDATE_READY (and _BEST) ones for that txid.
        CANDIDATE_BEST,
        //! A REQUESTED entry.
        REQUESTED,
        //! A CANDIDATE entry that's not CANDIDATE_DELAYED or CANDIDATE_BEST.
        CANDIDATE_READY,
        //! A COMPLETED entry.
        COMPLETED,
    };

    //! Tag for the EntryPeer-based index.
    struct ByPeer {};
    //! Tag for the EntryTxid-based index.
    struct ByTxid {};
    //! Tag for the EntryTime-based index.
    struct ByTime {};

    //! The ByPeer index is sorted by (peer, state == CANDIDATE_BEST, txid)
    using EntryPeer = std::tuple<uint64_t, bool, const uint256&>;

    //! The ByTxid index is sorted by (txid, state, priority [CANDIDATE_READY]; 0 [otherwise])
    using EntryTxid = std::tuple<const uint256&, State, uint64_t>;

    //! The ByTime index is sorted by (0 [CANDIDATE_DELAYED,REQUESTED]; 1 [COMPLETED];
    // 2 [CANDIDATE_READY,CANDIDATE_BEST], time)
    using EntryTime = std::pair<int, std::chrono::microseconds>;

    //! The current sequence number. Increases for every announcement. This is used to sort txids returned by
    //! GetRequestable in announcement order.
    uint64_t m_sequence{0};

    //! An announcement entry.
    struct Entry {
        //! Txid that was announced.
        const uint256 m_txid;
        //! For CANDIDATE_{DELAYED,BEST,READY} the reqtime; for REQUESTED the exptime
        std::chrono::microseconds m_time;
        //! What peer the request was from.
        const uint64_t m_peer;
        //! What sequence number this announcement has.
        const uint64_t m_sequence : 59;
        //! Whether the request was delayed (which deprioritizes it, even after reqtime has passed).
        const bool m_delayed : 1;
        //! What state this announcement is in
        //! This is a uint8_t instead of a State to silence a GCC warning.
        uint8_t m_state : 3;
        //! Whether this was the very first announcement for this txid.
        //! It's only modified right after insertion, at which point it doesn't affect position
        //! in any of the indexes. Make it mutable to enable changing without triggering reindex.
        mutable bool m_first : 1;

        //! Convert the m_state variable to a State enum.
        State GetState() const { return State(m_state); }
        //! Convert a State to a uint8_t and store it in m_state.
        void SetState(State state) { m_state = uint8_t(state); }

        //! Whether this entry is selected. There can be at most 1 selected peer per txid.
        bool IsSelected() const
        {
            return GetState() == State::CANDIDATE_BEST || GetState() == State::REQUESTED;
        }

        //! Whether this entry is waiting for a certain time to pass.
        bool IsWaiting() const
        {
            return GetState() == State::REQUESTED || GetState() == State::CANDIDATE_DELAYED;
        }

        //! Whether this entry can feasibly be selected if the current IsSelected() one disappears.
        bool IsSelectable() const
        {
            return GetState() == State::CANDIDATE_READY || GetState() == State::CANDIDATE_BEST;
        }

        //! Construct a new entry from scratch
        Entry(const uint256& txid, uint64_t peer, std::chrono::microseconds reqtime, uint64_t sequence) :
            m_txid(txid), m_time(reqtime), m_peer(peer), m_sequence(sequence),
            m_delayed(reqtime != std::chrono::microseconds::min()), m_state(uint8_t(State::CANDIDATE_DELAYED)),
            m_first(false) {}

        //! Compute this Entry's priority.
        uint64_t ComputePriority(const PriorityComputer& computer) const
        {
            return computer(m_txid, m_peer, m_delayed, m_first);
        }

        //! Extract the EntryPeer from this Entry.
        EntryPeer ExtractPeer() const { return EntryPeer{m_peer, GetState() == State::CANDIDATE_BEST, m_txid}; }

        //! Extract the EntryTxid from this Entry.
        EntryTxid ExtractTxid(const PriorityComputer& computer) const
        {
            return EntryTxid{m_txid, GetState(), GetState() == State::CANDIDATE_READY ? ComputePriority(computer) : 0};
        }

        //! Extract the EntryTime from this Entry.
        EntryTime ExtractTime() const { return EntryTime{IsWaiting() ? 0 : IsSelectable() ? 2 : 1, m_time}; }
    };

    //! This tracker's priority computer.
    const PriorityComputer m_computer;

    /** An extractor for EntryTxids (with encapsulated PriorityComputer reference).
     *
     * See https://www.boost.org/doc/libs/1_54_0/libs/multi_index/doc/reference/key_extraction.html#key_extractors
     * for more information about the key extraction concept.
     */
    struct EntryTxidExtractor {
    private:
        const PriorityComputer& m_computer;
    public:
        EntryTxidExtractor(const PriorityComputer& computer) : m_computer(computer) {}
        using result_type = EntryTxid; // Needed to comply with key extractor interface
        result_type operator()(const Entry& entry) const { return entry.ExtractTxid(m_computer); }
    };

    //! Data type for the main data structure (Entry objects with ByPeer/ByTxid/ByTime indexes).
    using Index = boost::multi_index_container<
        Entry,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByPeer>,
                boost::multi_index::const_mem_fun<Entry, EntryPeer, &Entry::ExtractPeer>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByTxid>,
                EntryTxidExtractor
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByTime>,
                boost::multi_index::const_mem_fun<Entry, EntryTime, &Entry::ExtractTime>
            >
        >
    >;

    //! This tracker's main data structure.
    Index m_index;

    //! Per-peer statistics object.
    struct PeerInfo {
        size_t m_total = 0; //!< Total number of entries for this peer.
        size_t m_requested = 0; //!< Total number of REQUESTED entries for this peer.

        friend bool operator==(const PeerInfo& a, const PeerInfo& b)
        {
            return std::tie(a.m_total, a.m_requested) == std::tie(b.m_total, b.m_requested);
        }
    };

    //! Map with this tracker's per-peer statistics.
    std::unordered_map<uint64_t, PeerInfo> m_peerinfo;

    //! Wrapper around Index::...::erase that keeps m_peerinfo up to date.
    template<typename Tag>
    typename Index::index<Tag>::type::iterator Erase(typename Index::index<Tag>::type::iterator it)
    {
        auto peerit = m_peerinfo.find(it->m_peer);
        peerit->second.m_requested -= it->GetState() == State::REQUESTED;
        if (--peerit->second.m_total == 0) m_peerinfo.erase(peerit);
        return m_index.get<Tag>().erase(it);
    }

    //! Wrapper around Index::...::modify that keeps m_peerinfo up to date.
    template<typename Tag, typename Modifier>
    void Modify(typename Index::index<Tag>::type::iterator it, Modifier modifier)
    {
        auto peerit = m_peerinfo.find(it->m_peer);
        peerit->second.m_requested -= it->GetState() == State::REQUESTED;
        m_index.get<Tag>().modify(it, std::move(modifier));
        peerit->second.m_requested += it->GetState() == State::REQUESTED;
    }

    //! Change the state of an entry to something non-IsSelected(). If it was IsSelected(), the next best entry will
    //! be marked CANDIDATE_BEST.
    void ChangeAndReselect(typename Index::index<ByTxid>::type::iterator it, State new_state);

    //! Convert a CANDIDATE_DELAYED entry into a CANDIDATE_READY. If this makes it the new best CANDIDATE_READY (and no
    //! REQUESTED exists) and better than the CANDIDATE_BEST (if any), it becomes the new CANDIDATE_BEST.
    void PromoteCandidateNew(typename Index::index<ByTxid>::type::iterator it);

    //! Convert any entry to a COMPLETED one. If there are no non-COMPLETED entries left for this txid, they are all
    //! deleted. If this was a REQUESTED entry, and there are other CANDIDATEs left, the best one is made
    //! CANDIDATE_BEST. Returns whether the Entry still exists.
    bool MakeCompleted(typename Index::index<ByTxid>::type::iterator it);

    //! Make the data structure consistent with a given point in time:
    //! - REQUESTED entries with exptime <= now are turned into COMPLETED.
    //! - CANDIDATE_DELAYED entries with reqtime <= now are turned into CANDIDATE_{READY,BEST}.
    //! - CANDIDATE_{READY,BEST} entries with reqtime > now are turned into CANDIDATE_DELAYED.
    void SetTimePoint(std::chrono::microseconds now);

public:
    //! Construct a TxRequestTracker with the specified parameters.
    TxRequestTracker(std::chrono::microseconds timeout, bool deterministic = false);

    // Disable copying and assigning (a default copy won't work due the stateful EntryTxidExtractor).
    TxRequestTracker(const TxRequestTracker&) = delete;
    TxRequestTracker& operator=(const TxRequestTracker&) = delete;

    //! A peer went offline, delete any data related to it.
    void DeletedPeer(uint64_t uint64_t);

    //! For whatever reason, we no longer need this txid. Delete any data related to it.
    void AlreadyHaveTx(const uint256& txid);

    //! We received a new inv, enter it into the data structure.
    void ReceivedInv(uint64_t peer, const uint256& txid, std::chrono::microseconds reqtime);

    //! Find the txids to request now from peer.
    std::vector<uint256> GetRequestable(uint64_t peer, std::chrono::microseconds now);

    //! Inform the data structure that a txid was requested. This can only be called for txids returned by the last
    //! GetRequestable call (which must have been for the same peer), with only other RequestedTx and AlreadyHaveTx
    //! calls in between (which must have been for the same peer but different txids).
    void RequestedTx(uint64_t peer, const uint256& txid, std::chrono::microseconds now);

    //! We received a response (a tx, or a NOTFOUND) for txid from peer. Note that if a good tx is received (such
    //! that we don't need it anymore), AlreadyHaveTx should be called instead of (or in addition to)
    //! ReceivedResponse.
    void ReceivedResponse(uint64_t peer, const uint256& txid);

    //! Count how many in-flight transactions a peer has.
    size_t CountInFlight(uint64_t peer) const;

    //! Count how many transactions are being tracked for a peer (including timed-out ones and in-flight ones).
    size_t CountTracked(uint64_t peer) const;

    //! Count how many announcements are being tracked in total across all peers and transactions.
    size_t Size() const { return m_index.size(); }

    //! Query configuration parameter timeout.
    std::chrono::microseconds GetTimeout() const { return m_timeout; }
};

#endif // BITCOIN_TXREQUEST_H
