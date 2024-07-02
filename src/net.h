// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include <bip324.h>
#include <chainparams.h>
#include <common/bloom.h>
#include <compat/compat.h>
#include <consensus/amount.h>
#include <crypto/siphash.h>
#include <hash.h>
#include <i2p.h>
#include <kernel/messagestartchars.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <node/connection_types.h>
#include <node/protocol_version.h>
#include <policy/feerate.h>
#include <protocol.h>
#include <random.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <util/check.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

class AddrMan;
class BanMan;
class CChainParams;
class CNode;
class CScheduler;
struct bilingual_str;

/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static constexpr std::chrono::minutes TIMEOUT_INTERVAL{20};
/** Run the feeler connection loop once every 2 minutes. **/
static constexpr auto FEELER_INTERVAL = 2min;
/** Run the extra block-relay-only connection loop once every 5 minutes. **/
static constexpr auto EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL = 5min;
/** Maximum length of incoming protocol messages (no message over 4 MB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 4 * 1000 * 1000;
/** Maximum length of the user agent string in `version` message */
static const unsigned int MAX_SUBVERSION_LENGTH = 256;
/** Maximum number of automatic outgoing nodes over which we'll relay everything (blocks, tx, addrs, etc) */
static const int MAX_OUTBOUND_FULL_RELAY_CONNECTIONS = 8;
/** Maximum number of addnode outgoing nodes */
static const int MAX_ADDNODE_CONNECTIONS = 8;
/** Maximum number of block-relay-only outgoing connections */
static const int MAX_BLOCK_RELAY_ONLY_CONNECTIONS = 2;
/** Maximum number of feeler connections */
static const int MAX_FEELER_CONNECTIONS = 1;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** The maximum number of peer connections to maintain. */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;
/** The default for -maxuploadtarget. 0 = Unlimited */
static const std::string DEFAULT_MAX_UPLOAD_TARGET{"0M"};
/** Default for blocks only*/
static const bool DEFAULT_BLOCKSONLY = false;
/** -peertimeout default */
static const int64_t DEFAULT_PEER_CONNECT_TIMEOUT = 60;
/** Number of file descriptors required for message capture **/
static const int NUM_FDS_MESSAGE_CAPTURE = 1;
/** Interval for ASMap Health Check **/
static constexpr std::chrono::hours ASMAP_HEALTH_CHECK_INTERVAL{24};

static constexpr bool DEFAULT_FORCEDNSSEED{false};
static constexpr bool DEFAULT_DNSSEED{true};
static constexpr bool DEFAULT_FIXEDSEEDS{true};
static const size_t DEFAULT_MAXRECEIVEBUFFER = 5 * 1000;
static const size_t DEFAULT_MAXSENDBUFFER    = 1 * 1000;

static constexpr bool DEFAULT_V2_TRANSPORT{true};

typedef int64_t NodeId;

struct AddedNodeParams {
    std::string m_added_node;
    bool m_use_v2transport;
};

struct AddedNodeInfo {
    AddedNodeParams m_params;
    CService resolvedAddress;
    bool fConnected;
    bool fInbound;
};

class CNodeStats;
class CClientUIInterface;

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

/**
 * Look up IP addresses from all interfaces on the machine and add them to the
 * list of local addresses to self-advertise.
 * The loopback interface is skipped.
 */
void Discover();

uint16_t GetListenPort();

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_MAPPED, // address reported by UPnP or NAT-PMP
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

/** Returns a local address that we should advertise to this peer. */
std::optional<CService> GetLocalAddrForPeer(CNode& node);

bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
void RemoveLocal(const CService& addr);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
CService GetLocalAddress(const CNode& peer);

extern bool fDiscover;
extern bool fListen;

/** Subversion as sent to the P2P network in `version` messages */
extern std::string strSubVersion;

struct LocalServiceInfo {
    int nScore;
    uint16_t nPort;
};

