// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <logging.h>
#include <test/util/setup_common.h>


static void Logging(benchmark::Bench& bench, const std::vector<const char*>& extra_args, const std::function<void()>& log)
{
    TestingSetup test_setup{
        CBaseChainParams::REGTEST,
        extra_args,
    };

    bench.run([&] { log(); });
}

static void LogPrintfWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] { LogPrintf("%s\n", "test"); });
}
static void LogPrintfWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] { LogPrintf("%s\n", "test"); });
}
static void LogPrintfCategoryWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] { LogPrintfCategory(BCLog::NET, "%s\n", "test"); });
}
static void LogPrintfCategoryWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] { LogPrintfCategory(BCLog::NET, "%s\n", "test"); });
}
static void LogPrintLevelWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", "test"); });
}
static void LogPrintLevelWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", "test"); });
}
static void LogPrintWithCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LogPrintWithoutCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=0"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LogNoDebugLogFile(benchmark::Bench& bench)
{
    Logging(bench, {"-nodebuglogfile", "-debug=1"}, [] {
        LogPrintf("%s\n", "test");
        LogPrintfCategory(BCLog::NET, "%s\n", "test");
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", "test");
        LogPrint(BCLog::NET, "%s\n", "test");
    });
}

BENCHMARK(LogPrintfWithThreadNames);
BENCHMARK(LogPrintfWithoutThreadNames);
BENCHMARK(LogPrintfCategoryWithThreadNames);
BENCHMARK(LogPrintfCategoryWithoutThreadNames);
BENCHMARK(LogPrintLevelWithThreadNames);
BENCHMARK(LogPrintLevelWithoutThreadNames);
BENCHMARK(LogPrintWithCategory);
BENCHMARK(LogPrintWithoutCategory);
BENCHMARK(LogNoDebugLogFile);
