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

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void initialize_addrman()
{
    SelectParams(CBaseChainParams::REGTEST);
}

FUZZ_TARGET_INIT(data_stream_addr_man, initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CDataStream data_stream = ConsumeDataStream(fuzzed_data_provider);
    CAddrMan addr_man(/* asmap */ std::vector<bool>(), /* deterministic */ false, /* consistency_check_ratio */ 0);
    try {
        ReadFromStream(addr_man, data_stream);
    } catch (const std::exception&) {
    }
}

class CAddrManDeterministic : public CAddrMan
{
public:
    FuzzedDataProvider& m_fuzzed_data_provider;

    explicit CAddrManDeterministic(std::vector<bool> asmap, FuzzedDataProvider& fuzzed_data_provider)
        : CAddrMan(std::move(asmap), /* deterministic */ true, /* consistency_check_ratio */ 0)
        , m_fuzzed_data_provider(fuzzed_data_provider)
    {
        WITH_LOCK(cs, insecure_rand = FastRandomContext{ConsumeUInt256(fuzzed_data_provider)});
    }

    /**
     * Generate a random address. Always returns a valid address.
     */
    CNetAddr RandAddr() EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        CNetAddr addr;
        if (m_fuzzed_data_provider.remaining_bytes() > 1 && m_fuzzed_data_provider.ConsumeBool()) {
            addr = ConsumeNetAddr(m_fuzzed_data_provider);
        } else {
            // The networks [1..6] correspond to CNetAddr::BIP155Network (private).
            static const std::map<uint8_t, uint8_t> net_len_map = {{1, ADDR_IPV4_SIZE},
                                                                   {2, ADDR_IPV6_SIZE},
                                                                   {4, ADDR_TORV3_SIZE},
                                                                   {5, ADDR_I2P_SIZE},
                                                                   {6, ADDR_CJDNS_SIZE}};
            uint8_t net = insecure_rand.randrange(5) + 1; // [1..5]
            if (net == 3) {
                net = 6;
            }

            CDataStream s(SER_NETWORK, PROTOCOL_VERSION | ADDRV2_FORMAT);

            s << net;
            s << insecure_rand.randbytes(net_len_map.at(net));

            s >> addr;
        }

        // Return a dummy IPv4 5.5.5.5 if we generated an invalid address.
        if (!addr.IsValid()) {
            in_addr v4_addr = {};
            v4_addr.s_addr = 0x05050505;
            addr = CNetAddr{v4_addr};
        }

