// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/init.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/system.h>
#include <util/translation.h>

#include <algorithm>
#include <exception>
#include <optional>

namespace common {
std::optional<ConfigError> InitConfig(ArgsManager& args, SettingsAbortFn settings_abort_fn)
{
    try {
        if (!CheckDataDirOption(args)) {
            return ConfigError{ConfigStatus::FAILED, strprintf(_("Specified data directory \"%s\" does not exist."), args.GetArg("-datadir", ""))};
        }
        std::string error;
        if (!args.ReadConfigFiles(error, true)) {
            return ConfigError{ConfigStatus::FAILED, strprintf(_("Error reading configuration file: %s"), error)};
        }

        // Check for chain settings (Params() calls are only valid after this clause)
        SelectParams(args.GetChainName());

        // Create datadir if it does not exist.
        const auto base_path{args.GetDataDirBase()};
        if (!fs::exists(base_path)) {
            // When creating a *new* datadir, also create a "wallets" subdirectory,
            // whether or not the wallet is enabled now, so if the wallet is enabled
            // in the future, it will use the "wallets" subdirectory for creating
            // and listing wallets, rather than the top-level directory where
            // wallets could be mixed up with other files. For backwards
            // compatibility, wallet code will use the "wallets" subdirectory only
            // if it already exists, but never create it itself. There is discussion
            // in https://github.com/bitcoin/bitcoin/issues/16220 about ways to
            // change wallet code so it would no longer be necessary to create
            // "wallets" subdirectories here.
            fs::create_directories(base_path / "wallets");
        }
        const auto net_path{args.GetDataDirNet()};
        if (!fs::exists(net_path)) {
            fs::create_directories(net_path / "wallets");
        }

        // Create settings.json if -nosettings was not specified.
        if (args.GetSettingsPath()) {
            std::vector<std::string> details;
            if (!args.ReadSettingsFile(&details)) {
                const bilingual_str& message = _("Settings file could not be read");
                if (!settings_abort_fn) {
                    return ConfigError{ConfigStatus::FAILED, message, details};
                } else if (settings_abort_fn(message, details)) {
                    return ConfigError{ConfigStatus::ABORTED, message, details};
                } else {
                    details.clear(); // User chose to ignore the error and proceed.
                }
            }
            if (!args.WriteSettingsFile(&details)) {
                const bilingual_str& message = _("Settings file could not be written");
                return ConfigError{ConfigStatus::FAILED_WRITE, message, details};
            }
        }
    } catch (const std::exception& e) {
        return ConfigError{ConfigStatus::FAILED, Untranslated(e.what())};
    }
    return {};
}
} // namespace common
