// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_SETUP_COMMON_SETTINGS_H
#define BITCOIN_TEST_UTIL_SETUP_COMMON_SETTINGS_H

#include <common/setting.h>
#include <util/fs.h>

#include <string>
#include <vector>

constexpr inline auto TEST_DIR_PATH_ELEMENT{"test_common bitcoin"}; // Includes a space to catch possible path escape issues.

using TestdatadirSetting = common::Setting<
    "-testdatadir", fs::path, common::SettingOptions{.legacy = true},
    "Custom data directory (default: %s<random_string>)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, fs::PathToString(fs::temp_directory_path() / TEST_DIR_PATH_ELEMENT / "")); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

#endif // BITCOIN_TEST_UTIL_SETUP_COMMON_SETTINGS_H
