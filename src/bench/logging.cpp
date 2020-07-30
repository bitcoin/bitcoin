// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <logging.h>
#include <test/util/setup_common.h>


static void Logging(benchmark::State& state, const std::vector<const char*>& extra_args, const std::function<void()>& log)
{
    TestingSetup test_setup{
        CBaseChainParams::REGTEST,
        extra_args,
    };

    while (state.KeepRunning()) {
        log();
    }
}

static void LoggingYoThreadNames(benchmark::State& state)
{
    Logging(state, {"-logthreadnames=1"}, [] { LogPrintf("%s\n", "test"); });
}
static void LoggingNoThreadNames(benchmark::State& state)
{
    Logging(state, {"-logthreadnames=0"}, [] { LogPrintf("%s\n", "test"); });
}
static void LoggingYoCategory(benchmark::State& state)
{
    Logging(state, {"-logthreadnames=0", "-debug=net"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LoggingNoCategory(benchmark::State& state)
{
    Logging(state, {"-logthreadnames=0", "-debug=0"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LoggingNoFile(benchmark::State& state)
{
    Logging(state, {"-nodebuglogfile", "-debug=1"}, [] {LogPrintf("%s\n", "test"); LogPrint(BCLog::NET, "%s\n", "test"); });
}

BENCHMARK(LoggingYoThreadNames, 100000);
BENCHMARK(LoggingNoThreadNames, 100000);
BENCHMARK(LoggingYoCategory, 100000);
BENCHMARK(LoggingNoCategory, 40000000);
BENCHMARK(LoggingNoFile, 5000000);
