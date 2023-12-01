// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SV2_MESSAGES_H
#define BITCOIN_COMMON_SV2_MESSAGES_H

#include <consensus/validation.h>
#include <cstdint>
#include <net_message.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <string>
#include <vector>
#include <uint256.h>

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
    NEW_TEMPLATE = 0x71,
    SET_NEW_PREV_HASH = 0x72,
    REQUEST_TRANSACTION_DATA = 0x73,
    REQUEST_TRANSACTION_DATA_SUCCESS = 0x74,
    REQUEST_TRANSACTION_DATA_ERROR = 0x75,
    SUBMIT_SOLUTION = 0x76,
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
 * The work template for downstream devices. Can be used for future work or immediate work.
 * The NewTemplate will be matched to a cached block using the template id.
 */
struct Sv2NewTemplateMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::NEW_TEMPLATE;

    /**
     * Server’s identification of the template. Strictly increasing, the current UNIX
     * time may be used in place of an ID.
     */
    uint64_t m_template_id;

    /**
     * True if the template is intended for future SetNewPrevHash message sent on the channel.
     * If False, the job relates to the last sent SetNewPrevHash message on the channel
     * and the miner should start to work on the job immediately.
     */
    bool m_future_template;

    /**
     * Valid header version field that reflects the current network consensus.
     * The general purpose bits (as specified in BIP320) can be freely manipulated
     * by the downstream node. The downstream node MUST NOT rely on the upstream
     * node to set the BIP320 bits to any particular value.
     */
    uint32_t m_version;

    /**
     * The coinbase transaction nVersion field.
     */
    uint32_t m_coinbase_tx_version;

    /**
     * Up to 8 bytes (not including the length byte) which are to be placed at
     * the beginning of the coinbase field in the coinbase transaction.
     */
    CScript m_coinbase_prefix;

    /**
     * The coinbase transaction input’s nSequence field.
     */
    uint32_t m_coinbase_tx_input_sequence;

    /**
     * The value, in satoshis, available for spending in coinbase outputs added
     * by the client. Includes both transaction fees and block subsidy.
     */
    uint64_t m_coinbase_tx_value_remaining;

    /**
     * The number of transaction outputs included in coinbase_tx_outputs.
     */
    uint32_t m_coinbase_tx_outputs_count;

    /**
     * Bitcoin transaction outputs to be included as the last outputs in the coinbase transaction.
     */
    std::vector<CTxOut> m_coinbase_tx_outputs;

    /**
     * The locktime field in the coinbase transaction.
     */
    uint32_t m_coinbase_tx_locktime;

    /**
     * Merkle path hashes ordered from deepest.
     */
    std::vector<uint256> m_merkle_path;

    Sv2NewTemplateMsg() = default;
    explicit Sv2NewTemplateMsg(const CBlock& block, uint64_t template_id, bool future_template);

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_template_id
          << m_future_template
          << m_version
          << m_coinbase_tx_version
          << m_coinbase_prefix
          << m_coinbase_tx_input_sequence
          << m_coinbase_tx_value_remaining
          << m_coinbase_tx_outputs_count;

        // If there are more than 0 coinbase tx outputs, then we need to serialize them
        // as [B0_64K](https://github.com/stratum-mining/sv2-spec/blob/main/03-Protocol-Overview.md#31-data-types-mapping)
        if (m_coinbase_tx_outputs_count > 0) {
            std::vector<uint8_t> outputs_bytes;
            VectorWriter{outputs_bytes, 0, m_coinbase_tx_outputs.at(0)};

            s << static_cast<uint16_t>(outputs_bytes.size());
            s.write(MakeByteSpan(outputs_bytes));
        } else {
            // We will still need to send 2 bytes indicating an empty coinbase-tx_outputs array as a B0_64K.
            s << static_cast<uint16_t>(0);
        }

        s << m_coinbase_tx_locktime
          << m_merkle_path;
    }
};

/**
 * When the template provider creates a new valid best block, the template provider
 * MUST immediately send the SetNewPrevHash message. This message can also be used
 * for a future template, indicating the client can begin work on a previously
 * received and cached NewTemplate which contains the same template id.
 */
struct Sv2SetNewPrevHashMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::SET_NEW_PREV_HASH;

    /**
     * The id referenced in a previous NewTemplate message.
     */
    uint64_t m_template_id;

    /**
     * Previous block’s hash, as it must appear in the next block’s header.
     */
    uint256 m_prev_hash;

    /**
     * The nTime field in the block header at which the client should start (usually current time).
     * This is NOT the minimum valid nTime value.
     */
    uint32_t m_header_timestamp;

    /**
     * Block header field.
     */
    uint32_t m_nBits;

    /**
     * The maximum double-SHA256 hash value which would represent a valid block.
     * Note that this may be lower than the target implied by nBits in several cases,
     * including weak-block based block propagation.
     */
    uint256 m_target;

    Sv2SetNewPrevHashMsg() = default;
    explicit Sv2SetNewPrevHashMsg(const CBlock& block, uint64_t template_id);

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_template_id
          << m_prev_hash
          << m_header_timestamp
          << m_nBits
          << m_target;
    }
};

/**
 * The client (usually a Job Negotiator) sends a RequestTransactionData message
 * to the Template Provider asking for the full set of transaction data (excluding
 * the coinbase) in the block and any additional data relevant for validation
 * associated with the template_id.
 */
