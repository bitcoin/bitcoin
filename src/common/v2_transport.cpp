// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/v2_transport.h>

#include <logging.h>
#include <memusage.h>
#include <util/check.h>

namespace {

/** List of short messages as defined in BIP324, in order.
 *
 * Only message types that are actually implemented in this codebase need to be listed, as other
 * messages get ignored anyway - whether we know how to decode them or not.
 */
const std::array<std::string, 33> V2_MESSAGE_IDS = {
    "", // 12 bytes follow encoding the message type like in V1
    NetMsgType::ADDR,
    NetMsgType::BLOCK,
    NetMsgType::BLOCKTXN,
    NetMsgType::CMPCTBLOCK,
    NetMsgType::FEEFILTER,
    NetMsgType::FILTERADD,
    NetMsgType::FILTERCLEAR,
    NetMsgType::FILTERLOAD,
    NetMsgType::GETBLOCKS,
    NetMsgType::GETBLOCKTXN,
    NetMsgType::GETDATA,
    NetMsgType::GETHEADERS,
    NetMsgType::HEADERS,
    NetMsgType::INV,
    NetMsgType::MEMPOOL,
    NetMsgType::MERKLEBLOCK,
    NetMsgType::NOTFOUND,
    NetMsgType::PING,
    NetMsgType::PONG,
    NetMsgType::SENDCMPCT,
    NetMsgType::TX,
    NetMsgType::GETCFILTERS,
    NetMsgType::CFILTER,
    NetMsgType::GETCFHEADERS,
    NetMsgType::CFHEADERS,
    NetMsgType::GETCFCHECKPT,
    NetMsgType::CFCHECKPT,
    NetMsgType::ADDRV2,
    // Unimplemented message types that are assigned in BIP324:
    "",
    "",
    "",
    ""
};

class V2MessageMap
{
    std::unordered_map<std::string, uint8_t> m_map;

public:
    V2MessageMap() noexcept
    {
        for (size_t i = 1; i < std::size(V2_MESSAGE_IDS); ++i) {
            m_map.emplace(V2_MESSAGE_IDS[i], i);
        }
    }

    std::optional<uint8_t> operator()(const std::string& message_name) const noexcept
    {
        auto it = m_map.find(message_name);
        if (it == m_map.end()) return std::nullopt;
        return it->second;
    }
};

const V2MessageMap V2_MESSAGE_MAP;

std::vector<uint8_t> GenerateRandomGarbage() noexcept
{
    std::vector<uint8_t> ret;
    FastRandomContext rng;
    ret.resize(rng.randrange(V2Transport::MAX_GARBAGE_LEN + 1));
    rng.fillrand(MakeWritableByteSpan(ret));
    return ret;
}

} // namespace

void V2Transport::StartSendingHandshake() noexcept
{
    AssertLockHeld(m_send_mutex);
    Assume(m_send_state == SendState::AWAITING_KEY);
    Assume(m_send_buffer.empty());
    // Initialize the send buffer with ellswift pubkey + provided garbage.
    m_send_buffer.resize(EllSwiftPubKey::size() + m_send_garbage.size());
    std::copy(std::begin(m_cipher.GetOurPubKey()), std::end(m_cipher.GetOurPubKey()), MakeWritableByteSpan(m_send_buffer).begin());
    std::copy(m_send_garbage.begin(), m_send_garbage.end(), m_send_buffer.begin() + EllSwiftPubKey::size());
    // We cannot wipe m_send_garbage as it will still be used as AAD later in the handshake.
}

