// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BENCH_BITCOIN_SETTINGS_H
#define BITCOIN_BENCH_BENCH_BITCOIN_SETTINGS_H

#include <common/setting.h>

#include <cstdint>
#include <string>
#include <vector>

static const char* DEFAULT_BENCH_FILTER = ".*";
static constexpr int64_t DEFAULT_MIN_TIME_MS{10};
/** Priority level default value, run "all" priority levels */
static constexpr auto DEFAULT_PRIORITY{"all"};

using AsymptoteSetting = common::Setting<
    "-asymptote=<n1,n2,n3,...>", std::string, common::SettingOptions{.legacy = true},
    "Test asymptotic growth of the runtime of an algorithm, if supported by the benchmark">;

using FilterSetting = common::Setting<
    "-filter=<regex>", std::string, common::SettingOptions{.legacy = true},
    "Regular expression filter to select benchmark by name (default: %s)">
    ::DefaultFn<[] { return DEFAULT_BENCH_FILTER; }>;

using ListSetting = common::Setting<
    "-list", bool, common::SettingOptions{.legacy = true},
    "List benchmarks without executing them">;

using MinTimeSetting = common::Setting<
    "-min-time=<milliseconds>", int64_t, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Minimum runtime per benchmark, in milliseconds (default: %d)">
    ::Default<DEFAULT_MIN_TIME_MS>;

using OutputCsvSetting = common::Setting<
    "-output-csv=<output.csv>", fs::path, common::SettingOptions{.legacy = true},
    "Generate CSV file with the most important benchmark results">;

using OutputJsonSetting = common::Setting<
    "-output-json=<output.json>", fs::path, common::SettingOptions{.legacy = true},
    "Generate JSON file with all benchmark results">;

using SanityCheckSetting = common::Setting<
    "-sanity-check", bool, common::SettingOptions{.legacy = true},
    "Run benchmarks for only one iteration with no output">;

using PriorityLevelSetting = common::Setting<
    "-priority-level=<l1,l2,l3>", std::string, common::SettingOptions{.legacy = true},
    "Run benchmarks of one or multiple priority level(s) (%s), default: '%s'">
    ::DefaultFn<[] { return DEFAULT_PRIORITY; }>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, benchmark::ListPriorities(), DEFAULT_PRIORITY); }>;

#endif // BITCOIN_BENCH_BENCH_BITCOIN_SETTINGS_H
