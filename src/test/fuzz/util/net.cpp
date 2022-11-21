// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/compat.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <util/strencodings.h>

#include <cstdint>
#include <vector>

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const Network network = fuzzed_data_provider.PickValueInArray({Network::NET_IPV4, Network::NET_IPV6, Network::NET_INTERNAL, Network::NET_ONION});
    CNetAddr net_addr;
    if (network == Network::NET_IPV4) {
        in_addr v4_addr = {};
        v4_addr.s_addr = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        net_addr = CNetAddr{v4_addr};
    } else if (network == Network::NET_IPV6) {
        if (fuzzed_data_provider.remaining_bytes() >= 16) {
            in6_addr v6_addr = {};
            memcpy(v6_addr.s6_addr, fuzzed_data_provider.ConsumeBytes<uint8_t>(16).data(), 16);
            net_addr = CNetAddr{v6_addr, fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        }
    } else if (network == Network::NET_INTERNAL) {
        net_addr.SetInternal(fuzzed_data_provider.ConsumeBytesAsString(32));
    } else if (network == Network::NET_ONION) {
        auto pub_key{fuzzed_data_provider.ConsumeBytes<uint8_t>(ADDR_TORV3_SIZE)};
        pub_key.resize(ADDR_TORV3_SIZE);
        const bool ok{net_addr.SetSpecial(OnionToString(pub_key))};
        assert(ok);
    }
    return net_addr;
}
