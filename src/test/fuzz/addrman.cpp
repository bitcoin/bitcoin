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

int32_t GetCheckRatio()
{
    return std::clamp<int32_t>(g_setup->m_node.args->GetIntArg("-checkaddrman", 0), 0, 1000000);
}
} // namespace

void initialize_addrman()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::REGTEST);
    g_setup = testing_setup.get();
}

[[nodiscard]] inline NetGroupManager ConsumeNetGroupManager(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
    if (!SanityCheckASMap(asmap, 128)) asmap.clear();
    return NetGroupManager(asmap);
}

FUZZ_TARGET(data_stream_addr_man, .init = initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    DataStream data_stream = ConsumeDataStream(fuzzed_data_provider);
    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    AddrMan addr_man(netgroupman, /*deterministic=*/false, GetCheckRatio());
    try {
        ReadFromStream(addr_man, data_stream);
    } catch (const std::exception&) {
    }
}

/**
 * Generate a random address. Always returns a valid address.
 */
CNetAddr RandAddr(FuzzedDataProvider& fuzzed_data_provider, FastRandomContext& fast_random_context)
{
    CNetAddr addr;
    assert(!addr.IsValid());
    for (size_t i = 0; i < 8 && !addr.IsValid(); ++i) {
        if (fuzzed_data_provider.remaining_bytes() > 1 && fuzzed_data_provider.ConsumeBool()) {
            addr = ConsumeNetAddr(fuzzed_data_provider);
        } else {
            addr = ConsumeNetAddr(fuzzed_data_provider, &fast_random_context);
        }
    }

    // Return a dummy IPv4 5.5.5.5 if we generated an invalid address.
    if (!addr.IsValid()) {
        in_addr v4_addr = {};
        v4_addr.s_addr = 0x05050505;
        addr = CNetAddr{v4_addr};
    }

    return addr;
}

/** Fill addrman with lots of addresses from lots of sources.  */
void FillAddrman(AddrMan& addrman, FuzzedDataProvider& fuzzed_data_provider)
{
    // Add a fraction of the addresses to the "tried" table.
    // 0, 1, 2, 3 corresponding to 0%, 100%, 50%, 33%
    const size_t n = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 3);

    const size_t num_sources = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
    CNetAddr prev_source;
    // Generate a FastRandomContext seed to use inside the loops instead of
    // fuzzed_data_provider. When fuzzed_data_provider is exhausted it
    // just returns 0.
    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    for (size_t i = 0; i < num_sources; ++i) {
        const auto source = RandAddr(fuzzed_data_provider, fast_random_context);
        const size_t num_addresses = fast_random_context.randrange(500) + 1; // [1..500]

        for (size_t j = 0; j < num_addresses; ++j) {
            const auto addr = CAddress{CService{RandAddr(fuzzed_data_provider, fast_random_context), 8333}, NODE_NETWORK};
            const std::chrono::seconds time_penalty{fast_random_context.randrange(100000001)};
            addrman.Add({addr}, source, time_penalty);

            if (n > 0 && addrman.Size() % n == 0) {
                addrman.Good(addr, Now<NodeSeconds>());
            }

            // Add 10% of the addresses from more than one source.
            if (fast_random_context.randrange(10) == 0 && prev_source.IsValid()) {
                addrman.Add({addr}, prev_source, time_penalty);
            }
        }
        prev_source = source;
    }
}

class AddrManDeterministic : public AddrMan
{
public:
    explicit AddrManDeterministic(const NetGroupManager& netgroupman, FuzzedDataProvider& fuzzed_data_provider)
        : AddrMan(netgroupman, /*deterministic=*/true, GetCheckRatio())
    {
        WITH_LOCK(m_impl->cs, m_impl->insecure_rand = FastRandomContext{ConsumeUInt256(fuzzed_data_provider)});
    }

