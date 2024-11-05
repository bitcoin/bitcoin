// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_BENCH_BITCOIN_SETTINGS_H
#define BITCOIN_BENCH_BENCH_BITCOIN_SETTINGS_H

#include <cstdint>
#include <string>

static const char* DEFAULT_BENCH_FILTER = ".*";
static constexpr int64_t DEFAULT_MIN_TIME_MS{10};
/** Priority level default value, run "all" priority levels */
static constexpr auto DEFAULT_PRIORITY{"all"};

#endif // BITCOIN_BENCH_BENCH_BITCOIN_SETTINGS_H
