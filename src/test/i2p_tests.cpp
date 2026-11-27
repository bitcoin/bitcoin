// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <compat/compat.h>
#include <i2p.h>
#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <test/util/logging.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/threadinterrupt.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <optional>
#include <string>

/// Save the log level and the value of CreateSock and restore them when the test ends.
class EnvTestingSetup : public BasicTestingSetup
{
public:
    explicit EnvTestingSetup(const ChainType chainType = ChainType::MAIN,
                             TestOpts opts = {})
        : BasicTestingSetup{chainType, opts},
          m_prev_log_level{LogInstance().LogLevel()},
          m_create_sock_orig{CreateSock}
    {
        LogInstance().SetLogLevel(BCLog::Level::Trace);
    }

    ~EnvTestingSetup()
    {
        CreateSock = m_create_sock_orig;
        LogInstance().SetLogLevel(m_prev_log_level);
    }

private:
    const BCLog::Level m_prev_log_level;
    const decltype(CreateSock) m_create_sock_orig;
};

BOOST_FIXTURE_TEST_SUITE(i2p_tests, EnvTestingSetup)

BOOST_AUTO_TEST_CASE(unlimited_recv)
{
    CreateSock = [](int, int, int) {
        return std::make_unique<StaticContentsSock>(std::string(i2p::sam::MAX_MSG_SIZE + 1, 'a'));
    };

    auto interrupt{std::make_shared<CThreadInterrupt>()};
    const std::optional<CService> addr{Lookup("127.0.0.1", 9000, false)};
    const Proxy sam_proxy(addr.value(), /*tor_stream_isolation=*/false);
    i2p::sam::Session session(gArgs.GetDataDirNet() / "test_i2p_private_key", sam_proxy, interrupt);

    {
        ASSERT_DEBUG_LOG("Creating persistent I2P SAM session");
        ASSERT_DEBUG_LOG("too many bytes without a terminator");

        i2p::Connection conn;
        bool proxy_error;
        BOOST_REQUIRE(!session.Connect(CService{}, conn, proxy_error));
    }
}

