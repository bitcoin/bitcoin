#ifndef BITCOIN_BITCOIN_SETTINGS_H
#define BITCOIN_BITCOIN_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using IpcconnectSetting = common::Setting<
    "-ipcconnect", common::Unset, common::SettingOptions{.legacy = true}>;

using IpcfdSetting = common::Setting<
    "-ipcfd", common::Unset, common::SettingOptions{.legacy = true}>;

#endif // BITCOIN_BITCOIN_SETTINGS_H
