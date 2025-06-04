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
};

BOOST_AUTO_TEST_CASE(test_sockman)
{
    TestSockMan sockman;

    // This address won't actually get used because we stubbed CreateSock()
    const std::optional<CService> addr_bind{Lookup("0.0.0.0", 0, false)};
    BOOST_REQUIRE(addr_bind.has_value());

    // Init state
    BOOST_REQUIRE_EQUAL(sockman.GetListeningSocketCount(), 0);
    // Bind to mock Listening Socket
    bilingual_str str_error;
    BOOST_REQUIRE(sockman.BindAndStartListening(addr_bind.value(), str_error));
    // We are bound and listening
    BOOST_REQUIRE_EQUAL(sockman.GetListeningSocketCount(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
