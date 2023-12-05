// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <addrman.h>
#include <addrman_impl.h>
#include <chainparams.h>
#include <common/args.h>
#include <merkleblock.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/addrman.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <time.h>
#include <util/asmap.h>
#include <util/chaintype.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

void initialize_addrman()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::REGTEST);
    g_setup = testing_setup.get();
}

FUZZ_TARGET(data_stream_addr_man, .init = initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    DataStream data_stream = ConsumeDataStream(fuzzed_data_provider);
    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    AddrMan addr_man(netgroupman, /*deterministic=*/false, GetCheckRatio(g_setup->m_args));
    try {
        ReadFromStream(addr_man, data_stream);
    } catch (const std::exception&) {
    }
}

FUZZ_TARGET(addrman, .init = initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    auto addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio(g_setup->m_args));
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> serialized_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        DataStream ds{serialized_data};
        try {
            ds >> *addr_man_ptr;
        } catch (const std::ios_base::failure&) {
            addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio(g_setup->m_args));
        }
    }
    AddrManDeterministic& addr_man = *addr_man_ptr;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                addr_man.ResolveCollisions();
            },
            [&] {
                (void)addr_man.SelectTriedCollision();
            },
            [&] {
                std::vector<CAddress> addresses;
                LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
                    addresses.push_back(ConsumeAddress(fuzzed_data_provider));
                }
                addr_man.Add(addresses, ConsumeNetAddr(fuzzed_data_provider), std::chrono::seconds{ConsumeTime(fuzzed_data_provider, 0, 100000000)});
            },
            [&] {
                addr_man.Good(ConsumeService(fuzzed_data_provider), NodeSeconds{std::chrono::seconds{ConsumeTime(fuzzed_data_provider)}});
            },
            [&] {
                addr_man.Attempt(ConsumeService(fuzzed_data_provider), fuzzed_data_provider.ConsumeBool(), NodeSeconds{std::chrono::seconds{ConsumeTime(fuzzed_data_provider)}});
            },
            [&] {
                addr_man.Connected(ConsumeService(fuzzed_data_provider), NodeSeconds{std::chrono::seconds{ConsumeTime(fuzzed_data_provider)}});
            },
            [&] {
                addr_man.SetServices(ConsumeService(fuzzed_data_provider), ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS));
            });
    }
    const AddrMan& const_addr_man{addr_man};
    std::optional<Network> network;
    if (fuzzed_data_provider.ConsumeBool()) {
        network = fuzzed_data_provider.PickValueInArray(ALL_NETWORKS);
    }
    (void)const_addr_man.GetAddr(
        /*max_addresses=*/fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096),
        /*max_pct=*/fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096),
        network);
    (void)const_addr_man.Select(fuzzed_data_provider.ConsumeBool(), network);
    std::optional<bool> in_new;
    if (fuzzed_data_provider.ConsumeBool()) {
        in_new = fuzzed_data_provider.ConsumeBool();
    }
    (void)const_addr_man.Size(network, in_new);
    DataStream data_stream{};
    data_stream << const_addr_man;
}

// Check that serialize followed by unserialize produces the same addrman.
FUZZ_TARGET(addrman_serdeser, .init = initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    AddrManDeterministic addr_man1{netgroupman, fuzzed_data_provider, GetCheckRatio(g_setup->m_args)};
    AddrManDeterministic addr_man2{netgroupman, fuzzed_data_provider, GetCheckRatio(g_setup->m_args)};

    DataStream data_stream{};

    FillAddrman(addr_man1, fuzzed_data_provider);
    data_stream << addr_man1;
    data_stream >> addr_man2;
    assert(addr_man1 == addr_man2);
}
