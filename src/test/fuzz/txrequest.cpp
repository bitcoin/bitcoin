// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <txrequest.h>
#include <test/fuzz/fuzz.h>
#include <crypto/common.h>
#include <crypto/siphash.h>

#include <bitset>
#include <cstdint>
#include <queue>
#include <vector>

namespace {

static constexpr int MAX_TXIDS = 16;
static constexpr int MAX_PEERS = 16;

//! Randomly generated TXIDs used in this test.
uint256 TXIDS[MAX_TXIDS];

/** Precomputed random timeouts/delays (~exponentially distributed). */
std::chrono::microseconds DELAYS[256];

struct Initializer
{
    Initializer()
    {
        // Use deterministic RNG to fill in txids and delays.
        // Non-determinism hurts fuzzing.
        FastRandomContext rng(true);
        for (int txidnum = 0; txidnum < MAX_TXIDS; ++txidnum) {
            TXIDS[txidnum] = rng.rand256();
        }
        for (int i = 0; i < 16; ++i) {
            DELAYS[i] = std::chrono::microseconds{1 + i};
        }
        for (int i = 16; i < 256; ++i) {
            DELAYS[i] = DELAYS[i - 1] + std::chrono::microseconds{1 + rng.randbits(i / 10)};
        }
    }
} g_initializer;

/** Tester class for TxRequestTracker
 *
 * It includes a naive reimplementation of its behavior, for a limited set
 * of MAX_TXIDS distinct txids, and MAX_PEERS peer identifiers.
 *
 * All of the public member functions perform the same operation on
 * an actual TxRequestTracker and on the state of the reimplementation.
 * The output of GetRequestable is compared with the expected value
 * as well.
 *
 * Check() calls the TxRequestTracker's sanity check, plus compares the
 * output of the constant accessors (Size(), CountInFlight(), CountTracked())
 * with expected values.
 */
class Tester
{
    //! TxRequestTracker object being tested.
    TxRequestTracker m_tracker;

    //! States for txid/peer combinations in the naive data structure.
    enum class State {
        NOTHING, //!< Absence of this txid/peer combination

        // Note that this implementation does not distinguish between BEST/NEW/OTHER variants of CANDIDATE.
        CANDIDATE,
        REQUESTED,
        COMPLETED,
    };

    //! Sequence numbers, incremented whenever a new CANDIDATE is added.
    uint64_t m_sequence{0};

    //! List of future 'events' (all inserted reqtimes/exptimes). This is used to implement AdvanceToEvent.
    std::priority_queue<std::chrono::microseconds, std::vector<std::chrono::microseconds>, std::greater<std::chrono::microseconds>> m_events;

    //! Information about a txid/peer combination.
    struct Entry
    {
        std::chrono::microseconds m_time;
        uint64_t m_sequence;
        State m_state{State::NOTHING};
        bool m_delayed;
        bool m_first;
    };

    //! Information about all txid/peer combination.
    Entry m_entries[MAX_TXIDS][MAX_PEERS];

    //! The current time; can move forward and backward.
    std::chrono::microseconds m_now{112223333};

    //! The last peer we've called GetRequestable for, or -1 if none.
    //! Also reset to -1 whenever an operation is performed that removes the ability to call RequestedTx.
    int m_get_requestable_last_peer = -1;

    //! The txidnums returned by the last GetRequestable, which AlreadyHaveTx or RequestedTx haven't been called on yet.
    std::bitset<MAX_TXIDS> m_get_requestable_last_result;

    //! Check if there are any entries for txidnum (including COMPLETED ones)
    bool HaveAny(int txidnum)
    {
        for (int peer = 0; peer < MAX_PEERS; ++peer) {
            if (m_entries[txidnum][peer].m_state != State::NOTHING) return true;
        }
        return false;
    }

