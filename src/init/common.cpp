// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <clientversion.h>
#include <common/args.h>
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
    argsman.AddArg("-debuglogfile=<file>", strprintf("Specify location of debug log file (default: %s). Relative paths will be prefixed by a net-specific datadir location. Pass -nodebuglogfile to disable writing the log to a file.", DEFAULT_DEBUGLOGFILE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-loglevel=<level>|<category>:<level>[,<category>:<level>...]",
        strprintf("Set the global or per-category severity level for logging. "
            "Possible values for <level> are %s (default: info). <category> can be: %s. "
            "A global level (e.g. -loglevel=debug) enables all categories at that level; "
            "a per-category entry (e.g. -loglevel=net:trace) enables one category. "
            "Multiple entries can be comma-separated or given as separate -loglevel arguments "
            "and later settings override earlier settings.",
            LogInstance().LogLevelsString(), LogInstance().LogCategoriesString()),
        ArgsManager::DISALLOW_ELISION, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-debug=<category>", "Output debug and trace logging (default: -nodebug, supplying <category> is optional). "
        "If <category> is not supplied or if <category> is 1 or \"all\", output all debug logging. If <category> is 0 or \"none\", any other categories are ignored. Other valid values for <category> are: " + LogInstance().LogCategoriesString() + ". This option can be specified multiple times to output multiple categories. "
        "Specifying -debug=1 is equivalent to specifying -loglevel=debug, and -debug=<category> is equivalent to -loglevel=<category>:debug. -loglevel takes precedence over -debug if both are used together.",
        ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-debugexclude=<category>", "Exclude debug and trace logging for a category. Can be used in conjunction with -debug=1 to output debug and trace logging for all categories except the specified category. This option can be specified multiple times to exclude multiple categories. This takes priority over \"-debug\" and \"-loglevel\". "
        "Specifying -debugexclude=<category> is equivalent to specifying -loglevel=category:info",
        ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logips", strprintf("Include IP addresses in log output (default: %u)", DEFAULT_LOGIPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logtimestamps", strprintf("Prepend debug output with timestamp (default: %u)", DEFAULT_LOGTIMESTAMPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logthreadnames", strprintf("Prepend debug output with name of the originating thread (default: %u)", DEFAULT_LOGTHREADNAMES), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logsourcelocations", strprintf("Prepend debug output with name of the originating source location (source file, line number and function name) (default: %u)", DEFAULT_LOGSOURCELOCATIONS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logtimemicros", strprintf("Add microsecond precision to debug timestamps (default: %u)", DEFAULT_LOGTIMEMICROS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-loglevelalways", strprintf("Always prepend a category and level (default: %u)", DEFAULT_LOGLEVELALWAYS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logratelimit", strprintf("Apply rate limiting to unconditional logging to mitigate disk-filling attacks (default: %u)", BCLog::DEFAULT_LOGRATELIMIT), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -daemon. To disable logging to file, set -nodebuglogfile)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-shrinkdebugfile", "Shrink debug log file on client startup (default: 1 when no -debug)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
}

void SetLoggingOptions(const ArgsManager& args)
{
    LogInstance().m_print_to_file = !args.IsArgNegated("-debuglogfile");
    LogInstance().m_file_path = AbsPathForConfigVal(args, args.GetPathArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    LogInstance().m_print_to_console = args.GetBoolArg("-printtoconsole", !args.GetBoolArg("-daemon", false));
    LogInstance().m_log_timestamps = args.GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
    LogInstance().m_log_time_micros = args.GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
    LogInstance().m_log_threadnames = args.GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);
    LogInstance().m_log_sourcelocations = args.GetBoolArg("-logsourcelocations", DEFAULT_LOGSOURCELOCATIONS);
    LogInstance().m_always_print_category_level = args.GetBoolArg("-loglevelalways", DEFAULT_LOGLEVELALWAYS);

    fLogIPs = args.GetBoolArg("-logips", DEFAULT_LOGIPS);
}

util::Result<void> SetLoggingLevel(const ArgsManager& args)
{
    for (const std::string& level_arg : args.GetArgs("-loglevel")) {
        bool seen_per_category = false;
        for (const std::string& token : SplitString(level_arg, ',')) {
            if (token.find(':') == std::string::npos) {
                // Global level token — must come first in a comma-separated value
                if (seen_per_category) {
                    return util::Error{strprintf(_("In %s=%s, global level must be specified before any category-specific levels."), "-loglevel", level_arg)};
                }
                if (!LogInstance().SetLogLevel(token)) {
                    return util::Error{strprintf(_("Unsupported global logging level %s=%s. Valid values: %s."), "-loglevel", token, LogInstance().LogLevelsString())};
                }
                // Global level: reset per-category overrides and flags
                LogInstance().SetCategoryLogLevel({});
                if (LogInstance().LogLevel() >= util::log::Level::Info) {
                    LogInstance().DisableCategory(BCLog::ALL);
                } else {
                    LogInstance().EnableCategory(BCLog::ALL);
                }
            } else {
                // Category-specific level: -loglevel=net:trace (or comma token "net:trace")
                const auto& toks = SplitString(token, ':');
                if (!(toks.size() == 2 && LogInstance().SetCategoryLogLevel(toks[0], toks[1]))) {
                    return util::Error{strprintf(_("Unsupported category-specific logging level %1$s=%2$s. Expected %1$s=<category>:<loglevel>. Valid categories: %3$s. Valid loglevels: %4$s."), "-loglevel", token, LogInstance().LogCategoriesString(), LogInstance().LogLevelsString())};
                }
                if (const auto flag = BCLog::Logger::GetLogCategory(toks[0])) {
                    LogInstance().EnableCategory(*flag);
                }
                seen_per_category = true;
            }
        }
    }
    return {};
}

util::Result<void> SetLoggingCategories(const ArgsManager& args)
{
    const std::vector<std::string> categories = args.GetArgs("-debug");

    // Special-case: Disregard any debugging categories appearing before -debug=0/none
    const auto last_negated = std::find_if(categories.rbegin(), categories.rend(),
                                           [](const std::string& cat) { return cat == "0" || cat == "none"; });

    const auto categories_to_process = (last_negated == categories.rend()) ? categories : std::ranges::subrange(last_negated.base(), categories.end());

    for (const auto& cat : categories_to_process) {
        if (!LogInstance().EnableCategory(cat)) {
            return util::Error{strprintf(_("Unsupported logging category %s=%s."), "-debug", cat)};
        }
    }

    return {};
}

util::Result<void> SetLoggingExcludes(const ArgsManager& args)
{
    for (const std::string& cat : args.GetArgs("-debugexclude")) {
        if (!LogInstance().DisableCategory(cat)) {
            return util::Error{strprintf(_("Unsupported logging category %s=%s."), "-debugexclude", cat)};
        }
    }

    LogInfo("Log output may contain privacy-sensitive information. Be cautious when sharing logs.");

    return {};
}

bool StartLogging(const ArgsManager& args)
{
    if (LogInstance().m_print_to_file) {
        if (args.GetBoolArg("-shrinkdebugfile", LogInstance().DefaultShrinkDebugFile())) {
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
    if (args.IsArgNegated("-conf")) {
        LogInfo("Config file: <disabled>");
    } else if (fs::is_directory(config_file_path)) {
        LogWarning("Config file: %s (is directory, not file)", fs::PathToString(config_file_path));
    } else if (fs::exists(config_file_path)) {
        LogInfo("Config file: %s", fs::PathToString(config_file_path));
    } else if (args.IsArgSet("-conf")) {
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
