// Copyright (c) 2020-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <bench/bench.h>
#include <random.h>
#include <util/check.h>
#include <util/time.h>

#include <optional>
#include <vector>

/* A "source" is a source address from which we have received a bunch of other addresses. */

static constexpr size_t NUM_SOURCES = 64;
static constexpr size_t NUM_ADDRESSES_PER_SOURCE = 256;

static std::vector<CAddress> g_sources;
static std::vector<std::vector<CAddress>> g_addresses;

static void CreateAddresses()
{
    if (g_sources.size() > 0) { // already created
        return;
    }

    FastRandomContext rng(uint256(std::vector<unsigned char>(32, 123)));

    auto randAddr = [&rng]() {
        in6_addr addr;
        memcpy(&addr, rng.randbytes(sizeof(addr)).data(), sizeof(addr));

        uint16_t port;
        memcpy(&port, rng.randbytes(sizeof(port)).data(), sizeof(port));
        if (port == 0) {
            port = 1;
        }

        CAddress ret(CService(addr, port), NODE_NETWORK);

        ret.nTime = GetAdjustedTime();

        return ret;
    };

    for (size_t source_i = 0; source_i < NUM_SOURCES; ++source_i) {
        g_sources.emplace_back(randAddr());
        g_addresses.emplace_back();
        for (size_t addr_i = 0; addr_i < NUM_ADDRESSES_PER_SOURCE; ++addr_i) {
            g_addresses[source_i].emplace_back(randAddr());
        }
    }
}

static void AddAddressesToAddrMan(CAddrMan& addrman)
{
    for (size_t source_i = 0; source_i < NUM_SOURCES; ++source_i) {
        addrman.Add(g_addresses[source_i], g_sources[source_i]);
    }
}

static void FillAddrMan(CAddrMan& addrman)
{
    CreateAddresses();

    AddAddressesToAddrMan(addrman);
}

/* Benchmarks */

static void AddrManAdd(benchmark::Bench& bench)
{
    CreateAddresses();

    bench.run([&] {
        CAddrMan addrman{/* asmap */ std::vector<bool>(), /* deterministic */ false, /* consistency_check_ratio */ 0};
        AddAddressesToAddrMan(addrman);
    });
}

static void AddrManSelect(benchmark::Bench& bench)
{
    CAddrMan addrman(/* asmap */ std::vector<bool>(), /* deterministic */ false, /* consistency_check_ratio */ 0);

    FillAddrMan(addrman);

    bench.run([&] {
        const auto& address = addrman.Select();
        assert(address.GetPort() > 0);
    });
}

static void AddrManGetAddr(benchmark::Bench& bench)
{
    CAddrMan addrman(/* asmap */ std::vector<bool>(), /* deterministic */ false, /* consistency_check_ratio */ 0);

    FillAddrMan(addrman);

    bench.run([&] {
        const auto& addresses = addrman.GetAddr(/* max_addresses */ 2500, /* max_pct */ 23, /* network */ std::nullopt);
        assert(addresses.size() > 0);
    });
}

static void AddrManAddThenGood(benchmark::Bench& bench)
{
    auto markSomeAsGood = [](CAddrMan& addrman) {
        for (size_t source_i = 0; source_i < NUM_SOURCES; ++source_i) {
            for (size_t addr_i = 0; addr_i < NUM_ADDRESSES_PER_SOURCE; ++addr_i) {
                addrman.Good(g_addresses[source_i][addr_i]);
            }
        }
    };

    CreateAddresses();

    bench.run([&] {
        // To make the benchmark independent of the number of evaluations, we always prepare a new addrman.
        // This is necessary because CAddrMan::Good() method modifies the object, affecting the timing of subsequent calls
        // to the same method and we want to do the same amount of work in every loop iteration.
        //
        // This has some overhead (exactly the result of AddrManAdd benchmark), but that overhead is constant so improvements in
        // CAddrMan::Good() will still be noticeable.
        CAddrMan addrman(/* asmap */ std::vector<bool>(), /* deterministic */ false, /* consistency_check_ratio */ 0);
        AddAddressesToAddrMan(addrman);

        markSomeAsGood(addrman);
    });
}

BENCHMARK(AddrManAdd);
BENCHMARK(AddrManSelect);
BENCHMARK(AddrManGetAddr);
BENCHMARK(AddrManAddThenGood);
