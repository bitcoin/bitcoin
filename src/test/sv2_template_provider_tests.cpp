#include <boost/test/unit_test.hpp>
#include <common/sv2_messages.h>
#include <interfaces/mining.h>
#include <node/sv2_template_provider.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/sock.h>

BOOST_FIXTURE_TEST_SUITE(sv2_template_provider_tests, TestChain100Setup)

/**
  * A class for testing the Template Provider. Each TPTester encapsulates a
  * Sv2TemplateProvider (the one being tested) as well as a Sv2Cipher
  * to act as the other side.
  */
class TPTester {
private:
    std::unique_ptr<Sv2Transport> m_peer_transport; //!< Transport for peer
    std::unique_ptr<Sock> m_peer_socket;

public:
    std::unique_ptr<Sv2TemplateProvider> m_tp; //!< Sv2TemplateProvider being tested

    TPTester(interfaces::Mining& mining)
    {
        m_tp = std::make_unique<Sv2TemplateProvider>(mining);
    }

    bool start()
    {
        bool started = m_tp->Start(Sv2TemplateProviderOptions { .port = 18447 });
        if (! started) return false;
        // Avoid "Connection refused" on CI:
        UninterruptibleSleep(std::chrono::milliseconds{1000});
        return true;
    }

    void SendPeerBytes()
    {
        int flags = MSG_NOSIGNAL | MSG_DONTWAIT;
        ssize_t sent = 0;
        const auto& [data, more, _m_message_type] =m_peer_transport->GetBytesToSend(/*have_next_message=*/false);
        BOOST_REQUIRE(data.size() > 0);
        sent = m_peer_socket->Send(data.data(), data.size(), flags);
        m_peer_transport->MarkBytesSent(sent);

        BOOST_REQUIRE(sent > 0);
    }

    // Have the peer receive and process bytes:
    size_t PeerReceiveBytes()
    {
        uint8_t bytes_received_buf[0x10000];
        const auto num_bytes_received = m_peer_socket->Recv(bytes_received_buf, sizeof(bytes_received_buf), MSG_DONTWAIT);
        if(num_bytes_received == -1) {
            // This sometimes happens on macOS native CI
            return 0;
        }

        // Have peer process received bytes:
        Span<const uint8_t> received = Span(bytes_received_buf).subspan(0, num_bytes_received);
        BOOST_REQUIRE(m_peer_transport->ReceivedBytes(received));

        return num_bytes_received;
    }

    /* Create a new client and perform handshake */
    void handshake()
    {
        m_peer_transport.reset();

        auto peer_static_key{GenerateRandomKey()};
        m_peer_transport = std::make_unique<Sv2Transport>(std::move(peer_static_key), m_tp->m_authority_pubkey);

        // Connect client via socket to Template Provider

        std::optional<CService> tp{Lookup("127.0.0.1", 18447, /*fAllowLookup=*/false)};
        BOOST_REQUIRE(tp);
        m_peer_socket = ConnectDirectly(*tp, /*manual_connection=*/true);
        BOOST_REQUIRE(m_peer_socket);

        // Flush transport for handshake part 1
        SendPeerBytes();

        // Read handshake part 2 from transport
        constexpr auto timeout = std::chrono::milliseconds(1000);
        Sock::Event occurred;
        BOOST_REQUIRE(m_peer_socket->Wait(timeout, Sock::RECV, &occurred));
        BOOST_REQUIRE(occurred != 0);

        BOOST_REQUIRE_EQUAL(PeerReceiveBytes(), Sv2HandshakeState::HANDSHAKE_STEP2_SIZE);
    }

    void receiveMessage(Sv2NetMsg& msg)
    {
        // Client encrypts message and puts it on the transport:
        CSerializedNetMsg net_msg{std::move(msg)};
        BOOST_REQUIRE(m_peer_transport->SetMessageToSend(net_msg));
        SendPeerBytes();
    }

