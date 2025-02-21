// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <logging.h>
#include <test/util/setup_common.h>

#include <functional>
#include <vector>

using namespace std::literals;

// All but 2 of the benchmarks should have roughly similar performance:
//
// LogWithoutDebug should be ~3 orders of magnitude faster, as nothing is logged.
//
// LogWithoutWriteToFile should be ~2 orders of magnitude faster, as it avoids disk writes.

static void Logging(benchmark::Bench& bench, const std::vector<const char*>& extra_args, const std::function<void()>& log)
{
    // Reset any enabled logging categories from a previous benchmark run.
    LogInstance().DisableCategory(BCLog::LogFlags::ALL);

    TestingSetup test_setup{
        ChainType::REGTEST,
        {.extra_args = extra_args},
    };

    bench.run([&] { log(); });
}

static void LogWithDebug(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=net"}, [] { LogDebug(BCLog::NET, "%s\n", "test"); });
}

static void LogWithoutDebug(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0", "-debug=0"}, [] { LogDebug(BCLog::NET, "%s\n", "test"); });
}

static void LogWithThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=1"}, [] { LogInfo("%s\n", "test"); });
}

static void LogWithoutThreadNames(benchmark::Bench& bench)
{
    Logging(bench, {"-logthreadnames=0"}, [] { LogInfo("%s\n", "test"); });
}

static void LogWithoutWriteToFile(benchmark::Bench& bench)
{
    // Disable writing the log to a file, as used for unit tests and fuzzing in `MakeNoLogFileContext`.
    Logging(bench, {"-nodebuglogfile", "-debug=1"}, [] {
        LogInfo("%s\n", "test");
        LogDebug(BCLog::NET, "%s\n", "test");
    });
}

namespace BCLog {
    std::string LogEscapeMessage(std::string_view str);
}

static void LogEscapeMessageNormal(benchmark::Bench& bench) {
    const std::vector<std::string> normal_logs{
        "UpdateTip: new best=000000000000000000003c4c32d94a9363241a84d42cbbc1ec9f5f12f84f4feb height=875371 version=0x20000004 log2_work=95.334853 tx=1133590694 date='2024-12-19T01:57:26Z' progress=0.993026 cache=8.3MiB(56574txo)"s,
        "received: headers (162 bytes) peer=15"s,
        "Config file arg: datadir=/Users/bitcoin/data"s,
        "Verified block header at height 12345 hash: 你好 🔤"s
    };

    bench.batch(normal_logs.size()).run([&] {
        for (const auto& msg : normal_logs) {
            const auto& res{BCLog::LogEscapeMessage(msg)};
            ankerl::nanobench::doNotOptimizeAway(res);
        }
    });
}

static void LogEscapeMessageSuspicious(benchmark::Bench& bench) {
    // Based on the test cases
    const std::vector<std::string> suspicious_logs{
        "Received strange message\0from peer=12"s,
        "Got malformed packet\x01\x02\x03\x7F from peer=13"s,
        "Peer=14 sent:\x0D\x0Econtent-length: 100\x0D\x0A"s,
        "Validation failed on\x1B[31mERROR\x1B[0m block=123456"s
    };

   bench.batch(suspicious_logs.size()).run([&] {
        for (const auto& msg : suspicious_logs) {
            const auto& res{BCLog::LogEscapeMessage(msg)};
            ankerl::nanobench::doNotOptimizeAway(res);
        }
    });
}

BENCHMARK(LogWithDebug, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogWithoutDebug, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogWithThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogWithoutThreadNames, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogWithoutWriteToFile, benchmark::PriorityLevel::HIGH);
BENCHMARK(LogEscapeMessageNormal, benchmark::PriorityLevel::LOW);
BENCHMARK(LogEscapeMessageSuspicious, benchmark::PriorityLevel::LOW);
