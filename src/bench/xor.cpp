// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>
#include <util/byte_units.h>

#include <cmath>
#include <cstddef>
#include <map>
#include <vector>

static void XorObfuscationBench(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    constexpr size_t bytes{10_MiB};
    auto test_data{rng.randbytes<std::byte>(bytes)};

    std::vector key_bytes{rng.randbytes<std::byte>(8)};
    uint64_t key;
    std::memcpy(&key, key_bytes.data(), 8);

    size_t offset{0};
    bench.batch(bytes / 1_MiB).unit("MiB").run([&] {
        util::Xor(test_data, key_bytes, offset++);
        ankerl::nanobench::doNotOptimizeAway(test_data);
    });
}

BENCHMARK(XorObfuscationBench, benchmark::PriorityLevel::HIGH);
