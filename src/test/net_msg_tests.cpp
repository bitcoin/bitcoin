// Copyright (c) 2022-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <net_processing.h>
#include <netbase.h>
#include <primitives/block.h>
#include <protocol.h>
#include <streams.h>
#include <test/util/logging.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <assert.h>
#include <atomic>
#include <memory>
#include <optional>
#include <ratio>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(net_msg_tests, NetTestingSetup)

BOOST_AUTO_TEST_CASE(initial_messages_exchange)
{
    std::unordered_map<std::string, size_t> count_sent_messages;
    const auto pipes{m_sockets_pipes.PopFront()};

    // Wait for all messages due to the initial handshake to be Send() to the socket.
    // The FEEFILTER is the last one, so quit when we get that.
    for (;;) {
        auto msg = pipes->send.GetNetMsg();
        if (!msg.has_value()) {
            break;
        }
        ++count_sent_messages[msg->m_type];
        if (msg->m_type == NetMsgType::FEEFILTER) {
            break;
        }
    }

    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::VERSION], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::WTXIDRELAY], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::SENDADDRV2], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::VERACK], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::GETADDR], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::SENDCMPCT], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::PING], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::GETHEADERS], 1);
    BOOST_CHECK_EQUAL(count_sent_messages[NetMsgType::FEEFILTER], 1);
}

BOOST_AUTO_TEST_CASE(addr)
{
    const auto pipes{m_sockets_pipes.PopFront()};
    std::vector<CAddress> addresses{5};

    ASSERT_DEBUG_LOG_WAIT(strprintf("Received addr: %u addresses", addresses.size()), 30s);
    pipes->recv.PushNetMsg(NetMsgType::ADDRV2, CAddress::V2_NETWORK(addresses));
}

BOOST_AUTO_TEST_CASE(getblocks)
{
    const auto pipes{m_sockets_pipes.PopFront()};
    std::vector<uint256> hashes{5};
    CBlockLocator block_locator{std::move(hashes)};
    uint256 hash_stop;

    ASSERT_DEBUG_LOG_WAIT("getblocks -1 to end", 30s);
    pipes->recv.PushNetMsg(NetMsgType::GETBLOCKS, block_locator, hash_stop);
}

BOOST_AUTO_TEST_CASE(ping)
{
    const auto pipes{m_sockets_pipes.PopFront()};

    auto WaitForPingStats = [this](std::chrono::microseconds min,
                                   std::chrono::microseconds last,
                                   std::chrono::microseconds wait) {
        BOOST_REQUIRE_EQUAL(m_node.connman->GetNodeCount(ConnectionDirection::Both), 1);

        const auto deadline = std::chrono::steady_clock::now() + 30s;
        bool end{false};

        // Collect the ping stats and check if they are as expected. Retry if not as expected.
        for (;;) {
            m_node.connman->ForEachNode([&](CNode* node) {
                CNodeStateStats stats;
                BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(node->GetId(), stats));
                const auto min_actual{node->m_min_ping_time.load().count()};
                const auto last_actual{node->m_last_ping_time.load().count()};
                const auto wait_actual{stats.m_ping_wait.count()};
                const bool ok{min_actual == min.count() &&
                              last_actual == last.count() &&
                              wait_actual == wait.count()};

                if (ok) {
                    end = true;
                } else if (std::chrono::steady_clock::now() > deadline) {
                    BOOST_CHECK_EQUAL(min_actual, min.count());
                    BOOST_CHECK_EQUAL(last_actual, last.count());
                    BOOST_CHECK_EQUAL(wait_actual, wait.count());
                    end = true;
                }
            });
            if (end) {
                break;
            }
            std::this_thread::sleep_for(100ms);
        }
    };

    auto GetPingNonceSent = [&]() {
        for (;;) {
            auto msg = pipes->send.GetNetMsg();
            assert(msg.has_value());
            if (msg->m_type == NetMsgType::PING) {
                uint64_t nonce;
                msg->m_recv >> nonce;
                return nonce;
            }
        }
    };

    auto SendPing = [&]() {
        {
            ASSERT_DEBUG_LOG_WAIT("sending ping", 30s);
            SetMockTime(GetMockTime() + PING_INTERVAL + 1s);
        }
        return GetPingNonceSent();
    };

    BOOST_TEST_MESSAGE("Ensure initial messages exchange has completed with the sending of a ping "
                       "with nonce != 0 and the ping stats indicate a pending ping.");
    BOOST_REQUIRE_NE(GetPingNonceSent(), 0);
    auto time_elapsed = 1s;
    SetMockTime(GetMockTime() + time_elapsed);
    WaitForPingStats(/*min=*/std::chrono::microseconds::max(), /*last=*/0us, /*wait=*/time_elapsed);

    BOOST_TEST_MESSAGE("Check that receiving a PONG without nonce cancels our PING");
    {
        ASSERT_DEBUG_LOG_WAIT("Short payload", 30s);
        pipes->recv.PushNetMsg(NetMsgType::PONG);
    }
    WaitForPingStats(/*min=*/std::chrono::microseconds::max(), /*last=*/0us, /*wait=*/0us);

    BOOST_TEST_MESSAGE("Check that receiving an unrequested PONG is logged and ignored");
    {
        ASSERT_DEBUG_LOG_WAIT("Unsolicited pong without ping", 30s);
        pipes->recv.PushNetMsg(NetMsgType::PONG, /*nonce=*/uint64_t{0});
    }

    BOOST_TEST_MESSAGE("Check that receiving a PONG with the wrong nonce does not cancel our PING");
    uint64_t nonce{SendPing()};
    {
        ASSERT_DEBUG_LOG_WAIT("Nonce mismatch", 30s);
        pipes->recv.PushNetMsg(NetMsgType::PONG, nonce + 1);
    }
    time_elapsed = 5s;
    SetMockTime(GetMockTime() + time_elapsed);
    WaitForPingStats(/*min=*/std::chrono::microseconds::max(), /*last=*/0us, /*wait=*/time_elapsed);

    BOOST_TEST_MESSAGE("Check that receiving a PONG with nonce=0 cancels our PING");
    {
        ASSERT_DEBUG_LOG_WAIT("Nonce zero", 30s);
        pipes->recv.PushNetMsg(NetMsgType::PONG, /*nonce=*/uint64_t{0});
    }
    WaitForPingStats(/*min=*/std::chrono::microseconds::max(), /*last=*/0us, /*wait=*/0us);

    BOOST_TEST_MESSAGE("Check that receiving a PONG with the correct nonce cancels our PING");
    nonce = SendPing();
    time_elapsed = 5s;
    SetMockTime(GetMockTime() + time_elapsed);
    pipes->recv.PushNetMsg(NetMsgType::PONG, nonce);
    WaitForPingStats(time_elapsed, time_elapsed, 0us);
}

BOOST_AUTO_TEST_CASE(redundant_verack)
{
    const auto pipes{m_sockets_pipes.PopFront()};

    ASSERT_DEBUG_LOG_WAIT("ignoring redundant verack message", 30s);
    pipes->recv.PushNetMsg(NetMsgType::VERACK);
}

BOOST_AUTO_TEST_SUITE_END()
