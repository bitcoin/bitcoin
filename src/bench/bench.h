// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BENCH_H
#define BITCOIN_BENCH_BENCH_H

#include <util/macros.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <bench/nanobench.h>

/*
 * Usage:

static void CODE_TO_TIME(benchmark::Bench& bench)
{
    ... do any setup needed...
    nanobench::Config().run([&] {
       ... do stuff you want to time...
    });
    ... do any cleanup needed...
}

BENCHMARK(CODE_TO_TIME);

 */

namespace benchmark {

using ankerl::nanobench::Bench;

typedef std::function<void(Bench&)> BenchFunction;

struct Args {
    std::string regex_filter;
    bool is_list_only;
    std::vector<double> asymptote;
    std::string output_csv;
    std::string output_json;
};

class BenchRunner
{
    typedef std::map<std::string, BenchFunction> BenchmarkMap;
    static BenchmarkMap& benchmarks();

public:
    BenchRunner(std::string name, BenchFunction func);

    static void RunAll(const Args& args);
};
}
// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo", foo);
#define BENCHMARK(n) \
    benchmark::BenchRunner PASTE2(bench_, PASTE2(__LINE__, n))(STRINGIZE(n), n);

#endif // BITCOIN_BENCH_BENCH_H
