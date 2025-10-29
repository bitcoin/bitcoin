// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <test/util/setup_common.h> // IWYU pragma: keep
#include <util/check.h>
#include <util/fs.h>

#include <chrono>
#include <compare>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

/**
 * Retrieves the available test setup command line arguments that may be used
 * in the benchmark. They will be used only if the benchmark utilizes a
 * 'BasicTestingSetup' or any child of it.
 */
static std::function<std::vector<const char*>()> g_bench_command_line_args{};
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS = []() {
    return g_bench_command_line_args();
};

/**
 * Retrieve the name of the currently in-use benchmark.
 * This is applicable only to benchmarks that utilize the unit test
 * framework context setup (e.g. ones using 'MakeNoLogFileContext<TestingSetup>()').
 * It places the datadir of each benchmark run within their respective benchmark name.
 */
static std::string g_running_benchmark_name;
const std::function<std::string()> G_TEST_GET_FULL_NAME = []() {
    return g_running_benchmark_name;
};

namespace {

void GenerateTemplateResults(const std::vector<ankerl::nanobench::Result>& benchmarkResults, const fs::path& file, const char* tpl)
{
    if (benchmarkResults.empty() || file.empty()) {
        // nothing to write, bail out
        return;
    }
    std::ofstream fout{file.std_path()};
    if (fout.is_open()) {
        ankerl::nanobench::render(tpl, benchmarkResults, fout);
        std::cout << "Created " << file << std::endl;
    } else {
        std::cout << "Could not write to file " << file << std::endl;
    }
}

} // namespace

namespace benchmark {

BenchRunner::BenchmarkMap& BenchRunner::benchmarks()
{
    static BenchmarkMap benchmarks_map;
    return benchmarks_map;
}

BenchRunner::BenchRunner(std::string name, BenchFunction func)
{
    Assert(benchmarks().try_emplace(std::move(name), std::move(func)).second);
}

void BenchRunner::RunAll(const Args& args)
{
    std::regex reFilter(args.regex_filter);
    std::smatch baseMatch;

    if (args.sanity_check) {
        std::cout << "Running with -sanity-check option, output is being suppressed as benchmark results will be useless." << std::endl;
    }

    bool is_thread_scaling = args.scale_threads && (args.regex_filter != ".*");
    if (args.scale_threads && !is_thread_scaling) {
        std::cout << "Warning: -scale-threads requires -filter to be set. Running with default thread count." << std::endl;
    }

    std::vector<int> thread_scales;
    if (is_thread_scaling) {
        // Scale from 1 to MAX_SCRIPTCHECK_THREADS
        constexpr int MAX_SCRIPTCHECK_THREADS = 15;
        for (int i = 1; i <= MAX_SCRIPTCHECK_THREADS; ++i) {
            thread_scales.push_back(i);
        }
        std::cout << "Running benchmarks with worker threads from 1 to " << MAX_SCRIPTCHECK_THREADS << std::endl;
    } else {
        thread_scales.push_back(0);  // Use default
    }

    std::vector<ankerl::nanobench::Result> benchmarkResults;
    for (const auto& [name, func] : benchmarks()) {

        if (!std::regex_match(name, baseMatch, reFilter)) {
            continue;
        }

        if (args.is_list_only) {
            std::cout << name << std::endl;
            continue;
        }

        for (int threads : thread_scales) {
            std::vector<std::string> current_setup_args = args.setup_args;
            if (threads > 0) {
                bool found = false;
                for (auto& arg : current_setup_args) {
                    if (arg.starts_with("-worker-threads=") == 0) {
                        arg = strprintf("-worker-threads=%d", threads);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    current_setup_args.push_back(strprintf("-worker-threads=%d", threads));
                }
            }

            g_bench_command_line_args = [&current_setup_args]() {
                std::vector<const char*> ret;
                ret.reserve(current_setup_args.size());
                for (const auto& arg : current_setup_args) ret.emplace_back(arg.c_str());
                return ret;
            };

            Bench bench;
            if (args.sanity_check) {
                bench.epochs(1).epochIterations(1);
                bench.output(nullptr);
            }

            // Update benchmark name to include thread count
            std::string bench_name = name;
            if (is_thread_scaling) {
                bench_name = strprintf("%s_%d_threads", name, threads);
            }
            bench.name(bench_name);
            g_running_benchmark_name = bench_name;

            if (args.min_time > 0ms) {
                // convert to nanos before dividing to reduce rounding errors
                std::chrono::nanoseconds min_time_ns = args.min_time;
                bench.minEpochTime(min_time_ns / bench.epochs());
            }

            if (args.asymptote.empty()) {
                func(bench);
            } else {
                for (auto n : args.asymptote) {
                    bench.complexityN(n);
                    func(bench);
                }
                std::cout << bench.complexityBigO() << std::endl;
            }

            if (!bench.results().empty()) {
                benchmarkResults.push_back(bench.results().back());
            }
        }
    }

    GenerateTemplateResults(benchmarkResults, args.output_csv, "# Benchmark, evals, iterations, total, min, max, median\n"
                                                               "{{#result}}{{name}}, {{epochs}}, {{average(iterations)}}, {{sumProduct(iterations, elapsed)}}, {{minimum(elapsed)}}, {{maximum(elapsed)}}, {{median(elapsed)}}\n"
                                                               "{{/result}}");
    GenerateTemplateResults(benchmarkResults, args.output_json, ankerl::nanobench::templates::json());
}

} // namespace benchmark
