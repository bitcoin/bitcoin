// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <obfuscation.h>
#include <random.h>
#include <span.h>

#include <cstddef>
#include <vector>

static void ObfuscationBench(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    auto data{frc.randbytes<std::byte>(1024)};

    std::array<std::byte, Obfuscation::SIZE_BYTES> key_bytes{};
    frc.fillrand(key_bytes);
    const Obfuscation obfuscation{key_bytes};

    size_t offset{0};
    bench.batch(data.size()).unit("byte").run([&] {
        obfuscation(data, offset++); // mutated differently each time
        ankerl::nanobench::doNotOptimizeAway(data);
    });
}

BENCHMARK(ObfuscationBench, benchmark::PriorityLevel::HIGH);
