#ifndef BITCOIN_TEST_GETARG_TESTS_SETTINGS_H
#define BITCOIN_TEST_GETARG_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using FooSetting = common::Setting<
    "foo", std::string, common::SettingOptions{.legacy = true}>;

using FooSettingInt = common::Setting<
    "foo", int64_t, common::SettingOptions{.legacy = true}>;

using FooSettingBool = common::Setting<
    "foo", bool, common::SettingOptions{.legacy = true}>;

using FooSetting2 = common::Setting<
    "-foo", std::string, common::SettingOptions{.legacy = true}>;

using FooSetting2Int = common::Setting<
    "-foo", int64_t, common::SettingOptions{.legacy = true}>;

using FooSetting2Bool = common::Setting<
    "-foo", bool, common::SettingOptions{.legacy = true}>;

using BarSetting = common::Setting<
    "-bar", int64_t, common::SettingOptions{.legacy = true}>;

using FoSetting = common::Setting<
    "-fo", bool, common::SettingOptions{.legacy = true}>;

using FooOSetting = common::Setting<
    "-fooo", bool, common::SettingOptions{.legacy = true}>;

using DirSetting = common::Setting<
    "-dir", fs::path, common::SettingOptions{.legacy = true}>;

#endif // BITCOIN_TEST_GETARG_TESTS_SETTINGS_H
