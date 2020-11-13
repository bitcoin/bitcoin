// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <addrman.h>
#include <chainparams.h>
#include <merkleblock.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <time.h>
#include <util/asmap.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void initialize()
{
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    SetMockTime(ConsumeTime(fuzzed_data_provider));
    CAddrMan addr_man;
    if (fuzzed_data_provider.ConsumeBool()) {
        addr_man.m_asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
        if (!SanityCheckASMap(addr_man.m_asmap)) {
            addr_man.m_asmap.clear();
        }
    }
    while (fuzzed_data_provider.ConsumeBool()) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 11)) {
        case 0: {
            addr_man.Clear();
            break;
        }
        case 1: {
            addr_man.ResolveCollisions();
            break;
        }
        case 2: {
            (void)addr_man.SelectTriedCollision();
            break;
        }
        case 3: {
            (void)addr_man.Select(fuzzed_data_provider.ConsumeBool());
            break;
        }
        case 4: {
            (void)addr_man.GetAddr(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
            break;
        }
        case 5: {
            const std::optional<CAddress> opt_address = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
            const std::optional<CNetAddr> opt_net_addr = ConsumeDeserializable<CNetAddr>(fuzzed_data_provider);
            if (opt_address && opt_net_addr) {
                addr_man.Add(*opt_address, *opt_net_addr, fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 100000000));
            }
            break;
        }
        case 6: {
            std::vector<CAddress> addresses;
            while (fuzzed_data_provider.ConsumeBool()) {
                const std::optional<CAddress> opt_address = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
                if (!opt_address) {
                    break;
                }
                addresses.push_back(*opt_address);
            }
            const std::optional<CNetAddr> opt_net_addr = ConsumeDeserializable<CNetAddr>(fuzzed_data_provider);
            if (opt_net_addr) {
                addr_man.Add(addresses, *opt_net_addr, fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 100000000));
            }
            break;
        }
        case 7: {
            const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
            if (opt_service) {
                addr_man.Good(*opt_service, fuzzed_data_provider.ConsumeBool(), ConsumeTime(fuzzed_data_provider));
            }
            break;
        }
        case 8: {
            const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
            if (opt_service) {
                addr_man.Attempt(*opt_service, fuzzed_data_provider.ConsumeBool(), ConsumeTime(fuzzed_data_provider));
            }
            break;
        }
        case 9: {
            const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
            if (opt_service) {
                addr_man.Connected(*opt_service, ConsumeTime(fuzzed_data_provider));
            }
            break;
        }
        case 10: {
            const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
            if (opt_service) {
                addr_man.SetServices(*opt_service, ServiceFlags{fuzzed_data_provider.ConsumeIntegral<uint64_t>()});
            }
            break;
        }
        case 11: {
            (void)addr_man.Check();
            break;
        }
        }
    }
    (void)addr_man.size();
    CDataStream data_stream(SER_NETWORK, PROTOCOL_VERSION);
    data_stream << addr_man;
}
