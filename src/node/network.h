// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_NETWORK_H
#define BITCOIN_NODE_NETWORK_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * A network type.
 * @note An address may belong to more than one network, for example `10.0.0.1`
 * belongs to both `NET_UNROUTABLE` and `NET_IPV4`.
 * Keep these sequential starting from 0 and `NET_MAX` as the last entry.
 * We have loops like `for (int i = 0; i < NET_MAX; ++i)` that expect to iterate
 * over all enum values and also `GetExtNetwork()` "extends" this enum by
 * introducing standalone constants starting from `NET_MAX`.
 */
enum Network {
    /// Addresses from these networks are not publicly routable on the global Internet.
    NET_UNROUTABLE = 0,

    /// IPv4
    NET_IPV4,

    /// IPv6
    NET_IPV6,

    /// TOR (v2 or v3)
    NET_ONION,

    /// I2P
    NET_I2P,

    /// CJDNS
    NET_CJDNS,

    /// A set of addresses that represent the hash of a string or FQDN. We use
    /// them in AddrMan to keep track of which DNS seeds were used.
    NET_INTERNAL,

    /// Dummy value to indicate the number of NET_* constants.
    NET_MAX,
};

/**
 * BIP155 network ids recognized by this software.
 */
enum class BIP155Network : uint8_t {
    IPV4 = 1,
    IPV6 = 2,
    TORV2 = 3,
    TORV3 = 4,
    I2P = 5,
    CJDNS = 6,
};

/**
 * Get the BIP155 network id of a network.
 * Must not be called for IsInternal() objects.
 * @returns BIP155 network id, except TORV2 which is no longer supported.
 */
BIP155Network GetBIP155Network(Network net);

enum Network ParseNetwork(const std::string& net_in);

std::string GetNetworkName(enum Network net);

/** Return a vector of publicly routable Network names; optionally append NET_UNROUTABLE. */
std::vector<std::string> GetNetworkNames(bool append_unroutable = false);

#endif // BITCOIN_NODE_NETWORK_H
