// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SV2_MESSAGES_H
#define BITCOIN_COMMON_SV2_MESSAGES_H

#include <net.h> // for CSerializedNetMsg and CNetMessage
#include <span.h>
#include <streams.h>
#include <string>
#include <util/check.h>

namespace node {
/**
 * A type used as the message length field in stratum v2 messages.
 */
using u24_t = uint8_t[3];

/**
 * All the stratum v2 message types handled by the template provider.
 */
enum class Sv2MsgType : uint8_t {
    COINBASE_OUTPUT_DATA_SIZE = 0x70,
};

/**
 * Set the coinbase outputs data len for the outputs that the client wants to add to the coinbase.
 * The template provider MUST NOT provide NewWork messages which would represent consensus-invalid blocks once this
 * additional size — along with a maximally-sized (100 byte) coinbase field — is added.
 */
struct Sv2CoinbaseOutputDataSizeMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE;

    /**
     * The maximum additional serialized bytes which the pool will add in coinbase transaction outputs.
     */
    uint32_t m_coinbase_output_max_additional_size;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_coinbase_output_max_additional_size;
    };


    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_coinbase_output_max_additional_size;
    }
};

/**
 * Header for all stratum v2 messages. Each header must contain the message type,
 * the length of the serialized message and a 2 byte extension field currently
 * not utilised by the template provider.
 */
class Sv2NetHeader
{
public:
    /**
     * Unique identifier of the message.
     */
    Sv2MsgType m_msg_type;

    /**
     * Serialized length of the message.
     */
    uint32_t m_msg_len;

    Sv2NetHeader() = default;
    explicit Sv2NetHeader(Sv2MsgType msg_type, uint32_t msg_len) : m_msg_type{msg_type}, m_msg_len{msg_len} {};

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        // The template provider currently does not use the extension_type field,
        // but the field is still required for all headers.
        uint16_t extension_type = 0;

        u24_t msg_len;
        msg_len[2] = (m_msg_len >> 16) & 0xff;
        msg_len[1] = (m_msg_len >> 8) & 0xff;
        msg_len[0] = m_msg_len & 0xff;

        s << extension_type
          << static_cast<uint8_t>(m_msg_type)
          << msg_len;
    };

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        // Ignore the first 2 bytes (extension type) as the template provider currently doesn't
        // interpret this field.
        s.ignore(2);

        uint8_t msg_type;
        s >> msg_type;
        m_msg_type = static_cast<Sv2MsgType>(msg_type);

        u24_t msg_len_bytes;
        for (unsigned int i = 0; i < sizeof(u24_t); ++i) {
            s >> msg_len_bytes[i];
        }

        m_msg_len = msg_len_bytes[2];
        m_msg_len = m_msg_len << 8 | msg_len_bytes[1];
        m_msg_len = m_msg_len << 8 | msg_len_bytes[0];
    }
};

/**
 * The networked form for all stratum v2 messages, contains a header and a serialized
 * payload from a referenced stratum v2 message.
 */
class Sv2NetMsg
{
public:
    Sv2MsgType m_msg_type;
    std::vector<uint8_t> m_msg;

    explicit Sv2NetMsg(const Sv2MsgType msg_type, const std::vector<uint8_t>&& msg) : m_msg_type{msg_type}, m_msg{msg} {};

    // Unwrap CSerializedNetMsg
    Sv2NetMsg(CSerializedNetMsg&& net_msg)
    {
        Assume(net_msg.m_type == "");
        DataStream ss(MakeByteSpan(net_msg.data));
        Unserialize(ss);
    };

    // Unwrap CNetMsg
    Sv2NetMsg(CNetMessage net_msg)
    {
        Unserialize(net_msg.m_recv);
    };

    operator CSerializedNetMsg()
    {
        CSerializedNetMsg net_msg;
        net_msg.m_type = "";
        DataStream ser;
        Serialize(ser);
        net_msg.data.resize(ser.size());
        std::transform(ser.begin(), ser.end(), net_msg.data.begin(),
                           [](std::byte b) { return static_cast<uint8_t>(b); });
        return net_msg;
    }

    operator CNetMessage()
    {
        DataStream msg;
        Serialize(msg);
        CNetMessage ret{std::move(msg)};
        return ret;
    }

    /**
     * Serializes the message M and sets an Sv2 network header.
     * @throws std::ios_base or std::out_of_range errors.
     */
    template <typename M>
    explicit Sv2NetMsg(const M& msg)
    {
        m_msg_type = msg.m_msg_type;

        // Serialize the sv2 message.
        VectorWriter{m_msg, 0, msg};
    }

    unsigned char* data() { return m_msg.data(); }
    size_t size() { return m_msg.size(); }

    operator Sv2NetHeader()
    {
        Sv2NetHeader hdr;
        hdr.m_msg_type = m_msg_type;
        hdr.m_msg_len = static_cast<uint32_t>(m_msg.size());
        return hdr;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        uint8_t msg_type;
        s >> msg_type;
        m_msg_type = static_cast<Sv2MsgType>(msg_type);
        m_msg.resize(s.size());
        s.read(MakeWritableByteSpan(m_msg));
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << static_cast<uint8_t>(m_msg_type);
        s.write(MakeByteSpan(m_msg));
    }

};

}

#endif // BITCOIN_COMMON_SV2_MESSAGES_H