V2Transport::V2Transport(NodeId nodeid, bool initiating, const CKey& key, Span<const std::byte> ent32, std::vector<uint8_t> garbage) noexcept
    : m_cipher{key, ent32}, m_initiating{initiating}, m_nodeid{nodeid},
      m_v1_fallback{nodeid},
      m_recv_state{initiating ? RecvState::KEY : RecvState::KEY_MAYBE_V1},
      m_send_garbage{std::move(garbage)},
      m_send_state{initiating ? SendState::AWAITING_KEY : SendState::MAYBE_V1}
{
    Assume(m_send_garbage.size() <= MAX_GARBAGE_LEN);
    // Start sending immediately if we're the initiator of the connection.
    if (initiating) {
        LOCK(m_send_mutex);
        StartSendingHandshake();
    }
}

V2Transport::V2Transport(NodeId nodeid, bool initiating) noexcept
    : V2Transport{nodeid, initiating, GenerateRandomKey(),
                  MakeByteSpan(GetRandHash()), GenerateRandomGarbage()} {}

void V2Transport::SetReceiveState(RecvState recv_state) noexcept
{
    AssertLockHeld(m_recv_mutex);
    // Enforce allowed state transitions.
    switch (m_recv_state) {
    case RecvState::KEY_MAYBE_V1:
        Assume(recv_state == RecvState::KEY || recv_state == RecvState::V1);
        break;
    case RecvState::KEY:
        Assume(recv_state == RecvState::GARB_GARBTERM);
        break;
    case RecvState::GARB_GARBTERM:
        Assume(recv_state == RecvState::VERSION);
        break;
    case RecvState::VERSION:
        Assume(recv_state == RecvState::APP);
        break;
    case RecvState::APP:
        Assume(recv_state == RecvState::APP_READY);
        break;
    case RecvState::APP_READY:
        Assume(recv_state == RecvState::APP);
        break;
    case RecvState::V1:
        Assume(false); // V1 state cannot be left
        break;
    }
    // Change state.
    m_recv_state = recv_state;
}

void V2Transport::SetSendState(SendState send_state) noexcept
{
    AssertLockHeld(m_send_mutex);
    // Enforce allowed state transitions.
    switch (m_send_state) {
    case SendState::MAYBE_V1:
        Assume(send_state == SendState::V1 || send_state == SendState::AWAITING_KEY);
        break;
    case SendState::AWAITING_KEY:
        Assume(send_state == SendState::READY);
        break;
    case SendState::READY:
    case SendState::V1:
        Assume(false); // Final states
        break;
    }
    // Change state.
    m_send_state = send_state;
}

bool V2Transport::ReceivedMessageComplete() const noexcept
{
    AssertLockNotHeld(m_recv_mutex);
    LOCK(m_recv_mutex);
    if (m_recv_state == RecvState::V1) return m_v1_fallback.ReceivedMessageComplete();

    return m_recv_state == RecvState::APP_READY;
}

void V2Transport::ProcessReceivedMaybeV1Bytes() noexcept
{
    AssertLockHeld(m_recv_mutex);
    AssertLockNotHeld(m_send_mutex);
    Assume(m_recv_state == RecvState::KEY_MAYBE_V1);
    // We still have to determine if this is a v1 or v2 connection. The bytes being received could
    // be the beginning of either a v1 packet (network magic + "version\x00\x00\x00\x00\x00"), or
    // of a v2 public key. BIP324 specifies that a mismatch with this 16-byte string should trigger
    // sending of the key.
    std::array<uint8_t, V1_PREFIX_LEN> v1_prefix = {0, 0, 0, 0, 'v', 'e', 'r', 's', 'i', 'o', 'n', 0, 0, 0, 0, 0};
    std::copy(std::begin(Params().MessageStart()), std::end(Params().MessageStart()), v1_prefix.begin());
    Assume(m_recv_buffer.size() <= v1_prefix.size());
    if (!std::equal(m_recv_buffer.begin(), m_recv_buffer.end(), v1_prefix.begin())) {
        // Mismatch with v1 prefix, so we can assume a v2 connection.
        SetReceiveState(RecvState::KEY); // Convert to KEY state, leaving received bytes around.
        // Transition the sender to AWAITING_KEY state and start sending.
        LOCK(m_send_mutex);
        SetSendState(SendState::AWAITING_KEY);
        StartSendingHandshake();
    } else if (m_recv_buffer.size() == v1_prefix.size()) {
        // Full match with the v1 prefix, so fall back to v1 behavior.
        LOCK(m_send_mutex);
        Span<const uint8_t> feedback{m_recv_buffer};
        // Feed already received bytes to v1 transport. It should always accept these, because it's
        // less than the size of a v1 header, and these are the first bytes fed to m_v1_fallback.
        bool ret = m_v1_fallback.ReceivedBytes(feedback);
        Assume(feedback.empty());
        Assume(ret);
        SetReceiveState(RecvState::V1);
        SetSendState(SendState::V1);
        // Reset v2 transport buffers to save memory.
        ClearShrink(m_recv_buffer);
        ClearShrink(m_send_buffer);
    } else {
        // We have not received enough to distinguish v1 from v2 yet. Wait until more bytes come.
    }
}

