// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONNECTION_H
#define BITCOIN_NODE_CONNECTION_H

/* This module encapsulates connecting with an individual peer,
 * translating messages into a byte stream and feeding it to a Sock
 * object. Protocol logic is left for net_processing/PeerManager, and
 * management of multiple peers is left for net/CConnman.
 */

#include <bip324.h>
#include <hash.h>
#include <i2p.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <node/connection_types.h>
#include <protocol.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <util/check.h>
#include <util/sock.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <vector>

/** Maximum length of incoming protocol messages (no message over 4 MB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 4 * 1000 * 1000;
/** Maximum length of the user agent string in `version` message */
static const unsigned int MAX_SUBVERSION_LENGTH = 256;

static const size_t DEFAULT_MAXRECEIVEBUFFER = 5 * 1000;
static const size_t DEFAULT_MAXSENDBUFFER    = 1 * 1000;

typedef int64_t NodeId;

struct CSerializedNetMsg {
    CSerializedNetMsg() = default;
    CSerializedNetMsg(CSerializedNetMsg&&) = default;
    CSerializedNetMsg& operator=(CSerializedNetMsg&&) = default;
    // No implicit copying, only moves.
    CSerializedNetMsg(const CSerializedNetMsg& msg) = delete;
    CSerializedNetMsg& operator=(const CSerializedNetMsg&) = delete;

    CSerializedNetMsg Copy() const
    {
        CSerializedNetMsg copy;
        copy.data = data;
        copy.m_type = m_type;
        return copy;
    }

    std::vector<unsigned char> data;
    std::string m_type;

    /** Compute total memory usage of this object (own memory + any dynamic memory). */
    size_t GetMemoryUsage() const noexcept;
};

extern const std::string NET_MESSAGE_TYPE_OTHER;
using mapMsgTypeSize = std::map</* message type */ std::string, /* total bytes */ uint64_t>;

class CNodeStats
{
public:
    NodeId nodeid;
    std::chrono::seconds m_last_send;
    std::chrono::seconds m_last_recv;
    std::chrono::seconds m_last_tx_time;
    std::chrono::seconds m_last_block_time;
    std::chrono::seconds m_connected;
    int64_t nTimeOffset;
    std::string m_addr_name;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    // We requested high bandwidth connection to peer
    bool m_bip152_highbandwidth_to;
    // Peer requested high bandwidth connection
    bool m_bip152_highbandwidth_from;
    int m_starting_height;
    uint64_t nSendBytes;
    mapMsgTypeSize mapSendBytesPerMsgType;
    uint64_t nRecvBytes;
    mapMsgTypeSize mapRecvBytesPerMsgType;
    NetPermissionFlags m_permission_flags;
    std::chrono::microseconds m_last_ping_time;
    std::chrono::microseconds m_min_ping_time;
    // Our address, as reported by the peer
    std::string addrLocal;
    // Address of this peer
    CAddress addr;
    // Bind address of our side of the connection
    CAddress addrBind;
    // Network the peer connected through
    Network m_network;
    uint32_t m_mapped_as;
    ConnectionType m_conn_type;
    /** Transport protocol type. */
    TransportProtocolType m_transport_type;
    /** BIP324 session id string in hex, if any. */
    std::string m_session_id;
};


/** Transport protocol agnostic message container.
 * Ideally it should only contain receive time, payload,
 * type and size.
 */
class CNetMessage {
public:
    CDataStream m_recv;                  //!< received message data
    std::chrono::microseconds m_time{0}; //!< time of message receipt
    uint32_t m_message_size{0};          //!< size of the payload
    uint32_t m_raw_message_size{0};      //!< used wire size of the message (including header/checksum)
    std::string m_type;

    CNetMessage(CDataStream&& recv_in) : m_recv(std::move(recv_in)) {}
    // Only one CNetMessage object will exist for the same message on either
    // the receive or processing queue. For performance reasons we therefore
    // delete the copy constructor and assignment operator to avoid the
    // possibility of copying CNetMessage objects.
    CNetMessage(CNetMessage&&) = default;
    CNetMessage(const CNetMessage&) = delete;
    CNetMessage& operator=(CNetMessage&&) = default;
    CNetMessage& operator=(const CNetMessage&) = delete;

    void SetVersion(int nVersionIn)
    {
        m_recv.SetVersion(nVersionIn);
    }
};

