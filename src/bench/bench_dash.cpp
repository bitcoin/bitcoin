// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <crypto/sha256.h>
#include <key.h>
#include <stacktraces.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <boost/lexical_cast.hpp>

#include <memory>

#include <bls/bls.h>

static const char* DEFAULT_BENCH_FILTER = ".*";

static void SetupBenchArgs()
{
    gArgs.AddArg("-?", "Print this help message and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-list", "List benchmarks without executing them", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-asymptote=n1,n2,n3,...", strprintf("Test asymptotic growth of the runtime of an algorithm, if supported by the benchmark"), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-output_csv=<output.csv>", "Generate CSV file with the most important benchmark results.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-output_json=<output.json>", "Generate JSON file with all benchmark results.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    // Hidden
    gArgs.AddArg("-h", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
    gArgs.AddArg("-help", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
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

int main(int argc, char** argv)
{
    SetupBenchArgs();
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error.c_str());
        return EXIT_FAILURE;
    }

    if (HelpRequested(gArgs)) {
        std::cout << gArgs.GetHelpMessage();
        return EXIT_SUCCESS;
    }

    benchmark::Args args;
    args.regex_filter = gArgs.GetArg("-filter", DEFAULT_BENCH_FILTER);
    args.is_list_only = gArgs.GetBoolArg("-list", false);
    args.asymptote = parseAsymptote(gArgs.GetArg("-asymptote", ""));
    args.output_csv = gArgs.GetArg("-output_csv", "");
    args.output_json = gArgs.GetArg("-output_json", "");

    benchmark::BenchRunner::RunAll(args);

    return EXIT_SUCCESS;
}