bool V2Transport::ProcessReceivedKeyBytes() noexcept
{
    AssertLockHeld(m_recv_mutex);
    AssertLockNotHeld(m_send_mutex);
    Assume(m_recv_state == RecvState::KEY);
    Assume(m_recv_buffer.size() <= EllSwiftPubKey::size());

    // As a special exception, if bytes 4-16 of the key on a responder connection match the
    // corresponding bytes of a V1 version message, but bytes 0-4 don't match the network magic
    // (if they did, we'd have switched to V1 state already), assume this is a peer from
    // another network, and disconnect them. They will almost certainly disconnect us too when
    // they receive our uniformly random key and garbage, but detecting this case specially
    // means we can log it.
    static constexpr std::array<uint8_t, 12> MATCH = {'v', 'e', 'r', 's', 'i', 'o', 'n', 0, 0, 0, 0, 0};
    static constexpr size_t OFFSET = std::tuple_size_v<MessageStartChars>;
    if (!m_initiating && m_recv_buffer.size() >= OFFSET + MATCH.size()) {
        if (std::equal(MATCH.begin(), MATCH.end(), m_recv_buffer.begin() + OFFSET)) {
            LogPrint(BCLog::NET, "V2 transport error: V1 peer with wrong MessageStart %s\n",
                     HexStr(Span(m_recv_buffer).first(OFFSET)));
            return false;
        }
    }

    if (m_recv_buffer.size() == EllSwiftPubKey::size()) {
        // Other side's key has been fully received, and can now be Diffie-Hellman combined with
        // our key to initialize the encryption ciphers.

        // Initialize the ciphers.
        EllSwiftPubKey ellswift(MakeByteSpan(m_recv_buffer));
        LOCK(m_send_mutex);
        m_cipher.Initialize(ellswift, m_initiating);

        // Switch receiver state to GARB_GARBTERM.
        SetReceiveState(RecvState::GARB_GARBTERM);
        m_recv_buffer.clear();

        // Switch sender state to READY.
        SetSendState(SendState::READY);

        // Append the garbage terminator to the send buffer.
        m_send_buffer.resize(m_send_buffer.size() + BIP324Cipher::GARBAGE_TERMINATOR_LEN);
        std::copy(m_cipher.GetSendGarbageTerminator().begin(),
                  m_cipher.GetSendGarbageTerminator().end(),
                  MakeWritableByteSpan(m_send_buffer).last(BIP324Cipher::GARBAGE_TERMINATOR_LEN).begin());

        // Construct version packet in the send buffer, with the sent garbage data as AAD.
        m_send_buffer.resize(m_send_buffer.size() + BIP324Cipher::EXPANSION + VERSION_CONTENTS.size());
        m_cipher.Encrypt(
            /*contents=*/VERSION_CONTENTS,
            /*aad=*/MakeByteSpan(m_send_garbage),
            /*ignore=*/false,
            /*output=*/MakeWritableByteSpan(m_send_buffer).last(BIP324Cipher::EXPANSION + VERSION_CONTENTS.size()));
        // We no longer need the garbage.
        ClearShrink(m_send_garbage);
    } else {
        // We still have to receive more key bytes.
    }
    return true;
}

