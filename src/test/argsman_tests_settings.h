#ifndef BITCOIN_TEST_ARGSMAN_TESTS_SETTINGS_H
#define BITCOIN_TEST_ARGSMAN_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using RegtestSetting = common::Setting<
    "-regtest", common::Unset, common::SettingOptions{.legacy = true},
    "regtest">;

using TestnetSetting = common::Setting<
    "-testnet", common::Unset, common::SettingOptions{.legacy = true},
    "testnet">;

#endif // BITCOIN_TEST_ARGSMAN_TESTS_SETTINGS_H
