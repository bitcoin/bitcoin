// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <test/util/setup_common.h>
#include <util/fs.h>
#include <util/string.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

using namespace std::chrono_literals;

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS{};

namespace {

void GenerateTemplateResults(const std::vector<ankerl::nanobench::Result>& benchmarkResults, const fs::path& file, const char* tpl)
{
    if (benchmarkResults.empty() || file.empty()) {
        // nothing to write, bail out
        return;
    }
    std::ofstream fout{file};
    if (fout.is_open()) {
        ankerl::nanobench::render(tpl, benchmarkResults, fout);
        std::cout << "Created " << file << std::endl;
    } else {
        std::cout << "Could not write to file " << file << std::endl;
    }
}

} // namespace

namespace benchmark {

// map a label to one or multiple priority levels
std::map<std::string, uint8_t> map_label_priority = {
    {"high", PriorityLevel::HIGH},
    {"low", PriorityLevel::LOW},
    {"all", 0xff}
};

std::string ListPriorities()
{
    using item_t = std::pair<std::string, uint8_t>;
    auto sort_by_priority = [](item_t a, item_t b){ return a.second < b.second; };
    std::set<item_t, decltype(sort_by_priority)> sorted_priorities(map_label_priority.begin(), map_label_priority.end(), sort_by_priority);
    return Join(sorted_priorities, ',', [](const auto& entry){ return entry.first; });
}

uint8_t StringToPriority(const std::string& str)
{
    auto it = map_label_priority.find(str);
    if (it == map_label_priority.end()) throw std::runtime_error(strprintf("Unknown priority level %s", str));
    return it->second;
}

BenchRunner::BenchmarkMap& BenchRunner::benchmarks()
{
    static BenchmarkMap benchmarks_map;
    return benchmarks_map;
}

BenchRunner::BenchRunner(std::string name, BenchFunction func, PriorityLevel level)
{
    benchmarks().insert(std::make_pair(name, std::make_pair(func, level)));
}

void BenchRunner::RunAll(const Args& args)
{
    std::regex reFilter(args.regex_filter);
    std::smatch baseMatch;

    if (args.sanity_check) {
        std::cout << "Running with -sanity-check option, output is being suppressed as benchmark results will be useless." << std::endl;
    }

    std::vector<ankerl::nanobench::Result> benchmarkResults;
    for (const auto& [name, bench_func] : benchmarks()) {
        const auto& [func, priority_level] = bench_func;

        if (!(priority_level & args.priority)) {
            continue;
        }

        if (!std::regex_match(name, baseMatch, reFilter)) {
            continue;
        }

        if (args.is_list_only) {
            std::cout << name << std::endl;
            continue;
        }

        Bench bench;
        if (args.sanity_check) {
            bench.epochs(1).epochIterations(1);
            bench.output(nullptr);
        }
        bench.name(name);
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

    GenerateTemplateResults(benchmarkResults, args.output_csv, "# Benchmark, evals, iterations, total, min, max, median\n"
                                                               "{{#result}}{{name}}, {{epochs}}, {{average(iterations)}}, {{sumProduct(iterations, elapsed)}}, {{minimum(elapsed)}}, {{maximum(elapsed)}}, {{median(elapsed)}}\n"
                                                               "{{/result}}");
    GenerateTemplateResults(benchmarkResults, args.output_json, ankerl::nanobench::templates::json());
}

} // namespace benchmark
