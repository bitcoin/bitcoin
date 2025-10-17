#ifndef BITCOIN_BITCOIN_WALLET_SETTINGS_H
#define BITCOIN_BITCOIN_WALLET_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using VersionSetting = common::Setting<
    "-version", bool, common::SettingOptions{.legacy = true},
    "Print version and exit">;

using DataDirSetting = common::Setting<
    "-datadir=<dir>", std::string, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Specify data directory">;

using WalletSetting = common::Setting<
    "-wallet=<wallet-name>", std::string, common::SettingOptions{.legacy = true, .network_only = true},
    "Specify wallet name">;

using DumpFileSetting = common::Setting<
    "-dumpfile=<file name>", std::string, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "When used with 'dump', writes out the records to this file. When used with 'createfromdump', loads the records into a new wallet.">;

using DebugSetting = common::Setting<
    "-debug=<category>", bool, common::SettingOptions{.legacy = true},
    "Output debugging information (default: 0).">
    ::Category<OptionsCategory::DEBUG_TEST>;

using PrintToConsoleSetting = common::Setting<
    "-printtoconsole", bool, common::SettingOptions{.legacy = true},
    "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise).">
    ::Category<OptionsCategory::DEBUG_TEST>;

#endif // BITCOIN_BITCOIN_WALLET_SETTINGS_H
