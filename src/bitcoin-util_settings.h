#ifndef BITCOIN_BITCOIN_UTIL_SETTINGS_H
#define BITCOIN_BITCOIN_UTIL_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using VersionSetting = common::Setting<
    "-version", bool, common::SettingOptions{.legacy = true},
    "Print version and exit">;

#endif // BITCOIN_BITCOIN_UTIL_SETTINGS_H
