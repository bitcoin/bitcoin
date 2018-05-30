// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <crypto/sha256.h>
#include <key.h>
#include <stacktraces.h>
#include <validation.h>
#include <util/system.h>
#include <random.h>

#include <boost/lexical_cast.hpp>

#include <memory>

#include <bls/bls.h>

static const int64_t DEFAULT_BENCH_EVALUATIONS = 5;
static const char* DEFAULT_BENCH_FILTER = ".*";
static const char* DEFAULT_BENCH_SCALING = "1.0";
static const char* DEFAULT_BENCH_PRINTER = "console";
static const char* DEFAULT_PLOT_PLOTLYURL = "https://cdn.plot.ly/plotly-latest.min.js";
static const int64_t DEFAULT_PLOT_WIDTH = 1024;
static const int64_t DEFAULT_PLOT_HEIGHT = 768;

void InitBLSTests();
void CleanupBLSTests();
void CleanupBLSDkgTests();

static fs::path SetDataDir()
{
    fs::path ret = fs::temp_directory_path() / "bench_dash" / fs::unique_path();
    fs::create_directories(ret);
    gArgs.ForceSetArg("-datadir", ret.string());
    return ret;
}

static void SetupBenchArgs()
{
    gArgs.AddArg("-?", "Print this help message and exit", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-list", "List benchmarks without executing them. Can be combined with -scaling and -filter", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-evals=<n>", strprintf("Number of measurement evaluations to perform. (default: %u)", DEFAULT_BENCH_EVALUATIONS), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-filter=<regex>", strprintf("Regular expression filter to select benchmark by name (default: %s)", DEFAULT_BENCH_FILTER), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-scaling=<n>", strprintf("Scaling factor for benchmark's runtime (default: %u)", DEFAULT_BENCH_SCALING), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-printer=(console|plot)", strprintf("Choose printer format. console: print data to console. plot: Print results as HTML graph (default: %s)", DEFAULT_BENCH_PRINTER), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-plot-plotlyurl=<uri>", strprintf("URL to use for plotly.js (default: %s)", DEFAULT_PLOT_PLOTLYURL), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-plot-width=<x>", strprintf("Plot width in pixel (default: %u)", DEFAULT_PLOT_WIDTH), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-plot-height=<x>", strprintf("Plot height in pixel (default: %u)", DEFAULT_PLOT_HEIGHT), false, OptionsCategory::OPTIONS);

    // Hidden
    gArgs.AddArg("-h", "", false, OptionsCategory::HIDDEN);
    gArgs.AddArg("-help", "", false, OptionsCategory::HIDDEN);
}

int main(int argc, char** argv)
{
    SetupBenchArgs();
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        fprintf(stderr, "Error parsing command line arguments: %s\n", error.c_str());
        return false;
    }

    if (HelpRequested(gArgs)) {
        std::cout << gArgs.GetHelpMessage();

        return 0;
    }

    // Set the datadir after parsing the bench options
    const fs::path bench_datadir{SetDataDir()};

    SHA256AutoDetect();

    RegisterPrettySignalHandlers();
    RegisterPrettyTerminateHander();

    RandomInit();
    ECC_Start();
    ECCVerifyHandle verifyHandle;

    BLSInit();
    InitBLSTests();
    SetupEnvironment();

    int64_t evaluations = gArgs.GetArg("-evals", DEFAULT_BENCH_EVALUATIONS);
    std::string regex_filter = gArgs.GetArg("-filter", DEFAULT_BENCH_FILTER);
    std::string scaling_str = gArgs.GetArg("-scaling", DEFAULT_BENCH_SCALING);
    bool is_list_only = gArgs.GetBoolArg("-list", false);

    double scaling_factor = boost::lexical_cast<double>(scaling_str);


    std::unique_ptr<benchmark::Printer> printer(new benchmark::ConsolePrinter());
    std::string printer_arg = gArgs.GetArg("-printer", DEFAULT_BENCH_PRINTER);
    if ("plot" == printer_arg) {
        printer.reset(new benchmark::PlotlyPrinter(
            gArgs.GetArg("-plot-plotlyurl", DEFAULT_PLOT_PLOTLYURL),
            gArgs.GetArg("-plot-width", DEFAULT_PLOT_WIDTH),
            gArgs.GetArg("-plot-height", DEFAULT_PLOT_HEIGHT)));
    }

    benchmark::BenchRunner::RunAll(*printer, evaluations, scaling_factor, regex_filter, is_list_only);

    fs::remove_all(bench_datadir);

    // need to be called before global destructors kick in (PoolAllocator is needed due to many BLSSecretKeys)
    CleanupBLSDkgTests();
    CleanupBLSTests();

    ECC_Stop();
}
