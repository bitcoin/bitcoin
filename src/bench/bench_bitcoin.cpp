// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

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

static void SetupBenchArgs()
{
    SetupHelpOptions(gArgs);

    gArgs.AddArg("-list", "List benchmarks without executing them. Can be combined with -scaling and -filter", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-evals=<n>", strprintf("Number of measurement evaluations to perform. (default: %u)", DEFAULT_BENCH_EVALUATIONS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-scaling=<n>", strprintf("Scaling factor for benchmark's runtime (default: %u)", DEFAULT_BENCH_SCALING), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-printer=(console|plot)", strprintf("Choose printer format. console: print data to console. plot: Print results as HTML graph (default: %s)", DEFAULT_BENCH_PRINTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-plot-plotlyurl=<uri>", strprintf("URL to use for plotly.js (default: %s)", DEFAULT_PLOT_PLOTLYURL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-plot-width=<x>", strprintf("Plot width in pixel (default: %u)", DEFAULT_PLOT_WIDTH), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-plot-height=<x>", strprintf("Plot height in pixel (default: %u)", DEFAULT_PLOT_HEIGHT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
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

    int64_t evaluations = gArgs.GetArg("-evals", DEFAULT_BENCH_EVALUATIONS);
    std::string regex_filter = gArgs.GetArg("-filter", DEFAULT_BENCH_FILTER);
    std::string scaling_str = gArgs.GetArg("-scaling", DEFAULT_BENCH_SCALING);
    bool is_list_only = gArgs.GetBoolArg("-list", false);

    double scaling_factor;
    if (!ParseDouble(scaling_str, &scaling_factor)) {
        tfm::format(std::cerr, "Error parsing scaling factor as double: %s\n", scaling_str.c_str());
        return EXIT_FAILURE;
    }

    std::unique_ptr<benchmark::Printer> printer = MakeUnique<benchmark::ConsolePrinter>();
    std::string printer_arg = gArgs.GetArg("-printer", DEFAULT_BENCH_PRINTER);
    if ("plot" == printer_arg) {
        printer.reset(new benchmark::PlotlyPrinter(
            gArgs.GetArg("-plot-plotlyurl", DEFAULT_PLOT_PLOTLYURL),
            gArgs.GetArg("-plot-width", DEFAULT_PLOT_WIDTH),
            gArgs.GetArg("-plot-height", DEFAULT_PLOT_HEIGHT)));
    }

    benchmark::BenchRunner::RunAll(*printer, evaluations, scaling_factor, regex_filter, is_list_only);

    return EXIT_SUCCESS;
}
