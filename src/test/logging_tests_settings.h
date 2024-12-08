#ifndef BITCOIN_TEST_LOGGING_TESTS_SETTINGS_H
#define BITCOIN_TEST_LOGGING_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using LoglevelSetting2 = common::Setting<
    "-loglevel", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "...">
    ::Category<OptionsCategory::DEBUG_TEST>;

using LoglevelSetting3 = common::Setting<
    "-loglevel", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "...">
    ::Category<OptionsCategory::DEBUG_TEST>;

using LoglevelSetting4 = common::Setting<
    "-loglevel", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "...">
    ::Category<OptionsCategory::DEBUG_TEST>;

#endif // BITCOIN_TEST_LOGGING_TESTS_SETTINGS_H
