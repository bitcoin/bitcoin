// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <clientversion.h>
#include <logging.h>
#include <node/interface_ui.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/string.h>
#include <util/system.h>
#include <util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <string>
#include <vector>

namespace init {
void AddLoggingArgs(ArgsManager& argsman)
{
    argsman.AddArg("-debuglogfile=<file>", strprintf("Specify location of debug log file (default: %s). Relative paths will be prefixed by a net-specific datadir location. Pass -nodebuglogfile to disable writing the log to a file.", DEFAULT_DEBUGLOGFILE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-debug=<category>", "Output debug and trace logging (default: -nodebug, supplying <category> is optional). "
        "If <category> is not supplied or if <category> = 1, output all debug and trace logging. <category> can be: " + LogInstance().LogCategoriesString() + ". This option can be specified multiple times to output multiple categories.",
        ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-debugexclude=<category>", "Exclude debug and trace logging for a category. Can be used in conjunction with -debug=1 to output debug and trace logging for all categories except the specified category. This option can be specified multiple times to exclude multiple categories.", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logips", strprintf("Include IP addresses in debug output (default: %u)", DEFAULT_LOGIPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-loglevel=<level>|<category>:<level>", strprintf("Set the global or per-category severity level for logging categories enabled with the -debug configuration option or the logging RPC: %s (default=%s); warning and error levels are always logged. If <category>:<level> is supplied, the setting will override the global one and may be specified multiple times to set multiple category-specific levels. <category> can be: %s.", LogInstance().LogLevelsString(), LogInstance().LogLevelToStr(BCLog::DEFAULT_LOG_LEVEL), LogInstance().LogCategoriesString()), ArgsManager::DISALLOW_NEGATION | ArgsManager::DISALLOW_ELISION | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logtimestamps", strprintf("Prepend debug output with timestamp (default: %u)", DEFAULT_LOGTIMESTAMPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
#ifdef HAVE_THREAD_LOCAL
    argsman.AddArg("-logthreadnames", strprintf("Prepend debug output with name of the originating thread (only available on platforms supporting thread_local) (default: %u)", DEFAULT_LOGTHREADNAMES), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
#else
    argsman.AddHiddenArgs({"-logthreadnames"});
#endif
    argsman.AddArg("-logsourcelocations", strprintf("Prepend debug output with name of the originating source location (source file, line number and function name) (default: %u)", DEFAULT_LOGSOURCELOCATIONS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logtimemicros", strprintf("Add microsecond precision to debug timestamps (default: %u)", DEFAULT_LOGTIMEMICROS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -daemon. To disable logging to file, set -nodebuglogfile)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-shrinkdebugfile", "Shrink debug.log file on client startup (default: 1 when no -debug)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
}

void SetLoggingOptions(const ArgsManager& args)
{
    LogInstance().m_print_to_file = !args.IsArgNegated("-debuglogfile");
    LogInstance().m_file_path = AbsPathForConfigVal(args, args.GetPathArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    LogInstance().m_print_to_console = args.GetBoolArg("-printtoconsole", !args.GetBoolArg("-daemon", false));
    LogInstance().m_log_timestamps = args.GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
    LogInstance().m_log_time_micros = args.GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
#ifdef HAVE_THREAD_LOCAL
    LogInstance().m_log_threadnames = args.GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);
#endif
    LogInstance().m_log_sourcelocations = args.GetBoolArg("-logsourcelocations", DEFAULT_LOGSOURCELOCATIONS);

    fLogIPs = args.GetBoolArg("-logips", DEFAULT_LOGIPS);
}

void SetLoggingLevel(const ArgsManager& args)
{
    if (args.IsArgSet("-loglevel")) {
        for (const std::string& level_str : args.GetArgs("-loglevel")) {
            if (level_str.find_first_of(':', 3) == std::string::npos) {
                // user passed a global log level, i.e. -loglevel=<level>
                if (!LogInstance().SetLogLevel(level_str)) {
                    InitWarning(strprintf(_("Unsupported global logging level -loglevel=%s. Valid values: %s."), level_str, LogInstance().LogLevelsString()));
                }
            } else {
                // user passed a category-specific log level, i.e. -loglevel=<category>:<level>
                const auto& toks = SplitString(level_str, ':');
                if (!(toks.size() == 2 && LogInstance().SetCategoryLogLevel(toks[0], toks[1]))) {
                    InitWarning(strprintf(_("Unsupported category-specific logging level -loglevel=%s. Expected -loglevel=<category>:<loglevel>. Valid categories: %s. Valid loglevels: %s."), level_str, LogInstance().LogCategoriesString(), LogInstance().LogLevelsString()));
                }
            }
        }
    }
}

void SetLoggingCategories(const ArgsManager& args)
{
    if (args.IsArgSet("-debug")) {
        // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
        const std::vector<std::string> categories = args.GetArgs("-debug");

        if (std::none_of(categories.begin(), categories.end(),
            [](std::string cat){return cat == "0" || cat == "none";})) {
            for (const auto& cat : categories) {
                if (!LogInstance().EnableCategory(cat)) {
                    InitWarning(strprintf(_("Unsupported logging category %s=%s."), "-debug", cat));
                }
            }
        }
    }

    // Now remove the logging categories which were explicitly excluded
    for (const std::string& cat : args.GetArgs("-debugexclude")) {
        if (!LogInstance().DisableCategory(cat)) {
            InitWarning(strprintf(_("Unsupported logging category %s=%s."), "-debugexclude", cat));
        }
    }
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
            return InitError(strprintf(Untranslated("Could not open debug log file %s"),
                fs::PathToString(LogInstance().m_file_path)));
    }

    if (!LogInstance().m_log_timestamps)
        LogPrintf("Startup time: %s\n", FormatISO8601DateTime(GetTime()));
    LogPrintf("Default data directory %s\n", fs::PathToString(GetDefaultDataDir()));
    LogPrintf("Using data directory %s\n", fs::PathToString(gArgs.GetDataDirNet()));

    // Only log conf file usage message if conf file actually exists.
    fs::path config_file_path = args.GetConfigFilePath();
    if (fs::exists(config_file_path)) {
        LogPrintf("Config file: %s\n", fs::PathToString(config_file_path));
    } else if (args.IsArgSet("-conf")) {
        // Warn if no conf file exists at path provided by user
        InitWarning(strprintf(_("The specified config file %s does not exist"), fs::PathToString(config_file_path)));
    } else {
        // Not categorizing as "Warning" because it's the default behavior
        LogPrintf("Config file: %s (not found, skipping)\n", fs::PathToString(config_file_path));
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
    LogPrintf(PACKAGE_NAME " version %s\n", version_string);
}
} // namespace init
