// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SV2_TRANSPORT_H
#define BITCOIN_COMMON_SV2_TRANSPORT_H

#include <common/sv2_messages.h>
#include <common/sv2_noise.h>
#include <net.h> // For Transport, CNetMessage and CSerializedNetMsg
#include <sync.h>

static constexpr size_t SV2_HEADER_PLAIN_SIZE{6};
static constexpr size_t SV2_HEADER_ENCRYPTED_SIZE{SV2_HEADER_PLAIN_SIZE + Poly1305::TAGLEN};

using node::Sv2NetHeader;
using node::Sv2NetMsg;

class Sv2Transport final : public Transport
{
public:

    // The sender side and receiver side of Sv2Transport are state machines that are transitioned
    // through, based on what has been received. The receive state corresponds to the contents of,
    // and bytes received to, the receive buffer. The send state controls what can be appended to
    // the send buffer and what can be sent from it.

    /** State type that defines the current contents of the receive buffer and/or how the next
     *  received bytes added to it will be interpreted.
     *
     * Diagram:
     *
     *   start(responder)
     *        |              start(initiator)
     *        |                     |            /---------\
     *        |                     |            |         |
     *        v                     v            v         |
     *  HANDSHAKE_STEP_1 -> HANDSHAKE_STEP_2 -> APP -> APP_READY
     */
    enum class RecvState : uint8_t {
        /** Handshake Act 1: -> E */
        HANDSHAKE_STEP_1,

        /** Handshake Act 2: <- e, ee, s, es, SIGNATURE_NOISE_MESSAGE */
        HANDSHAKE_STEP_2,

        /** Application packet.
         *
         * A packet is received, and decrypted/verified. If that succeeds, the
         * state becomes APP_READY and the decrypted message is kept in m_message
         * until it is retrieved by GetMessage(). */
        APP,

        /** Nothing (an application packet is available for GetMessage()).
         *
         * Nothing can be received in this state. When the message is retrieved
         * by GetMessage(), the state becomes APP again. */
        APP_READY,
    };

    /** State type that controls the sender side.
     *
     * Diagram:
     *
     *   start(initiator)
     *        |             start(responder)
     *        |                  |
     *        |                  |
     *        v                  v
     *  HANDSHAKE_STEP_1 -> HANDSHAKE_STEP_2 -> READY
     */
    enum class SendState : uint8_t {
        /** Handshake Act 1: -> E */
        HANDSHAKE_STEP_1,

        /** Handshake Act 2: <- e, ee, s, es, SIGNATURE_NOISE_MESSAGE */
        HANDSHAKE_STEP_2,

        /** Normal sending state.
         *
         * In this state, the ciphers are initialized, so packets can be sent.
         * In this state a message can be provided if the send buffer is empty. */
        READY,
    };

private:

    /** Cipher state. */
    Sv2Cipher m_cipher;

    /** Whether we are the initiator side. */
    const bool m_initiating;

    /** Lock for receiver-side fields. */
    mutable Mutex m_recv_mutex ACQUIRED_BEFORE(m_send_mutex);
    /** Receive buffer; meaning is determined by m_recv_state. */
    std::vector<uint8_t> m_recv_buffer GUARDED_BY(m_recv_mutex);
    /** AAD expected in next received packet (currently used only for garbage). */
    std::vector<uint8_t> m_recv_aad GUARDED_BY(m_recv_mutex);
    /** Current receiver state. */
    RecvState m_recv_state GUARDED_BY(m_recv_mutex);

    /** Lock for sending-side fields. If both sending and receiving fields are accessed,
     *  m_recv_mutex must be acquired before m_send_mutex. */
    mutable Mutex m_send_mutex ACQUIRED_AFTER(m_recv_mutex);
    /** The send buffer; meaning is determined by m_send_state. */
    std::vector<uint8_t> m_send_buffer GUARDED_BY(m_send_mutex);
    /** How many bytes from the send buffer have been sent so far. */
    uint32_t m_send_pos GUARDED_BY(m_send_mutex) {0};
    /** The garbage sent, or to be sent (MAYBE_V1 and AWAITING_KEY state only). */
    std::vector<uint8_t> m_send_garbage GUARDED_BY(m_send_mutex);
    /** Type of the message being sent. */
    std::string m_send_type GUARDED_BY(m_send_mutex);
    /** Current sender state. */
    SendState m_send_state GUARDED_BY(m_send_mutex);

