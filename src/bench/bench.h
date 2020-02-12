// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BENCH_H
#define BITCOIN_BENCH_BENCH_H

#include <functional>
#include <limits>
#include <map>
#include <string>
#include <chrono>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

// Simple micro-benchmarking framework; API mostly matches a subset of the Google Benchmark
// framework (see https://github.com/google/benchmark)
// Why not use the Google Benchmark framework? Because adding Yet Another Dependency
// (that uses cmake as its build system and has lots of features we don't need) isn't
// worth it.

/*
 * Usage:

static void CODE_TO_TIME(benchmark::State& state)
{
    ... do any setup needed...
    while (state.KeepRunning()) {
       ... do stuff you want to time...
    }
    ... do any cleanup needed...
}

BENCHMARK(CODE_TO_TIME);

 */
 
namespace benchmark {
    // In case high_resolution_clock is steady, prefer that, otherwise use steady_clock.
    struct best_clock {
        using hi_res_clock = std::chrono::high_resolution_clock;
        using steady_clock = std::chrono::steady_clock;
        using type = std::conditional<hi_res_clock::is_steady, hi_res_clock, steady_clock>::type;
    };
    using clock = best_clock::type;
    using time_point = clock::time_point;
    using duration = clock::duration;

    class State {
        std::string name;
        duration maxElapsed;
        time_point beginTime, lastTime;
        duration minTime, maxTime;
        uint64_t count;
        uint64_t countMask;
        uint64_t beginCycles;
        uint64_t lastCycles;
        uint64_t minCycles;
        uint64_t maxCycles;
    public:
        State(std::string _name, duration _maxElapsed) :
            name(_name),
            maxElapsed(_maxElapsed),
            minTime(duration::max()),
            maxTime(duration::zero()),
            count(0),
            countMask(1),
            beginCycles(0),
            lastCycles(0),
            minCycles(std::numeric_limits<uint64_t>::max()),
            maxCycles(std::numeric_limits<uint64_t>::min()) {
        }
        bool KeepRunning();
    };

    typedef std::function<void(State&)> BenchFunction;

    class BenchRunner
    {
        typedef std::map<std::string, BenchFunction> BenchmarkMap;
        static BenchmarkMap &benchmarks();

    public:
        BenchRunner(std::string name, BenchFunction func);

        static void RunAll(duration elapsedTimeForOne = std::chrono::seconds(1));
    };
}

// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo", foo);
#define BENCHMARK(n) \
    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n);

#endif // BITCOIN_BENCH_BENCH_H