/** The Transport converts one connection's sent messages to wire bytes, and received bytes back. */
class Transport {
public:
    virtual ~Transport() {}

    struct Info
    {
        TransportProtocolType transport_type;
        std::optional<uint256> session_id;
    };

    /** Retrieve information about this transport. */
    virtual Info GetInfo() const noexcept = 0;

    // 1. Receiver side functions, for decoding bytes received on the wire into transport protocol
    // agnostic CNetMessage (message type & payload) objects.

    /** Returns true if the current message is complete (so GetReceivedMessage can be called). */
    virtual bool ReceivedMessageComplete() const = 0;

    /** Feed wire bytes to the transport.
     *
     * @return false if some bytes were invalid, in which case the transport can't be used anymore.
     *
     * Consumed bytes are chopped off the front of msg_bytes.
     */
    virtual bool ReceivedBytes(Span<const uint8_t>& msg_bytes) = 0;

    /** Retrieve a completed message from transport.
     *
     * This can only be called when ReceivedMessageComplete() is true.
     *
     * If reject_message=true is returned the message itself is invalid, but (other than false
     * returned by ReceivedBytes) the transport is not in an inconsistent state.
     */
    virtual CNetMessage GetReceivedMessage(std::chrono::microseconds time, bool& reject_message) = 0;

    // 2. Sending side functions, for converting messages into bytes to be sent over the wire.

    /** Set the next message to send.
     *
     * If no message can currently be set (perhaps because the previous one is not yet done being
     * sent), returns false, and msg will be unmodified. Otherwise msg is enqueued (and
     * possibly moved-from) and true is returned.
     */
    virtual bool SetMessageToSend(CSerializedNetMsg& msg) noexcept = 0;

    /** Return type for GetBytesToSend, consisting of:
     *  - Span<const uint8_t> to_send: span of bytes to be sent over the wire (possibly empty).
     *  - bool more: whether there will be more bytes to be sent after the ones in to_send are
     *    all sent (as signaled by MarkBytesSent()).
     *  - const std::string& m_type: message type on behalf of which this is being sent
     *    ("" for bytes that are not on behalf of any message).
     */
    using BytesToSend = std::tuple<
        Span<const uint8_t> /*to_send*/,
        bool /*more*/,
        const std::string& /*m_type*/
    >;

    /** Get bytes to send on the wire, if any, along with other information about it.
     *
     * As a const function, it does not modify the transport's observable state, and is thus safe
     * to be called multiple times.
     *
     * @param[in] have_next_message If true, the "more" return value reports whether more will
     *            be sendable after a SetMessageToSend call. It is set by the caller when they know
     *            they have another message ready to send, and only care about what happens
     *            after that. The have_next_message argument only affects this "more" return value
     *            and nothing else.
     *
     *            Effectively, there are three possible outcomes about whether there are more bytes
     *            to send:
     *            - Yes:     the transport itself has more bytes to send later. For example, for
     *                       V1Transport this happens during the sending of the header of a
     *                       message, when there is a non-empty payload that follows.
     *            - No:      the transport itself has no more bytes to send, but will have bytes to
     *                       send if handed a message through SetMessageToSend. In V1Transport this
     *                       happens when sending the payload of a message.
     *            - Blocked: the transport itself has no more bytes to send, and is also incapable
     *                       of sending anything more at all now, if it were handed another
     *                       message to send. This occurs in V2Transport before the handshake is
     *                       complete, as the encryption ciphers are not set up for sending
     *                       messages before that point.
     *
     *            The boolean 'more' is true for Yes, false for Blocked, and have_next_message
     *            controls what is returned for No.
     *
     * @return a BytesToSend object. The to_send member returned acts as a stream which is only
     *         ever appended to. This means that with the exception of MarkBytesSent (which pops
     *         bytes off the front of later to_sends), operations on the transport can only append
     *         to what is being returned. Also note that m_type and to_send refer to data that is
     *         internal to the transport, and calling any non-const function on this object may
     *         invalidate them.
     */
    virtual BytesToSend GetBytesToSend(bool have_next_message) const noexcept = 0;

    /** Report how many bytes returned by the last GetBytesToSend() have been sent.
     *
     * bytes_sent cannot exceed to_send.size() of the last GetBytesToSend() result.
     *
     * If bytes_sent=0, this call has no effect.
     */
    virtual void MarkBytesSent(size_t bytes_sent) noexcept = 0;