    void ProcessOurResponse(size_t reply_bytes_expected)
    {
        // Respond to peer
        constexpr auto timeout = std::chrono::milliseconds(1000);
        Sock::Event occurred;
        BOOST_REQUIRE(m_peer_socket->Wait(timeout, Sock::RECV, &occurred));
        BOOST_REQUIRE(occurred != 0);

        size_t bytes_received = PeerReceiveBytes();


        if (bytes_received == 0 && reply_bytes_expected != 0) {
            // Try one more time
            Sock::Event occurred;
            BOOST_REQUIRE(m_peer_socket->Wait(timeout, Sock::RECV, &occurred));
            BOOST_REQUIRE(occurred != 0);

            bytes_received = PeerReceiveBytes();
            BOOST_REQUIRE(bytes_received != 0);
        }

        // Wait for a possible second message (if MSG_MORE was set)
        BOOST_REQUIRE(m_peer_socket->Wait(timeout, Sock::RECV, &occurred));

        if (occurred != 0) {
            // Have peer process response bytes:
            bytes_received += PeerReceiveBytes();
        }

        BOOST_REQUIRE_EQUAL(bytes_received, reply_bytes_expected);
    }

    bool IsConnected()
    {
        LOCK(m_tp->m_clients_mutex);
        return m_tp->ConnectedClients() > 0;
    }

    bool IsFullyConnected()
    {
        LOCK(m_tp->m_clients_mutex);
        return m_tp->FullyConnectedClients() > 0;
    }

    Sv2NetMsg SetupConnectionMsg()
    {
        std::vector<uint8_t> bytes{
            0x02,                                                 // protocol
            0x02, 0x00,                                           // min_version
            0x02, 0x00,                                           // max_version
            0x01, 0x00, 0x00, 0x00,                               // flags
            0x07, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x30,       // endpoint_host
            0x61, 0x21,                                           // endpoint_port
            0x07, 0x42, 0x69, 0x74, 0x6d, 0x61, 0x69, 0x6e,       // vendor
            0x08, 0x53, 0x39, 0x69, 0x20, 0x31, 0x33, 0x2e, 0x35, // hardware_version
            0x1c, 0x62, 0x72, 0x61, 0x69, 0x69, 0x6e, 0x73, 0x2d, 0x6f, 0x73, 0x2d, 0x32, 0x30,
            0x31, 0x38, 0x2d, 0x30, 0x39, 0x2d, 0x32, 0x32, 0x2d, 0x31, 0x2d, 0x68, 0x61, 0x73,
            0x68, // firmware
            0x10, 0x73, 0x6f, 0x6d, 0x65, 0x2d, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2d, 0x75,
            0x75, 0x69, 0x64, // device_id
        };

        return node::Sv2NetMsg{node::Sv2MsgType::SETUP_CONNECTION, std::move(bytes)};
    }
};

BOOST_AUTO_TEST_CASE(client_tests)
{
    auto mining{interfaces::MakeMining(m_node)};
    TPTester tester{*mining};
    BOOST_REQUIRE(tester.start());

    BOOST_REQUIRE(!tester.IsConnected());
    tester.handshake();
    BOOST_REQUIRE(tester.IsConnected());
    BOOST_REQUIRE(!tester.IsFullyConnected());

    // After the handshake the client must send a SetupConnection message to the
    // Template Provider.

    // An empty SetupConnection message should cause disconnection
    node::Sv2NetMsg sv2_msg{node::Sv2MsgType::SETUP_CONNECTION, {}};
    tester.receiveMessage(sv2_msg);
    tester.ProcessOurResponse(0);

    BOOST_REQUIRE(!tester.IsConnected());

    BOOST_TEST_MESSAGE("Reconnect after empty message");

    // Reconnect
    tester.handshake();
    BOOST_TEST_MESSAGE("Handshake done, send SetupConnectionMsg");

    node::Sv2NetMsg setup{tester.SetupConnectionMsg()};
    tester.receiveMessage(setup);
    // SetupConnection.Success is 6 bytes
    tester.ProcessOurResponse(SV2_HEADER_ENCRYPTED_SIZE + 6 + Poly1305::TAGLEN);
    BOOST_REQUIRE(tester.IsFullyConnected());

    std::vector<uint8_t> coinbase_output_max_additional_size_bytes{
        0x01, 0x00, 0x00, 0x00
    };
    node::Sv2NetMsg msg{node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE, std::move(coinbase_output_max_additional_size_bytes)};
    // No reply expected, not yet implemented
    tester.receiveMessage(msg);
}

BOOST_AUTO_TEST_SUITE_END()