    //! Delete txids whose only entries are COMPLETED.
    void Cleanup(int txidnum)
    {
        bool all_nothing = true;
        for (int peer = 0; peer < MAX_PEERS; ++peer) {
            const Entry& entry = m_entries[txidnum][peer];
            if (entry.m_state == State::CANDIDATE || entry.m_state == State::REQUESTED) return;
            if (entry.m_state != State::NOTHING) all_nothing = false;
        }
        if (all_nothing) return;
        for (int peer = 0; peer < MAX_PEERS; ++peer) {
            m_entries[txidnum][peer].m_state = State::NOTHING;
        }
    }

    //! Find the current best peer to request from for a txid (or -1 if none).
    int GetSelected(int txidnum) const
    {
        int ret = -1;
        uint64_t ret_priority = 0;
        for (int peer = 0; peer < MAX_PEERS; ++peer) {
            // Return -1 if there already is a (non-expired) in-flight request.
            if (m_entries[txidnum][peer].m_state == State::REQUESTED) return -1;
            // If it's a viable candidate, see if it has lower priority than the best one so far.
            if (m_entries[txidnum][peer].m_state == State::CANDIDATE && m_entries[txidnum][peer].m_time <= m_now) {
                uint64_t peer_priority = m_tracker.GetPriorityComputer()(TXIDS[txidnum], peer, m_entries[txidnum][peer].m_delayed, m_entries[txidnum][peer].m_first);
                if (ret == -1 || peer_priority < ret_priority) std::tie(ret, ret_priority) = std::tie(peer, peer_priority);
            }
        }
        return ret;
    }

public:
    Tester(std::chrono::microseconds timeout) : m_tracker(timeout, true)
    {
        assert(m_tracker.GetTimeout() == timeout);
    }

    std::chrono::microseconds Now() const { return m_now; }

    void AdvanceTime(std::chrono::microseconds offset)
    {
        m_now += offset;
        while (!m_events.empty() && m_events.top() <= m_now) m_events.pop();
    }

    void AdvanceToEvent()
    {
        while (!m_events.empty() && m_events.top() <= m_now) m_events.pop();
        if (!m_events.empty()) {
            m_now = m_events.top();
            m_events.pop();
        }
    }

    void DeletedPeer(int peer)
    {
        // Removes the ability to call RequestedTx until the next GetRequestable.
        m_get_requestable_last_peer = -1;

        // Apply to naive structure: all entries for that peer are wiped.
        for (int txidnum = 0; txidnum < MAX_TXIDS; ++txidnum) {
            if (m_entries[txidnum][peer].m_state != State::NOTHING) {
                m_entries[txidnum][peer].m_state = State::NOTHING;
                Cleanup(txidnum);
            }
        }

        // Call TxRequestTracker's implementation.
        m_tracker.DeletedPeer(peer);
    }

    void AlreadyHaveTx(int txidnum)
    {
        // RequestedTx cannot be called on this txidnum anymore.
        m_get_requestable_last_result.reset(txidnum);

        // Apply to naive structure: all entries for that txid are wiped.
        for (int peer = 0; peer < MAX_PEERS; ++peer) {
            m_entries[txidnum][peer].m_state = State::NOTHING;
        }
        // No need to call Cleanup(txidnum); it's already all NOTHING.

        // Call TxRequestTracker's implementation.
        m_tracker.AlreadyHaveTx(TXIDS[txidnum]);
    }

    void ReceivedInv(int peer, int txidnum, std::chrono::microseconds reqtime)
    {
        // Removes the ability to call RequestedTx until the next GetRequestable.
        m_get_requestable_last_peer = -1;

        // Apply to naive structure: if no entry for txidnum/peer combination
        // already, create a new CANDIDATE; otherwise do nothing.
        Entry& entry = m_entries[txidnum][peer];
        if (entry.m_state == State::NOTHING) {
            entry.m_first = !HaveAny(txidnum);
            entry.m_delayed = reqtime != std::chrono::microseconds::min();
            entry.m_state = State::CANDIDATE;
            entry.m_time = reqtime;
            entry.m_sequence = m_sequence++;

            // Add event so that AdvanceToEvent can quickly jump to the point where its reqtime passes.
            if (reqtime > m_now) m_events.push(reqtime);
        }

        // Call TxRequestTracker's implementation.
        m_tracker.ReceivedInv(peer, TXIDS[txidnum], reqtime);
    }

