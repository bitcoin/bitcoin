// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <util/obfuscation.h>

#include <cstddef>
#include <vector>

static void ObfuscationBench(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    auto data{frc.randbytes<std::byte>(1024)};
    const auto key{frc.randbytes<Obfuscation::KEY_SIZE>()};

    size_t offset{0};
    bench.batch(data.size()).unit("byte").run([&] {
        Obfuscation().Xor(data, key, offset++); // mutated differently each time
        ankerl::nanobench::doNotOptimizeAway(data);
    });
}

BENCHMARK(ObfuscationBench, benchmark::PriorityLevel::HIGH);
