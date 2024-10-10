// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <common/init.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/fs.h>
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

        // Record original datadir and config paths before parsing the config
        // file. It is possible for the config file to contain a datadir= line
        // that changes the datadir path after it is parsed. This is useful for
        // CLI tools to let them use a different data storage location without
        // needing to pass it every time on the command line. (It is not
        // possible for the config file to cause another configuration to be
        // used, though. Specifying a conf= option in the config file causes a
        // parse error, and specifying a datadir= location containing another
        // bitcoin.conf file just ignores the other file.)
        const fs::path orig_datadir_path{args.GetDataDirBase()};
        const fs::path orig_config_path{AbsPathForConfigVal(args, args.GetPathArg("-conf", BITCOIN_CONF_FILENAME), /*net_specific=*/false)};

        std::string error;
        if (!args.ReadConfigFiles(error, true)) {
            return ConfigError{ConfigStatus::FAILED, strprintf(_("Error reading configuration file: %s"), error)};
        }

        // Check for chain settings (Params() calls are only valid after this clause)
        SelectParams(args.GetChainType());

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

        // Show an error or warning if there is a bitcoin.conf file in the
        // datadir that is being ignored.
        const fs::path base_config_path = base_path / BITCOIN_CONF_FILENAME;
        if (fs::exists(base_config_path) && !fs::equivalent(orig_config_path, base_config_path)) {
            const std::string cli_config_path = args.GetArg("-conf", "");
            const std::string config_source = cli_config_path.empty()
                ? strprintf("data directory %s", fs::quoted(fs::PathToString(orig_datadir_path)))
                : strprintf("command line argument %s", fs::quoted("-conf=" + cli_config_path));
            const std::string error = strprintf(
                "Data directory %1$s contains a %2$s file which is ignored, because a different configuration file "
                "%3$s from %4$s is being used instead. Possible ways to address this would be to:\n"
                "- Delete or rename the %2$s file in data directory %1$s.\n"
                "- Change datadir= or conf= options to specify one configuration file, not two, and use "
                "includeconf= to include any other configuration files.\n"
                "- Set allowignoredconf=1 option to treat this condition as a warning, not an error.",
                fs::quoted(fs::PathToString(base_path)),
                fs::quoted(BITCOIN_CONF_FILENAME),
                fs::quoted(fs::PathToString(orig_config_path)),
                config_source);
            if (args.GetBoolArg("-allowignoredconf", false)) {
                LogInfo("Warning: %s\n", error);
            } else {
                return ConfigError{ConfigStatus::FAILED, Untranslated(error)};
            }
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
