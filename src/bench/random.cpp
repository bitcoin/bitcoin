// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <random.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <numeric>

namespace {

template<typename RNG>
void BenchRandom_rand64(benchmark::Bench& bench, RNG&& rng) noexcept
{
    bench.batch(1).unit("number").run([&] {
        rng.rand64();
    });
}

template<typename RNG>
void BenchRandom_rand32(benchmark::Bench& bench, RNG&& rng) noexcept
{
    bench.batch(1).unit("number").run([&] {
        rng.rand32();
    });
}

template<typename RNG>
void BenchRandom_randbool(benchmark::Bench& bench, RNG&& rng) noexcept
{
    bench.batch(1).unit("number").run([&] {
        rng.randbool();
    });
}

template<typename RNG>
void BenchRandom_randbits(benchmark::Bench& bench, RNG&& rng) noexcept
{
    bench.batch(64).unit("number").run([&] {
        for (int i = 1; i <= 64; ++i) {
            rng.randbits(i);
        }
    });
}

template<int RANGE, typename RNG>
void BenchRandom_randrange(benchmark::Bench& bench, RNG&& rng) noexcept
{
    bench.batch(RANGE).unit("number").run([&] {
        for (int i = 1; i <= RANGE; ++i) {
            rng.randrange(i);
        }
    });
}

template<int RANGE, typename RNG>
void BenchRandom_stdshuffle(benchmark::Bench& bench, RNG&& rng) noexcept
{
    uint64_t data[RANGE];
    std::iota(std::begin(data), std::end(data), uint64_t(0));
    bench.batch(RANGE).unit("number").run([&] {
        std::shuffle(std::begin(data), std::end(data), rng);
    });
}

void FastRandom_rand64(benchmark::Bench& bench) { BenchRandom_rand64(bench, FastRandomContext(true)); }
void FastRandom_rand32(benchmark::Bench& bench) { BenchRandom_rand32(bench, FastRandomContext(true)); }
void FastRandom_randbool(benchmark::Bench& bench) { BenchRandom_randbool(bench, FastRandomContext(true)); }
void FastRandom_randbits(benchmark::Bench& bench) { BenchRandom_randbits(bench, FastRandomContext(true)); }
void FastRandom_randrange100(benchmark::Bench& bench) { BenchRandom_randrange<100>(bench, FastRandomContext(true)); }
void FastRandom_randrange1000(benchmark::Bench& bench) { BenchRandom_randrange<1000>(bench, FastRandomContext(true)); }
void FastRandom_randrange1000000(benchmark::Bench& bench) { BenchRandom_randrange<1000000>(bench, FastRandomContext(true)); }
void FastRandom_stdshuffle100(benchmark::Bench& bench) { BenchRandom_stdshuffle<100>(bench, FastRandomContext(true)); }

void InsecureRandom_rand64(benchmark::Bench& bench) { BenchRandom_rand64(bench, InsecureRandomContext(251438)); }
void InsecureRandom_rand32(benchmark::Bench& bench) { BenchRandom_rand32(bench, InsecureRandomContext(251438)); }
void InsecureRandom_randbool(benchmark::Bench& bench) { BenchRandom_randbool(bench, InsecureRandomContext(251438)); }
void InsecureRandom_randbits(benchmark::Bench& bench) { BenchRandom_randbits(bench, InsecureRandomContext(251438)); }
void InsecureRandom_randrange100(benchmark::Bench& bench) { BenchRandom_randrange<100>(bench, InsecureRandomContext(251438)); }
void InsecureRandom_randrange1000(benchmark::Bench& bench) { BenchRandom_randrange<1000>(bench, InsecureRandomContext(251438)); }
void InsecureRandom_randrange1000000(benchmark::Bench& bench) { BenchRandom_randrange<1000000>(bench, InsecureRandomContext(251438)); }
void InsecureRandom_stdshuffle100(benchmark::Bench& bench) { BenchRandom_stdshuffle<100>(bench, InsecureRandomContext(251438)); }

} // namespace

BENCHMARK(FastRandom_rand64, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_rand32, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_randbool, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_randbits, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_randrange100, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_randrange1000, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_randrange1000000, benchmark::PriorityLevel::HIGH);
BENCHMARK(FastRandom_stdshuffle100, benchmark::PriorityLevel::HIGH);

BENCHMARK(InsecureRandom_rand64, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_rand32, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_randbool, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_randbits, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_randrange100, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_randrange1000, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_randrange1000000, benchmark::PriorityLevel::HIGH);
BENCHMARK(InsecureRandom_stdshuffle100, benchmark::PriorityLevel::HIGH);
