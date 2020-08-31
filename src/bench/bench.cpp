// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <chainparams.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <regex>

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

namespace {

void GenerateTemplateResults(const std::vector<ankerl::nanobench::Result>& benchmarkResults, const std::string& filename, const char* tpl)
{
    if (benchmarkResults.empty() || filename.empty()) {
        // nothing to write, bail out
        return;
    }
    std::ofstream fout(filename);
    if (fout.is_open()) {
        ankerl::nanobench::render(tpl, benchmarkResults, fout);
    } else {
        std::cout << "Could write to file '" << filename << "'" << std::endl;
    }

    std::cout << "Created '" << filename << "'" << std::endl;
}

} // namespace

benchmark::BenchRunner::BenchmarkMap& benchmark::BenchRunner::benchmarks()
{
    static std::map<std::string, BenchFunction> benchmarks_map;
    return benchmarks_map;
}

benchmark::BenchRunner::BenchRunner(std::string name, benchmark::BenchFunction func)
{
    benchmarks().insert(std::make_pair(name, func));
}

void benchmark::BenchRunner::RunAll(const Args& args)
{
    std::regex reFilter(args.regex_filter);
    std::smatch baseMatch;

    std::vector<ankerl::nanobench::Result> benchmarkResults;
    for (const auto& p : benchmarks()) {
        if (!std::regex_match(p.first, baseMatch, reFilter)) {
            continue;
        }

        if (args.is_list_only) {
            std::cout << p.first << std::endl;
            continue;
        }

        Bench bench;
        bench.name(p.first);
        if (args.asymptote.empty()) {
            p.second(bench);
        } else {
            for (auto n : args.asymptote) {
                bench.complexityN(n);
                p.second(bench);
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
