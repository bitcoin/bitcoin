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
static const std::vector<size_t> DEFAULT_ASYMPTOTIC_FACTORS;

static void SetupBenchArgs()
{
    SetupHelpOptions(gArgs);

    gArgs.AddArg("-list", "List benchmarks without executing them. Can be combined with -scaling and -filter", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-evals=<n>", strprintf("Number of measurement evaluations to perform. (default: %u)", DEFAULT_BENCH_EVALUATIONS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-scaling=<n>", strprintf("Scaling factor for benchmark's runtime (default: %u)", DEFAULT_BENCH_SCALING), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-asymptote=n", 
            strprintf("Certain benchmarks can have tunable parameters (e.g, number of transactions) to test the "
                      "asymptotic growth of the runtime of an algorithm easily. "
                      "These arguments are positional and are defined differently per benchmark. "
                      "Thus asymptote should be used with a narrowly scoped filter to a single test case. "
                      "For example, this script tests ComplexMemPoolAsymptotic's max descendants per tx parameter "
                      "growing by powers of 2 and prints out the median: "
                      "`for x in {1..10}; do ./src/bench/bench_bitcoin -filter=ComplexMemPoolAsymptotic -asymptote=100 -asymptote=100 -asymptote=$((2**$x)); done | grep ComplexMemPoolAsymptotic --line-buffered | cut -d, -f7`"), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
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
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
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
    std::vector<std::string> asymptotes = gArgs.GetArgs("-asymptote");

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
    std::string printer_arg = gArgs.GetArg("-printer", DEFAULT_BENCH_PRINTER);
    if ("plot" == printer_arg) {
        printer.reset(new benchmark::PlotlyPrinter(
            gArgs.GetArg("-plot-plotlyurl", DEFAULT_PLOT_PLOTLYURL),
            gArgs.GetArg("-plot-width", DEFAULT_PLOT_WIDTH),
            gArgs.GetArg("-plot-height", DEFAULT_PLOT_HEIGHT)));
    }

    std::vector<size_t> asymptotic_factors;
    for (auto& s: asymptotes) {
        uint64_t arg;
        if (!ParseUInt64(s, &arg))
            tfm::format(std::cerr, "Error parsing scaling factor as double: %s\n", s);
        if (arg > std::numeric_limits<size_t>::max())
            tfm::format(std::cerr, "Error parsing scaling factor as size_t: %s\n", s);
        asymptotic_factors.emplace_back((size_t)arg);
    }

    benchmark::BenchRunner::RunAll(*printer, evaluations, scaling_factor, regex_filter, is_list_only, asymptotic_factors);

    return EXIT_SUCCESS;
}
