// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_V2_TRANSPORT_H
#define BITCOIN_COMMON_V2_TRANSPORT_H

#include <bip324.h>
#include <common/transport.h>
#include <common/v1_transport.h>
#include <sync.h>

class V2Transport final : public Transport
{
private:
    /** Contents of the version packet to send. BIP324 stipulates that senders should leave this
     *  empty, and receivers should ignore it. Future extensions can change what is sent as long as
     *  an empty version packet contents is interpreted as no extensions supported. */
    static constexpr std::array<std::byte, 0> VERSION_CONTENTS = {};

    /** The length of the V1 prefix to match bytes initially received by responders with to
     *  determine if their peer is speaking V1 or V2. */
    static constexpr size_t V1_PREFIX_LEN = 16;

    // The sender side and receiver side of V2Transport are state machines that are transitioned
    // through, based on what has been received. The receive state corresponds to the contents of,
    // and bytes received to, the receive buffer. The send state controls what can be appended to
    // the send buffer and what can be sent from it.

    /** State type that defines the current contents of the receive buffer and/or how the next
     *  received bytes added to it will be interpreted.
     *
     * Diagram:
     *
     *   start(responder)
     *        |
     *        |  start(initiator)                           /---------\
     *        |          |                                  |         |
     *        v          v                                  v         |
     *  KEY_MAYBE_V1 -> KEY -> GARB_GARBTERM -> VERSION -> APP -> APP_READY
     *        |
     *        \-------> V1
     */
    enum class RecvState : uint8_t {
        /** (Responder only) either v2 public key or v1 header.
         *
         * This is the initial state for responders, before data has been received to distinguish
         * v1 from v2 connections. When that happens, the state becomes either KEY (for v2) or V1
         * (for v1). */
        KEY_MAYBE_V1,

        /** Public key.
         *
         * This is the initial state for initiators, during which the other side's public key is
         * received. When that information arrives, the ciphers get initialized and the state
         * becomes GARB_GARBTERM. */
        KEY,

        /** Garbage and garbage terminator.
         *
         * Whenever a byte is received, the last 16 bytes are compared with the expected garbage
         * terminator. When that happens, the state becomes VERSION. If no matching terminator is
         * received in 4111 bytes (4095 for the maximum garbage length, and 16 bytes for the
         * terminator), the connection aborts. */
        GARB_GARBTERM,

        /** Version packet.
         *
         * A packet is received, and decrypted/verified. If that fails, the connection aborts. The
         * first received packet in this state (whether it's a decoy or not) is expected to
         * authenticate the garbage received during the GARB_GARBTERM state as associated
         * authenticated data (AAD). The first non-decoy packet in this state is interpreted as
         * version negotiation (currently, that means ignoring the contents, but it can be used for
         * negotiating future extensions), and afterwards the state becomes APP. */
        VERSION,

        /** Application packet.
         *
         * A packet is received, and decrypted/verified. If that succeeds, the state becomes
         * APP_READY and the decrypted contents is kept in m_recv_decode_buffer until it is
         * retrieved as a message by GetMessage(). */
        APP,

        /** Nothing (an application packet is available for GetMessage()).
         *
         * Nothing can be received in this state. When the message is retrieved by GetMessage,
         * the state becomes APP again. */
        APP_READY,

        /** Nothing (this transport is using v1 fallback).
         *
         * All receive operations are redirected to m_v1_fallback. */
        V1,
    };

    /** State type that controls the sender side.
     *
     * Diagram:
     *
     *  start(responder)
     *      |
     *      |      start(initiator)
     *      |            |
     *      v            v
     *  MAYBE_V1 -> AWAITING_KEY -> READY
     *      |
     *      \-----> V1
     */
    enum class SendState : uint8_t {
        /** (Responder only) Not sending until v1 or v2 is detected.
         *
         * This is the initial state for responders. The send buffer is empty.
         * When the receiver determines whether this
         * is a V1 or V2 connection, the sender state becomes AWAITING_KEY (for v2) or V1 (for v1).
         */
        MAYBE_V1,

