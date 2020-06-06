// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <txrequest.h>
#include <uint256.h>

#include <test/util/setup_common.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txrequest_tests, BasicTestingSetup)

namespace {

constexpr std::chrono::microseconds TEST_TIMEOUT = std::chrono::minutes{2};
constexpr std::chrono::microseconds TEST_DELAY = std::chrono::seconds{2};

constexpr std::chrono::microseconds ReqTime(std::chrono::microseconds now, bool delay)
{
    return delay ? now + TEST_DELAY : std::chrono::microseconds::min();
}

constexpr int TEST_ITERATIONS = 1000;

uint256 GenTxid(const TxRequestTracker& tracker, const std::vector<uint64_t>& order)
{
    const auto& computer = tracker.GetPriorityComputer();

    uint256 ret;
    bool ok;
    do {
        ret = InsecureRand256();

        ok = true;
        for (size_t pos = 1; pos < order.size(); ++pos) {
            if (computer(ret, order[pos - 1], true, false) >= computer(ret, order[pos], true, false)) {
                ok = false;
                break;
            }
        }
    } while(!ok);

    return ret;
}

std::vector<uint64_t> GenPeers(size_t num)
{
    std::set<uint64_t> retset;
    while (retset.size() < num) {
        retset.insert(InsecureRandBits(63));
    }

    std::vector<uint64_t> ret(retset.begin(), retset.end());
    Shuffle(ret.begin(), ret.end(), g_insecure_rand_ctx);
    return ret;
}

std::chrono::microseconds RandomTime(int bits=44) { return std::chrono::microseconds{InsecureRandBits(bits)}; }

void CheckRequestable(TxRequestTracker& tracker, uint64_t peer, std::chrono::microseconds now, const std::vector<uint256>& expected)
{
    BOOST_CHECK(tracker.GetRequestable(peer, now) == expected);
    tracker.SanityCheck();
    tracker.TimeSanityCheck(now);
}

void TestBasicNonDelayed()
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(1);
    auto txid = GenTxid(tracker, {});
    auto now = RandomTime();

    tracker.ReceivedInv(peers[0], txid, ReqTime(now, false)); // Announce from non-delayed peer
    CheckRequestable(tracker, peers[0], now, {txid}); // Becomes requestable immediately.
    BOOST_CHECK_EQUAL(tracker.Size(), 1U);
}

void TestBasicDelayed()
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(1);
    auto txid = GenTxid(tracker, {});
    auto now = RandomTime();

    tracker.ReceivedInv(peers[0], txid, ReqTime(now, true)); // Announce from delayed peer
    CheckRequestable(tracker, peers[0], now, {}); // Does not become requestable immediately.
    now += TEST_DELAY - std::chrono::microseconds{1};
    CheckRequestable(tracker, peers[0], now, {}); // Does not become requestable after 1.999999s.
    now += std::chrono::microseconds{1};
    CheckRequestable(tracker, peers[0], now, {txid}); // Does become requestable after 2s.
    BOOST_CHECK_EQUAL(tracker.Size(), 1U);
}

void TestFirstAnnouncementWins(bool delay)
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(2);
    auto txid = GenTxid(tracker, {});
    auto now = RandomTime();

    tracker.ReceivedInv(peers[0], txid, ReqTime(now, delay)); // Announce from peer 0
    tracker.ReceivedInv(peers[1], txid, ReqTime(now, delay)); // Announce from peer 1
    if (delay) now += TEST_DELAY;
    CheckRequestable(tracker, peers[0], now, {txid}); // Becomes requestable immediately from peer 0.
    CheckRequestable(tracker, peers[1], now, {}); // And not from peer 1 (very first announcement always wins)
    BOOST_CHECK_EQUAL(tracker.Size(), 2U);
}

void TestNoDelayPreference()
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(2);
    auto txid = GenTxid(tracker, {});
    auto now = RandomTime();

    tracker.ReceivedInv(peers[0], txid, ReqTime(now, true)); // Announce from delayed peer 0
    CheckRequestable(tracker, peers[0], now, {}); // Does not become requestable
    now += TEST_DELAY;
    CheckRequestable(tracker, peers[0], now, {txid}); // Becomes requestable after enough time
    now += RandomTime(32);
    CheckRequestable(tracker, peers[0], now, {txid}); // Remains requestable after arbitrary amounts of time
    tracker.ReceivedInv(peers[1], txid, ReqTime(now, false)); // Then announced by non-delayed peer 1
    CheckRequestable(tracker, peers[1], now, {txid}); // Immediately becomes requestable from peer 1 (non-delayed)
    CheckRequestable(tracker, peers[0], now, {}); // And stops being requestable from peer 0 (can only be assigned to one peer)
    BOOST_CHECK_EQUAL(tracker.Size(), 2U);
}