    void RequestedTx(int txidnum)
    {
        // Must be called with a txid that was returned by GetRequestable
        int peer = m_get_requestable_last_peer;
        if (peer == -1) return;
        if (!m_get_requestable_last_result[txidnum]) return;
        m_get_requestable_last_result.reset(txidnum);

        // Apply to naive structure: convert CANDIDATE to REQUESTED.
        assert(m_entries[txidnum][peer].m_state == State::CANDIDATE);
        m_entries[txidnum][peer].m_state = State::REQUESTED;
        m_entries[txidnum][peer].m_time = m_now + m_tracker.GetTimeout();

        // Add event so that AdvanceToEvent can quickly jump to the point where its exptime passes.
        m_events.push(m_now + m_tracker.GetTimeout());

        // Call TxRequestTracker's implementation.
        m_tracker.RequestedTx(peer, TXIDS[txidnum], m_now);
    }

    void ReceivedResponse(int peer, int txidnum)
    {
        // Removes the ability to call RequestedTx until the next GetRequestable.
        m_get_requestable_last_peer = -1;

        // Apply to naive structure: convert anything to COMPLETED.
        if (m_entries[txidnum][peer].m_state != State::NOTHING) {
            m_entries[txidnum][peer].m_state = State::COMPLETED;
            Cleanup(txidnum);
        }

        // Call TxRequestTracker's implementation.
        m_tracker.ReceivedResponse(peer, TXIDS[txidnum]);
    }

    void GetRequestable(int peer)
    {
        // Enables future calls to RequestedTx for this peer, and the call's response as txids.
        m_get_requestable_last_peer = peer;
        m_get_requestable_last_result.reset();

        // Implement using naive structure:
        std::vector<std::pair<uint64_t, int>> result; //!< list of (sequence number, txidnum) pairs.
        for (int txidnum = 0; txidnum < MAX_TXIDS; ++txidnum) {
            // Mark any expired REQUESTED entries as COMPLETED.
            for (int peer2 = 0; peer2 < MAX_PEERS; ++peer2) {
                if (m_entries[txidnum][peer2].m_state == State::REQUESTED && m_entries[txidnum][peer2].m_time <= m_now) {
                    m_entries[txidnum][peer2].m_state = State::COMPLETED;
                    break;
                }
            }
            // And delete txids with only COMPLETED entries left.
            Cleanup(txidnum);
            // CANDIDATEs for which this entry has the lowest priority get returned.
            if (m_entries[txidnum][peer].m_state == State::CANDIDATE && GetSelected(txidnum) == peer) {
                m_get_requestable_last_result.set(txidnum);
                result.emplace_back(m_entries[txidnum][peer].m_sequence, txidnum);
            }
        }
        // Sort the results by sequence number.
        std::sort(result.begin(), result.end());

        // Compare with TxRequestTracker's implementation.
        std::vector<uint256> actual = m_tracker.GetRequestable(peer, m_now);
        m_tracker.TimeSanityCheck(m_now); // Invoke time-dependent sanity check, which should hold after GetRequestable
        assert(result.size() == actual.size());
        for (size_t pos = 0; pos < actual.size(); ++pos) {
            assert(TXIDS[result[pos].second] == actual[pos]);
        }
    }

    void Check()
    {
        // Compare CountTracked and CountInFlight with naive structure.
        size_t total = 0;
        for (int peer = 0; peer < MAX_PEERS; ++peer) {
            size_t tracked = 0;
            size_t inflight = 0;
            for (int txidnum = 0; txidnum < MAX_TXIDS; ++txidnum) {
                tracked += m_entries[txidnum][peer].m_state != State::NOTHING;
                inflight += m_entries[txidnum][peer].m_state == State::REQUESTED;
            }
            assert(m_tracker.CountTracked(peer) == tracked);
            assert(m_tracker.CountInFlight(peer) == inflight);
            total += tracked;
        }
        // Compare Size.
        assert(m_tracker.Size() == total);

        // Invoke internal consistency check of TxRequestTracker object.
        m_tracker.SanityCheck();
    }
};

} // namespace

