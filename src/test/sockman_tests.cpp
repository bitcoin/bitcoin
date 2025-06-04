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

    std::unique_ptr<Sock> AcceptConnectionFromListeningSocket(CService& addr)
    {
        return AcceptConnection(*m_listen.front(), addr);
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

    // Pick up the phone, there's no one there
    CService addr_connection;
    BOOST_REQUIRE(!sockman.AcceptConnectionFromListeningSocket(addr_connection));

    // Create a mock client and add it to the local CreateSock queue
    ConnectClient();
    // Accept the connection
    BOOST_REQUIRE(sockman.AcceptConnectionFromListeningSocket(addr_connection));
    BOOST_CHECK_EQUAL(addr_connection.ToStringAddrPort(), "5.5.5.5:6789");
}

BOOST_AUTO_TEST_SUITE_END()
