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

    void Init()
    {
        for (std::size_t direction_index = 0; direction_index < NUM_DIRECTIONS; direction_index++) {
            for (int network_index = 0; network_index < NET_MAX; network_index++) {
                for (std::size_t connection_index = 0; connection_index < NUM_CONNECTION_TYPES; connection_index++) {
                    for (std::size_t message_index = 0; message_index < NUM_NET_MESSAGE_TYPES + 1; message_index++) {
                        // +1 for the "other" message type
                        auto& stat = m_data
                            .at(direction_index)
                            .at(network_index)
                            .at(connection_index)
                            .at(message_index);

                        stat.msg_count = 0;
                        stat.byte_count = 0;
                    }
                }
            }
        }
    }

    /**
     * Increment the amount of bytes transferred by `bytes` and the amount of messages by 1.
     */
    void Record(Direction direction,
                Network net,
                ConnectionType conn_type,
                const std::string& msg_type,
                size_t byte_count);

    // The ...FromIndex() and ...ToIndex() methods below convert from/to
    // indexes of `m_data[]` to the actual values they represent. For example,
    // assuming MessageTypeToIndex("ping") == 15, then everything stored in
    // m_data[x][y][z][15] is traffic from "ping" messages (for any x, y or z).

    [[nodiscard]] static Direction DirectionFromIndex(size_t index);
    [[nodiscard]] static Network NetworkFromIndex(size_t index);
    [[nodiscard]] static ConnectionType ConnectionTypeFromIndex(size_t index);

    static std::string DirectionAsString(Direction direction);

private:
    // Helper methods to make sure the indexes associated with enums are reliable
    [[nodiscard]] static size_t DirectionToIndex(Direction direction);
    [[nodiscard]] static size_t NetworkToIndex(Network net);
    [[nodiscard]] static size_t ConnectionTypeToIndex(ConnectionType conn_type);
};

#endif // BITCOIN_NETSTATS_H
