// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BENCH_H
#define BITCOIN_BENCH_BENCH_H

#include <bench/nanobench.h> // IWYU pragma: export
#include <util/fs.h>
#include <util/macros.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

/*
 * Usage:

static void NameOfYourBenchmarkFunction(benchmark::Bench& bench)
{
    ...do any setup needed...

    bench.run([&] {
         ...do stuff you want to time; refer to src/bench/nanobench.h
            for more information and the options that can be passed here...
    });

    ...do any cleanup needed...
}

BENCHMARK(NameOfYourBenchmarkFunction);

 */

namespace benchmark {

using ankerl::nanobench::Bench;

using BenchFunction = std::function<void(Bench&)>;

struct Args {
    bool is_list_only;
    bool sanity_check;
    std::chrono::milliseconds min_time;
    std::vector<double> asymptote;
    fs::path output_csv;
    fs::path output_json;
    std::string regex_filter;
    std::vector<std::string> setup_args;
};

class BenchRunner
{
    // maps from "name" -> function
    using BenchmarkMap = std::map<std::string, BenchFunction>;
    static BenchmarkMap& benchmarks();

public:
    BenchRunner(std::string name, BenchFunction func);

    static void RunAll(const Args& args);
};
} // namespace benchmark

// BENCHMARK(foo); expands to:  benchmark::BenchRunner bench_runner_foo{"foo", foo};
#define BENCHMARK(n) \
    benchmark::BenchRunner PASTE2(bench_runner_, n) { STRINGIZE(n), n }

#endif // BITCOIN_BENCH_BENCH_H