BOOST_AUTO_TEST_CASE(listen_ok_accept_fail)
{
    size_t num_sockets{0};
    CreateSock = [&num_sockets](int, int, int) {
        // clang-format off
        ++num_sockets;
        // First socket is the control socket for creating the session.
        if (num_sockets == 1) {
            return std::make_unique<StaticContentsSock>(
                // reply to HELLO
                "HELLO REPLY RESULT=OK VERSION=3.1\n"
                // reply to DEST GENERATE
                "DEST REPLY PUB=WnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqFpxji10Qah0IXVYxZVqkcScM~Yccf9v8BnNlaZbWtSoWnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqFpxji10Qah0IXVYxZVqkcScM~Yccf9v8BnNlaZbWtSoWnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqFpxji10Qah0IXVYxZVqkcScM~Yccf9v8BnNlaZbWtSoWnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqLE4SD-yjT48UNI7qiTUfIPiDitCoiTTz2cr4QGfw89rBQAEAAcAAA== PRIV=WnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqFpxji10Qah0IXVYxZVqkcScM~Yccf9v8BnNlaZbWtSoWnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqFpxji10Qah0IXVYxZVqkcScM~Yccf9v8BnNlaZbWtSoWnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqFpxji10Qah0IXVYxZVqkcScM~Yccf9v8BnNlaZbWtSoWnGOLXRBqHQhdVjFlWqRxJwz9hxx~2~wGc2Vplta1KhacY4tdEGodCF1WMWVapHEnDP2HHH~b~AZzZWmW1rUqLE4SD-yjT48UNI7qiTUfIPiDitCoiTTz2cr4QGfw89rBQAEAAcAAOvuCIKTyv5f~1QgGq7XQl-IqBULTB5WzB3gw5yGPtd1p0AeoADrq1ccZggLPQ4ZLUsGK-HVw373rcTfvxrcuwenqVjiN4tbbYLWtP7xXGWj6fM6HyORhU63GphrjEePpMUHDHXd3o7pWGM-ieVVQSK~1MzF9P93pQWI3Do52EeNAayz4HbpPjNhVBzG1hUEFwznfPmUZBPuaOR4-uBm1NEWEuONlNOCctE4-U0Ukh94z-Qb55U5vXjR5G4apmBblr68t6Wm1TKlzpgFHzSqLryh3stWqrOKY1H0z9eZ2z1EkHFOpD5LyF6nf51e-lV7HLMl44TYzoEHK8RRVodtLcW9lacVdBpv~tOzlZERIiDziZODPETENZMz5oy9DQ7UUw==\n"
                // reply to SESSION CREATE
                "SESSION STATUS RESULT=OK\n"
                // dummy to avoid reporting EOF on the socket
                "a"
            );
        }
        // Subsequent sockets are for recreating the session or for listening and accepting incoming connections.
        if (num_sockets % 2 == 0) {
            // Replies to Listen() and Accept()
            return std::make_unique<StaticContentsSock>(
                // reply to HELLO
                "HELLO REPLY RESULT=OK VERSION=3.1\n"
                // reply to STREAM ACCEPT
                "STREAM STATUS RESULT=OK\n"
                // continued reply to STREAM ACCEPT, violating the protocol described at
                // https://i2p.net/en/docs/api/samv3/#accept-response
                // should be base64, something like
                // "IchV608baDoXbqzQKSqFDmTXPVgoDbPAhZJvNRXXxi4hyFXrTxtoOhdurNApKoUOZNc9WCgNs8CFkm81FdfGLiHIVetPG2g6F26s0CkqhQ5k1z1YKA2zwIWSbzUV18YuIchV608baDoXbqzQKSqFDmTXPVgoDbPAhZJvNRXXxi4hyFXrTxtoOhdurNApKoUOZNc9WCgNs8CFkm81FdfGLiHIVetPG2g6F26s0CkqhQ5k1z1YKA2zwIWSbzUV18YuIchV608baDoXbqzQKSqFDmTXPVgoDbPAhZJvNRXXxi4hyFXrTxtoOhdurNApKoUOZNc9WCgNs8CFkm81FdfGLiHIVetPG2g6F26s0CkqhQ5k1z1YKA2zwIWSbzUV18YuIchV608baDoXbqzQKSqFDmTXPVgoDbPAhZJvNRXXxi4hyFXrTxtoOhdurNApKoUOZNc9WCgNs8CFkm81FdfGLlSreVaCuCS5sdb-8ToWULWP7kt~lRPDeUNxQMq3cRSBBQAEAAcAAA==\n"
                "STREAM STATUS RESULT=I2P_ERROR MESSAGE=\"Session was closed\"\n"
            );
        } else {
            // Another control socket, but without creating a destination (it is cached in the session).
            return std::make_unique<StaticContentsSock>(
                // reply to HELLO
                "HELLO REPLY RESULT=OK VERSION=3.1\n"
                // reply to SESSION CREATE
                "SESSION STATUS RESULT=OK\n"
                // dummy to avoid reporting EOF on the socket
                "a"
            );
        }
        // clang-format on
    };

    auto interrupt{std::make_shared<CThreadInterrupt>()};
    const CService addr{in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT), /*port=*/7656};
    const Proxy sam_proxy(addr, /*tor_stream_isolation=*/false);
    i2p::sam::Session session(gArgs.GetDataDirNet() / "test_i2p_private_key",
                              sam_proxy,
                              interrupt);

    i2p::Connection conn;
    for (size_t i = 0; i < 5; ++i) {
        ASSERT_DEBUG_LOG("Creating persistent I2P SAM session");
        ASSERT_DEBUG_LOG("Persistent I2P SAM session" /* ... created */);
        ASSERT_DEBUG_LOG("Error accepting");
        ASSERT_DEBUG_LOG("Destroying I2P SAM session");
        BOOST_REQUIRE(session.Listen(conn));
        BOOST_REQUIRE(!session.Accept(conn));
    }
}

BOOST_AUTO_TEST_CASE(damaged_private_key)
{
    CreateSock = [](int, int, int) {
        return std::make_unique<StaticContentsSock>("HELLO REPLY RESULT=OK VERSION=3.1\n"
                                                    "SESSION STATUS RESULT=OK DESTINATION=\n");
    };

    const auto i2p_private_key_file = m_args.GetDataDirNet() / "test_i2p_private_key_damaged";

    for (const auto& [file_contents, expected_error] : std::vector<std::tuple<std::string, std::string>>{
             {"", "The private key is too short (0 < 387)"},

             {"abcd", "The private key is too short (4 < 387)"},

             {std::string(386, '\0'), "The private key is too short (386 < 387)"},

             {std::string(385, '\0') + '\0' + '\1',
              "Certificate length (1) designates that the private key should be 388 bytes, but it is only "
              "387 bytes"},

             {std::string(385, '\0') + '\0' + '\5' + "abcd",
              "Certificate length (5) designates that the private key should be 392 bytes, but it is only "
              "391 bytes"}}) {
        BOOST_REQUIRE(WriteBinaryFile(i2p_private_key_file, file_contents));

        auto interrupt{std::make_shared<CThreadInterrupt>()};
        const CService addr{in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT), /*port=*/7656};
        const Proxy sam_proxy{addr, /*tor_stream_isolation=*/false};
        i2p::sam::Session session(i2p_private_key_file, sam_proxy, interrupt);

        {
            ASSERT_DEBUG_LOG("Creating persistent I2P SAM session");
            ASSERT_DEBUG_LOG(expected_error);

            i2p::Connection conn;
            bool proxy_error;
            BOOST_CHECK(!session.Connect(CService{}, conn, proxy_error));
        }
    }
}

