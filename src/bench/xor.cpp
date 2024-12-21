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

    const Obfuscation obfuscation{rng.rand64()};
    assert(obfuscation);

    size_t offset{0};
    bench.batch(bytes / 1_MiB).unit("MiB").run([&] {
        obfuscation(test_data, offset++);
        ankerl::nanobench::doNotOptimizeAway(test_data);
    });
}

BENCHMARK(XorObfuscationBench, benchmark::PriorityLevel::HIGH);
