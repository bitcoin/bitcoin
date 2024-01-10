// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <util/time.h>

static void BenchTimeDeprecated(benchmark::Bench& bench)
{
    bench.run([&] {
        (void)GetTime();
    });
}

static void BenchTimeMock(benchmark::Bench& bench)
{
    SetMockTime(111);
    bench.run([&] {
        (void)GetTime<std::chrono::seconds>();
    });
    SetMockTime(0);
}

static void BenchTimeMillis(benchmark::Bench& bench)
{
    bench.run([&] {
        (void)GetTime<std::chrono::milliseconds>();
    });
}

static void BenchTimeMillisSys(benchmark::Bench& bench)
{
    bench.run([&] {
        (void)TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    });
}

BENCHMARK(BenchTimeDeprecated, benchmark::PriorityLevel::HIGH);
BENCHMARK(BenchTimeMillis, benchmark::PriorityLevel::HIGH);
BENCHMARK(BenchTimeMillisSys, benchmark::PriorityLevel::HIGH);
BENCHMARK(BenchTimeMock, benchmark::PriorityLevel::HIGH);