    /** Return the memory usage of this transport attributable to buffered data to send. */
    virtual size_t GetSendMemoryUsage() const noexcept = 0;

    // 3. Miscellaneous functions.

    /** Whether upon disconnections, a reconnect with V1 is warranted. */
    virtual bool ShouldReconnectV1() const noexcept = 0;
};

class V1Transport final : public Transport
{
private:
    MessageStartChars m_magic_bytes;
    const NodeId m_node_id; // Only for logging
    mutable Mutex m_recv_mutex; //!< Lock for receive state
    mutable CHash256 hasher GUARDED_BY(m_recv_mutex);
    mutable uint256 data_hash GUARDED_BY(m_recv_mutex);
    bool in_data GUARDED_BY(m_recv_mutex); // parsing header (false) or data (true)
    CDataStream hdrbuf GUARDED_BY(m_recv_mutex); // partially received header
    CMessageHeader hdr GUARDED_BY(m_recv_mutex); // complete header
    CDataStream vRecv GUARDED_BY(m_recv_mutex); // received message data
    unsigned int nHdrPos GUARDED_BY(m_recv_mutex);
    unsigned int nDataPos GUARDED_BY(m_recv_mutex);

    const uint256& GetMessageHash() const EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    int readHeader(Span<const uint8_t> msg_bytes) EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);
    int readData(Span<const uint8_t> msg_bytes) EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex);

    void Reset() EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex) {
        AssertLockHeld(m_recv_mutex);
        vRecv.clear();
        hdrbuf.clear();
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        data_hash.SetNull();
        hasher.Reset();
    }

    bool CompleteInternal() const noexcept EXCLUSIVE_LOCKS_REQUIRED(m_recv_mutex)
    {
        AssertLockHeld(m_recv_mutex);
        if (!in_data) return false;
        return hdr.nMessageSize == nDataPos;
    }

    /** Lock for sending state. */
    mutable Mutex m_send_mutex;
    /** The header of the message currently being sent. */
    std::vector<uint8_t> m_header_to_send GUARDED_BY(m_send_mutex);
    /** The data of the message currently being sent. */
    CSerializedNetMsg m_message_to_send GUARDED_BY(m_send_mutex);
    /** Whether we're currently sending header bytes or message bytes. */
    bool m_sending_header GUARDED_BY(m_send_mutex) {false};
    /** How many bytes have been sent so far (from m_header_to_send, or from m_message_to_send.data). */
    size_t m_bytes_sent GUARDED_BY(m_send_mutex) {0};

public:
    V1Transport(const NodeId node_id, int nTypeIn, int nVersionIn) noexcept;

    bool ReceivedMessageComplete() const override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex)
    {
        AssertLockNotHeld(m_recv_mutex);
        return WITH_LOCK(m_recv_mutex, return CompleteInternal());
    }

    Info GetInfo() const noexcept override;

    bool ReceivedBytes(Span<const uint8_t>& msg_bytes) override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex)
    {
        AssertLockNotHeld(m_recv_mutex);
        LOCK(m_recv_mutex);
        int ret = in_data ? readData(msg_bytes) : readHeader(msg_bytes);
        if (ret < 0) {
            Reset();
        } else {
            msg_bytes = msg_bytes.subspan(ret);
        }
        return ret >= 0;
    }

    CNetMessage GetReceivedMessage(std::chrono::microseconds time, bool& reject_message) override EXCLUSIVE_LOCKS_REQUIRED(!m_recv_mutex);

    bool SetMessageToSend(CSerializedNetMsg& msg) noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);
    BytesToSend GetBytesToSend(bool have_next_message) const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);
    void MarkBytesSent(size_t bytes_sent) noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);
    size_t GetSendMemoryUsage() const noexcept override EXCLUSIVE_LOCKS_REQUIRED(!m_send_mutex);
    bool ShouldReconnectV1() const noexcept override { return false; }
};

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
    /** Deserialization type. */
    const int m_recv_type;
    /** Deserialization version number. */
    const int m_recv_version;
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
     * @param[in] type_in     the serialization type of returned CNetMessages.
     * @param[in] version_in  the serialization version of returned CNetMessages.
     */
    V2Transport(NodeId nodeid, bool initiating, int type_in, int version_in) noexcept;

    /** Construct a V2 transport with specified keys and garbage (test use only). */
    V2Transport(NodeId nodeid, bool initiating, int type_in, int version_in, const CKey& key, Span<const std::byte> ent32, std::vector<uint8_t> garbage) noexcept;

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

