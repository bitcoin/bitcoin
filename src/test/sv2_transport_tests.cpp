// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_noise.h>
#include <common/sv2_transport.h>
#include <logging.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <node/timeoffsets.h>
#include <util/bitdeque.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <ios>
#include <memory>
#include <optional>
#include <string>

using namespace std::literals;
using node::Sv2NetMsg;
using node::Sv2CoinbaseOutputDataSizeMsg;
using node::Sv2MsgType;

BOOST_FIXTURE_TEST_SUITE(sv2_transport_tests, RegTestingSetup)

namespace {

/** A class for scenario-based tests of Sv2Transport
 *
 * Each Sv2TransportTester encapsulates a Sv2Transport (the one being tested),
 * and can be told to interact with it. To do so, it also encapsulates a Sv2Cipher
 * to act as the other side. A second Sv2Transport is not used, as doing so would
 * not permit scenarios that involve sending invalid data.
 */
class Sv2TransportTester
{
    std::unique_ptr<Sv2Transport> m_transport; //!< Sv2Transport being tested
    std::unique_ptr<Sv2Cipher> m_peer_cipher;  //!< Cipher to help with the other side
    bool m_test_initiator;    //!< Whether m_transport is the initiator (true) or responder (false)

    std::vector<uint8_t> m_to_send;  //!< Bytes we have queued up to send to m_transport->
    std::vector<uint8_t> m_received; //!< Bytes we have received from m_transport->
    std::deque<Sv2NetMsg> m_msg_to_send; //!< Messages to be sent *by* m_transport to us.

public:
    /** Construct a tester object. test_initiator: whether the tested transport is initiator. */

    explicit Sv2TransportTester(bool test_initiator) : m_test_initiator(test_initiator)
    {
        auto initiator_static_key{GenerateRandomKey()};
        auto responder_static_key{GenerateRandomKey()};
        auto responder_authority_key{GenerateRandomKey()};

        // Create certificates
        auto epoch_now = std::chrono::system_clock::now().time_since_epoch();
        uint16_t version = 0;
        uint32_t valid_from = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(epoch_now).count());
        uint32_t valid_to =  std::numeric_limits<unsigned int>::max();

        auto responder_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                    XOnlyPubKey(responder_static_key.GetPubKey()), responder_authority_key);