bool V2Transport::ProcessReceivedGarbageBytes() noexcept
{
    AssertLockHeld(m_recv_mutex);
    Assume(m_recv_state == RecvState::GARB_GARBTERM);
    Assume(m_recv_buffer.size() <= MAX_GARBAGE_LEN + BIP324Cipher::GARBAGE_TERMINATOR_LEN);
    if (m_recv_buffer.size() >= BIP324Cipher::GARBAGE_TERMINATOR_LEN) {
        if (MakeByteSpan(m_recv_buffer).last(BIP324Cipher::GARBAGE_TERMINATOR_LEN) == m_cipher.GetReceiveGarbageTerminator()) {
            // Garbage terminator received. Store garbage to authenticate it as AAD later.
            m_recv_aad = std::move(m_recv_buffer);
            m_recv_aad.resize(m_recv_aad.size() - BIP324Cipher::GARBAGE_TERMINATOR_LEN);
            m_recv_buffer.clear();
            SetReceiveState(RecvState::VERSION);
        } else if (m_recv_buffer.size() == MAX_GARBAGE_LEN + BIP324Cipher::GARBAGE_TERMINATOR_LEN) {
            // We've reached the maximum length for garbage + garbage terminator, and the
            // terminator still does not match. Abort.
            LogPrint(BCLog::NET, "V2 transport error: missing garbage terminator, peer=%d\n", m_nodeid);
            return false;
        } else {
            // We still need to receive more garbage and/or garbage terminator bytes.
        }
    } else {
        // We have less than GARBAGE_TERMINATOR_LEN (16) bytes, so we certainly need to receive
        // more first.
    }
    return true;
}

bool V2Transport::ProcessReceivedPacketBytes() noexcept
{
    AssertLockHeld(m_recv_mutex);
    Assume(m_recv_state == RecvState::VERSION || m_recv_state == RecvState::APP);

    // The maximum permitted contents length for a packet, consisting of:
    // - 0x00 byte: indicating long message type encoding
    // - 12 bytes of message type
    // - payload
    static constexpr size_t MAX_CONTENTS_LEN =
        1 + CMessageHeader::COMMAND_SIZE +
        std::min<size_t>(MAX_SIZE, MAX_PROTOCOL_MESSAGE_LENGTH);

    if (m_recv_buffer.size() == BIP324Cipher::LENGTH_LEN) {
        // Length descriptor received.
        m_recv_len = m_cipher.DecryptLength(MakeByteSpan(m_recv_buffer));
        if (m_recv_len > MAX_CONTENTS_LEN) {
            LogPrint(BCLog::NET, "V2 transport error: packet too large (%u bytes), peer=%d\n", m_recv_len, m_nodeid);
            return false;
        }
    } else if (m_recv_buffer.size() > BIP324Cipher::LENGTH_LEN && m_recv_buffer.size() == m_recv_len + BIP324Cipher::EXPANSION) {
        // Ciphertext received, decrypt it into m_recv_decode_buffer.
        // Note that it is impossible to reach this branch without hitting the branch above first,
        // as GetMaxBytesToProcess only allows up to LENGTH_LEN into the buffer before that point.
        m_recv_decode_buffer.resize(m_recv_len);
        bool ignore{false};
        bool ret = m_cipher.Decrypt(
            /*input=*/MakeByteSpan(m_recv_buffer).subspan(BIP324Cipher::LENGTH_LEN),
            /*aad=*/MakeByteSpan(m_recv_aad),
            /*ignore=*/ignore,
            /*contents=*/MakeWritableByteSpan(m_recv_decode_buffer));
        if (!ret) {
            LogPrint(BCLog::NET, "V2 transport error: packet decryption failure (%u bytes), peer=%d\n", m_recv_len, m_nodeid);
            return false;
        }
        // We have decrypted a valid packet with the AAD we expected, so clear the expected AAD.
        ClearShrink(m_recv_aad);
        // Feed the last 4 bytes of the Poly1305 authentication tag (and its timing) into our RNG.
        RandAddEvent(ReadLE32(m_recv_buffer.data() + m_recv_buffer.size() - 4));

        // At this point we have a valid packet decrypted into m_recv_decode_buffer. If it's not a
        // decoy, which we simply ignore, use the current state to decide what to do with it.
        if (!ignore) {
            switch (m_recv_state) {
            case RecvState::VERSION:
                // Version message received; transition to application phase. The contents is
                // ignored, but can be used for future extensions.
                SetReceiveState(RecvState::APP);
                break;
            case RecvState::APP:
                // Application message decrypted correctly. It can be extracted using GetMessage().
                SetReceiveState(RecvState::APP_READY);
                break;
            default:
                // Any other state is invalid (this function should not have been called).
                Assume(false);
            }
        }
        // Wipe the receive buffer where the next packet will be received into.
        ClearShrink(m_recv_buffer);
        // In all but APP_READY state, we can wipe the decoded contents.
        if (m_recv_state != RecvState::APP_READY) ClearShrink(m_recv_decode_buffer);
    } else {
        // We either have less than 3 bytes, so we don't know the packet's length yet, or more
        // than 3 bytes but less than the packet's full ciphertext. Wait until those arrive.
    }
    return true;
}

