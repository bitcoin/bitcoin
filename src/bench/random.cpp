// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <random.h>

static void FastRandom_32bit(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    bench.run([&] {
        rng.rand32();
    });
}

static void FastRandom_1bit(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    bench.run([&] {
        rng.randbool();
    });
}

BENCHMARK(FastRandom_32bit, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_1bit, benchmark::PriorityLevel::HIGH);