        if (test_initiator) {
            m_transport = std::make_unique<Sv2Transport>(initiator_static_key, XOnlyPubKey(responder_authority_key.GetPubKey()));
            m_peer_cipher = std::make_unique<Sv2Cipher>(std::move(responder_static_key), std::move(responder_certificate));
        } else {
            m_transport = std::make_unique<Sv2Transport>(responder_static_key, responder_certificate);
            m_peer_cipher = std::make_unique<Sv2Cipher>(std::move(initiator_static_key), XOnlyPubKey(responder_authority_key.GetPubKey()));
        }
    }

    /** Data type returned by Interact:
     *
     * - std::nullopt: transport error occurred
     * - otherwise: a vector of
     *   - std::nullopt: invalid message received
     *   - otherwise: a Sv2NetMsg retrieved
     */
    using InteractResult = std::optional<std::vector<std::optional<Sv2NetMsg>>>;

    void LogProgress(bool should_progress, bool progress, bool pretend_no_progress) {
        if (!should_progress) {
            BOOST_TEST_MESSAGE("[Interact] !should_progress");
        } else if  (!progress) {
            BOOST_TEST_MESSAGE("[Interact] should_progress && !progress");
        } else if (pretend_no_progress) {
            BOOST_TEST_MESSAGE("[Interact] pretend !progress");
        }
    }

    /** Send/receive scheduled/available bytes and messages.
     *
     * This is the only function that interacts with the transport being tested; everything else is
     * scheduling things done by Interact(), or processing things learned by it.
     */
    InteractResult Interact()
    {
        std::vector<std::optional<Sv2NetMsg>> ret;
        while (true) {
            bool progress{false};
            // Send bytes from m_to_send to the transport.
            if (!m_to_send.empty()) {
                size_t n_bytes_to_send = 1 + InsecureRandRange(m_to_send.size());
                BOOST_TEST_MESSAGE(strprintf("[Interact] send %d of %d bytes", n_bytes_to_send, m_to_send.size()));
                Span<const uint8_t> to_send = Span{m_to_send}.first(n_bytes_to_send);
                size_t old_len = to_send.size();
                if (!m_transport->ReceivedBytes(to_send)) {
                    BOOST_TEST_MESSAGE("[Interact] transport error");
                    return std::nullopt;
                }
                if (old_len != to_send.size()) {
                    progress = true;
                    m_to_send.erase(m_to_send.begin(), m_to_send.begin() + (old_len - to_send.size()));
                }
            }
            // Retrieve messages received by the transport.
            bool should_progress = m_transport->ReceivedMessageComplete();
            bool pretend_no_progress = InsecureRandBool();
            LogProgress(should_progress, progress, pretend_no_progress);
            if (should_progress && (!progress || pretend_no_progress)) {
                bool dummy_reject_message = false;
                CNetMessage net_msg = m_transport->GetReceivedMessage(std::chrono::microseconds(0), dummy_reject_message);
                Sv2NetMsg msg(std::move(net_msg));
                ret.emplace_back(std::move(msg));
                progress = true;
            }
            // Enqueue a message to be sent by the transport to us.
            should_progress = !m_msg_to_send.empty();
            pretend_no_progress = InsecureRandBool();
            LogProgress(should_progress, progress, pretend_no_progress);
            if (should_progress && (!progress || pretend_no_progress)) {
                BOOST_TEST_MESSAGE("Shoehorn into CSerializedNetMsg");
                CSerializedNetMsg msg{m_msg_to_send.front()};
                BOOST_TEST_MESSAGE("Call SetMessageToSend");
                if (m_transport->SetMessageToSend(msg)) {
                    BOOST_TEST_MESSAGE("Finished SetMessageToSend");
                    m_msg_to_send.pop_front();
                    progress = true;
                }
            }
            // Receive bytes from the transport.
            const auto& [recv_bytes, _more, _m_type] = m_transport->GetBytesToSend(!m_msg_to_send.empty());
            should_progress = !recv_bytes.empty();
            pretend_no_progress = InsecureRandBool();
            LogProgress(should_progress, progress, pretend_no_progress);
            if (should_progress && (!progress || pretend_no_progress)) {
                size_t to_receive = 1 + InsecureRandRange(recv_bytes.size());
                BOOST_TEST_MESSAGE(strprintf("[Interact] receive %d of %d bytes", to_receive, recv_bytes.size()));
                m_received.insert(m_received.end(), recv_bytes.begin(), recv_bytes.begin() + to_receive);
                progress = true;
                m_transport->MarkBytesSent(to_receive);
            }
            if (!progress) break;
        }
        return ret;
    }

    /** Schedule bytes to be sent to the transport. */
    void Send(Span<const uint8_t> data)
    {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Send: %s\n", HexStr(data));
        m_to_send.insert(m_to_send.end(), data.begin(), data.end());
    }

    /** Schedule bytes to be sent to the transport. */
    void Send(Span<const std::byte> data) { Send(MakeUCharSpan(data)); }

    /** Schedule a message to be sent to us by the transport. */
    void AddMessage(Sv2NetMsg msg)
    {
        m_msg_to_send.push_back(std::move(msg));
    }

    /**
     * If we are the initiator, the send buffer should contain our ephemeral public
     * key. Pass this to the peer cipher and clear the buffer.
     *
     * If we are the responder, put the peer ephemeral public key on our receive buffer.
     */
    void ProcessHandshake1() {
        if (m_test_initiator) {
            BOOST_REQUIRE(m_received.size() == Sv2HandshakeState::ELLSWIFT_PUB_KEY_SIZE);
            m_peer_cipher->GetHandshakeState().ReadMsgEphemeralPK(MakeWritableByteSpan(m_received));
            m_received.clear();
        } else {
            BOOST_REQUIRE(m_to_send.empty());
            m_to_send.resize(Sv2HandshakeState::ELLSWIFT_PUB_KEY_SIZE);
            m_peer_cipher->GetHandshakeState().WriteMsgEphemeralPK(MakeWritableByteSpan(m_to_send));
        }

    }

    /** Expect key to have been received from transport and process it.
     *
     * Many other Sv2TransportTester functions cannot be called until after
     * ProcessHandshake2() has been called, as no encryption keys are set up before that point.
     */
    void ProcessHandshake2()
    {
        if (m_test_initiator) {
            BOOST_REQUIRE(m_to_send.empty());

            // Have the peer cypher write the second part of the handshake into our receive buffer
            m_to_send.resize(Sv2HandshakeState::HANDSHAKE_STEP2_SIZE);
            m_peer_cipher->GetHandshakeState().WriteMsgES(MakeWritableByteSpan(m_to_send));

            // At this point the peer is done with the handshake:
            m_peer_cipher->FinishHandshake();
        } else {
            BOOST_REQUIRE(m_received.size() == Sv2HandshakeState::HANDSHAKE_STEP2_SIZE);
            BOOST_REQUIRE(m_peer_cipher->GetHandshakeState().ReadMsgES(MakeWritableByteSpan(m_received)));
            m_received.clear();

            m_peer_cipher->FinishHandshake();
        }
    }

    /** Schedule an encrypted packet with specified content to be sent to transport
     *  (only after ReceiveKey). */
    void SendPacket(Sv2NetMsg msg)
    {
        // TODO: randomly break stuff

        std::vector<std::byte> ciphertext;
        const size_t encrypted_payload_size = Sv2Cipher::EncryptedMessageSize(msg.size());
        ciphertext.resize(SV2_HEADER_ENCRYPTED_SIZE + encrypted_payload_size);
        Span<std::byte> buffer_span{MakeWritableByteSpan(ciphertext)};

        // Header
        DataStream ss_header_plain{};
        ss_header_plain << Sv2NetHeader(msg);
        LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Header: %s\n", HexStr(ss_header_plain));
        Span<std::byte> header_encrypted{buffer_span.subspan(0, SV2_HEADER_ENCRYPTED_SIZE)};
        BOOST_REQUIRE(m_peer_cipher->EncryptMessage(ss_header_plain, header_encrypted));

        // Payload
        Span<const std::byte> payload_plain = MakeByteSpan(msg);
        // TODO: truncate very long messages, about 100 bytes at the start and end
        //       is probably enough for most debugging.
        LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Payload: %s\n", HexStr(payload_plain));
        Span<std::byte> payload_encrypted{buffer_span.subspan(SV2_HEADER_ENCRYPTED_SIZE, encrypted_payload_size)};
        BOOST_REQUIRE(m_peer_cipher->EncryptMessage(payload_plain, payload_encrypted));

        // Schedule it for sending.
        Send(ciphertext);
    }

    /** Expect application packet to have been received, with specified message type and payload.
     *  (only after ReceiveKey). */
    void ReceiveMessage(Sv2NetMsg expected_msg)
    {
        // When processing a packet, at least enough bytes for its length descriptor must be received.
        BOOST_REQUIRE(m_received.size() >= SV2_HEADER_ENCRYPTED_SIZE);

        auto header_encrypted{MakeWritableByteSpan(m_received).subspan(0, SV2_HEADER_ENCRYPTED_SIZE)};
        std::array<std::byte, SV2_HEADER_PLAIN_SIZE> header_plain;
        BOOST_REQUIRE(m_peer_cipher->DecryptMessage(header_encrypted, header_plain));

        // Decode header
        DataStream ss_header{header_plain};
        node::Sv2NetHeader header;
        ss_header >> header;

        BOOST_CHECK(header.m_msg_type == expected_msg.m_msg_type);

        size_t expanded_size = Sv2Cipher::EncryptedMessageSize(header.m_msg_len);
        BOOST_REQUIRE(m_received.size() >= SV2_HEADER_ENCRYPTED_SIZE + expanded_size);

        Span<std::byte> encrypted_payload{MakeWritableByteSpan(m_received).subspan(SV2_HEADER_ENCRYPTED_SIZE, expanded_size)};
        Span<std::byte> payload = encrypted_payload.subspan(0, header.m_msg_len);

        BOOST_REQUIRE(m_peer_cipher->DecryptMessage(encrypted_payload, payload));

        std::vector<uint8_t> decode_buffer;
        decode_buffer.resize(header.m_msg_len);

        std::transform(payload.begin(), payload.end(), decode_buffer.begin(),
                           [](std::byte b) { return static_cast<uint8_t>(b); });

        // TODO: clear the m_received we used

        Sv2NetMsg message{header.m_msg_type, std::move(decode_buffer)};

        // TODO: compare payload
    }

    /** Test whether the transport's m_hash matches the other side. */
    void CompareHash() const
    {
        BOOST_REQUIRE(m_transport);
        BOOST_CHECK(m_transport->NoiseHash() == m_peer_cipher->GetHash());
    }

    void CheckRecvState(Sv2Transport::RecvState state) {
        BOOST_REQUIRE(m_transport);
        BOOST_CHECK_EQUAL(RecvStateAsString(m_transport->GetRecvState()), RecvStateAsString(state));
    }

    void CheckSendState(Sv2Transport::SendState state) {
        BOOST_REQUIRE(m_transport);
        BOOST_CHECK_EQUAL(SendStateAsString(m_transport->GetSendState()), SendStateAsString(state));
    }

    /** Introduce a bit error in the data scheduled to be sent. */
    // void Damage()
    // {
    //     BOOST_TEST_MESSAGE("[Interact] introduce a bit error");
    //     m_to_send[InsecureRandRange(m_to_send.size())] ^= (uint8_t{1} << InsecureRandRange(8));
    // }
};

} // namespace

