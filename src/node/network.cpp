// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/network.h>

#include <logging.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

enum Network ParseNetwork(const std::string& net_in)
{
    const std::string net{ToLower(net_in)};
    if (net == "ipv4") return NET_IPV4;
    if (net == "ipv6") return NET_IPV6;
    if (net == "onion") return NET_ONION;
    if (net == "tor") {
        LogPrintf("Warning: net name 'tor' is deprecated and will be removed in the future. You should use 'onion' instead.\n");
        return NET_ONION;
    }
    if (net == "i2p") return NET_I2P;
    if (net == "cjdns") return NET_CJDNS;
    return NET_UNROUTABLE;
}

std::string GetNetworkName(enum Network net)
{
    switch (net) {
    case NET_UNROUTABLE: return "not_publicly_routable";
    case NET_IPV4: return "ipv4";
    case NET_IPV6: return "ipv6";
    case NET_ONION: return "onion";
    case NET_I2P: return "i2p";
    case NET_CJDNS: return "cjdns";
    case NET_INTERNAL: return "internal";
    case NET_MAX: assert(false);
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

std::vector<std::string> GetNetworkNames(bool append_unroutable)
{
    std::vector<std::string> names;
    for (int n = 0; n < NET_MAX; ++n) {
        const enum Network network{static_cast<Network>(n)};
        if (network == NET_UNROUTABLE || network == NET_INTERNAL) continue;
        names.emplace_back(GetNetworkName(network));
    }
    if (append_unroutable) {
        names.emplace_back(GetNetworkName(NET_UNROUTABLE));
    }
    return names;
}