    /** Change the receive state. */
    void SetReceiveState(RecvState recv_state) noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    /** Change the send state. */
    void SetSendState(SendState send_state) noexcept EXCLUSIVE_LOCKS_REQUIRED(m_send_mutex);
    /** Given a packet's contents, find the message type (if valid), and strip it from contents. */
    static std::optional<std::string> GetMessageType(Span<const uint8_t>& contents) noexcept;
    /** Determine how many received bytes can be processed in one go (not allowed in V1 state). */
    size_t GetMaxBytesToProcess() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    /** Put our ephemeral public key in the send buffer. */
    void StartSendingHandshake() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_send_mutex, !m_recv_mutex);
    /** Put second part of the handshake in the send buffer. */
    void SendHandshakeReply() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_send_mutex, m_recv_mutex);
    /** Process bytes in m_recv_buffer, while in HANDSHAKE_STEP_1 state. */
    bool ProcessReceivedEphemeralKeyBytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex, !m_send_mutex);
    /** Process bytes in m_recv_buffer, while in HANDSHAKE_STEP_2 state. */
    bool ProcessReceivedHandshakeReplyBytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex, !m_send_mutex);

    /** Process bytes in m_recv_buffer, while in VERSION/APP state. */
    bool ProcessReceivedPacketBytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);

    /** In APP, the decrypted header, if m_recv_buffer.size() >=
    *  SV2_HEADER_ENCRYPTED_SIZE. Unspecified otherwise. */
    Sv2NetHeader m_header GUARDED_BY(m_recv_mutex);
    /* In APP_READY the last retrieved message. Unspecified otherwise */
    Sv2NetMsg m_message GUARDED_BY(m_recv_mutex);

public:
    /** Construct a Stratum v2 transport as the initiator
      *
      * @param[in] static_key a securely generated key

      */
    Sv2Transport(CKey static_key, XOnlyPubKey responder_authority_key) noexcept;

        /** Construct a Stratum v2 transport as the responder
      *
      * @param[in] static_key a securely generated key

      */
    Sv2Transport(CKey static_key, Sv2SignatureNoiseMessage certificate) noexcept;

    // Receive side functions.
    bool ReceivedMessageComplete() const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex);
    bool ReceivedBytes(Span<const uint8_t>& msg_bytes) noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex, !m_send_mutex);

    CNetMessage GetReceivedMessage(std::chrono::microseconds time, bool& reject_message) noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex);

    // Send side functions.
    bool SetMessageToSend(CSerializedNetMsg& msg) noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);

    BytesToSend GetBytesToSend(bool have_next_message) const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);

    void MarkBytesSent(size_t bytes_sent) noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);
    size_t GetSendMemoryUsage() const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);

    // Miscellaneous functions.
    bool ShouldReconnectV1() const noexcept override { return false; };
    Info GetInfo() const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex);

    // Test only
    uint256 NoiseHash() const { return m_cipher.GetHash(); };
    RecvState GetRecvState() EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex) {
        AssertLockNotHeld(m_recv_mutex);
        LOCK(m_recv_mutex);
        return  m_recv_state;
    };
    SendState GetSendState() EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex) {
        AssertLockNotHeld(m_send_mutex);
        LOCK(m_send_mutex);
        return  m_send_state;
    };
};

/** Convert TransportProtocolType enum to a string value */
std::string RecvStateAsString(Sv2Transport::RecvState state);
std::string SendStateAsString(Sv2Transport::SendState state);

#endif // BITCOIN_COMMON_SV2_TRANSPORT_H