void test_one_input(const std::vector<uint8_t>& buffer)
{
    // The first 4 bytes configure the timeout.
    if (buffer.size() < 4) return;
    auto timeout = DELAYS[buffer[0]] + DELAYS[buffer[1]] + DELAYS[buffer[2]] + DELAYS[buffer[3]];

    // Tester object (which encapsulates a TxRequestTracker).
    Tester tester(timeout);

    // Decode the input as a sequence of instructions with parameters
    auto it = buffer.begin() + 4;
    while (it != buffer.end()) {
        int cmd = *(it++) % 11;
        int peer, txidnum, delaynum;
        switch (cmd) {
        case 0: // Make time jump to the next event (m_time of PENDING or REQUESTED)
            tester.AdvanceToEvent();
            break;
        case 1: // Increase time (1 byte param: delaynum)
            delaynum = it == buffer.end() ? 0 : *(it++);
            tester.AdvanceTime(DELAYS[delaynum]);
            break;
        case 2: // Decrease time (1 byte param: delaynum)
            delaynum = it == buffer.end() ? 0 : *(it++);
            tester.AdvanceTime(-DELAYS[delaynum]);
            break;
        case 3: // Query for requestable txids (1 byte param: peer)
            peer = it == buffer.end() ? 0 : *(it++) % MAX_PEERS;
            tester.GetRequestable(peer);
            break;
        case 4: // Peer went offline (1 byte param: peer)
            peer = it == buffer.end() ? 0 : *(it++) % MAX_PEERS;
            tester.DeletedPeer(peer);
            break;
        case 5: // No longer need txid (1 byte param: txidnum)
            txidnum = it == buffer.end() ? 0 : *(it++) % MAX_TXIDS;
            tester.AlreadyHaveTx(txidnum);
            break;
        case 6: // Received inv from peer with no delay (2 byte param: peer, txidnum)
            peer = it == buffer.end() ? 0 : *(it++) % MAX_PEERS;
            txidnum = it == buffer.end() ? 0 : *(it++) % MAX_TXIDS;
            tester.ReceivedInv(peer, txidnum, std::chrono::microseconds::min());
            break;
        case 7: // Received inv from peer with delay (3 byte param: peer, txidnum, delaynum)
            peer = it == buffer.end() ? 0 : *(it++) % MAX_PEERS;
            txidnum = it == buffer.end() ? 0 : *(it++) % MAX_TXIDS;
            delaynum = it == buffer.end() ? 0 : *(it++);
            tester.ReceivedInv(peer, txidnum, tester.Now() + DELAYS[delaynum]);
            break;
        case 8: // Received inv from peer with negative delay (3 byte param: peer, txidnum, delaynum)
            peer = it == buffer.end() ? 0 : *(it++) % MAX_PEERS;
            txidnum = it == buffer.end() ? 0 : *(it++) % MAX_TXIDS;
            delaynum = it == buffer.end() ? 0 : *(it++);
            tester.ReceivedInv(peer, txidnum, tester.Now() - DELAYS[delaynum]);
            break;
        case 9: // Requested tx from peer (1 byte param: txidnum)
            txidnum = it == buffer.end() ? 0 : *(it++) % MAX_TXIDS;
            tester.RequestedTx(txidnum);
            break;
        case 10: // Received response (2 byte param: peernum, txidnum)
            peer = it == buffer.end() ? 0 : *(it++) % MAX_PEERS;
            txidnum = it == buffer.end() ? 0 : *(it++) % MAX_TXIDS;
            tester.ReceivedResponse(peer, txidnum);
            break;
        default:
            assert(false);
        }
    }

    tester.Check();
}