        return addr;
    }

    /**
     * Fill this addrman with lots of addresses from lots of sources.
     */
    void Fill()
    {
        LOCK(cs);

        // Add some of the addresses directly to the "tried" table.

        // 0, 1, 2, 3 corresponding to 0%, 100%, 50%, 33%
        const size_t n = m_fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 3);

        const size_t num_sources = m_fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
        CNetAddr prev_source;
        // Use insecure_rand inside the loops instead of m_fuzzed_data_provider because when
        // the latter is exhausted it just returns 0.
        for (size_t i = 0; i < num_sources; ++i) {
            const auto source = RandAddr();
            const size_t num_addresses = insecure_rand.randrange(500) + 1; // [1..500]

            for (size_t j = 0; j < num_addresses; ++j) {
                const auto addr = CAddress{CService{RandAddr(), 8333}, NODE_NETWORK};
                const auto time_penalty = insecure_rand.randrange(100000001);
                Add_(addr, source, time_penalty);

                if (n > 0 && mapInfo.size() % n == 0) {
                    Good_(addr, false, GetTime());
                }

                // Add 10% of the addresses from more than one source.
                if (insecure_rand.randrange(10) == 0 && prev_source.IsValid()) {
                    Add_(addr, prev_source, time_penalty);
                }
            }
            prev_source = source;
        }
    }

    /**
     * Compare with another AddrMan.
     * This compares:
     * - the values in `mapInfo` (the keys aka ids are ignored)
     * - vvNew entries refer to the same addresses
     * - vvTried entries refer to the same addresses
     */
    bool operator==(const CAddrManDeterministic& other)
    {
        LOCK2(cs, other.cs);

        if (mapInfo.size() != other.mapInfo.size() || nNew != other.nNew ||
            nTried != other.nTried) {
            return false;
        }

        // Check that all values in `mapInfo` are equal to all values in `other.mapInfo`.
        // Keys may be different.

        using CAddrInfoHasher = std::function<size_t(const CAddrInfo&)>;
        using CAddrInfoEq = std::function<bool(const CAddrInfo&, const CAddrInfo&)>;

        CNetAddrHash netaddr_hasher;

        CAddrInfoHasher addrinfo_hasher = [&netaddr_hasher](const CAddrInfo& a) {
            return netaddr_hasher(static_cast<CNetAddr>(a)) ^ netaddr_hasher(a.source) ^
                   a.nLastSuccess ^ a.nAttempts ^ a.nRefCount ^ a.fInTried;
        };

        CAddrInfoEq addrinfo_eq = [](const CAddrInfo& lhs, const CAddrInfo& rhs) {
            return static_cast<CNetAddr>(lhs) == static_cast<CNetAddr>(rhs) &&
                   lhs.source == rhs.source && lhs.nLastSuccess == rhs.nLastSuccess &&
                   lhs.nAttempts == rhs.nAttempts && lhs.nRefCount == rhs.nRefCount &&
                   lhs.fInTried == rhs.fInTried;
        };

        using Addresses = std::unordered_set<CAddrInfo, CAddrInfoHasher, CAddrInfoEq>;

        const size_t num_addresses{mapInfo.size()};

        Addresses addresses{num_addresses, addrinfo_hasher, addrinfo_eq};
        for (const auto& [id, addr] : mapInfo) {
            addresses.insert(addr);
        }

        Addresses other_addresses{num_addresses, addrinfo_hasher, addrinfo_eq};
        for (const auto& [id, addr] : other.mapInfo) {
            other_addresses.insert(addr);
        }

        if (addresses != other_addresses) {
            return false;
        }

        auto IdsReferToSameAddress = [&](int id, int other_id) EXCLUSIVE_LOCKS_REQUIRED(cs, other.cs) {
            if (id == -1 && other_id == -1) {
                return true;
            }
            if ((id == -1 && other_id != -1) || (id != -1 && other_id == -1)) {
                return false;
            }
            return mapInfo.at(id) == other.mapInfo.at(other_id);
        };

        // Check that `vvNew` contains the same addresses as `other.vvNew`. Notice - `vvNew[i][j]`
        // contains just an id and the address is to be found in `mapInfo.at(id)`. The ids
        // themselves may differ between `vvNew` and `other.vvNew`.
        for (size_t i = 0; i < ADDRMAN_NEW_BUCKET_COUNT; ++i) {
            for (size_t j = 0; j < ADDRMAN_BUCKET_SIZE; ++j) {
                if (!IdsReferToSameAddress(vvNew[i][j], other.vvNew[i][j])) {
                    return false;
                }
            }
        }

        // Same for `vvTried`.
        for (size_t i = 0; i < ADDRMAN_TRIED_BUCKET_COUNT; ++i) {
            for (size_t j = 0; j < ADDRMAN_BUCKET_SIZE; ++j) {
                if (!IdsReferToSameAddress(vvTried[i][j], other.vvTried[i][j])) {
                    return false;
                }
            }
        }

        return true;
    }
};

[[nodiscard]] inline std::vector<bool> ConsumeAsmap(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
    if (!SanityCheckASMap(asmap, 128)) asmap.clear();
    return asmap;
}

FUZZ_TARGET_INIT(addrman, initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    std::vector<bool> asmap = ConsumeAsmap(fuzzed_data_provider);
    auto addr_man_ptr = std::make_unique<CAddrManDeterministic>(asmap, fuzzed_data_provider);
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> serialized_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        CDataStream ds(serialized_data, SER_DISK, INIT_PROTO_VERSION);
        const auto ser_version{fuzzed_data_provider.ConsumeIntegral<int32_t>()};
        ds.SetVersion(ser_version);
        try {
            ds >> *addr_man_ptr;
        } catch (const std::ios_base::failure&) {
            addr_man_ptr = std::make_unique<CAddrManDeterministic>(asmap, fuzzed_data_provider);
        }
    }
    CAddrManDeterministic& addr_man = *addr_man_ptr;
    while (fuzzed_data_provider.ConsumeBool()) {
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
    (void)const_addr_man.GetAddr(
        /* max_addresses */ fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096),
        /* max_pct */ fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096),
        /* network */ std::nullopt);
    (void)const_addr_man.Select(fuzzed_data_provider.ConsumeBool());
    (void)const_addr_man.size();
    CDataStream data_stream(SER_NETWORK, PROTOCOL_VERSION);
    data_stream << const_addr_man;
}

// Check that serialize followed by unserialize produces the same addrman.
FUZZ_TARGET_INIT(addrman_serdeser, initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    std::vector<bool> asmap = ConsumeAsmap(fuzzed_data_provider);
    CAddrManDeterministic addr_man1{asmap, fuzzed_data_provider};
    CAddrManDeterministic addr_man2{asmap, fuzzed_data_provider};

    CDataStream data_stream(SER_NETWORK, PROTOCOL_VERSION);

    addr_man1.Fill();
    data_stream << addr_man1;
    data_stream >> addr_man2;
    assert(addr_man1 == addr_man2);
}
