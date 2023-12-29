// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/v1_transport.h>
#include <logging.h>
#include <util/check.h>

V1Transport::V1Transport(const NodeId node_id) noexcept
    : m_magic_bytes{Params().MessageStart()}, m_node_id{node_id}
{
    LOCK(m_recv_mutex);
    Reset();
}

Transport::Info V1Transport::GetInfo() const noexcept
{
    return {.transport_type = TransportProtocolType::V1, .session_id = {}};
}

int V1Transport::readHeader(Span<const uint8_t> msg_bytes)
{
    AssertLockHeld(m_recv_mutex);
    // copy data to temporary parsing buffer
    unsigned int nRemaining = CMessageHeader::HEADER_SIZE - nHdrPos;
    unsigned int nCopy = std::min<unsigned int>(nRemaining, msg_bytes.size());

    memcpy(&hdrbuf[nHdrPos], msg_bytes.data(), nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < CMessageHeader::HEADER_SIZE)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    }
    catch (const std::exception&) {
        LogPrint(BCLog::NET, "Header error: Unable to deserialize, peer=%d\n", m_node_id);
        return -1;
    }

    // Check start string, network magic
    if (hdr.pchMessageStart != m_magic_bytes) {
        LogPrint(BCLog::NET, "Header error: Wrong MessageStart %s received, peer=%d\n", HexStr(hdr.pchMessageStart), m_node_id);
        return -1;
    }

    // reject messages larger than MAX_SIZE or MAX_PROTOCOL_MESSAGE_LENGTH
    if (hdr.nMessageSize > MAX_SIZE || hdr.nMessageSize > MAX_PROTOCOL_MESSAGE_LENGTH) {
        LogPrint(BCLog::NET, "Header error: Size too large (%s, %u bytes), peer=%d\n", SanitizeString(hdr.GetCommand()), hdr.nMessageSize, m_node_id);
        return -1;
    }

    // switch state to reading message data
    in_data = true;

    return nCopy;
}

int V1Transport::readData(Span<const uint8_t> msg_bytes)
{
    AssertLockHeld(m_recv_mutex);
    unsigned int nRemaining = hdr.nMessageSize - nDataPos;
    unsigned int nCopy = std::min<unsigned int>(nRemaining, msg_bytes.size());

    if (vRecv.size() < nDataPos + nCopy) {
        // Allocate up to 256 KiB ahead, but never more than the total message size.
        vRecv.resize(std::min(hdr.nMessageSize, nDataPos + nCopy + 256 * 1024));
    }

    hasher.Write(msg_bytes.first(nCopy));
    memcpy(&vRecv[nDataPos], msg_bytes.data(), nCopy);
    nDataPos += nCopy;

    return nCopy;
}

const uint256& V1Transport::GetMessageHash() const
{
    AssertLockHeld(m_recv_mutex);
    assert(CompleteInternal());
    if (data_hash.IsNull())
        hasher.Finalize(data_hash);
    return data_hash;
}

CNetMessage V1Transport::GetReceivedMessage(const std::chrono::microseconds time, bool& reject_message)
{
    AssertLockNotHeld(m_recv_mutex);
    // Initialize out parameter
    reject_message = false;
    // decompose a single CNetMessage from the TransportDeserializer
    LOCK(m_recv_mutex);
    CNetMessage msg(std::move(vRecv));

    // store message type string, time, and sizes
    msg.m_type = hdr.GetCommand();
    msg.m_time = time;
    msg.m_message_size = hdr.nMessageSize;
    msg.m_raw_message_size = hdr.nMessageSize + CMessageHeader::HEADER_SIZE;

    uint256 hash = GetMessageHash();

    // We just received a message off the wire, harvest entropy from the time (and the message checksum)
    RandAddEvent(ReadLE32(hash.begin()));

    // Check checksum and header message type string
    if (memcmp(hash.begin(), hdr.pchChecksum, CMessageHeader::CHECKSUM_SIZE) != 0) {
        LogPrint(BCLog::NET, "Header error: Wrong checksum (%s, %u bytes), expected %s was %s, peer=%d\n",
                 SanitizeString(msg.m_type), msg.m_message_size,
                 HexStr(Span{hash}.first(CMessageHeader::CHECKSUM_SIZE)),
                 HexStr(hdr.pchChecksum),
                 m_node_id);
        reject_message = true;
    } else if (!hdr.IsCommandValid()) {
        LogPrint(BCLog::NET, "Header error: Invalid message type (%s, %u bytes), peer=%d\n",
                 SanitizeString(hdr.GetCommand()), msg.m_message_size, m_node_id);
        reject_message = true;
    }

    // Always reset the network deserializer (prepare for the next message)
    Reset();
    return msg;
}

bool V1Transport::SetMessageToSend(CSerializedNetMsg& msg) noexcept
{
    AssertLockNotHeld(m_send_mutex);
    // Determine whether a new message can be set.
    LOCK(m_send_mutex);
    if (m_sending_header || m_bytes_sent < m_message_to_send.data.size()) return false;

    // create dbl-sha256 checksum
    uint256 hash = Hash(msg.data);

    // create header
    CMessageHeader hdr(m_magic_bytes, msg.m_type.c_str(), msg.data.size());
    memcpy(hdr.pchChecksum, hash.begin(), CMessageHeader::CHECKSUM_SIZE);

    // serialize header
    m_header_to_send.clear();
    VectorWriter{m_header_to_send, 0, hdr};

    // update state
    m_message_to_send = std::move(msg);
    m_sending_header = true;
    m_bytes_sent = 0;
    return true;
}

Transport::BytesToSend V1Transport::GetBytesToSend(bool have_next_message) const noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    if (m_sending_header) {
        return {Span{m_header_to_send}.subspan(m_bytes_sent),
                // We have more to send after the header if the message has payload, or if there
                // is a next message after that.
                have_next_message || !m_message_to_send.data.empty(),
                m_message_to_send.m_type
               };
    } else {
        return {Span{m_message_to_send.data}.subspan(m_bytes_sent),
                // We only have more to send after this message's payload if there is another
                // message.
                have_next_message,
                m_message_to_send.m_type
               };
    }
}

void V1Transport::MarkBytesSent(size_t bytes_sent) noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    m_bytes_sent += bytes_sent;
    if (m_sending_header && m_bytes_sent == m_header_to_send.size()) {
        // We're done sending a message's header. Switch to sending its data bytes.
        m_sending_header = false;
        m_bytes_sent = 0;
    } else if (!m_sending_header && m_bytes_sent == m_message_to_send.data.size()) {
        // We're done sending a message's data. Wipe the data vector to reduce memory consumption.
        ClearShrink(m_message_to_send.data);
        m_bytes_sent = 0;
    }
}

size_t V1Transport::GetSendMemoryUsage() const noexcept
{
    AssertLockNotHeld(m_send_mutex);
    LOCK(m_send_mutex);
    // Don't count sending-side fields besides m_message_to_send, as they're all small and bounded.
    return m_message_to_send.GetMemoryUsage();
}
