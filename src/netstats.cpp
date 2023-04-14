// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netstats.h>

NetStats::Direction NetStats::DirectionFromIndex(size_t index)
{
    switch (index) {
    case 0: return Direction::SENT;
    case 1: return Direction::RECV;
    }

    assert(false);
}

size_t NetStats::DirectionToIndex(Direction direction)
{
    switch (direction) {
    case Direction::SENT: return 0;
    case Direction::RECV: return 1;
    }

    assert(false);
}

Network NetStats::NetworkFromIndex(size_t index)
{
    switch (index) {
    case 0: return Network::NET_UNROUTABLE;
    case 1: return Network::NET_IPV4;
    case 2: return Network::NET_IPV6;
    case 3: return Network::NET_ONION;
    case 4: return Network::NET_I2P;
    case 5: return Network::NET_CJDNS;
    case 6: return Network::NET_INTERNAL;
    }

    assert(false);
}

size_t NetStats::NetworkToIndex(Network net)
{
    switch (net) {
    case Network::NET_UNROUTABLE: return 0;
    case Network::NET_IPV4: return 1;
    case Network::NET_IPV6: return 2;
    case Network::NET_ONION: return 3;
    case Network::NET_I2P: return 4;
    case Network::NET_CJDNS: return 5;
    case Network::NET_INTERNAL: return 6;
    case Network::NET_MAX: assert(false);
    }
    assert(false);
}

ConnectionType NetStats::ConnectionTypeFromIndex(size_t index)
{
    switch (index) {
    case 0: return ConnectionType::INBOUND;
    case 1: return ConnectionType::OUTBOUND_FULL_RELAY;
    case 2: return ConnectionType::MANUAL;
    case 3: return ConnectionType::FEELER;
    case 4: return ConnectionType::BLOCK_RELAY;
    case 5: return ConnectionType::ADDR_FETCH;
    }

    assert(false);
}

size_t NetStats::ConnectionTypeToIndex(ConnectionType conn_type)
{
    switch (conn_type) {
    case ConnectionType::INBOUND: return 0;
    case ConnectionType::OUTBOUND_FULL_RELAY: return 1;
    case ConnectionType::MANUAL: return 2;
    case ConnectionType::FEELER: return 3;
    case ConnectionType::BLOCK_RELAY: return 4;
    case ConnectionType::ADDR_FETCH: return 5;
    }
    assert(false);
}

void NetStats::Record(Direction direction,
                      Network net,
                      ConnectionType conn_type,
                      const std::string& msg_type,
                      size_t byte_count)
{
    auto& data = m_data
        .at(DirectionToIndex(direction))
        .at(NetworkToIndex(net))
        .at(ConnectionTypeToIndex(conn_type))
        .at(messageTypeToIndex(msg_type));

    ++data.msg_count;
    data.byte_count += byte_count;
}
