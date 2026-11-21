// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_NETIF_H
#define BITCOIN_COMMON_NETIF_H

#include <netaddress.h>

#include <optional>
#include <vector>

//! Query the OS for the default gateway for `network`. This only makes sense for NET_IPV4 and NET_IPV6.
//! Returns std::nullopt if it cannot be found, or there is no support for this OS.
std::optional<CNetAddr> QueryDefaultGateway(Network network);

//! Return all local non-loopback IPv4 and IPv6 network addresses.
std::vector<CNetAddr> GetLocalAddresses();

//! Ask the OS whether there is a route to the global internet on `network` (NET_IPV4 or NET_IPV6),
//! without sending any packets. This is done by connect()ing an unbound UDP socket to global
//! unicast addresses, which only performs a route lookup and source address selection.
//! Returns std::nullopt if this could not be determined: only specific errors (no route, no
//! address, address family not supported) are treated as "no route", other errors as "unknown".
std::optional<bool> HasGlobalRoute(Network network);

#endif // BITCOIN_COMMON_NETIF_H
