// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <crypto/sha256.h>
#include <key.h>
#include <validation.h>
#include <util.h>
#include <random.h>

#include <boost/lexical_cast.hpp>

#include <memory>

static const int64_t DEFAULT_BENCH_EVALUATIONS = 5;
static const char* DEFAULT_BENCH_FILTER = ".*";
static const char* DEFAULT_BENCH_SCALING = "1.0";
static const char* DEFAULT_BENCH_PRINTER = "console";
static const char* DEFAULT_PLOT_PLOTLYURL = "https://cdn.plot.ly/plotly-latest.min.js";
static const int64_t DEFAULT_PLOT_WIDTH = 1024;
static const int64_t DEFAULT_PLOT_HEIGHT = 768;

int
main(int argc, char** argv)
{
    gArgs.ParseParameters(argc, argv);

    if (gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") || gArgs.IsArgSet("-help")) {
        std::cout << HelpMessageGroup(_("Options:"))
                  << HelpMessageOpt("-?", _("Print this help message and exit"))
                  << HelpMessageOpt("-list", _("List benchmarks without executing them. Can be combined with -scaling and -filter"))
                  << HelpMessageOpt("-evals=<n>", strprintf(_("Number of measurement evaluations to perform. (default: %u)"), DEFAULT_BENCH_EVALUATIONS))
                  << HelpMessageOpt("-filter=<regex>", strprintf(_("Regular expression filter to select benchmark by name (default: %s)"), DEFAULT_BENCH_FILTER))
                  << HelpMessageOpt("-scaling=<n>", strprintf(_("Scaling factor for benchmark's runtime (default: %u)"), DEFAULT_BENCH_SCALING))
                  << HelpMessageOpt("-printer=(console|plot)", strprintf(_("Choose printer format. console: print data to console. plot: Print results as HTML graph (default: %s)"), DEFAULT_BENCH_PRINTER))
                  << HelpMessageOpt("-plot-plotlyurl=<uri>", strprintf(_("URL to use for plotly.js (default: %s)"), DEFAULT_PLOT_PLOTLYURL))
                  << HelpMessageOpt("-plot-width=<x>", strprintf(_("Plot width in pixel (default: %u)"), DEFAULT_PLOT_WIDTH))
                  << HelpMessageOpt("-plot-height=<x>", strprintf(_("Plot height in pixel (default: %u)"), DEFAULT_PLOT_HEIGHT));

        return 0;
    }

    SHA256AutoDetect();
    RandomInit();
    ECC_Start();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file

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

    ECC_Stop();
}
