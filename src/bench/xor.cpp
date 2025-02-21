// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>

#include <cmath>
#include <cstddef>
#include <map>
#include <vector>

static void Xor(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    auto test_data{rng.randbytes<std::byte>(1 << 20)};

    const Obfuscation obfuscation{rng.rand64()};
    assert(obfuscation);

    size_t offset{0};
    bench.batch(test_data.size()).unit("byte").run([&] {
        obfuscation(test_data, offset++);
        ankerl::nanobench::doNotOptimizeAway(test_data);
    });
}

BENCHMARK(Xor, benchmark::PriorityLevel::HIGH);