BOOST_AUTO_TEST_CASE(sv2_transport_initiator_test)
{
    // A mostly normal scenario, testing a transport in initiator mode.
    // Interact() introduces randomness, so run multiple times
    for (int i = 0; i < 10; ++i) {
        BOOST_TEST_MESSAGE(strprintf("\nIteration %d (initiator)", i));
        Sv2TransportTester tester(true);
        // As the initiator, our ephemeral public key is immedidately put
        // onto the buffer.
        tester.CheckSendState(Sv2Transport::SendState::HANDSHAKE_STEP_2);
        tester.CheckRecvState(Sv2Transport::RecvState::HANDSHAKE_STEP_2);
        auto ret = tester.Interact();
        BOOST_REQUIRE(ret && ret->empty());
        tester.ProcessHandshake1();
        ret = tester.Interact();
        BOOST_REQUIRE(ret && ret->empty());
        tester.ProcessHandshake2();
        ret = tester.Interact();
        BOOST_REQUIRE(ret && ret->empty());
        tester.CheckSendState(Sv2Transport::SendState::READY);
        tester.CheckRecvState(Sv2Transport::RecvState::APP);
        tester.CompareHash();
    }
}

BOOST_AUTO_TEST_CASE(sv2_transport_responder_test)
{
    // Normal scenario, with a transport in responder node.
    for (int i = 0; i < 10; ++i) {
        BOOST_TEST_MESSAGE(strprintf("\nIteration %d (responder)", i));
        Sv2TransportTester tester(false);
        tester.CheckSendState(Sv2Transport::SendState::HANDSHAKE_STEP_2);
        tester.CheckRecvState(Sv2Transport::RecvState::HANDSHAKE_STEP_1);
        tester.ProcessHandshake1();
        auto ret = tester.Interact();
        BOOST_REQUIRE(ret && ret->empty());
        tester.CheckSendState(Sv2Transport::SendState::READY);
        tester.CheckRecvState(Sv2Transport::RecvState::APP);

        // Have the test cypher process our handshake reply
        tester.ProcessHandshake2();
        tester.CompareHash();

        // Handshake complete, have the initiator send us a message:
        Sv2CoinbaseOutputDataSizeMsg body{4000};
        Sv2NetMsg msg{body};
        BOOST_REQUIRE(msg.m_msg_type == Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE);

        tester.SendPacket(msg);
        ret = tester.Interact();
        BOOST_REQUIRE(ret && ret->size() == 1);
        BOOST_CHECK((*ret)[0] &&
                    (*ret)[0]->m_msg_type == Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE);

        tester.CompareHash();

        // Send a message back to the initiator
        tester.AddMessage(msg);
        ret = tester.Interact();
        BOOST_REQUIRE(ret && ret->size() == 0);
        tester.ReceiveMessage(msg);

        // TODO: send / receive message larger than the chunk size
    }
}


BOOST_AUTO_TEST_SUITE_END()
