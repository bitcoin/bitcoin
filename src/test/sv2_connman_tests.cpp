#include <boost/test/unit_test.hpp>
#include <common/sv2_connman.h>
#include <common/sv2_messages.h>
#include <common/sv2_transport.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/sock.h>

#include <memory>

BOOST_FIXTURE_TEST_SUITE(sv2_connman_tests, TestChain100Setup)

/**
  * A class for testing the Sv2Connman. Each ConnTester encapsulates a
  * Sv2Connman (the one being tested) as well as a Sv2Cipher
  * to act as the other side.
  */
class ConnTester : Sv2EventsInterface {
private:
    std::unique_ptr<Sv2Transport> m_peer_transport; //!< Transport for peer
    // Sockets that will be returned by the Sv2Connman's listening socket Accept() method.
    std::shared_ptr<DynSock::Queue> m_sv2connman_accepted_sockets{std::make_shared<DynSock::Queue>()};

    std::shared_ptr<DynSock::Pipes> m_current_client_pipes;

    XOnlyPubKey m_connman_authority_pubkey;

public:
    std::unique_ptr<Sv2Connman> m_connman; //!< Sv2Connman being tested

    ConnTester()
    {
        CreateSock = [this](int, int, int) -> std::unique_ptr<Sock> {
            // This will be the bind/listen socket from m_connman. It will
            // create other sockets via its Accept() method.
            return std::make_unique<DynSock>(std::make_shared<DynSock::Pipes>(), m_sv2connman_accepted_sockets);
        };

        CKey static_key;
        static_key.MakeNewKey(true);
        auto authority_key{GenerateRandomKey()};
        m_connman_authority_pubkey = XOnlyPubKey(authority_key.GetPubKey());

        // Generate and sign certificate
        auto now{GetTime<std::chrono::seconds>()};
        uint16_t version = 0;
        // Start validity a little bit in the past to account for clock difference
        uint32_t valid_from = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now).count()) - 3600;
        uint32_t valid_to =  std::numeric_limits<unsigned int>::max(); // 2106
        Sv2SignatureNoiseMessage certificate{version, valid_from, valid_to, XOnlyPubKey(static_key.GetPubKey()), authority_key};

        m_connman = std::make_unique<Sv2Connman>(TP_SUBPROTOCOL, static_key, m_connman_authority_pubkey, certificate);

        BOOST_REQUIRE(m_connman->Start(this, "127.0.0.1", 18447));
    }

    ~ConnTester()
    {
        CreateSock = CreateSockOS;
    }

    void SendPeerBytes()
    {
        const auto& [data, more, _m_message_type] = m_peer_transport->GetBytesToSend(/*have_next_message=*/false);
        BOOST_REQUIRE(data.size() > 0);
        // Schedule data to be returned by the next Recv() call from
        // Sv2Connman on the socket it has accepted.
        m_current_client_pipes->recv.PushBytes(data.data(), data.size());
        m_peer_transport->MarkBytesSent(data.size());
    }

    // Have the peer receive and process bytes:
    size_t PeerReceiveBytes()
    {
        uint8_t buf[0x10000];
        // Get the data that has been written to the accepted socket with Send() by Sv2Connman.
        // Wait until the bytes appear in the "send" pipe.
        ssize_t n;
        for (;;) {
            n = m_current_client_pipes->send.GetBytes(buf, sizeof(buf), 0);
            if (n != -1 || errno != EAGAIN) {
                break;
            }
            UninterruptibleSleep(50ms);
        }

        // Inform client's transport that some bytes have been received (sent by Sv2Connman).
        if (n > 0) {
            Span<const uint8_t> s(buf, n);
            BOOST_REQUIRE(m_peer_transport->ReceivedBytes(s));
        }

        return n;
    }

    /* Create a new client and perform handshake */
    void handshake()
    {
        m_peer_transport.reset();

        auto peer_static_key{GenerateRandomKey()};
        m_peer_transport = std::make_unique<Sv2Transport>(std::move(peer_static_key), m_connman_authority_pubkey);

        // Have Sv2Connman's listen socket's Accept() simulate a newly arrived connection.
        m_current_client_pipes = std::make_shared<DynSock::Pipes>();
        m_sv2connman_accepted_sockets->Push(
            std::make_unique<DynSock>(m_current_client_pipes, std::make_shared<DynSock::Queue>()));

        // Flush transport for handshake part 1
        SendPeerBytes();

        // Read handshake part 2 from transport
        BOOST_REQUIRE_EQUAL(PeerReceiveBytes(), Sv2HandshakeState::HANDSHAKE_STEP2_SIZE);

        BOOST_REQUIRE(IsConnected());
    }

    void receiveMessage(Sv2NetMsg& msg)
    {
        // Client encrypts message and puts it on the transport:
        CSerializedNetMsg net_msg{std::move(msg)};
        BOOST_REQUIRE(m_peer_transport->SetMessageToSend(net_msg));
        SendPeerBytes();
    }

    bool IsConnected()
    {
        LOCK(m_connman->m_clients_mutex);
        return m_connman->ConnectedClients() > 0;
    }

    bool IsFullyConnected()
    {
        LOCK(m_connman->m_clients_mutex);
        return m_connman->FullyConnectedClients() > 0;
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

    void ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) override {
    }

};

BOOST_AUTO_TEST_CASE(client_tests)
{
    ConnTester tester{};

    BOOST_REQUIRE(!tester.IsConnected());
    tester.handshake();
    BOOST_REQUIRE(!tester.IsFullyConnected());

    // After the handshake the client must send a SetupConnection message to the
    // Template Provider.

    // An empty SetupConnection message should cause disconnection
    node::Sv2NetMsg sv2_msg{node::Sv2MsgType::SETUP_CONNECTION, {}};
    tester.receiveMessage(sv2_msg);
    BOOST_REQUIRE_EQUAL(tester.PeerReceiveBytes(), 0);

    BOOST_REQUIRE(!tester.IsConnected());

    BOOST_TEST_MESSAGE("Reconnect after empty message");

    // Reconnect
    tester.handshake();
    BOOST_TEST_MESSAGE("Handshake done, send SetupConnectionMsg");

    node::Sv2NetMsg setup{tester.SetupConnectionMsg()};
    tester.receiveMessage(setup);
    // SetupConnection.Success is 6 bytes
    BOOST_REQUIRE_EQUAL(tester.PeerReceiveBytes(), SV2_HEADER_ENCRYPTED_SIZE + 6 + Poly1305::TAGLEN);
    BOOST_REQUIRE(tester.IsFullyConnected());

    std::vector<uint8_t> coinbase_output_max_additional_size_bytes{
        0x01, 0x00, 0x00, 0x00
    };
    node::Sv2NetMsg msg{node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE, std::move(coinbase_output_max_additional_size_bytes)};
    // No reply expected, not yet implemented
    tester.receiveMessage(msg);
}

BOOST_AUTO_TEST_SUITE_END()