    /**
     * Compare with another AddrMan.
     * This compares:
     * - the values in `mapInfo` (the keys aka ids are ignored)
     * - vvNew entries refer to the same addresses
     * - vvTried entries refer to the same addresses
     */
    bool operator==(const AddrManDeterministic& other) const
    {
        LOCK2(m_impl->cs, other.m_impl->cs);

        if (m_impl->mapInfo.size() != other.m_impl->mapInfo.size() || m_impl->nNew != other.m_impl->nNew ||
            m_impl->nTried != other.m_impl->nTried) {
            return false;
        }

        // Check that all values in `mapInfo` are equal to all values in `other.mapInfo`.
        // Keys may be different.

        auto addrinfo_hasher = [](const AddrInfo& a) {
            CSipHasher hasher(0, 0);
            auto addr_key = a.GetKey();
            auto source_key = a.source.GetAddrBytes();
            hasher.Write(TicksSinceEpoch<std::chrono::seconds>(a.m_last_success));
            hasher.Write(a.nAttempts);
            hasher.Write(a.nRefCount);
            hasher.Write(a.fInTried);
            hasher.Write(a.GetNetwork());
            hasher.Write(a.source.GetNetwork());
            hasher.Write(addr_key.size());
            hasher.Write(source_key.size());
            hasher.Write(addr_key);
            hasher.Write(source_key);
            return (size_t)hasher.Finalize();
        };

        auto addrinfo_eq = [](const AddrInfo& lhs, const AddrInfo& rhs) {
            return std::tie(static_cast<const CService&>(lhs), lhs.source, lhs.m_last_success, lhs.nAttempts, lhs.nRefCount, lhs.fInTried) ==
                   std::tie(static_cast<const CService&>(rhs), rhs.source, rhs.m_last_success, rhs.nAttempts, rhs.nRefCount, rhs.fInTried);
        };

        using Addresses = std::unordered_set<AddrInfo, decltype(addrinfo_hasher), decltype(addrinfo_eq)>;

        const size_t num_addresses{m_impl->mapInfo.size()};

        Addresses addresses{num_addresses, addrinfo_hasher, addrinfo_eq};
        for (const auto& [id, addr] : m_impl->mapInfo) {
            addresses.insert(addr);
        }

        Addresses other_addresses{num_addresses, addrinfo_hasher, addrinfo_eq};
        for (const auto& [id, addr] : other.m_impl->mapInfo) {
            other_addresses.insert(addr);
        }

        if (addresses != other_addresses) {
            return false;
        }

        auto IdsReferToSameAddress = [&](int id, int other_id) EXCLUSIVE_LOCKS_REQUIRED(m_impl->cs, other.m_impl->cs) {
            if (id == -1 && other_id == -1) {
                return true;
            }
            if ((id == -1 && other_id != -1) || (id != -1 && other_id == -1)) {
                return false;
            }
            return m_impl->mapInfo.at(id) == other.m_impl->mapInfo.at(other_id);
        };

        // Check that `vvNew` contains the same addresses as `other.vvNew`. Notice - `vvNew[i][j]`
        // contains just an id and the address is to be found in `mapInfo.at(id)`. The ids
        // themselves may differ between `vvNew` and `other.vvNew`.
        for (size_t i = 0; i < ADDRMAN_NEW_BUCKET_COUNT; ++i) {
            for (size_t j = 0; j < ADDRMAN_BUCKET_SIZE; ++j) {
                if (!IdsReferToSameAddress(m_impl->vvNew[i][j], other.m_impl->vvNew[i][j])) {
                    return false;
                }
            }
        }

        // Same for `vvTried`.
        for (size_t i = 0; i < ADDRMAN_TRIED_BUCKET_COUNT; ++i) {
            for (size_t j = 0; j < ADDRMAN_BUCKET_SIZE; ++j) {
                if (!IdsReferToSameAddress(m_impl->vvTried[i][j], other.m_impl->vvTried[i][j])) {
                    return false;
                }
            }
        }

        return true;
    }
};

FUZZ_TARGET(addrman, .init = initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    auto addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider);
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> serialized_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        DataStream ds{serialized_data};
        try {
            ds >> *addr_man_ptr;
        } catch (const std::ios_base::failure&) {
            addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider);
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
                auto net_addr = ConsumeNetAddr(fuzzed_data_provider);
                auto time_penalty = std::chrono::seconds{ConsumeTime(fuzzed_data_provider, 0, 100000000)};
                addr_man.Add(addresses, net_addr, time_penalty);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto time = NodeSeconds{std::chrono::seconds{ConsumeTime(fuzzed_data_provider)}};
                addr_man.Good(addr, time);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto count_failure = fuzzed_data_provider.ConsumeBool();
                auto time = NodeSeconds{std::chrono::seconds{ConsumeTime(fuzzed_data_provider)}};
                addr_man.Attempt(addr, count_failure, time);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto time = NodeSeconds{std::chrono::seconds{ConsumeTime(fuzzed_data_provider)}};
                addr_man.Connected(addr, time);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto n_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
                addr_man.SetServices(addr, n_services);
            });
    }
    const AddrMan& const_addr_man{addr_man};
    std::optional<Network> network;
    if (fuzzed_data_provider.ConsumeBool()) {
        network = fuzzed_data_provider.PickValueInArray(ALL_NETWORKS);
    }
    auto max_addresses = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    auto filtered = fuzzed_data_provider.ConsumeBool();
    (void)const_addr_man.GetAddr(max_addresses, max_pct, network, filtered);
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
    AddrManDeterministic addr_man1{netgroupman, fuzzed_data_provider};
    AddrManDeterministic addr_man2{netgroupman, fuzzed_data_provider};

    DataStream data_stream{};

    FillAddrman(addr_man1, fuzzed_data_provider);
    data_stream << addr_man1;
    data_stream >> addr_man2;
    assert(addr_man1 == addr_man2);
}