struct CNodeOptions
{
    NetPermissionFlags permission_flags = NetPermissionFlags::None;
    std::unique_ptr<i2p::sam::Session> i2p_sam_session = nullptr;
    bool prefer_evict = false;
    size_t recv_flood_size{DEFAULT_MAXRECEIVEBUFFER * 1000};
    bool use_v2transport = false;
};

/** Information about a peer */
class CNode
{
public:
    /** Transport serializer/deserializer. The receive side functions are only called under cs_vRecv, while
     * the sending side functions are only called under cs_vSend. */
    const std::unique_ptr<Transport> m_transport;

    const NetPermissionFlags m_permission_flags;

    /**
     * Socket used for communication with the node.
     * May not own a Sock object (after `CloseSocketDisconnect()` or during tests).
     * `shared_ptr` (instead of `unique_ptr`) is used to avoid premature close of
     * the underlying file descriptor by one thread while another thread is
     * poll(2)-ing it for activity.
     * @see https://github.com/bitcoin/bitcoin/issues/21744 for details.
     */
    std::shared_ptr<Sock> m_sock GUARDED_BY(m_sock_mutex);

    /** Sum of GetMemoryUsage of all vSendMsg entries. */
    size_t m_send_memusage GUARDED_BY(cs_vSend){0};
    /** Total number of bytes sent on the wire to this peer. */
    uint64_t nSendBytes GUARDED_BY(cs_vSend){0};
    /** Messages still to be fed to m_transport->SetMessageToSend. */
    std::deque<CSerializedNetMsg> vSendMsg GUARDED_BY(cs_vSend);
    Mutex cs_vSend;
    Mutex m_sock_mutex;
    Mutex cs_vRecv;

    uint64_t nRecvBytes GUARDED_BY(cs_vRecv){0};

    std::atomic<std::chrono::seconds> m_last_send{0s};
    std::atomic<std::chrono::seconds> m_last_recv{0s};
    //! Unix epoch time at peer connection
    const std::chrono::seconds m_connected;
    std::atomic<int64_t> nTimeOffset{0};
    // Address of this peer
    const CAddress addr;
    // Bind address of our side of the connection
    const CAddress addrBind;
    const std::string m_addr_name;
    /** The pszDest argument provided to ConnectNode(). Only used for reconnections. */
    const std::string m_dest;
    //! Whether this peer is an inbound onion, i.e. connected via our Tor onion service.
    const bool m_inbound_onion;
    std::atomic<int> nVersion{0};
    Mutex m_subver_mutex;
    /**
     * cleanSubVer is a sanitized string of the user agent byte array we read
     * from the wire. This cleaned string can safely be logged or displayed.
     */
    std::string cleanSubVer GUARDED_BY(m_subver_mutex){};
    const bool m_prefer_evict{false}; // This peer is preferred for eviction.
    bool HasPermission(NetPermissionFlags permission) const {
        return NetPermissions::HasFlag(m_permission_flags, permission);
    }
    /** fSuccessfullyConnected is set to true on receiving VERACK from the peer. */
    std::atomic_bool fSuccessfullyConnected{false};
    // Setting fDisconnect to true will cause the node to be disconnected the
    // next time DisconnectNodes() runs
    std::atomic_bool fDisconnect{false};
    CSemaphoreGrant grantOutbound;
    std::atomic<int> nRefCount{0};

    const uint64_t nKeyedNetGroup;
    std::atomic_bool fPauseRecv{false};
    std::atomic_bool fPauseSend{false};

    const ConnectionType m_conn_type;

    /** Move all messages from the received queue to the processing queue. */
    void MarkReceivedMsgsForProcessing()
        EXCLUSIVE_LOCKS_REQUIRED(!m_msg_process_queue_mutex);

    /** Poll the next message from the processing queue of this connection.
     *
     * Returns std::nullopt if the processing queue is empty, or a pair
     * consisting of the message and a bool that indicates if the processing
     * queue has more entries. */
    std::optional<std::pair<CNetMessage, bool>> PollMessage()
        EXCLUSIVE_LOCKS_REQUIRED(!m_msg_process_queue_mutex);

