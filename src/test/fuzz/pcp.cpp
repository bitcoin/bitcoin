// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>

#include <common/pcp.h>
#include <util/check.h>
#include <util/threadinterrupt.h>

using namespace std::literals;

//! Fixed nonce to use in PCP port mapping requests.
constexpr PCPMappingNonce PCP_NONCE{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};

//! Number of attempts to request a NAT-PMP or PCP port mapping to the gateway.
constexpr int NUM_TRIES{5};

//! Timeout for each attempt to request a port mapping.
constexpr std::chrono::duration TIMEOUT{100ms};

void port_map_target_init()
{
    LogInstance().DisableLogging();
}

FUZZ_TARGET(pcp_request_port_map, .init = port_map_target_init)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    // Create a mocked socket between random (and potentially invalid) client and gateway addresses.
    CreateSock = [&](int domain, int type, int protocol) {
        if ((domain == AF_INET || domain == AF_INET6) && type == SOCK_DGRAM && protocol == IPPROTO_UDP) {
            return std::make_unique<FuzzedSock>(fuzzed_data_provider);
        }
        return std::unique_ptr<FuzzedSock>();
    };

    // Perform the port mapping request. The mocked socket will return fuzzer-provided data.
    const auto gateway_addr{ConsumeNetAddr(fuzzed_data_provider)};
    const auto local_addr{ConsumeNetAddr(fuzzed_data_provider)};
    const auto port{fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    const auto lifetime{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
    CThreadInterrupt interrupt;
    const auto res{PCPRequestPortMap(PCP_NONCE, gateway_addr, local_addr, port, lifetime, interrupt, NUM_TRIES, TIMEOUT)};

    // In case of success the mapping must be consistent with the request.
    if (const MappingResult* mapping = std::get_if<MappingResult>(&res)) {
        Assert(mapping);
        Assert(mapping->internal.GetPort() == port);
        mapping->ToString();
    }
}

FUZZ_TARGET(natpmp_request_port_map, .init = port_map_target_init)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    // Create a mocked socket between random (and potentially invalid) client and gateway addresses.
    CreateSock = [&](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) {
            return std::make_unique<FuzzedSock>(fuzzed_data_provider);
        }
        return std::unique_ptr<FuzzedSock>();
    };

    // Perform the port mapping request. The mocked socket will return fuzzer-provided data.
    const auto gateway_addr{ConsumeNetAddr(fuzzed_data_provider)};
    const auto port{fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    const auto lifetime{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
    CThreadInterrupt interrupt;
    const auto res{NATPMPRequestPortMap(gateway_addr, port, lifetime, interrupt, NUM_TRIES, TIMEOUT)};

    // In case of success the mapping must be consistent with the request.
    if (const MappingResult* mapping = std::get_if<MappingResult>(&res)) {
        Assert(mapping);
        Assert(mapping->internal.GetPort() == port);
        mapping->ToString();
    }
}
