// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <clientversion.h>
#include <common/args.h>
#include <init/common_settings.h>
#include <init_settings.h>
#include <logging.h>
#include <node/interface_ui.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/result.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using util::SplitString;

namespace init {
void AddLoggingArgs(ArgsManager& argsman)
{
    DebugLogFileSetting::Register(argsman);
    DebugSetting::Register(argsman);
    DebugExcludeSetting::Register(argsman);
    LogIpsSetting::Register(argsman);
    LogLevelSetting::Register(argsman);
    LogTimestampsSetting::Register(argsman);
    LogThreadNamesSetting::Register(argsman);
    LogSourceLocationsSetting::Register(argsman);
    LogTimeMicrosSetting::Register(argsman);
    LogLevelAlwaysSetting::Register(argsman);
    LogratelimitSetting::Register(argsman);
    PrintToConsoleSetting::Register(argsman);
    ShrinkDebugFileSetting::Register(argsman);
}

void SetLoggingOptions(const ArgsManager& args)
{
    LogInstance().m_print_to_file = !DebugLogFileSetting::Value(args).isFalse();
    LogInstance().m_file_path = AbsPathForConfigVal(args, DebugLogFileSetting::Get(args));
    LogInstance().m_print_to_console = PrintToConsoleSetting::Get(args, !DaemonSetting::Get(args, false));
    LogInstance().m_log_timestamps = LogTimestampsSetting::Get(args);
    LogInstance().m_log_time_micros = LogTimeMicrosSetting::Get(args);
    LogInstance().m_log_threadnames = LogThreadNamesSetting::Get(args);
    LogInstance().m_log_sourcelocations = LogSourceLocationsSetting::Get(args);
    LogInstance().m_always_print_category_level = LogLevelAlwaysSetting::Get(args);

    fLogIPs = LogIpsSetting::Get(args);
}

util::Result<void> SetLoggingLevel(const ArgsManager& args)
{
        for (const std::string& level_str : LogLevelSetting::Get(args)) {
            if (level_str.find_first_of(':', 3) == std::string::npos) {
                // user passed a global log level, i.e. -loglevel=<level>
                if (!LogInstance().SetLogLevel(level_str)) {
                    return util::Error{strprintf(_("Unsupported global logging level %s=%s. Valid values: %s."), "-loglevel", level_str, LogInstance().LogLevelsString())};
                }
            } else {
                // user passed a category-specific log level, i.e. -loglevel=<category>:<level>
                const auto& toks = SplitString(level_str, ':');
                if (!(toks.size() == 2 && LogInstance().SetCategoryLogLevel(toks[0], toks[1]))) {
                    return util::Error{strprintf(_("Unsupported category-specific logging level %1$s=%2$s. Expected %1$s=<category>:<loglevel>. Valid categories: %3$s. Valid loglevels: %4$s."), "-loglevel", level_str, LogInstance().LogCategoriesString(), LogInstance().LogLevelsString())};
                }
            }
        }
    return {};
}

util::Result<void> SetLoggingCategories(const ArgsManager& args)
{
        const std::vector<std::string> categories = DebugSetting::Get(args);

        // Special-case: Disregard any debugging categories appearing before -debug=0/none
        const auto last_negated = std::find_if(categories.rbegin(), categories.rend(),
                                               [](const std::string& cat) { return cat == "0" || cat == "none"; });

        const auto categories_to_process = (last_negated == categories.rend()) ? categories : std::ranges::subrange(last_negated.base(), categories.end());

        for (const auto& cat : categories_to_process) {
            if (!LogInstance().EnableCategory(cat)) {
                return util::Error{strprintf(_("Unsupported logging category %s=%s."), "-debug", cat)};
            }
        }

    // Now remove the logging categories which were explicitly excluded
    for (const std::string& cat : DebugExcludeSetting::Get(args)) {
        if (!LogInstance().DisableCategory(cat)) {
            return util::Error{strprintf(_("Unsupported logging category %s=%s."), "-debugexclude", cat)};
        }
    }
    return {};
}

bool StartLogging(const ArgsManager& args)
{
    if (LogInstance().m_print_to_file) {
        if (ShrinkDebugFileSetting::Get(args)) {
            // Do this first since it both loads a bunch of debug.log into memory,
            // and because this needs to happen before any other debug.log printing
            LogInstance().ShrinkDebugFile();
        }
    }
    if (!LogInstance().StartLogging()) {
            return InitError(Untranslated(strprintf("Could not open debug log file %s",
                fs::PathToString(LogInstance().m_file_path))));
    }

    if (!LogInstance().m_log_timestamps) {
        LogInfo("Startup time: %s", FormatISO8601DateTime(GetTime()));
    }
    LogInfo("Default data directory %s", fs::PathToString(GetDefaultDataDir()));
    LogInfo("Using data directory %s", fs::PathToString(gArgs.GetDataDirNet()));

    // Only log conf file usage message if conf file actually exists.
    fs::path config_file_path = args.GetConfigFilePath();
    if (ConfSetting::Value(args).isFalse()) {
        LogInfo("Config file: <disabled>");
    } else if (fs::is_directory(config_file_path)) {
        LogWarning("Config file: %s (is directory, not file)", fs::PathToString(config_file_path));
    } else if (fs::exists(config_file_path)) {
        LogInfo("Config file: %s", fs::PathToString(config_file_path));
    } else if (!ConfSetting::Value(args).isNull()) {
        InitWarning(strprintf(_("The specified config file %s does not exist"), fs::PathToString(config_file_path)));
    } else {
        // Not categorizing as "Warning" because it's the default behavior
        LogInfo("Config file: %s (not found, skipping)", fs::PathToString(config_file_path));
    }

    // Log the config arguments to debug.log
    args.LogArgs();

    return true;
}

void LogPackageVersion()
{
    std::string version_string = FormatFullVersion();
#ifdef DEBUG
    version_string += " (debug build)";
#else
    version_string += " (release build)";
#endif
    LogInfo(CLIENT_NAME " version %s", version_string);
}
} // namespace init