size_t V2Transport::GetMaxBytesToProcess() noexcept
{
    AssertLockHeld(m_recv_mutex);
    switch (m_recv_state) {
    case RecvState::KEY_MAYBE_V1:
        // During the KEY_MAYBE_V1 state we do not allow more than the length of v1 prefix into the
        // receive buffer.
        Assume(m_recv_buffer.size() <= V1_PREFIX_LEN);
        // As long as we're not sure if this is a v1 or v2 connection, don't receive more than what
        // is strictly necessary to distinguish the two (16 bytes). If we permitted more than
        // the v1 header size (24 bytes), we may not be able to feed the already-received bytes
        // back into the m_v1_fallback V1 transport.
        return V1_PREFIX_LEN - m_recv_buffer.size();
    case RecvState::KEY:
        // During the KEY state, we only allow the 64-byte key into the receive buffer.
        Assume(m_recv_buffer.size() <= EllSwiftPubKey::size());
        // As long as we have not received the other side's public key, don't receive more than
        // that (64 bytes), as garbage follows, and locating the garbage terminator requires the
        // key exchange first.
        return EllSwiftPubKey::size() - m_recv_buffer.size();
    case RecvState::GARB_GARBTERM:
        // Process garbage bytes one by one (because terminator may appear anywhere).
        return 1;
    case RecvState::VERSION:
    case RecvState::APP:
        // These three states all involve decoding a packet. Process the length descriptor first,
        // so that we know where the current packet ends (and we don't process bytes from the next
        // packet or decoy yet). Then, process the ciphertext bytes of the current packet.
        if (m_recv_buffer.size() < BIP324Cipher::LENGTH_LEN) {
            return BIP324Cipher::LENGTH_LEN - m_recv_buffer.size();
        } else {
            // Note that BIP324Cipher::EXPANSION is the total difference between contents size
            // and encoded packet size, which includes the 3 bytes due to the packet length.
            // When transitioning from receiving the packet length to receiving its ciphertext,
            // the encrypted packet length is left in the receive buffer.
            return BIP324Cipher::EXPANSION + m_recv_len - m_recv_buffer.size();
        }
    case RecvState::APP_READY:
        // No bytes can be processed until GetMessage() is called.
        return 0;
    case RecvState::V1:
        // Not allowed (must be dealt with by the caller).
        Assume(false);
        return 0;
    }
    Assume(false); // unreachable
    return 0;
}

