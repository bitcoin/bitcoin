// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sockman.h>
#include <test/util/setup_common.h>
#include <util/translation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(sockman_tests, SocketTestingSetup)

BOOST_AUTO_TEST_CASE(test_sockman)
{
    class TestSockMan : public SockMan
    {
    public:
        // Connections are added from the SockMan I/O thread
        // but the test reads them from the main thread.
        Mutex m_connections_mutex;
        std::vector<std::pair<Id, CService>> m_connections;

        // Received data is written here by the SockMan I/O thread
        // and tested by the main thread.
        Mutex m_received_mutex;
        std::vector<uint8_t> m_received;
        std::vector<uint8_t> m_respond{'o', 'k'};

        size_t GetConnectionsCount() EXCLUSIVE_LOCKS_REQUIRED(!m_connections_mutex)
        {
            LOCK(m_connections_mutex);
            return m_connections.size();
        }

        std::pair<Id, CService> GetFirstConnection() EXCLUSIVE_LOCKS_REQUIRED(!m_connections_mutex)
        {
            LOCK(m_connections_mutex);
            return m_connections.front();
        }

        std::vector<uint8_t> GetReceivedData() EXCLUSIVE_LOCKS_REQUIRED(!m_received_mutex)
        {
            LOCK(m_received_mutex);
            return m_received;
        }

    private:
        virtual bool EventNewConnectionAccepted(Id id,
                                            const CService& me,
                                            const CService& them) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_connections_mutex)
        {
            LOCK(m_connections_mutex);
            m_connections.emplace_back(id, them);
            return true;
        }

        // When we receive data just store it in a member variable for testing.
        virtual void EventGotData(Id id, std::span<const uint8_t> data) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_received_mutex)
        {
            LOCK(m_received_mutex);
            m_received.assign(data.begin(), data.end());
        };
        virtual void EventGotEOF(Id id) override {};
        virtual void EventGotPermanentReadError(Id id, const std::string& errmsg) override {};

        // As soon as we can send data to the connected socket, send the preloaded response.
        // Data is sent by the SockMan I/O thread and read by the main test thread,
        // but the Mutex in SockMan::ConnectionSockets guards this.
        virtual void EventReadyToSend(Id id, bool& cancel_recv) override
        {
            cancel_recv = false;
            if (m_respond.size() > 0) {
                std::string errmsg;
                ssize_t sent = SendBytes(id, m_respond, /*will_send_more=*/false, errmsg);
                // Larger payloads may require multiple write operations in production
                // but since there is no actual network socket involved in the test
                // we should be able to simplify.
                BOOST_CHECK_EQUAL(sent, m_respond.size());
                // Only send the response once
                m_respond.clear();
            }
        }
    };

    TestSockMan sockman;

    // This address won't actually get used because we stubbed CreateSock()
    const std::optional<CService> addr_bind{Lookup("0.0.0.0", 0, false)};
    bilingual_str strError;

    // Init state
    BOOST_REQUIRE_EQUAL(sockman.m_listen.size(), 0);
    // Bind to mock Listening Socket
    BOOST_REQUIRE(sockman.BindAndStartListening(addr_bind.value(), strError));
    // We are bound and listening
    BOOST_REQUIRE_EQUAL(sockman.m_listen.size(), 1);

    // Name the SockMan I/O thread
    SockMan::Options options{"test_sockman"};
    // Start the I/O loop
    sockman.StartSocketsThreads(options);

    // No connections yet
    BOOST_CHECK_EQUAL(sockman.GetConnectionsCount(), 0);

    // Create a mock client with a data payload to send
    // and add it to the local CreateSock queue
    const std::vector<uint8_t> request = {'b', 'i', 't', 's'};
    auto pipes{ConnectClient(request)};

    // Wait up to a minute to find and connect the client in the I/O loop
    int attempts{6000};
    while (sockman.GetConnectionsCount() < 1) {
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Inspect the connection
    auto client{sockman.GetFirstConnection()};
    BOOST_CHECK_EQUAL(client.second.ToStringAddrPort(), "5.5.5.5:6789");

    // Wait up to a minute to read the data from the connection
    attempts = 6000;
    while (!std::ranges::equal(sockman.GetReceivedData(), request)) {
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Wait up to a minute to write our response data back to the connection
    attempts = 6000;
    size_t expected_response_size = sockman.m_respond.size();
    std::vector<uint8_t> actually_received(expected_response_size);
    while (!std::ranges::equal(actually_received, sockman.m_respond)) {
        // Read the data received by the mock socket
        ssize_t bytes_read = pipes->send.GetBytes((void *)actually_received.data(), expected_response_size);
        // We may need to wait a few loop iterations in the socket thread
        // but once we can write, we can expect all the data at once.
        if (bytes_read >= 0) {
            BOOST_CHECK_EQUAL(bytes_read, expected_response_size);
        }
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Close connection
    BOOST_REQUIRE(sockman.CloseConnection(client.first));
    // Stop the I/O loop and shutdown
    sockman.interruptNet();
    sockman.JoinSocketsThreads();
    sockman.StopListening();
}

BOOST_AUTO_TEST_SUITE_END()
