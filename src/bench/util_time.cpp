// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <utiltime.h>

static void BenchTimeDeprecated(benchmark::State& state)
{
    while (state.KeepRunning()) {
        (void)GetTime();
    }
}

static void BenchTimeMock(benchmark::State& state)
{
    SetMockTime(111);
    while (state.KeepRunning()) {
        (void)GetTime<std::chrono::seconds>();
    }
    SetMockTime(0);
}

static void BenchTimeMillis(benchmark::State& state)
{
    while (state.KeepRunning()) {
        (void)GetTime<std::chrono::milliseconds>();
    }
}

static void BenchTimeMillisSys(benchmark::State& state)
{
    while (state.KeepRunning()) {
        (void)GetTimeMillis();
    }
}

BENCHMARK(BenchTimeDeprecated, 100000000);
BENCHMARK(BenchTimeMillis, 6000000);
BENCHMARK(BenchTimeMillisSys, 6000000);
BENCHMARK(BenchTimeMock, 300000000);
