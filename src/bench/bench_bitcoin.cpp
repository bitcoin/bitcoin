// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <crypto/sha256.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <memory>

static const char* DEFAULT_BENCH_FILTER = ".*";

static void SetupBenchArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-asymptote=n1,n2,n3,...", "Test asymptotic growth of the runtime of an algorithm, if supported by the benchmark", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-list", "List benchmarks without executing them", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-output_csv=<output.csv>", "Generate CSV file with the most important benchmark results", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-output_json=<output.json>", "Generate JSON file with all benchmark results", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
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
    ArgsManager argsman;
    SetupBenchArgs(argsman);
    SHA256AutoDetect();
    std::string error;
    if (!argsman.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }

    if (HelpRequested(argsman)) {
        std::cout << argsman.GetHelpMessage();

        return EXIT_SUCCESS;
    }

    benchmark::Args args;
    args.regex_filter = argsman.GetArg("-filter", DEFAULT_BENCH_FILTER);
    args.is_list_only = argsman.GetBoolArg("-list", false);
    args.asymptote = parseAsymptote(argsman.GetArg("-asymptote", ""));
    args.output_csv = argsman.GetArg("-output_csv", "");
    args.output_json = argsman.GetArg("-output_json", "");

    benchmark::BenchRunner::RunAll(args);

    return EXIT_SUCCESS;
}
