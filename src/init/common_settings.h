#ifndef BITCOIN_INIT_COMMON_SETTINGS_H
#define BITCOIN_INIT_COMMON_SETTINGS_H

#include <common/setting.h>
#include <logging.h>

#include <string>
#include <vector>

using DebugSetting = common::Setting<
    "-debug=<category>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Output debug and trace logging (default: -nodebug, supplying <category> is optional). "
        "If <category> is not supplied or if <category> is 1 or \"all\", output all debug logging. If <category> is 0 or \"none\", any other categories are ignored. Other valid values for <category> are: %s. This option can be specified multiple times to output multiple categories.">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, LogInstance().LogCategoriesString()); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using DebugSettingBool = common::Setting<
    "-debug=<category>", bool, common::SettingOptions{.legacy = true}>;

using PrinttoconsoleSetting = common::Setting<
    "-printtoconsole", bool, common::SettingOptions{.legacy = true},
    "Send trace/debug info to console (default: 1 when no -daemon. To disable logging to file, set -nodebuglogfile)">
    ::Category<OptionsCategory::DEBUG_TEST>;

using DebuglogfileSetting = common::Setting<
    "-debuglogfile=<file>", fs::path, common::SettingOptions{.legacy = true},
    "Specify location of debug log file (default: %s). Relative paths will be prefixed by a net-specific datadir location. Pass -nodebuglogfile to disable writing the log to a file.">
    ::DefaultFn<[] { return DEFAULT_DEBUGLOGFILE; }>;

using DebugexcludeSetting = common::Setting<
    "-debugexclude=<category>", std::vector<std::string>, common::SettingOptions{.legacy = true},
    "Exclude debug and trace logging for a category. Can be used in conjunction with -debug=1 to output debug and trace logging for all categories except the specified category. This option can be specified multiple times to exclude multiple categories. This takes priority over \"-debug\"">
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogipsSetting = common::Setting<
    "-logips", bool, common::SettingOptions{.legacy = true},
    "Include IP addresses in debug output (default: %u)">
    ::Default<DEFAULT_LOGIPS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LoglevelSetting = common::Setting<
    "-loglevel=<level>|<category>:<level>", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true, .disallow_negation = true, .disallow_elision = true},
    "Set the global or per-category severity level for logging categories enabled with the -debug configuration option or the logging RPC. Possible values are %s (default=%s). The following levels are always logged: error, warning, info. If <category>:<level> is supplied, the setting will override the global one and may be specified multiple times to set multiple category-specific levels. <category> can be: %s.">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, LogInstance().LogLevelsString(), LogInstance().LogLevelToStr(BCLog::DEFAULT_LOG_LEVEL), LogInstance().LogCategoriesString()); }>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogtimestampsSetting = common::Setting<
    "-logtimestamps", bool, common::SettingOptions{.legacy = true},
    "Prepend debug output with timestamp (default: %u)">
    ::Default<DEFAULT_LOGTIMESTAMPS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogthreadnamesSetting = common::Setting<
    "-logthreadnames", bool, common::SettingOptions{.legacy = true},
    "Prepend debug output with name of the originating thread (default: %u)">
    ::Default<DEFAULT_LOGTHREADNAMES>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogsourcelocationsSetting = common::Setting<
    "-logsourcelocations", bool, common::SettingOptions{.legacy = true},
    "Prepend debug output with name of the originating source location (source file, line number and function name) (default: %u)">
    ::Default<DEFAULT_LOGSOURCELOCATIONS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LogtimemicrosSetting = common::Setting<
    "-logtimemicros", bool, common::SettingOptions{.legacy = true, .debug_only = true},
    "Add microsecond precision to debug timestamps (default: %u)">
    ::Default<DEFAULT_LOGTIMEMICROS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using LoglevelalwaysSetting = common::Setting<
    "-loglevelalways", bool, common::SettingOptions{.legacy = true},
    "Always prepend a category and level (default: %u)">
    ::Default<DEFAULT_LOGLEVELALWAYS>
    ::Category<OptionsCategory::DEBUG_TEST>;

using ShrinkdebugfileSetting = common::Setting<
    "-shrinkdebugfile", bool, common::SettingOptions{.legacy = true},
    "Shrink debug.log file on client startup (default: 1 when no -debug)">
    ::DefaultFn<[] { return LogInstance().DefaultShrinkDebugFile(); }>
    ::HelpArgs<>
    ::Category<OptionsCategory::DEBUG_TEST>;

#endif // BITCOIN_INIT_COMMON_SETTINGS_H