    /** Account for the total size of a sent message in the per msg type connection stats. */
    void AccountForSentBytes(const std::string& msg_type, size_t sent_bytes)
        EXCLUSIVE_LOCKS_REQUIRED(cs_vSend)
    {
        mapSendBytesPerMsgType[msg_type] += sent_bytes;
    }

    bool IsOutboundOrBlockRelayConn() const {
        switch (m_conn_type) {
            case ConnectionType::OUTBOUND_FULL_RELAY:
            case ConnectionType::BLOCK_RELAY:
                return true;
            case ConnectionType::INBOUND:
            case ConnectionType::MANUAL:
            case ConnectionType::ADDR_FETCH:
            case ConnectionType::FEELER:
                return false;
        } // no default case, so the compiler can warn about missing cases

        assert(false);
    }

    bool IsFullOutboundConn() const {
        return m_conn_type == ConnectionType::OUTBOUND_FULL_RELAY;
    }

    bool IsManualConn() const {
        return m_conn_type == ConnectionType::MANUAL;
    }

    bool IsManualOrFullOutboundConn() const
    {
        switch (m_conn_type) {
        case ConnectionType::INBOUND:
        case ConnectionType::FEELER:
        case ConnectionType::BLOCK_RELAY:
        case ConnectionType::ADDR_FETCH:
                return false;
        case ConnectionType::OUTBOUND_FULL_RELAY:
        case ConnectionType::MANUAL:
                return true;
        } // no default case, so the compiler can warn about missing cases

        assert(false);
    }

    bool IsBlockOnlyConn() const {
        return m_conn_type == ConnectionType::BLOCK_RELAY;
    }

    bool IsFeelerConn() const {
        return m_conn_type == ConnectionType::FEELER;
    }

    bool IsAddrFetchConn() const {
        return m_conn_type == ConnectionType::ADDR_FETCH;
    }

    bool IsInboundConn() const {
        return m_conn_type == ConnectionType::INBOUND;
    }

    bool ExpectServicesFromConn() const {
        switch (m_conn_type) {
            case ConnectionType::INBOUND:
            case ConnectionType::MANUAL:
            case ConnectionType::FEELER:
                return false;
            case ConnectionType::OUTBOUND_FULL_RELAY:
            case ConnectionType::BLOCK_RELAY:
            case ConnectionType::ADDR_FETCH:
                return true;
        } // no default case, so the compiler can warn about missing cases

        assert(false);
    }

    /**
     * Get network the peer connected through.
     *
     * Returns Network::NET_ONION for *inbound* onion connections,
     * and CNetAddr::GetNetClass() otherwise. The latter cannot be used directly
     * because it doesn't detect the former, and it's not the responsibility of
     * the CNetAddr class to know the actual network a peer is connected through.
     *
     * @return network the peer connected through.
     */
    Network ConnectedThroughNetwork() const;

    /** Whether this peer connected through a privacy network. */
    [[nodiscard]] bool IsConnectedThroughPrivacyNet() const;

    // We selected peer as (compact blocks) high-bandwidth peer (BIP152)
    std::atomic<bool> m_bip152_highbandwidth_to{false};
    // Peer selected us as (compact blocks) high-bandwidth peer (BIP152)
    std::atomic<bool> m_bip152_highbandwidth_from{false};

    /** Whether this peer provides all services that we want. Used for eviction decisions */
    std::atomic_bool m_has_all_wanted_services{false};

    /** Whether we should relay transactions to this peer. This only changes
     * from false to true. It will never change back to false. */
    std::atomic_bool m_relays_txs{false};

    /** Whether this peer has loaded a bloom filter. Used only in inbound
     *  eviction logic. */
    std::atomic_bool m_bloom_filter_loaded{false};

    /** UNIX epoch time of the last block received from this peer that we had
     * not yet seen (e.g. not already received from another peer), that passed
     * preliminary validity checks and was saved to disk, even if we don't
     * connect the block or it eventually fails connection. Used as an inbound
     * peer eviction criterium in CConnman::AttemptToEvictConnection. */
    std::atomic<std::chrono::seconds> m_last_block_time{0s};

    /** UNIX epoch time of the last transaction received from this peer that we
     * had not yet seen (e.g. not already received from another peer) and that
     * was accepted into our mempool. Used as an inbound peer eviction criterium
     * in CConnman::AttemptToEvictConnection. */
    std::atomic<std::chrono::seconds> m_last_tx_time{0s};