extern GlobalMutex g_maplocalhost_mutex;
extern std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(g_maplocalhost_mutex);

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
class CNetMessage
{
public:
    DataStream m_recv;                   //!< received message data
    std::chrono::microseconds m_time{0}; //!< time of message receipt
    uint32_t m_message_size{0};          //!< size of the payload
    uint32_t m_raw_message_size{0};      //!< used wire size of the message (including header/checksum)
    std::string m_type;

    explicit CNetMessage(DataStream&& recv_in) : m_recv(std::move(recv_in)) {}
    // Only one CNetMessage object will exist for the same message on either
    // the receive or processing queue. For performance reasons we therefore
    // delete the copy constructor and assignment operator to avoid the
    // possibility of copying CNetMessage objects.
    CNetMessage(CNetMessage&&) = default;
    CNetMessage(const CNetMessage&) = delete;
    CNetMessage& operator=(CNetMessage&&) = default;
    CNetMessage& operator=(const CNetMessage&) = delete;
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
    const MessageStartChars m_magic_bytes;
    const NodeId m_node_id; // Only for logging
    mutable Mutex m_recv_mutex; //!< Lock for receive state
    mutable CHash256 hasher GUARDED_BY(m_recv_mutex);
    mutable uint256 data_hash GUARDED_BY(m_recv_mutex);
    bool in_data GUARDED_BY(m_recv_mutex); // parsing header (false) or data (true)
    DataStream hdrbuf GUARDED_BY(m_recv_mutex){}; // partially received header
    CMessageHeader hdr GUARDED_BY(m_recv_mutex); // complete header
    DataStream vRecv GUARDED_BY(m_recv_mutex){}; // received message data
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
    explicit V1Transport(const NodeId node_id) noexcept;

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

/**
 * Interface for message handling
 */
class NetEventsInterface
{
public:
    /** Mutex for anything that is only accessed via the msg processing thread */
    static Mutex g_msgproc_mutex;

    /** Initialize a peer (setup state, queue any initial messages) */
    virtual void InitializeNode(CNode& node, ServiceFlags our_services) = 0;

    /** Handle removal of a peer (clear state) */
    virtual void FinalizeNode(const CNode& node) = 0;

    /**
     * Callback to determine whether the given set of service flags are sufficient
     * for a peer to be "relevant".
     */
    virtual bool HasAllDesirableServiceFlags(ServiceFlags services) const = 0;

    /**
    * Process protocol messages received from a given node
    *
    * @param[in]   pnode           The node which we have received messages from.
    * @param[in]   interrupt       Interrupt condition for processing threads
    * @return                      True if there is more work to be done
    */
    virtual bool ProcessMessages(CNode* pnode, std::atomic<bool>& interrupt) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;

    /**
    * Send queued protocol messages to a given node.
    *
    * @param[in]   pnode           The node which we are sending messages to.
    * @return                      True if there is more work to be done
    */
    virtual bool SendMessages(CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;


protected:
    /**
     * Protected destructor so that instances can only be deleted by derived classes.
     * If that restriction is no longer desired, this should be made public and virtual.
     */
    ~NetEventsInterface() = default;
};

class CConnman
{
public:

    struct Options
    {
        ServiceFlags nLocalServices = NODE_NONE;
        int m_max_automatic_connections = 0;
        CClientUIInterface* uiInterface = nullptr;
        NetEventsInterface* m_msgproc = nullptr;
        BanMan* m_banman = nullptr;
        unsigned int nSendBufferMaxSize = 0;
        unsigned int nReceiveFloodSize = 0;
        uint64_t nMaxOutboundLimit = 0;
        int64_t m_peer_connect_timeout = DEFAULT_PEER_CONNECT_TIMEOUT;
        std::vector<std::string> vSeedNodes;
        std::vector<NetWhitelistPermissions> vWhitelistedRangeIncoming;
        std::vector<NetWhitelistPermissions> vWhitelistedRangeOutgoing;
        std::vector<NetWhitebindPermissions> vWhiteBinds;
        std::vector<CService> vBinds;
        std::vector<CService> onion_binds;
        /// True if the user did not specify -bind= or -whitebind= and thus
        /// we should bind on `0.0.0.0` (IPv4) and `::` (IPv6).
        bool bind_on_any;
        bool m_use_addrman_outgoing = true;
        std::vector<std::string> m_specified_outgoing;
        std::vector<std::string> m_added_nodes;
        bool m_i2p_accept_incoming;
        bool whitelist_forcerelay = DEFAULT_WHITELISTFORCERELAY;
        bool whitelist_relay = DEFAULT_WHITELISTRELAY;
    };

