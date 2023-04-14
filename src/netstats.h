// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETSTATS_H
#define BITCOIN_NETSTATS_H

#include <node/connection_types.h>
#include <protocol.h>

#include <array>
#include <atomic>
#include <cstddef>

/**
 * Placeholder for total network traffic. Split by direction, network, connection
 * type and message type (byte and message counts).
 */
class NetStats
{
public:
    struct MsgStat {
        std::atomic_uint64_t byte_count; //!< Number of bytes transferred.
        std::atomic_uint64_t msg_count;  //!< Number of messages transferred.

        MsgStat() = default;

        MsgStat(const MsgStat& x) : byte_count{x.byte_count.load()}, msg_count{x.msg_count.load()} {}

        MsgStat(std::atomic_uint64_t x, std::atomic_uint64_t y) : byte_count{x.load()}, msg_count{y.load()} {}
    };

    enum class Direction { SENT,
                           RECV };

    /// Number of elements in `Direction`.
    static constexpr size_t NUM_DIRECTIONS{2};

    // Innermost array
    using MsgStatArray = std::array<MsgStat, NUM_NET_MESSAGE_TYPES + 1>; // Add 1 for "Other" message type

    // Second innermost array
    using ConnectionTypeArray = std::array<MsgStatArray, NUM_CONNECTION_TYPES>;

    // Third innermost array
    using NetMaxArray = std::array<ConnectionTypeArray, NET_MAX>;

    // Outer array
    using MultiDimensionalStats = std::array<NetMaxArray, NUM_DIRECTIONS>;

    MultiDimensionalStats m_data;
};

#endif // BITCOIN_NETSTATS_H
