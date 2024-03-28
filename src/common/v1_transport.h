// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_V1_TRANSPORT_H
#define BITCOIN_COMMON_V1_TRANSPORT_H

#include <chainparams.h>
#include <common/transport.h>
#include <hash.h>
#include <kernel/messagestartchars.h>
#include <protocol.h>
#include <sync.h>

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

#endif // BITCOIN_COMMON_V1_TRANSPORT_H
