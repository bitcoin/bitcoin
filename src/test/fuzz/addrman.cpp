// Copyright (c) 2020-2021 The Bitcoin Core developers
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

void initialize_addrman()
{
    SelectParams(CBaseChainParams::REGTEST);
}

class CAddrManDeterministic : public CAddrMan
{
public:
    void MakeDeterministic(const uint256& random_seed)
    {
        insecure_rand = FastRandomContext{random_seed};
        Clear();
    }
};

FUZZ_TARGET_INIT(addrman, initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    CAddrManDeterministic addr_man;
    addr_man.MakeDeterministic(ConsumeUInt256(fuzzed_data_provider));
    if (fuzzed_data_provider.ConsumeBool()) {
        addr_man.m_asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
        if (!SanityCheckASMap(addr_man.m_asmap)) {
            addr_man.m_asmap.clear();
        }
    }
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                addr_man.Clear();
            },
            [&] {
                addr_man.ResolveCollisions();
            },
            [&] {
                (void)addr_man.SelectTriedCollision();
            },
            [&] {
                const std::optional<CAddress> opt_address = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
                const std::optional<CNetAddr> opt_net_addr = ConsumeDeserializable<CNetAddr>(fuzzed_data_provider);
                if (opt_address && opt_net_addr) {
                    addr_man.Add(*opt_address, *opt_net_addr, fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 100000000));
                }
            },
            [&] {
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
            },
            [&] {
                const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
                if (opt_service) {
                    addr_man.Good(*opt_service, ConsumeTime(fuzzed_data_provider));
                }
            },
            [&] {
                const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
                if (opt_service) {
                    addr_man.Attempt(*opt_service, fuzzed_data_provider.ConsumeBool(), ConsumeTime(fuzzed_data_provider));
                }
            },
            [&] {
                const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
                if (opt_service) {
                    addr_man.Connected(*opt_service, ConsumeTime(fuzzed_data_provider));
                }
            },
            [&] {
                const std::optional<CService> opt_service = ConsumeDeserializable<CService>(fuzzed_data_provider);
                if (opt_service) {
                    addr_man.SetServices(*opt_service, ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS));
                }
            });
    }
    const CAddrMan& const_addr_man{addr_man};
    (void)/*const_*/addr_man.GetAddr(
        /* max_addresses */ fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096),
        /* max_pct */ fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096),
        /* network */ std::nullopt);
    (void)/*const_*/addr_man.Select(fuzzed_data_provider.ConsumeBool());
    (void)const_addr_man.size();
    CDataStream data_stream(SER_NETWORK, PROTOCOL_VERSION);
    data_stream << const_addr_man;
}