    /** Last measured round-trip time. Used only for RPC/GUI stats/debugging.*/
    std::atomic<std::chrono::microseconds> m_last_ping_time{0us};

    /** Lowest measured round-trip time. Used as an inbound peer eviction
     * criterium in CConnman::AttemptToEvictConnection. */
    std::atomic<std::chrono::microseconds> m_min_ping_time{std::chrono::microseconds::max()};

    CNode(NodeId id,
          std::shared_ptr<Sock> sock,
          const CAddress& addrIn,
          uint64_t nKeyedNetGroupIn,
          uint64_t nLocalHostNonceIn,
          const CAddress& addrBindIn,
          const std::string& addrNameIn,
          ConnectionType conn_type_in,
          bool inbound_onion,
          CNodeOptions&& node_opts = {});
    CNode(const CNode&) = delete;
    CNode& operator=(const CNode&) = delete;

    NodeId GetId() const {
        return id;
    }

    uint64_t GetLocalNonce() const {
        return nLocalHostNonce;
    }

    int GetRefCount() const
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    /**
     * Receive bytes from the buffer and deserialize them into messages.
     *
     * @param[in]   msg_bytes   The raw data
     * @param[out]  complete    Set True if at least one message has been
     *                          deserialized and is ready to be processed
     * @return  True if the peer should stay connected,
     *          False if the peer should be disconnected from.
     */
    bool ReceiveMsgBytes(Span<const uint8_t> msg_bytes, bool& complete) EXCLUSIVE_LOCKS_REQUIRED(!cs_vRecv);

    void SetCommonVersion(int greatest_common_version)
    {
        Assume(m_greatest_common_version == INIT_PROTO_VERSION);
        m_greatest_common_version = greatest_common_version;
    }
    int GetCommonVersion() const
    {
        return m_greatest_common_version;
    }

    CService GetAddrLocal() const EXCLUSIVE_LOCKS_REQUIRED(!m_addr_local_mutex);
    //! May not be called more than once
    void SetAddrLocal(const CService& addrLocalIn) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_local_mutex);

    CNode* AddRef()
    {
        nRefCount++;
        return this;
    }

    void Release()
    {
        nRefCount--;
    }

    void CloseSocketDisconnect() EXCLUSIVE_LOCKS_REQUIRED(!m_sock_mutex);

    void CopyStats(CNodeStats& stats) EXCLUSIVE_LOCKS_REQUIRED(!m_subver_mutex, !m_addr_local_mutex, !cs_vSend, !cs_vRecv);

    std::string ConnectionTypeAsString() const { return ::ConnectionTypeAsString(m_conn_type); }

    /** A ping-pong round trip has completed successfully. Update latest and minimum ping times. */
    void PongReceived(std::chrono::microseconds ping_time) {
        m_last_ping_time = ping_time;
        m_min_ping_time = std::min(m_min_ping_time.load(), ping_time);
    }

private:
    const NodeId id;
    const uint64_t nLocalHostNonce;
    std::atomic<int> m_greatest_common_version{INIT_PROTO_VERSION};

    const size_t m_recv_flood_size;
    std::list<CNetMessage> vRecvMsg; // Used only by SocketHandler thread

    Mutex m_msg_process_queue_mutex;
    std::list<CNetMessage> m_msg_process_queue GUARDED_BY(m_msg_process_queue_mutex);
    size_t m_msg_process_queue_size GUARDED_BY(m_msg_process_queue_mutex){0};

    // Our address, as reported by the peer
    CService addrLocal GUARDED_BY(m_addr_local_mutex);
    mutable Mutex m_addr_local_mutex;

    mapMsgTypeSize mapSendBytesPerMsgType GUARDED_BY(cs_vSend);
    mapMsgTypeSize mapRecvBytesPerMsgType GUARDED_BY(cs_vRecv);

    /**
     * If an I2P session is created per connection (for outbound transient I2P
     * connections) then it is stored here so that it can be destroyed when the
     * socket is closed. I2P sessions involve a data/transport socket (in `m_sock`)
     * and a control socket (in `m_i2p_sam_session`). For transient sessions, once
     * the data socket is closed, the control socket is not going to be used anymore
     * and is just taking up resources. So better close it as soon as `m_sock` is
     * closed.
     * Otherwise this unique_ptr is empty.
     */
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session GUARDED_BY(m_sock_mutex);
};

#endif // BITCOIN_NODE_CONNECTION_H