    void Init(const Options& connOptions) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex, !m_total_bytes_sent_mutex)
    {
        AssertLockNotHeld(m_total_bytes_sent_mutex);

        nLocalServices = connOptions.nLocalServices;
        m_max_automatic_connections = connOptions.m_max_automatic_connections;
        m_max_outbound_full_relay = std::min(MAX_OUTBOUND_FULL_RELAY_CONNECTIONS, m_max_automatic_connections);
        m_max_outbound_block_relay = std::min(MAX_BLOCK_RELAY_ONLY_CONNECTIONS, m_max_automatic_connections - m_max_outbound_full_relay);
        m_max_automatic_outbound = m_max_outbound_full_relay + m_max_outbound_block_relay + m_max_feeler;
        m_max_inbound = std::max(0, m_max_automatic_connections - m_max_automatic_outbound);
        m_use_addrman_outgoing = connOptions.m_use_addrman_outgoing;
        m_client_interface = connOptions.uiInterface;
        m_banman = connOptions.m_banman;
        m_msgproc = connOptions.m_msgproc;
        nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
        nReceiveFloodSize = connOptions.nReceiveFloodSize;
        m_peer_connect_timeout = std::chrono::seconds{connOptions.m_peer_connect_timeout};
        {
            LOCK(m_total_bytes_sent_mutex);
            nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
        }
        vWhitelistedRangeIncoming = connOptions.vWhitelistedRangeIncoming;
        vWhitelistedRangeOutgoing = connOptions.vWhitelistedRangeOutgoing;
        {
            LOCK(m_added_nodes_mutex);
            // Attempt v2 connection if we support v2 - we'll reconnect with v1 if our
            // peer doesn't support it or immediately disconnects us for another reason.
            const bool use_v2transport(GetLocalServices() & NODE_P2P_V2);
            for (const std::string& added_node : connOptions.m_added_nodes) {
                m_added_node_params.push_back({added_node, use_v2transport});
            }
        }
        m_onion_binds = connOptions.onion_binds;
        whitelist_forcerelay = connOptions.whitelist_forcerelay;
        whitelist_relay = connOptions.whitelist_relay;
    }

    CConnman(uint64_t seed0, uint64_t seed1, AddrMan& addrman, const NetGroupManager& netgroupman,
             const CChainParams& params, bool network_active = true);

    ~CConnman();

    bool Start(CScheduler& scheduler, const Options& options) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !m_added_nodes_mutex, !m_addr_fetches_mutex, !mutexMsgProc);