bool V2Transport::ReceivedBytes(Span<const uint8_t>& msg_bytes) noexcept
{
    AssertLockNotHeld(m_recv_mutex);
    /** How many bytes to allocate in the receive buffer at most above what is received so far. */
    static constexpr size_t MAX_RESERVE_AHEAD = 256 * 1024;

    LOCK(m_recv_mutex);
    if (m_recv_state == RecvState::V1) return m_v1_fallback.ReceivedBytes(msg_bytes);

    // Process the provided bytes in msg_bytes in a loop. In each iteration a nonzero number of
    // bytes (decided by GetMaxBytesToProcess) are taken from the beginning om msg_bytes, and
    // appended to m_recv_buffer. Then, depending on the receiver state, one of the
    // ProcessReceived*Bytes functions is called to process the bytes in that buffer.
    while (!msg_bytes.empty()) {
        // Decide how many bytes to copy from msg_bytes to m_recv_buffer.
        size_t max_read = GetMaxBytesToProcess();

        // Reserve space in the buffer if there is not enough.
        if (m_recv_buffer.size() + std::min(msg_bytes.size(), max_read) > m_recv_buffer.capacity()) {
            switch (m_recv_state) {
            case RecvState::KEY_MAYBE_V1:
            case RecvState::KEY:
            case RecvState::GARB_GARBTERM:
                // During the initial states (key/garbage), allocate once to fit the maximum (4111
                // bytes).
                m_recv_buffer.reserve(MAX_GARBAGE_LEN + BIP324Cipher::GARBAGE_TERMINATOR_LEN);
                break;
            case RecvState::VERSION:
            case RecvState::APP: {
                // During states where a packet is being received, as much as is expected but never
                // more than MAX_RESERVE_AHEAD bytes in addition to what is received so far.
                // This means attackers that want to cause us to waste allocated memory are limited
                // to MAX_RESERVE_AHEAD above the largest allowed message contents size, and to
                // MAX_RESERVE_AHEAD more than they've actually sent us.
                size_t alloc_add = std::min(max_read, msg_bytes.size() + MAX_RESERVE_AHEAD);
                m_recv_buffer.reserve(m_recv_buffer.size() + alloc_add);
                break;
            }
            case RecvState::APP_READY:
                // The buffer is empty in this state.
                Assume(m_recv_buffer.empty());
                break;
            case RecvState::V1:
                // Should have bailed out above.
                Assume(false);
                break;
            }
        }

        // Can't read more than provided input.
        max_read = std::min(msg_bytes.size(), max_read);
        // Copy data to buffer.
        m_recv_buffer.insert(m_recv_buffer.end(), UCharCast(msg_bytes.data()), UCharCast(msg_bytes.data() + max_read));
        msg_bytes = msg_bytes.subspan(max_read);

        // Process data in the buffer.
        switch (m_recv_state) {
        case RecvState::KEY_MAYBE_V1:
            ProcessReceivedMaybeV1Bytes();
            if (m_recv_state == RecvState::V1) return true;
            break;

        case RecvState::KEY:
            if (!ProcessReceivedKeyBytes()) return false;
            break;

        case RecvState::GARB_GARBTERM:
            if (!ProcessReceivedGarbageBytes()) return false;
            break;

        case RecvState::VERSION:
        case RecvState::APP:
            if (!ProcessReceivedPacketBytes()) return false;
            break;

        case RecvState::APP_READY:
            return true;

        case RecvState::V1:
            // We should have bailed out before.
            Assume(false);
            break;
        }
        // Make sure we have made progress before continuing.
        Assume(max_read > 0);
    }

    return true;
}

