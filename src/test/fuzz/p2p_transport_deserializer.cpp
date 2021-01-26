// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <net.h>
#include <protocol.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

void initialize_p2p_transport_deserializer()
{
    SelectParams(CBaseChainParams::REGTEST);
}

void test_deserializer(std::unique_ptr<TransportDeserializer>& deserializer, Span<const uint8_t> msg_bytes, const int header_size) {
    size_t original_size = msg_bytes.size();
    while (msg_bytes.size() > 0) {
        const int handled = deserializer->Read(msg_bytes);
        if (handled < 0) {
            break;
        }
        if (deserializer->Complete()) {
            const std::chrono::microseconds m_time{std::numeric_limits<int64_t>::max()};
            uint32_t out_err_raw_size{0};
            Optional<CNetMessage> result{deserializer->GetMessage(m_time, out_err_raw_size)};
            if (result) {
                assert(result->m_command.size() <= CMessageHeader::COMMAND_SIZE);
                assert(result->m_raw_message_size <= original_size);
                assert(result->m_raw_message_size == header_size + result->m_message_size);
                assert(result->m_time == m_time);
            }
        }
    }
}
FUZZ_TARGET_INIT(p2p_transport_deserializer, initialize_p2p_transport_deserializer)
{
    // Construct deserializer, with a dummy NodeId
    std::unique_ptr<TransportDeserializer> v1_deserializer = MakeUnique<V1TransportDeserializer>(Params(), (NodeId)0, SER_NETWORK, INIT_PROTO_VERSION);
    Span<const uint8_t> msg_bytes_v1{buffer};
    test_deserializer(v1_deserializer, msg_bytes_v1, CMessageHeader::HEADER_SIZE);

    const CPrivKey k1(32, 0);
    const CPrivKey k2(32, 0);
    Span<const uint8_t> msg_bytes_v2{buffer};
    std::unique_ptr<TransportDeserializer> v2_deserializer = MakeUnique<V2TransportDeserializer>(V2TransportDeserializer((NodeId)0, k1, k2));
    test_deserializer(v2_deserializer, msg_bytes_v2, CHACHA20_POLY1305_AEAD_AAD_LEN + CHACHA20_POLY1305_AEAD_TAG_LEN);
}
