// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SV2_MESSAGES_H
#define BITCOIN_COMMON_SV2_MESSAGES_H

#include <net.h> // for CSerializedNetMsg and CNetMessage
#include <consensus/validation.h>
#include <cstdint>
#include <primitives/transaction.h>
#include <script/script.h>
#include <span.h>
#include <streams.h>
#include <string>
#include <vector>
#include <uint256.h>
#include <util/check.h>

class CBlock;
struct CMutableTransaction;
class CTxOut;
class ArithToUint256;

namespace node {
/**
 * A type used as the message length field in stratum v2 messages.
 */
using u24_t = uint8_t[3];

/**
 * All the stratum v2 message types handled by the template provider.
 */
enum class Sv2MsgType : uint8_t {
    SETUP_CONNECTION = 0x00,
    SETUP_CONNECTION_SUCCESS = 0x01,
    SETUP_CONNECTION_ERROR = 0x02,
    COINBASE_OUTPUT_DATA_SIZE = 0x70,
};

struct Sv2SetupConnectionMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::SETUP_CONNECTION;

    /**
     * Specifies the subprotocol for the new connection. It will always be TemplateDistribution
     * (0x02).
     */
    uint8_t m_protocol;

    /**
     * The minimum protocol version the client supports (currently must be 2).
     */
    uint16_t m_min_version;

    /**
     * The maximum protocol version the client supports (currently must be 2).
     */
    uint16_t m_max_version;

    /**
     * Flags indicating optional protocol features the client supports. Each protocol
     * from the protocol field has its own values/flags.
     */
    uint32_t m_flags;

    /**
     * ASCII text indicating the hostname or IP address.
     */
    std::string m_endpoint_host;

    /**
     * Connecting port value.
     */
    uint16_t m_endpoint_port;

    /**
     * Vendor name of the connecting device.
     */
    std::string m_vendor;

    /**
     * Hardware version of the connecting device.
     */
    std::string m_hardware_version;

    /**
     * Firmware of the connecting device.
     */
    std::string m_firmware;

    /**
     * Unique identifier of the device as defined by the vendor.
     */
    std::string m_device_id;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_protocol
          >> m_min_version
          >> m_max_version
          >> m_flags
          >> m_endpoint_host
          >> m_endpoint_port
          >> m_vendor
          >> m_hardware_version
          >> m_firmware
          >> m_device_id;
    }
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
 * Response to the SetupConnection message if the server accepts the connection.
 * The client is required to verify the set of feature flags that the server
 * supports and act accordingly.
 */
struct Sv2SetupConnectionSuccessMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::SETUP_CONNECTION_SUCCESS;

    /**
     * Selected version proposed by the connecting node that the upstream node supports.
     * This version will be used on the connection for the rest of its life.
     */
    uint16_t m_used_version;

    /**
     * Flags indicating optional protocol features the server supports. Each protocol
     * from protocol field has its own values/flags.
     */
    uint32_t m_flags;

    explicit Sv2SetupConnectionSuccessMsg(uint16_t used_version, uint32_t flags) : m_used_version{used_version}, m_flags{flags} {};

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_used_version
          << m_flags;
    }
};

/**
 * Response to the SetupConnection message if the server rejects the connection.
 */
struct Sv2SetupConnectionErrorMsg
{
    static constexpr auto m_msg_type = Sv2MsgType::SETUP_CONNECTION_ERROR;

    /**
     * Flags indicating optional protocol features the server supports. Each protocol
     * from protocol field has its own values/flags.
     */
    uint32_t m_flags;

    /**
     * Human-readable error codes.
     */
    std::string m_error_code;

    explicit Sv2SetupConnectionErrorMsg(uint32_t flags, std::string&& error_code) : m_flags{flags}, m_error_code{std::move(error_code)} {};

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_flags
          << m_error_code;
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
