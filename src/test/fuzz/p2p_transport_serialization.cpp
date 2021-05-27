// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <hash.h>
#include <net.h>
#include <netmessagemaker.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

void initialize_p2p_transport_serialization()
{
    SelectParams(CBaseChainParams::REGTEST);
}

FUZZ_TARGET_INIT(p2p_transport_serialization, initialize_p2p_transport_serialization)
{
    // Construct deserializer, with a dummy NodeId
    V1TransportDeserializer deserializer{Params(), (NodeId)0, SER_NETWORK, INIT_PROTO_VERSION};
    V1TransportSerializer serializer{};
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    auto checksum_assist = fuzzed_data_provider.ConsumeBool();
    auto magic_bytes_assist = fuzzed_data_provider.ConsumeBool();
    std::vector<uint8_t> mutable_msg_bytes;

    auto header_bytes_remaining = CMessageHeader::HEADER_SIZE;
    if (magic_bytes_assist) {
        auto msg_start = Params().MessageStart();
        for (size_t i = 0; i < CMessageHeader::MESSAGE_SIZE_SIZE; ++i) {
            mutable_msg_bytes.push_back(msg_start[i]);
        }
        header_bytes_remaining -= CMessageHeader::MESSAGE_SIZE_SIZE;
    }

    if (checksum_assist) {
        header_bytes_remaining -= CMessageHeader::CHECKSUM_SIZE;
    }

    auto header_random_bytes = fuzzed_data_provider.ConsumeBytes<uint8_t>(header_bytes_remaining);
    mutable_msg_bytes.insert(mutable_msg_bytes.end(), header_random_bytes.begin(), header_random_bytes.end());
    auto payload_bytes = fuzzed_data_provider.ConsumeRemainingBytes<uint8_t>();

    if (checksum_assist && mutable_msg_bytes.size() == CMessageHeader::CHECKSUM_OFFSET) {
        CHash256 hasher;
        unsigned char hsh[32];
        hasher.Write(payload_bytes);
        hasher.Finalize(hsh);
        for (size_t i = 0; i < CMessageHeader::CHECKSUM_SIZE; ++i) {
           mutable_msg_bytes.push_back(hsh[i]);
        }
    }

    mutable_msg_bytes.insert(mutable_msg_bytes.end(), payload_bytes.begin(), payload_bytes.end());
    Span<const uint8_t> msg_bytes{mutable_msg_bytes};
    while (msg_bytes.size() > 0) {
        const int handled = deserializer.Read(msg_bytes);
        if (handled < 0) {
            break;
        }
        if (deserializer.Complete()) {
            const std::chrono::microseconds m_time{std::numeric_limits<int64_t>::max()};
            uint32_t out_err_raw_size{0};
            std::optional<CNetMessage> result{deserializer.GetMessage(m_time, out_err_raw_size)};
            if (result) {
                assert(result->m_command.size() <= CMessageHeader::COMMAND_SIZE);
                assert(result->m_raw_message_size <= mutable_msg_bytes.size());
                assert(result->m_raw_message_size == CMessageHeader::HEADER_SIZE + result->m_message_size);
                assert(result->m_time == m_time);

                std::vector<unsigned char> header;
                auto msg = CNetMsgMaker{result->m_recv.GetVersion()}.Make(result->m_command, MakeUCharSpan(result->m_recv));
                serializer.prepareForTransport(msg, header);
            }
        }
    }
}
