// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <chainparams.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <regex>

const RegTestingSetup* g_testing_setup = nullptr;
const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

benchmark::BenchRunner::BenchmarkMap& benchmark::BenchRunner::benchmarks()
{
    static std::map<std::string, BenchFunction> benchmarks_map;
    return benchmarks_map;
}

benchmark::BenchRunner::BenchRunner(std::string name, benchmark::BenchFunction func)
{
    benchmarks().insert(std::make_pair(name, func));
}

void benchmark::BenchRunner::RunAll(const std::string& filter, bool is_list_only)
{
    std::regex reFilter(filter);
    std::smatch baseMatch;

    std::vector<Result> benchmarkResults;
    for (const auto& p : benchmarks()) {
        if (!std::regex_match(p.first, baseMatch, reFilter)) {
            continue;
        }

        if (is_list_only) {
            std::cout << p.first << std::endl;
            continue;
        }

        RegTestingSetup test{};
        assert(g_testing_setup == nullptr);
        g_testing_setup = &test;
        {
            LOCK(cs_main);
            assert(::ChainActive().Height() == 0);
            const bool witness_enabled{IsWitnessEnabled(::ChainActive().Tip(), Params().GetConsensus())};
            assert(witness_enabled);
        }

        Bench bench;
        bench.name(p.first);
        p.second(bench);
        benchmarkResults.push_back(bench.results().back());
        g_testing_setup = nullptr;
    }

    // Generate legacy CSV data to "benchmarkresults.csv"
    std::ofstream fout("benchmarkresults.csv");

    // Base58CheckEncode, 5, 320000, 2.70232, 1.68246e-06, 1.69408e-06, 1.69268e-06
    if (fout.is_open()) {
        templates::generate("# Benchmark, evals, iterations, total, min, max, median\n"
                            "{{#result}}{{name}}, {{epochs}}, {{average(iterations)}}, {{sumProduct(iterations, elapsed)}}, {{minimum(elapsed)}}, {{maximum(elapsed)}}, {{median(elapsed)}}\n"
                            "{{/result}}",
            benchmarkResults, fout);
    }
}