    void StopThreads();
    void StopNodes();
    void Stop()
    {
        StopThreads();
        StopNodes();
    };

    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    bool GetNetworkActive() const { return fNetworkActive; };
    bool GetUseAddrmanOutgoing() const { return m_use_addrman_outgoing; };
    void SetNetworkActive(bool active);
    void OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant&& grant_outbound, const char* strDest, ConnectionType conn_type, bool use_v2transport) EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);
    bool CheckIncomingNonce(uint64_t nonce);
    void ASMapHealthCheck();

    // alias for thread safety annotations only, not defined
    RecursiveMutex& GetNodesMutex() const LOCK_RETURNED(m_nodes_mutex);

    bool ForNode(NodeId id, std::function<bool(CNode* pnode)> func);

    void PushMessage(CNode* pnode, CSerializedNetMsg&& msg) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    using NodeFn = std::function<void(CNode*)>;
    void ForEachNode(const NodeFn& func)
    {
        LOCK(m_nodes_mutex);
        for (auto&& node : m_nodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };

    void ForEachNode(const NodeFn& func) const
    {
        LOCK(m_nodes_mutex);
        for (auto&& node : m_nodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };

    // Addrman functions
    /**
     * Return all or many randomly selected addresses, optionally by network.
     *
     * @param[in] max_addresses  Maximum number of addresses to return (0 = all).
     * @param[in] max_pct        Maximum percentage of addresses to return (0 = all).
     * @param[in] network        Select only addresses of this network (nullopt = all).
     * @param[in] filtered       Select only addresses that are considered high quality (false = all).
     */
    std::vector<CAddress> GetAddresses(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered = true) const;
    /**
     * Cache is used to minimize topology leaks, so it should
     * be used for all non-trusted calls, for example, p2p.
     * A non-malicious call (from RPC or a peer with addr permission) should
     * call the function without a parameter to avoid using the cache.
     */
    std::vector<CAddress> GetAddresses(CNode& requestor, size_t max_addresses, size_t max_pct);

    // This allows temporarily exceeding m_max_outbound_full_relay, with the goal of finding
    // a peer that is better than all our current peers.
    void SetTryNewOutboundPeer(bool flag);
    bool GetTryNewOutboundPeer() const;

    void StartExtraBlockRelayPeers();

    // Count the number of full-relay peer we have.
    int GetFullOutboundConnCount() const;
    // Return the number of outbound peers we have in excess of our target (eg,
    // if we previously called SetTryNewOutboundPeer(true), and have since set
    // to false, we may have extra peers that we wish to disconnect). This may
    // return a value less than (num_outbound_connections - num_outbound_slots)
    // in cases where some outbound connections are not yet fully connected, or
    // not yet fully disconnected.
    int GetExtraFullOutboundCount() const;
    // Count the number of block-relay-only peers we have over our limit.
    int GetExtraBlockRelayCount() const;

    bool AddNode(const AddedNodeParams& add) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    bool RemoveAddedNode(const std::string& node) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    bool AddedNodesContain(const CAddress& addr) const EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    std::vector<AddedNodeInfo> GetAddedNodeInfo(bool include_connected) const EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);

    /**
     * Attempts to open a connection. Currently only used from tests.
     *
     * @param[in]   address     Address of node to try connecting to
     * @param[in]   conn_type   ConnectionType::OUTBOUND, ConnectionType::BLOCK_RELAY,
     *                          ConnectionType::ADDR_FETCH or ConnectionType::FEELER
     * @param[in]   use_v2transport  Set to true if node attempts to connect using BIP 324 v2 transport protocol.
     * @return      bool        Returns false if there are no available
     *                          slots for this connection:
     *                          - conn_type not a supported ConnectionType
     *                          - Max total outbound connection capacity filled
     *                          - Max connection capacity for type is filled
     */
    bool AddConnection(const std::string& address, ConnectionType conn_type, bool use_v2transport) EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);

    size_t GetNodeCount(ConnectionDirection) const;
    uint32_t GetMappedAS(const CNetAddr& addr) const;
    void GetNodeStats(std::vector<CNodeStats>& vstats) const;
    bool DisconnectNode(const std::string& node);
    bool DisconnectNode(const CSubNet& subnet);
    bool DisconnectNode(const CNetAddr& addr);
    bool DisconnectNode(NodeId id);

    //! Used to convey which local services we are offering peers during node
    //! connection.
    //!
    //! The data returned by this is used in CNode construction,
    //! which is used to advertise which services we are offering
    //! that peer during `net_processing.cpp:PushNodeVersion()`.
    ServiceFlags GetLocalServices() const;

    uint64_t GetMaxOutboundTarget() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    std::chrono::seconds GetMaxOutboundTimeframe() const;

    //! check if the outbound target is reached
    //! if param historicalBlockServingLimit is set true, the function will
    //! response true if the limit for serving historical blocks has been reached
    bool OutboundTargetReached(bool historicalBlockServingLimit) const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    //! response the bytes left in the current max outbound cycle
    //! in case of no limit, it will always response 0
    uint64_t GetOutboundTargetBytesLeft() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    std::chrono::seconds GetMaxOutboundTimeLeftInCycle() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    uint64_t GetTotalBytesRecv() const;
    uint64_t GetTotalBytesSent() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    /** Get a unique deterministic randomizer. */
    CSipHasher GetDeterministicRandomizer(uint64_t id) const;

    void WakeMessageHandler() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);

    /** Return true if we should disconnect the peer for failing an inactivity check. */
    bool ShouldRunInactivityChecks(const CNode& node, std::chrono::seconds now) const;

    bool MultipleManualOrFullOutboundConns(Network net) const EXCLUSIVE_LOCKS_REQUIRED(m_nodes_mutex);