std::optional<std::string> V2Transport::GetMessageType(Span<const uint8_t>& contents) noexcept
{
    if (contents.size() == 0) return std::nullopt; // Empty contents
    uint8_t first_byte = contents[0];
    contents = contents.subspan(1); // Strip first byte.

    if (first_byte != 0) {
        // Short (1 byte) encoding.
        if (first_byte < std::size(V2_MESSAGE_IDS)) {
            // Valid short message id.
            return V2_MESSAGE_IDS[first_byte];
        } else {
            // Unknown short message id.
            return std::nullopt;
        }
    }

    if (contents.size() < CMessageHeader::COMMAND_SIZE) {
        return std::nullopt; // Long encoding needs 12 message type bytes.
    }

    size_t msg_type_len{0};
    while (msg_type_len < CMessageHeader::COMMAND_SIZE && contents[msg_type_len] != 0) {
        // Verify that message type bytes before the first 0x00 are in range.
        if (contents[msg_type_len] < ' ' || contents[msg_type_len] > 0x7F) {
            return {};
        }
        ++msg_type_len;
    }
    std::string ret{reinterpret_cast<const char*>(contents.data()), msg_type_len};
    while (msg_type_len < CMessageHeader::COMMAND_SIZE) {
        // Verify that message type bytes after the first 0x00 are also 0x00.
        if (contents[msg_type_len] != 0) return {};
        ++msg_type_len;
    }
    // Strip message type bytes of contents.
    contents = contents.subspan(CMessageHeader::COMMAND_SIZE);
    return ret;
}

CNetMessage V2Transport::GetReceivedMessage(std::chrono::microseconds time, bool& reject_message) noexcept
{
    AssertLockNotHeld(m_recv_mutex);
    LOCK(m_recv_mutex);
    if (m_recv_state == RecvState::V1) return m_v1_fallback.GetReceivedMessage(time, reject_message);

    Assume(m_recv_state == RecvState::APP_READY);
    Span<const uint8_t> contents{m_recv_decode_buffer};
    auto msg_type = GetMessageType(contents);
    CNetMessage msg{DataStream{}};
    // Note that BIP324Cipher::EXPANSION also includes the length descriptor size.
    msg.m_raw_message_size = m_recv_decode_buffer.size() + BIP324Cipher::EXPANSION;
    if (msg_type) {
        reject_message = false;
        msg.m_type = std::move(*msg_type);
        msg.m_time = time;
        msg.m_message_size = contents.size();
        msg.m_recv.resize(contents.size());
        std::copy(contents.begin(), contents.end(), UCharCast(msg.m_recv.data()));
    } else {
        LogPrint(BCLog::NET, "V2 transport error: invalid message type (%u bytes contents), peer=%d\n", m_recv_decode_buffer.size(), m_nodeid);
        reject_message = true;
    }
    ClearShrink(m_recv_decode_buffer);
    SetReceiveState(RecvState::APP);

    return msg;
}

bool V2Transport::SetMessageToSend(CSerializedNetMsg& msg) noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    if (m_send_state == SendState::V1) return m_v1_fallback.SetMessageToSend(msg);
    // We only allow adding a new message to be sent when in the READY state (so the packet cipher
    // is available) and the send buffer is empty. This limits the number of messages in the send
    // buffer to just one, and leaves the responsibility for queueing them up to the caller.
    if (!(m_send_state == SendState::READY && m_send_buffer.empty())) return false;
    // Construct contents (encoding message type + payload).
    std::vector<uint8_t> contents;
    auto short_message_id = V2_MESSAGE_MAP(msg.m_type);
    if (short_message_id) {
        contents.resize(1 + msg.data.size());
        contents[0] = *short_message_id;
        std::copy(msg.data.begin(), msg.data.end(), contents.begin() + 1);
    } else {
        // Initialize with zeroes, and then write the message type string starting at offset 1.
        // This means contents[0] and the unused positions in contents[1..13] remain 0x00.
        contents.resize(1 + CMessageHeader::COMMAND_SIZE + msg.data.size(), 0);
        std::copy(msg.m_type.begin(), msg.m_type.end(), contents.data() + 1);
        std::copy(msg.data.begin(), msg.data.end(), contents.begin() + 1 + CMessageHeader::COMMAND_SIZE);
    }
    // Construct ciphertext in send buffer.
    m_send_buffer.resize(contents.size() + BIP324Cipher::EXPANSION);
    m_cipher.Encrypt(MakeByteSpan(contents), {}, false, MakeWritableByteSpan(m_send_buffer));
    m_send_type = msg.m_type;
    // Release memory
    ClearShrink(msg.data);
    return true;
}