        /** Waiting for the other side's public key.
         *
         * This is the initial state for initiators. The public key and garbage is sent out. When
         * the receiver receives the other side's public key and transitions to GARB_GARBTERM, the
         * sender state becomes READY. */
        AWAITING_KEY,

        /** Normal sending state.
         *
         * In this state, the ciphers are initialized, so packets can be sent. When this state is
         * entered, the garbage terminator and version packet are appended to the send buffer (in
         * addition to the key and garbage which may still be there). In this state a message can be
         * provided if the send buffer is empty. */
        READY,

        /** This transport is using v1 fallback.
         *
         * All send operations are redirected to m_v1_fallback. */
        V1,
    };

    /** Cipher state. */
    BIP324Cipher m_cipher;
    /** Whether we are the initiator side. */
    const bool m_initiating;
    /** NodeId (for debug logging). */
    const NodeId m_nodeid;
    /** Encapsulate a V1Transport to fall back to. */
    V1Transport m_v1_fallback;

    /** Lock for receiver-side fields. */
    mutable Mutex m_recv_mutex ACQUIRED_BEFORE(m_send_mutex);
    /** In {VERSION, APP}, the decrypted packet length, if m_recv_buffer.size() >=
     *  BIP324Cipher::LENGTH_LEN. Unspecified otherwise. */
    uint32_t m_recv_len GUARDED_BY(m_recv_mutex) {0};
    /** Receive buffer; meaning is determined by m_recv_state. */
    std::vector<uint8_t> m_recv_buffer GUARDED_BY(m_recv_mutex);
    /** AAD expected in next received packet (currently used only for garbage). */
    std::vector<uint8_t> m_recv_aad GUARDED_BY(m_recv_mutex);
    /** Buffer to put decrypted contents in, for converting to CNetMessage. */
    std::vector<uint8_t> m_recv_decode_buffer GUARDED_BY(m_recv_mutex);
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
    /** Whether we've sent at least 24 bytes (which would trigger disconnect for V1 peers). */
    bool m_sent_v1_header_worth GUARDED_BY(m_send_mutex) {false};

    /** Change the receive state. */
    void SetReceiveState(RecvState recv_state) noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    /** Change the send state. */
    void SetSendState(SendState send_state) noexcept EXCLUSIVE_LOCKS_REQUIRED(m_send_mutex);
    /** Given a packet's contents, find the message type (if valid), and strip it from contents. */
    static std::optional<std::string> GetMessageType(Span<const uint8_t>& contents) noexcept;
    /** Determine how many received bytes can be processed in one go (not allowed in V1 state). */
    size_t GetMaxBytesToProcess() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    /** Put our public key + garbage in the send buffer. */
    void StartSendingHandshake() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_send_mutex);
    /** Process bytes in m_recv_buffer, while in KEY_MAYBE_V1 state. */
    void ProcessReceivedMaybeV1Bytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex, !m_send_mutex);
    /** Process bytes in m_recv_buffer, while in KEY state. */
    bool ProcessReceivedKeyBytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex, !m_send_mutex);
    /** Process bytes in m_recv_buffer, while in GARB_GARBTERM state. */
    bool ProcessReceivedGarbageBytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    /** Process bytes in m_recv_buffer, while in VERSION/APP state. */
    bool ProcessReceivedPacketBytes() noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);

public:
    static constexpr uint32_t MAX_GARBAGE_LEN = 4095;

    /** Construct a V2 transport with securely generated random keys.
     *
     * @param[in] nodeid      the node's NodeId (only for debug log output).
     * @param[in] initiating  whether we are the initiator side.
     */
    V2Transport(NodeId nodeid, bool initiating) noexcept;

    /** Construct a V2 transport with specified keys and garbage (test use only). */
    V2Transport(NodeId nodeid, bool initiating, const CKey& key, Span<const std::byte> ent32, std::vector<uint8_t> garbage) noexcept;

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
    bool ShouldReconnectV1() const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex, !m_send_mutex);
    Info GetInfo() const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex);
};

#endif // BITCOIN_COMMON_V2_TRANSPORT_H
