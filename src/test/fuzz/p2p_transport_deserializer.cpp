// Copyright (c) 2019 The Widecoin Core developers
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

void initialize()
{
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    // Construct deserializer, with a dummy NodeId
    V1TransportDeserializer deserializer{Params(), (NodeId)0, SER_NETWORK, INIT_PROTO_VERSION};
    const char* pch = (const char*)buffer.data();
    size_t n_bytes = buffer.size();
    while (n_bytes > 0) {
        const int handled = deserializer.Read(pch, n_bytes);
        if (handled < 0) {
            break;
        }
        pch += handled;
        n_bytes -= handled;
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