Transport::BytesToSend V2Transport::GetBytesToSend(bool have_next_message) const noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    if (m_send_state == SendState::V1) return m_v1_fallback.GetBytesToSend(have_next_message);

    if (m_send_state == SendState::MAYBE_V1) Assume(m_send_buffer.empty());
    Assume(m_send_pos <= m_send_buffer.size());
    return {
        Span{m_send_buffer}.subspan(m_send_pos),
        // We only have more to send after the current m_send_buffer if there is a (next)
        // message to be sent, and we're capable of sending packets. */
        have_next_message && m_send_state == SendState::READY,
        m_send_type
    };
}

void V2Transport::MarkBytesSent(size_t bytes_sent) noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    if (m_send_state == SendState::V1) return m_v1_fallback.MarkBytesSent(bytes_sent);

    if (m_send_state == SendState::AWAITING_KEY && m_send_pos == 0 && bytes_sent > 0) {
        LogPrint(BCLog::NET, "start sending v2 handshake to peer=%d\n", m_nodeid);
    }

    m_send_pos += bytes_sent;
    Assume(m_send_pos <= m_send_buffer.size());
    if (m_send_pos >= CMessageHeader::HEADER_SIZE) {
        m_sent_v1_header_worth = true;
    }
    // Wipe the buffer when everything is sent.
    if (m_send_pos == m_send_buffer.size()) {
        m_send_pos = 0;
        ClearShrink(m_send_buffer);
    }
}

bool V2Transport::ShouldReconnectV1() const noexcept
{
    AssertLockNotHeld(m_send_mutex);
    AssertLockNotHeld(m_recv_mutex);
    // Only outgoing connections need reconnection.
    if (!m_initiating) return false;

    LOCK(m_recv_mutex);
    // We only reconnect in the very first state and when the receive buffer is empty. Together
    // these conditions imply nothing has been received so far.
    if (m_recv_state != RecvState::KEY) return false;
    if (!m_recv_buffer.empty()) return false;
    // Check if we've sent enough for the other side to disconnect us (if it was V1).
    LOCK(m_send_mutex);
    return m_sent_v1_header_worth;
}

size_t V2Transport::GetSendMemoryUsage() const noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    if (m_send_state == SendState::V1) return m_v1_fallback.GetSendMemoryUsage();

    return sizeof(m_send_buffer) + memusage::DynamicUsage(m_send_buffer);
}

Transport::Info V2Transport::GetInfo() const noexcept
{
    AssertLockNotHeld(m_recv_mutex);
    LOCK(m_recv_mutex);
    if (m_recv_state == RecvState::V1) return m_v1_fallback.GetInfo();

    Transport::Info info;

    // Do not report v2 and session ID until the version packet has been received
    // and verified (confirming that the other side very likely has the same keys as us).
    if (m_recv_state != RecvState::KEY_MAYBE_V1 && m_recv_state != RecvState::KEY &&
        m_recv_state != RecvState::GARB_GARBTERM && m_recv_state != RecvState::VERSION) {
        info.transport_type = TransportProtocolType::V2;
        info.session_id = uint256(MakeUCharSpan(m_cipher.GetSessionID()));
    } else {
        info.transport_type = TransportProtocolType::DETECTING;
    }

    return info;
}
