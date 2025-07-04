// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/connection_types.h>
#include <cassert>

std::string ConnectionTypeAsString(ConnectionType conn_type)
{
    switch (conn_type) {
    case ConnectionType::INBOUND:
        return "inbound";
    case ConnectionType::MANUAL:
        return "manual";
    case ConnectionType::FEELER:
        return "feeler";
    case ConnectionType::OUTBOUND_FULL_RELAY:
        return "outbound-full-relay";
    case ConnectionType::BLOCK_RELAY:
        return "block-relay-only";
    case ConnectionType::ADDR_FETCH:
        return "addr-fetch";
    case ConnectionType::PRIVATE_BROADCAST:
        return "private-broadcast";
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

std::string TransportTypeAsString(TransportProtocolType transport_type)
{
    switch (transport_type) {
    case TransportProtocolType::DETECTING:
        return "detecting";
    case TransportProtocolType::V1:
        return "v1";
    case TransportProtocolType::V2:
        return "v2";
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}