private:
    struct ListenSocket {
    public:
        std::shared_ptr<Sock> sock;
        inline void AddSocketPermissionFlags(NetPermissionFlags& flags) const { NetPermissions::AddFlag(flags, m_permissions); }
        ListenSocket(std::shared_ptr<Sock> sock_, NetPermissionFlags permissions_)
            : sock{sock_}, m_permissions{permissions_}
        {
        }

    private:
        NetPermissionFlags m_permissions;
    };

    //! returns the time left in the current max outbound cycle
    //! in case of no limit, it will always return 0
    std::chrono::seconds GetMaxOutboundTimeLeftInCycle_() const EXCLUSIVE_LOCKS_REQUIRED(m_total_bytes_sent_mutex);

    bool BindListenPort(const CService& bindAddr, bilingual_str& strError, NetPermissionFlags permissions);
    bool Bind(const CService& addr, unsigned int flags, NetPermissionFlags permissions);
    bool InitBinds(const Options& options);

    void ThreadOpenAddedConnections() EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex, !m_unused_i2p_sessions_mutex, !m_reconnections_mutex);
    void AddAddrFetch(const std::string& strDest) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex);
    void ProcessAddrFetch() EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_unused_i2p_sessions_mutex);
    void ThreadOpenConnections(std::vector<std::string> connect, Span<const std::string> seed_nodes) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_added_nodes_mutex, !m_nodes_mutex, !m_unused_i2p_sessions_mutex, !m_reconnections_mutex);
    void ThreadMessageHandler() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    void ThreadI2PAcceptIncoming();
    void AcceptConnection(const ListenSocket& hListenSocket);

    /**
     * Create a `CNode` object from a socket that has just been accepted and add the node to
     * the `m_nodes` member.
     * @param[in] sock Connected socket to communicate with the peer.
     * @param[in] permission_flags The peer's permissions.
     * @param[in] addr_bind The address and port at our side of the connection.
     * @param[in] addr The address and port at the peer's side of the connection.
     */
    void CreateNodeFromAcceptedSocket(std::unique_ptr<Sock>&& sock,
                                      NetPermissionFlags permission_flags,
                                      const CAddress& addr_bind,
                                      const CAddress& addr);

    void DisconnectNodes() EXCLUSIVE_LOCKS_REQUIRED(!m_reconnections_mutex, !m_nodes_mutex);
    void NotifyNumConnectionsChanged();
    /** Return true if the peer is inactive and should be disconnected. */
    bool InactivityCheck(const CNode& node) const;

    /**
     * Generate a collection of sockets to check for IO readiness.
     * @param[in] nodes Select from these nodes' sockets.
     * @return sockets to check for readiness
     */
    Sock::EventsPerSock GenerateWaitSockets(Span<CNode* const> nodes);

    /**
     * Check connected and listening sockets for IO readiness and process them accordingly.
     */
    void SocketHandler() EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);

    /**
     * Do the read/write for connected sockets that are ready for IO.
     * @param[in] nodes Nodes to process. The socket of each node is checked against `what`.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerConnected(const std::vector<CNode*>& nodes,
                                const Sock::EventsPerSock& events_per_sock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);

    /**
     * Accept incoming connections, one from each read-ready listening socket.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerListening(const Sock::EventsPerSock& events_per_sock);

    void ThreadSocketHandler() EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc, !m_nodes_mutex, !m_reconnections_mutex);
    void ThreadDNSAddressSeed() EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_nodes_mutex);

    uint64_t CalculateKeyedNetGroup(const CAddress& ad) const;

    CNode* FindNode(const CNetAddr& ip);
    CNode* FindNode(const std::string& addrName);
    CNode* FindNode(const CService& addr);

    /**
     * Determine whether we're already connected to a given address, in order to
     * avoid initiating duplicate connections.
     */
    bool AlreadyConnectedToAddress(const CAddress& addr);

    bool AttemptToEvictConnection();
    CNode* ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure, ConnectionType conn_type, bool use_v2transport) EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);
    void AddWhitelistPermissionFlags(NetPermissionFlags& flags, const CNetAddr &addr, const std::vector<NetWhitelistPermissions>& ranges) const;

    void DeleteNode(CNode* pnode);

    NodeId GetNewNodeId();

    /** (Try to) send data from node's vSendMsg. Returns (bytes_sent, data_left). */
    std::pair<size_t, bool> SocketSendData(CNode& node) const EXCLUSIVE_LOCKS_REQUIRED(node.cs_vSend);

    void DumpAddresses();

    // Network stats
    void RecordBytesRecv(uint64_t bytes);
    void RecordBytesSent(uint64_t bytes) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);

    /**
     Return reachable networks for which we have no addresses in addrman and therefore
     may require loading fixed seeds.
     */
    std::unordered_set<Network> GetReachableEmptyNetworks() const;

    /**
     * Return vector of current BLOCK_RELAY peers.
     */
    std::vector<CAddress> GetCurrentBlockRelayOnlyConns() const;

    /**
     * Search for a "preferred" network, a reachable network to which we
     * currently don't have any OUTBOUND_FULL_RELAY or MANUAL connections.
     * There needs to be at least one address in AddrMan for a preferred
     * network to be picked.
     *
     * @param[out]    network        Preferred network, if found.
     *
     * @return           bool        Whether a preferred network was found.
     */
    bool MaybePickPreferredNetwork(std::optional<Network>& network);

    // Whether the node should be passed out in ForEach* callbacks
    static bool NodeFullyConnected(const CNode* pnode);

    uint16_t GetDefaultPort(Network net) const;
    uint16_t GetDefaultPort(const std::string& addr) const;

    // Network usage totals
    mutable Mutex m_total_bytes_sent_mutex;
    std::atomic<uint64_t> nTotalBytesRecv{0};
    uint64_t nTotalBytesSent GUARDED_BY(m_total_bytes_sent_mutex) {0};

    // outbound limit & stats
    uint64_t nMaxOutboundTotalBytesSentInCycle GUARDED_BY(m_total_bytes_sent_mutex) {0};
    std::chrono::seconds nMaxOutboundCycleStartTime GUARDED_BY(m_total_bytes_sent_mutex) {0};
    uint64_t nMaxOutboundLimit GUARDED_BY(m_total_bytes_sent_mutex);

    // P2P timeout in seconds
    std::chrono::seconds m_peer_connect_timeout;

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<NetWhitelistPermissions> vWhitelistedRangeIncoming;
    // Whitelisted ranges for outgoing connections.
    std::vector<NetWhitelistPermissions> vWhitelistedRangeOutgoing;

    unsigned int nSendBufferMaxSize{0};
    unsigned int nReceiveFloodSize{0};

    std::vector<ListenSocket> vhListenSocket;
    std::atomic<bool> fNetworkActive{true};
    bool fAddressesInitialized{false};
    AddrMan& addrman;
    const NetGroupManager& m_netgroupman;
    std::deque<std::string> m_addr_fetches GUARDED_BY(m_addr_fetches_mutex);
    Mutex m_addr_fetches_mutex;

    // connection string and whether to use v2 p2p
    std::vector<AddedNodeParams> m_added_node_params GUARDED_BY(m_added_nodes_mutex);

    mutable Mutex m_added_nodes_mutex;
    std::vector<CNode*> m_nodes GUARDED_BY(m_nodes_mutex);
    std::list<CNode*> m_nodes_disconnected;
    mutable RecursiveMutex m_nodes_mutex;
    std::atomic<NodeId> nLastNodeId{0};
    unsigned int nPrevNodeCount{0};

    // Stores number of full-tx connections (outbound and manual) per network
    std::array<unsigned int, Network::NET_MAX> m_network_conn_counts GUARDED_BY(m_nodes_mutex) = {};

    /**
     * Cache responses to addr requests to minimize privacy leak.
     * Attack example: scraping addrs in real-time may allow an attacker
     * to infer new connections of the victim by detecting new records
     * with fresh timestamps (per self-announcement).
     */
    struct CachedAddrResponse {
        std::vector<CAddress> m_addrs_response_cache;
        std::chrono::microseconds m_cache_entry_expiration{0};
    };

    /**
     * Addr responses stored in different caches
     * per (network, local socket) prevent cross-network node identification.
     * If a node for example is multi-homed under Tor and IPv6,
     * a single cache (or no cache at all) would let an attacker
     * to easily detect that it is the same node by comparing responses.
     * Indexing by local socket prevents leakage when a node has multiple
     * listening addresses on the same network.
     *
     * The used memory equals to 1000 CAddress records (or around 40 bytes) per
     * distinct Network (up to 5) we have/had an inbound peer from,
     * resulting in at most ~196 KB. Every separate local socket may
     * add up to ~196 KB extra.
     */
    std::map<uint64_t, CachedAddrResponse> m_addr_response_caches;

    /**
     * Services this node offers.
     *
     * This data is replicated in each Peer instance we create.
     *
     * This data is not marked const, but after being set it should not
     * change.
     *
     * \sa Peer::our_services
     */
    ServiceFlags nLocalServices;

    std::unique_ptr<CSemaphore> semOutbound;
    std::unique_ptr<CSemaphore> semAddnode;

    /**
     * Maximum number of automatic connections permitted, excluding manual
     * connections but including inbounds. May be changed by the user and is
     * potentially limited by the operating system (number of file descriptors).
     */
    int m_max_automatic_connections;

    /*
     * Maximum number of peers by connection type. Might vary from defaults
     * based on -maxconnections init value.
     */

    // How many full-relay (tx, block, addr) outbound peers we want
    int m_max_outbound_full_relay;

    // How many block-relay only outbound peers we want
    // We do not relay tx or addr messages with these peers
    int m_max_outbound_block_relay;

    int m_max_addnode{MAX_ADDNODE_CONNECTIONS};
    int m_max_feeler{MAX_FEELER_CONNECTIONS};
    int m_max_automatic_outbound;
    int m_max_inbound;

    bool m_use_addrman_outgoing;
    CClientUIInterface* m_client_interface;
    NetEventsInterface* m_msgproc;
    /** Pointer to this node's banman. May be nullptr - check existence before dereferencing. */
    BanMan* m_banman;

    /**
     * Addresses that were saved during the previous clean shutdown. We'll
     * attempt to make block-relay-only connections to them.
     */
    std::vector<CAddress> m_anchors;

    /** SipHasher seeds for deterministic randomness */
    const uint64_t nSeed0, nSeed1;

    /** flag for waking the message processor. */
    bool fMsgProcWake GUARDED_BY(mutexMsgProc);

    std::condition_variable condMsgProc;
    Mutex mutexMsgProc;
    std::atomic<bool> flagInterruptMsgProc{false};

    /**
     * This is signaled when network activity should cease.
     * A pointer to it is saved in `m_i2p_sam_session`, so make sure that
     * the lifetime of `interruptNet` is not shorter than
     * the lifetime of `m_i2p_sam_session`.
     */
    CThreadInterrupt interruptNet;

    /**
     * I2P SAM session.
     * Used to accept incoming and make outgoing I2P connections from a persistent
     * address.
     */
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session;

    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    std::thread threadMessageHandler;
    std::thread threadI2PAcceptIncoming;

    /** flag for deciding to connect to an extra outbound peer,
     *  in excess of m_max_outbound_full_relay
     *  This takes the place of a feeler connection */
    std::atomic_bool m_try_another_outbound_peer;

    /** flag for initiating extra block-relay-only peer connections.
     *  this should only be enabled after initial chain sync has occurred,
     *  as these connections are intended to be short-lived and low-bandwidth.
     */
    std::atomic_bool m_start_extra_block_relay_peers{false};

    /**
     * A vector of -bind=<address>:<port>=onion arguments each of which is
     * an address and port that are designated for incoming Tor connections.
     */
    std::vector<CService> m_onion_binds;

    /**
     * flag for adding 'forcerelay' permission to whitelisted inbound
     * and manual peers with default permissions.
     */
    bool whitelist_forcerelay;

    /**
     * flag for adding 'relay' permission to whitelisted inbound
     * and manual peers with default permissions.
     */
    bool whitelist_relay;

    /**
     * Mutex protecting m_i2p_sam_sessions.
     */
    Mutex m_unused_i2p_sessions_mutex;

    /**
     * A pool of created I2P SAM transient sessions that should be used instead
     * of creating new ones in order to reduce the load on the I2P network.
     * Creating a session in I2P is not cheap, thus if this is not empty, then
     * pick an entry from it instead of creating a new session. If connecting to
     * a host fails, then the created session is put to this pool for reuse.
     */
    std::queue<std::unique_ptr<i2p::sam::Session>> m_unused_i2p_sessions GUARDED_BY(m_unused_i2p_sessions_mutex);

    /**
     * Mutex protecting m_reconnections.
     */
    Mutex m_reconnections_mutex;

    /** Struct for entries in m_reconnections. */
    struct ReconnectionInfo
    {
        CAddress addr_connect;
        CSemaphoreGrant grant;
        std::string destination;
        ConnectionType conn_type;
        bool use_v2transport;
    };

    /**
     * List of reconnections we have to make.
     */
    std::list<ReconnectionInfo> m_reconnections GUARDED_BY(m_reconnections_mutex);

    /** Attempt reconnections, if m_reconnections non-empty. */
    void PerformReconnections() EXCLUSIVE_LOCKS_REQUIRED(!m_reconnections_mutex, !m_unused_i2p_sessions_mutex);

    /**
     * Cap on the size of `m_unused_i2p_sessions`, to ensure it does not
     * unexpectedly use too much memory.
     */
    static constexpr size_t MAX_UNUSED_I2P_SESSIONS_SIZE{10};

    /**
     * RAII helper to atomically create a copy of `m_nodes` and add a reference
     * to each of the nodes. The nodes are released when this object is destroyed.
     */
    class NodesSnapshot
    {
    public:
        explicit NodesSnapshot(const CConnman& connman, bool shuffle)
        {
            {
                LOCK(connman.m_nodes_mutex);
                m_nodes_copy = connman.m_nodes;
                for (auto& node : m_nodes_copy) {
                    node->AddRef();
                }
            }
            if (shuffle) {
                Shuffle(m_nodes_copy.begin(), m_nodes_copy.end(), FastRandomContext{});
            }
        }

        ~NodesSnapshot()
        {
            for (auto& node : m_nodes_copy) {
                node->Release();
            }
        }

        const std::vector<CNode*>& Nodes() const
        {
            return m_nodes_copy;
        }

    private:
        std::vector<CNode*> m_nodes_copy;
    };

    const CChainParams& m_params;

    friend struct ConnmanTestMsg;
};

/** Defaults to `CaptureMessageToFile()`, but can be overridden by unit tests. */
extern std::function<void(const CAddress& addr,
                          const std::string& msg_type,
                          Span<const unsigned char> data,
                          bool is_incoming)>
    CaptureMessage;

#endif // BITCOIN_NET_H
