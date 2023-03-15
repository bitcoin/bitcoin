// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <clientversion.h>
#include <crypto/sha256.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

static const char* DEFAULT_BENCH_FILTER = ".*";
static constexpr int64_t DEFAULT_MIN_TIME_MS{10};
/** Priority level default value, run "all" priority levels */
static const std::string DEFAULT_PRIORITY{"all"};

static void SetupBenchArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-asymptote=<n1,n2,n3,...>", "Test asymptotic growth of the runtime of an algorithm, if supported by the benchmark", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-list", "List benchmarks without executing them", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-min-time=<milliseconds>", strprintf("Minimum runtime per benchmark, in milliseconds (default: %d)", DEFAULT_MIN_TIME_MS), ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
    argsman.AddArg("-output-csv=<output.csv>", "Generate CSV file with the most important benchmark results", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-output-json=<output.json>", "Generate JSON file with all benchmark results", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-sanity-check", "Run benchmarks for only one iteration with no output", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-priority-level=<l1,l2,l3>", strprintf("Run benchmarks of one or multiple priority level(s) (%s), default: '%s'",
                                                           benchmark::ListPriorities(), DEFAULT_PRIORITY), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
}

// parses a comma separated list like "10,20,30,50"
static std::vector<double> parseAsymptote(const std::string& str) {
    std::stringstream ss(str);
    std::vector<double> numbers;
    double d;
    char c;
    while (ss >> d) {
        numbers.push_back(d);
        ss >> c;
    }
    return numbers;
}

static uint8_t parsePriorityLevel(const std::string& str) {
    uint8_t levels{0};
    for (const auto& level: SplitString(str, ',')) {
        levels |= benchmark::StringToPriority(level);
    }
    return levels;
}

int main(int argc, char** argv)
{
    ArgsManager argsman;
    SetupBenchArgs(argsman);
    SHA256AutoDetect();
    std::string error;
    if (!argsman.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }

    if (HelpRequested(argsman)) {
        std::cout << "Usage:  bench_bitcoin [options]\n"
                     "\n"
                  << argsman.GetHelpMessage()
                  << "Description:\n"
                     "\n"
                     "  bench_bitcoin executes microbenchmarks. The quality of the benchmark results\n"
                     "  highly depend on the stability of the machine. It can sometimes be difficult\n"
                     "  to get stable, repeatable results, so here are a few tips:\n"
                     "\n"
                     "  * Use pyperf [1] to disable frequency scaling, turbo boost etc. For best\n"
                     "    results, use CPU pinning and CPU isolation (see [2]).\n"
                     "\n"
                     "  * Each call of run() should do exactly the same work. E.g. inserting into\n"
                     "    a std::vector doesn't do that as it will reallocate on certain calls. Make\n"
                     "    sure each run has exactly the same preconditions.\n"
                     "\n"
                     "  * If results are still not reliable, increase runtime with e.g.\n"
                     "    -min-time=5000 to let a benchmark run for at least 5 seconds.\n"
                     "\n"
                     "  * bench_bitcoin uses nanobench [3] for which there is extensive\n"
                     "    documentation available online.\n"
                     "\n"
                     "Environment Variables:\n"
                     "\n"
                     "  To attach a profiler you can run a benchmark in endless mode. This can be\n"
                     "  done with the environment variable NANOBENCH_ENDLESS. E.g. like so:\n"
                     "\n"
                     "    NANOBENCH_ENDLESS=MuHash ./bench_bitcoin -filter=MuHash\n"
                     "\n"
                     "  In rare cases it can be useful to suppress stability warnings. This can be\n"
                     "  done with the environment variable NANOBENCH_SUPPRESS_WARNINGS, e.g:\n"
                     "\n"
                     "    NANOBENCH_SUPPRESS_WARNINGS=1 ./bench_bitcoin\n"
                     "\n"
                     "Notes:\n"
                     "\n"
                     "  1. pyperf\n"
                     "     https://github.com/psf/pyperf\n"
                     "\n"
                     "  2. CPU pinning & isolation\n"
                     "     https://pyperf.readthedocs.io/en/latest/system.html\n"
                     "\n"
                     "  3. nanobench\n"
                     "     https://github.com/martinus/nanobench\n"
                     "\n";

        return EXIT_SUCCESS;
    }

    try {
        benchmark::Args args;
        args.asymptote = parseAsymptote(argsman.GetArg("-asymptote", ""));
        args.is_list_only = argsman.GetBoolArg("-list", false);
        args.min_time = std::chrono::milliseconds(argsman.GetIntArg("-min-time", DEFAULT_MIN_TIME_MS));
        args.output_csv = argsman.GetPathArg("-output-csv");
        args.output_json = argsman.GetPathArg("-output-json");
        args.regex_filter = argsman.GetArg("-filter", DEFAULT_BENCH_FILTER);
        args.sanity_check = argsman.GetBoolArg("-sanity-check", false);
        args.priority = parsePriorityLevel(argsman.GetArg("-priority-level", DEFAULT_PRIORITY));

        benchmark::BenchRunner::RunAll(args);

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }
}
