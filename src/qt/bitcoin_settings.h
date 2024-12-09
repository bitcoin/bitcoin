#ifndef BITCOIN_QT_BITCOIN_SETTINGS_H
#define BITCOIN_QT_BITCOIN_SETTINGS_H

#include <common/setting.h>
#include <qt/bitcoingui.h>
#include <qt/guiconstants.h>
#include <qt/intro.h>

#include <string>
#include <vector>

using ChoosedatadirSetting = common::Setting<
    "-choosedatadir", bool, common::SettingOptions{.legacy = true},
    "Choose data directory on startup (default: %u)">
    ::Default<DEFAULT_CHOOSE_DATADIR>
    ::Category<OptionsCategory::GUI>;

using LangSetting = common::Setting<
    "-lang=<lang>", std::string, common::SettingOptions{.legacy = true},
    "Set language, for example \"de_DE\" (default: system locale)">
    ::Category<OptionsCategory::GUI>;

using MinSetting = common::Setting<
    "-min", bool, common::SettingOptions{.legacy = true},
    "Start minimized">
    ::Category<OptionsCategory::GUI>;

using ResetguisettingsSetting = common::Setting<
    "-resetguisettings", bool, common::SettingOptions{.legacy = true},
    "Reset all settings changed in the GUI">
    ::Category<OptionsCategory::GUI>;

using SplashSetting = common::Setting<
    "-splash", bool, common::SettingOptions{.legacy = true},
    "Show splash screen on startup (default: %u)">
    ::Default<DEFAULT_SPLASHSCREEN>
    ::Category<OptionsCategory::GUI>;

using UiplatformSetting = common::Setting<
    "-uiplatform", std::string, common::SettingOptions{.legacy = true, .debug_only = true},
    "Select platform to customize UI for (one of windows, macosx, other; default: %s)">
    ::DefaultFn<[] { return BitcoinGUI::DEFAULT_UIPLATFORM; }>
    ::Category<OptionsCategory::GUI>;

#endif // BITCOIN_QT_BITCOIN_SETTINGS_H
