// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <util/strencodings.h>
#include <util/system.h>

#include <memory>

static const char* DEFAULT_BENCH_FILTER = ".*";

static void SetupBenchArgs()
{
    SetupHelpOptions(gArgs);

    gArgs.AddArg("-list", "List benchmarks without executing them. Can be combined with -scaling and -filter", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-asymptote=n1,n2,n3,...", strprintf("Test asymptotic growth of the runtime of an algorithm, if supported by the benchmark"), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
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
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }

    if (HelpRequested(gArgs)) {
        std::cout << gArgs.GetHelpMessage();

        return EXIT_SUCCESS;
    }

    std::string regex_filter = gArgs.GetArg("-filter", DEFAULT_BENCH_FILTER);
    bool is_list_only = gArgs.GetBoolArg("-list", false);
    std::vector<double> asymptote = parseAsymptote(gArgs.GetArg("-asymptote", ""));

    benchmark::BenchRunner::RunAll(regex_filter, is_list_only, asymptote);

    return EXIT_SUCCESS;
}
