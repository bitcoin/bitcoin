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

FUZZ_TARGET_INIT(p2p_transport_deserializer, initialize_p2p_transport_deserializer)
{
    // Construct deserializer, with a dummy NodeId
    V1TransportDeserializer deserializer{Params(), (NodeId)0, SER_NETWORK, INIT_PROTO_VERSION};
    Span<const uint8_t> msg_bytes{buffer};
    while (msg_bytes.size() > 0) {
        const int handled = deserializer.Read(msg_bytes);
        if (handled < 0) {
            break;
        }
        if (deserializer.Complete()) {
            const std::chrono::microseconds m_time{std::numeric_limits<int64_t>::max()};
            uint32_t out_err_raw_size{0};
            Optional<CNetMessage> result{deserializer.GetMessage(m_time, out_err_raw_size)};
            if (result) {
                assert(result->m_command.size() <= CMessageHeader::COMMAND_SIZE);
                assert(result->m_raw_message_size <= buffer.size());
                assert(result->m_raw_message_size == CMessageHeader::HEADER_SIZE + result->m_message_size);
                assert(result->m_time == m_time);
            }
        }
    }
}
