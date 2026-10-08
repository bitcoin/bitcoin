#ifndef BITCOIN_QT_TEST_OPTIONTESTS_SETTINGS_H
#define BITCOIN_QT_TEST_OPTIONTESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using ListenSetting = common::Setting<
    "-listen", bool, common::SettingOptions{.legacy = true}>;

using ListenOnionSetting = common::Setting<
    "-listenonion", bool, common::SettingOptions{.legacy = true}>;

#endif // BITCOIN_QT_TEST_OPTIONTESTS_SETTINGS_H
