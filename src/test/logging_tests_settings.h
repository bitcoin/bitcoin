#ifndef BITCOIN_TEST_LOGGING_TESTS_SETTINGS_H
#define BITCOIN_TEST_LOGGING_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using LogLevelSetting2 = common::Setting<
    "-loglevel", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "...">
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogLevelSetting3 = common::Setting<
    "-loglevel", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "...">
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogLevelSetting4 = common::Setting<
    "-loglevel", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "...">
    ::Category<OptionsCategory::DEBUG_TEST>;

#endif // BITCOIN_TEST_LOGGING_TESTS_SETTINGS_H