void TestPriorityPreference(bool delay, int remove_reason)
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(3);
    auto txid = GenTxid(tracker, {peers[1], peers[2]}); // A txid such that peer 1 is prioritized over peer 2
    auto now = RandomTime();

    tracker.ReceivedInv(peers[0], txid, ReqTime(now, delay)); // Announce from peer 0
    now += std::chrono::microseconds{1};
    tracker.ReceivedInv(peers[2], txid, ReqTime(now, delay)); // Slightly later, announce from peer 2
    now += std::chrono::microseconds{1};
    tracker.ReceivedInv(peers[1], txid, ReqTime(now, delay)); // Even later, announce from peer 1
    if (delay) now += TEST_DELAY;
    CheckRequestable(tracker, peers[0], now, {txid}); // As peer 0 was the very first to announce, it gets the request first
    CheckRequestable(tracker, peers[1], now, {});
    CheckRequestable(tracker, peers[2], now, {});
    if (remove_reason == 0) {
        tracker.DeletedPeer(peers[0]);
    } else if (remove_reason == 1) {
        tracker.RequestedTx(peers[0], txid, now);
        now += TEST_TIMEOUT;
    } else if (remove_reason == 2) {
        tracker.RequestedTx(peers[0], txid, now);
        tracker.ReceivedResponse(peers[0], txid);
    }

    CheckRequestable(tracker, peers[0], now, {}); // If peer 0 goes offline, or the request times out, or a NOTFOUND is received, does peer 1 gets it (despite peer 2 being earlier)
    CheckRequestable(tracker, peers[1], now, {txid});
    CheckRequestable(tracker, peers[2], now, {});
    if (remove_reason == 0) {
        tracker.DeletedPeer(peers[1]);
    } else if (remove_reason == 1) {
        tracker.RequestedTx(peers[1], txid, now);
        now += TEST_TIMEOUT;
    } else if (remove_reason == 2) {
        tracker.RequestedTx(peers[1], txid, now);
        tracker.ReceivedResponse(peers[1], txid);
    }
    CheckRequestable(tracker, peers[0], now, {}); // Only if peer 1 also goes offline, or the request times out, does peer 2 get it.
    CheckRequestable(tracker, peers[1], now, {});
    CheckRequestable(tracker, peers[2], now, {txid});
}

void TestOrderDependsOnPeers(bool delay)
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(3);
    auto txid1 = GenTxid(tracker, {peers[1], peers[2]}); // A txid such that peer 1 is prioritized over peer 2
    auto txid2 = GenTxid(tracker, {peers[2], peers[1]}); // A txid such that peer 2 is prioritized over peer 1
    auto now = RandomTime();

    for (auto peer : peers) {
        tracker.ReceivedInv(peer, txid1, ReqTime(now, delay)); // Announce both txids for all peers
        tracker.ReceivedInv(peer, txid2, ReqTime(now, delay));
    }
    if (delay) now += TEST_DELAY;
    CheckRequestable(tracker, peers[0], now, {txid1, txid2}); // Both are requestable
    tracker.DeletedPeer(peers[0]); // Peer 0 goes offline
    CheckRequestable(tracker, peers[1], now, {txid1}); // txid1 is now requestable from peer 1
    CheckRequestable(tracker, peers[2], now, {txid2}); // While txid2 is now requestable from peer 2
}

void TestRequestableOrder(bool delay)
{
    TxRequestTracker tracker(TEST_TIMEOUT);
    auto peers = GenPeers(2);
    auto txid1 = GenTxid(tracker, {});
    auto txid2 = GenTxid(tracker, {});
    auto now = RandomTime();

    tracker.ReceivedInv(peers[0], txid1, ReqTime(now, delay));
    tracker.ReceivedInv(peers[0], txid2, ReqTime(now, delay));
    tracker.ReceivedInv(peers[1], txid2, ReqTime(now, delay));
    now -= std::chrono::microseconds{1};
    tracker.ReceivedInv(peers[1], txid1, ReqTime(now, delay));
    if (delay) now += TEST_DELAY;
    now += std::chrono::microseconds{1};
    CheckRequestable(tracker, peers[0], now, {txid1, txid2}); // Check that for each peer, requests go out in announcement order,
    tracker.DeletedPeer(peers[0]);
    CheckRequestable(tracker, peers[1], now, {txid2, txid1}); // even when the clock went backwards during announcements.
}

void TestAll()
{
    TestBasicNonDelayed();
    TestBasicDelayed();
    TestNoDelayPreference();
    for (int delay = 0; delay < 2; ++delay) {
        TestFirstAnnouncementWins(delay);
        TestOrderDependsOnPeers(delay);
        TestRequestableOrder(delay);
        for (int reason = 0; reason < 3; ++reason) {
            TestPriorityPreference(delay, reason);
        }
    }
}

}

BOOST_AUTO_TEST_CASE(TxRequestTest)
{
    for (int i = 0; i < TEST_ITERATIONS; ++i) TestAll();
}

BOOST_AUTO_TEST_SUITE_END()