BOOST_AUTO_TEST_CASE(connect_direct_b32)
{
    const std::string b32{"udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p"};
    CService peer;
    BOOST_REQUIRE(peer.SetSpecial(b32));
    BOOST_REQUIRE_EQUAL(peer.GetPort(), I2P_SAM31_PORT);

    // Minimal valid private key: 385 bytes of keys followed by a zero certificate length.
    const auto i2p_private_key_file = m_args.GetDataDirNet() / "test_i2p_private_key_connect";
    BOOST_REQUIRE(WriteBinaryFile(i2p_private_key_file, std::string(387, '\0')));

    // Reply to STREAM CONNECT that the next connect socket will deliver.
    std::string stream_connect_result;
    // Captures what Connect() sends on the most recent connect socket.
    std::shared_ptr<DynSock::Pipes> connect_pipes;
    size_t num_sockets{0};

    CreateSock = [&](int, int, int) -> std::unique_ptr<Sock> {
        ++num_sockets;
        // First socket is the control socket for creating the session.
        if (num_sockets == 1) {
            return std::make_unique<StaticContentsSock>(
                // reply to HELLO
                "HELLO REPLY RESULT=OK VERSION=3.1\n"
                // reply to SESSION CREATE
                "SESSION STATUS RESULT=OK\n"
                // dummy to avoid reporting EOF on the socket
                "a");
        }
        // Subsequent sockets are used by Connect().
        connect_pipes = std::make_shared<DynSock::Pipes>();
        const std::string replies{"HELLO REPLY RESULT=OK VERSION=3.1\n"
                                  "STREAM STATUS RESULT=" + stream_connect_result + "\n"};
        connect_pipes->recv.PushBytes(replies.data(), replies.size());
        return std::make_unique<DynSock>(connect_pipes);
    };

    auto interrupt{std::make_shared<CThreadInterrupt>()};
    const CService addr{in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT), /*port=*/7656};
    const Proxy sam_proxy{addr, /*tor_stream_isolation=*/false};
    i2p::sam::Session session(i2p_private_key_file, sam_proxy, interrupt);

    // The peer's base32 address must be passed directly to STREAM CONNECT, without a prior NAMING LOOKUP.
    const auto check_sent = [&] {
        BOOST_REQUIRE(connect_pipes);
        std::string sent;
        char buf[1024];
        ssize_t n;
        while ((n = connect_pipes->send.GetBytes(buf, sizeof(buf))) > 0) {
            sent.append(buf, n);
        }
        const std::string prefix{"HELLO VERSION MIN=3.1 MAX=3.1\nSTREAM CONNECT ID="};
        const std::string suffix{" DESTINATION=" + b32 + " SILENT=false\n"};
        BOOST_REQUIRE_MESSAGE(sent.starts_with(prefix) && sent.ends_with(suffix) &&
                                  sent.size() > prefix.size() + suffix.size(),
                              "Unexpected requests sent: " << sent);
        const std::string session_id{sent.substr(prefix.size(), sent.size() - prefix.size() - suffix.size())};
        BOOST_CHECK_EQUAL(session_id.size(), 10U);
        BOOST_CHECK(IsHex(session_id));
    };

    for (const std::string result : {"CANT_REACH_PEER", "TIMEOUT"}) {
        stream_connect_result = result;
        ASSERT_DEBUG_LOG("Error connecting to " + b32 + ":0: \"STREAM STATUS RESULT=" + result + "\"");

        i2p::Connection conn;
        bool proxy_error{true};
        BOOST_CHECK(!session.Connect(peer, conn, proxy_error));
        // An unreachable peer is not a proxy error, so the attempt is recorded in addrman.
        BOOST_CHECK(!proxy_error);
        check_sent();
    }

    stream_connect_result = "OK";
    i2p::Connection conn;
    bool proxy_error{false};
    BOOST_CHECK(session.Connect(peer, conn, proxy_error));
    BOOST_CHECK(conn.sock);
    BOOST_CHECK(conn.peer == peer);
    check_sent();

    BOOST_CHECK_EQUAL(num_sockets, 4U);
}

BOOST_AUTO_TEST_SUITE_END()
