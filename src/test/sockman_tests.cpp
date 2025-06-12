// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sockman.h>
#include <test/util/setup_common.h>
#include <util/translation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(sockman_tests, SocketTestingSetup)

class TestSockMan : public SockMan
{
public:
    size_t GetListeningSocketCount() { return m_listen.size(); };

    // Connections are added from the SockMan I/O thread
    // but the test reads them from the main thread.
    std::vector<std::pair<Id, CService>> m_connections GUARDED_BY(m_connections_mutex);

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

private:
    Mutex m_connections_mutex;

    virtual bool EventNewConnectionAccepted(Id id,
                                        const CService& me,
                                        const CService& them) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_connections_mutex)
    {
        LOCK(m_connections_mutex);
        m_connections.emplace_back(id, them);
        return true;
    }
};

BOOST_AUTO_TEST_CASE(test_sockman)
{
    TestSockMan sockman;

    {
        // We can only bind to NET_IPV4 and NET_IPV6
        CService onion_address{Lookup("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaam2dqd.onion", 0, false).value()};
        auto result{sockman.BindAndStartListening(onion_address)};
        BOOST_REQUIRE(!result);
        BOOST_CHECK_EQUAL(util::ErrorString(result).original, "Bind address family for aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaam2dqd.onion:0 not supported");
    }

    // This VALID address won't actually get used because we stubbed CreateSock()
    CService addr_bind{Lookup("0.0.0.0", 0, false).value()};

    // Init state
    BOOST_REQUIRE_EQUAL(sockman.GetListeningSocketCount(), 0);
    // Bind to mock Listening Socket
    bilingual_str str_error;
    BOOST_REQUIRE(sockman.BindAndStartListening(addr_bind));
    // We are bound and listening
    BOOST_REQUIRE_EQUAL(sockman.GetListeningSocketCount(), 1);

    // Name the SockMan I/O thread
    SockMan::Options options{"test_sockman"};
    // Start the I/O loop
    sockman.StartSocketsThreads(options);

    // No connections yet
    BOOST_CHECK_EQUAL(sockman.GetConnectionsCount(), 0);

    // Create a mock client and add it to the local CreateSock queue
    ConnectClient();

    // Wait up to a minute to find and connect the client in the I/O loop
    int attempts{6000};
    while (sockman.GetConnectionsCount() < 1) {
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Inspect the connection
    auto client{sockman.GetFirstConnection()};
    BOOST_CHECK_EQUAL(client.second.ToStringAddrPort(), "5.5.5.5:6789");

    // Close connection
    BOOST_REQUIRE(sockman.CloseConnection(client.first));
    // Stop the I/O loop and shutdown
    sockman.InterruptNet();
    sockman.JoinSocketsThreads();
    sockman.StopListening();
}

BOOST_AUTO_TEST_SUITE_END()