struct Sv2RequestTransactionDataMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::REQUEST_TRANSACTION_DATA;

    /**
     * The template_id corresponding to a NewTemplate message.
     */
    uint64_t m_template_id;

    Sv2RequestTransactionDataMsg() = default;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_template_id;
    }
};

/**
 * A message for a successful request for transaction data. It contains the full
 * serialized transaction data from a NewTemplate according to the id.
 */
struct Sv2RequestTransactionDataSuccessMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::REQUEST_TRANSACTION_DATA_SUCCESS;

    /**
     * The template_id corresponding to a NewTemplate message.
     */
    uint64_t m_template_id;

    /**
     * Extra data which the Pool may require to validate the work
     */
    std::vector<uint8_t> m_excess_data;

    /**
     * List of full transactions requested by client found in the
     * corresponding template.
     */
    std::vector<CTransactionRef> m_transactions_list;

    Sv2RequestTransactionDataSuccessMsg() = default;

    explicit Sv2RequestTransactionDataSuccessMsg(uint64_t template_id, std::vector<uint8_t>&& excess_data, std::vector<CTransactionRef>&& transactions_list) : m_template_id{template_id}, m_excess_data{excess_data}, m_transactions_list{transactions_list} {};

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_template_id;

        // excess data is expected to be serialized as a B0_64K type.
        if (m_excess_data.empty()) {
            s << static_cast<uint16_t>(0);
        } else {
            s << static_cast<uint16_t>(m_excess_data.size());
            s.write(MakeByteSpan(m_excess_data));
        }

        // transactions list is expected to be serialized as a SEQ0_64K[B0_16M].
        s << static_cast<uint16_t>(m_transactions_list.size());
        for (const auto& tx : m_transactions_list) {
            DataStream ss_tx{};
            ss_tx << TX_WITH_WITNESS(*tx);
            auto tx_size = static_cast<uint32_t>(ss_tx.size());

            u24_t tx_byte_len;
            tx_byte_len[2] = (tx_size >> 16) & 0xff;
            tx_byte_len[1] = (tx_size >> 8) & 0xff;
            tx_byte_len[0] = tx_size & 0xff;

            s << tx_byte_len;
            s.write(MakeByteSpan(ss_tx));
        }
    };
};

/**
 * The error message for the client if the template provider is unable to send
 * the full serialized transaction data.
 */
struct Sv2RequestTransactionDataErrorMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::REQUEST_TRANSACTION_DATA_ERROR;

    /**
     * The template_id corresponding to a NewTemplate/RequestTransactionData message.
     */
    uint64_t m_template_id;

    /**
     * Human-readable error code on why no transaction data has been provided.
     */
    std::string m_error_code;

    explicit Sv2RequestTransactionDataErrorMsg(uint64_t template_id, std::string&& error_code) : m_template_id{template_id}, m_error_code{error_code} {};

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_template_id
          << m_error_code;
    }
};

/**
 * The client sends a SubmitSolution after finding a coinbase transaction/nonce
 * pair which double-SHA256 hashes at or below SetNewPrevHash::target. The template provider
 * finds the cached block according to the template id and reconstructs the block with the
 * values from SubmitSolution. The template provider must then propagate the block to the
 * Bitcoin Network.
 */
struct Sv2SubmitSolutionMsg
{
    /**
     * The default message type value for this Stratum V2 message.
     */
    static constexpr auto m_msg_type = Sv2MsgType::SUBMIT_SOLUTION;

    /**
     * The id referenced in a NewTemplate.
     */
    uint64_t m_template_id;

    /**
     * The version field in the block header. Bits not defined by BIP320 as additional
     * nonce MUST be the same as they appear in the NewWork message, other bits may
     * be set to any value.
     */
    uint32_t m_version;

    /**
     * The nTime field in the block header. This MUST be greater than or equal to
     * the header_timestamp field in the latest SetNewPrevHash message and lower
     * than or equal to that value plus the number of seconds since the receipt
     * of that message.
     */
    uint32_t m_header_timestamp;

    /**
     * The nonce field in the header.
     */
    uint32_t m_header_nonce;

    /**
     * The full serialized coinbase transaction, meeting all the requirements of the NewWork message, above.
     */
    CMutableTransaction m_coinbase_tx;

    Sv2SubmitSolutionMsg() = default;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> m_template_id >> m_version >> m_header_timestamp >> m_header_nonce;

        // Ignore the 2 byte length as the rest of the stream is assumed to be
        // the m_coinbase_tx.
        s.ignore(2);
        s >> TX_WITH_WITNESS(m_coinbase_tx);
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
    // TMP: Moved from private
    Sv2NetHeader m_sv2_header;
    std::vector<uint8_t> m_msg;

    // TMP;
    // TODO: DO move
    explicit Sv2NetMsg(const Sv2NetHeader&& sv2_header, const std::vector<uint8_t>&& msg) : m_sv2_header{sv2_header}, m_msg{msg} {};

    // TMP;

    /**
     * Serializes the message M and sets an Sv2 network header.
     * @throws std::ios_base or std::out_of_range errors.
     */
    template <typename M>
    explicit Sv2NetMsg(const M& msg)
    {
        // Serialize the sv2 message.
        VectorWriter{m_msg, 0, msg};

        // Create the header for the message.
        m_sv2_header = Sv2NetHeader{msg.m_msg_type, static_cast<uint32_t>(m_msg.size())};
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << m_sv2_header;
        s.write(MakeByteSpan(m_msg));
    }
};

}

#endif // BITCOIN_COMMON_SV2_MESSAGES_H
