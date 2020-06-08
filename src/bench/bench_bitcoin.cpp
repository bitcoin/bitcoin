// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <crypto/sha256.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <memory>

static const int64_t DEFAULT_BENCH_EVALUATIONS = 5;
static const char* DEFAULT_BENCH_FILTER = ".*";
static const char* DEFAULT_BENCH_SCALING = "1.0";
static const char* DEFAULT_BENCH_PRINTER = "console";
static const char* DEFAULT_PLOT_PLOTLYURL = "https://cdn.plot.ly/plotly-latest.min.js";
static const int64_t DEFAULT_PLOT_WIDTH = 1024;
static const int64_t DEFAULT_PLOT_HEIGHT = 768;

static void SetupBenchArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-list", "List benchmarks without executing them. Can be combined with -scaling and -filter", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-evals=<n>", strprintf("Number of measurement evaluations to perform. (default: %u)", DEFAULT_BENCH_EVALUATIONS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-scaling=<n>", strprintf("Scaling factor for benchmark's runtime (default: %u)", DEFAULT_BENCH_SCALING), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-printer=(console|plot)", strprintf("Choose printer format. console: print data to console. plot: Print results as HTML graph (default: %s)", DEFAULT_BENCH_PRINTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-plot-plotlyurl=<uri>", strprintf("URL to use for plotly.js (default: %s)", DEFAULT_PLOT_PLOTLYURL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-plot-width=<x>", strprintf("Plot width in pixel (default: %u)", DEFAULT_PLOT_WIDTH), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-plot-height=<x>", strprintf("Plot height in pixel (default: %u)", DEFAULT_PLOT_HEIGHT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
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

    int64_t evaluations = argsman.GetArg("-evals", DEFAULT_BENCH_EVALUATIONS);
    std::string regex_filter = argsman.GetArg("-filter", DEFAULT_BENCH_FILTER);
    std::string scaling_str = argsman.GetArg("-scaling", DEFAULT_BENCH_SCALING);
    bool is_list_only = argsman.GetBoolArg("-list", false);

    if (evaluations == 0) {
        return EXIT_SUCCESS;
    } else if (evaluations < 0) {
        tfm::format(std::cerr, "Error parsing evaluations argument: %d\n", evaluations);
        return EXIT_FAILURE;
    }

    double scaling_factor;
    if (!ParseDouble(scaling_str, &scaling_factor)) {
        tfm::format(std::cerr, "Error parsing scaling factor as double: %s\n", scaling_str);
        return EXIT_FAILURE;
    }

    std::unique_ptr<benchmark::Printer> printer = MakeUnique<benchmark::ConsolePrinter>();
    std::string printer_arg = argsman.GetArg("-printer", DEFAULT_BENCH_PRINTER);
    if ("plot" == printer_arg) {
        printer.reset(new benchmark::PlotlyPrinter(
            argsman.GetArg("-plot-plotlyurl", DEFAULT_PLOT_PLOTLYURL),
            argsman.GetArg("-plot-width", DEFAULT_PLOT_WIDTH),
            argsman.GetArg("-plot-height", DEFAULT_PLOT_HEIGHT)));
    }

    benchmark::BenchRunner::RunAll(*printer, evaluations, scaling_factor, regex_filter, is_list_only);

    return EXIT_SUCCESS;
}
