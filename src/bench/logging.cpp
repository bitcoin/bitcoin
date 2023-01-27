// Copyright (c) 2020-2022 The Bitcoin Core developers
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

static void LoggingYoThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] { LogPrintf("%s\n", "test"); });
}
static void LoggingNoThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] { LogPrintf("%s\n", "test"); });
}
static void LoggingYoCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LoggingNoCategory(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=0"}, [] { LogPrint(BCLog::NET, "%s\n", "test"); });
}
static void LoggingNoFile(benchmark::Bench& bench)
{
    Logging(bench, {"-nodebuglogfile", "-debug=1"}, [] {
        LogPrintf("%s\n", "test");
        LogPrint(BCLog::NET, "%s\n", "test");
    });
}

BENCHMARK(LoggingYoThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LoggingNoThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LoggingYoCategory, benchmark::PriorityLevel::HIGH);
BENCHMARK(LoggingNoCategory, benchmark::PriorityLevel::HIGH);
BENCHMARK(LoggingNoFile, benchmark::PriorityLevel::HIGH);
