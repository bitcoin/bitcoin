// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_ARGS_H
#define BITCOIN_COMMON_ARGS_H

#include <common/settings.h>
#include <compat/compat.h>
#include <sync.h>
#include <util/chaintype.h>
#include <util/fs.h>

#include <iosfwd>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <stdint.h>
#include <string>
#include <variant>
#include <vector>

class ArgsManager;

extern const char * const BITCOIN_CONF_FILENAME;
extern const char * const BITCOIN_SETTINGS_FILENAME;

// Return true if -datadir option points to a valid directory or is not specified.
bool CheckDataDirOption(const ArgsManager& args);

/**
 * Most paths passed as configuration arguments are treated as relative to
 * the datadir if they are not absolute.
 *
 * @param args Parsed arguments and settings.
 * @param path The path to be conditionally prefixed with datadir.
 * @param net_specific Use network specific datadir variant
 * @return The normalized path.
 */
fs::path AbsPathForConfigVal(const ArgsManager& args, const fs::path& path, bool net_specific = true);

inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

enum class OptionsCategory {
    OPTIONS,
    CONNECTION,
    WALLET,
    WALLET_DEBUG_TEST,
    ZMQ,
    DEBUG_TEST,
    CHAINPARAMS,
    NODE_RELAY,
    BLOCK_CREATION,
    RPC,
    GUI,
    COMMANDS,
    REGISTER_COMMANDS,
    CLI_COMMANDS,
    IPC,

    HIDDEN // Always the last option to avoid printing these in the help
};

struct KeyInfo {
    std::string name;
    std::string section;
    bool negated{false};
};

KeyInfo InterpretKey(std::string key);

std::optional<common::SettingsValue> InterpretValue(const KeyInfo& key, const std::string* value,
                                                         unsigned int flags, std::string& error);

struct SectionInfo {
    std::string m_name;
    std::string m_file;
    int m_line;
};

std::string SettingToString(const common::SettingsValue&, const std::string&);
int64_t SettingToInt(const common::SettingsValue&, int64_t);
bool SettingToBool(const common::SettingsValue&, bool);

class ArgsManager
{
public:
    /**
     * Flags controlling how config and command line arguments are parsed.
     *
     * The flags below provide very basic type checking, designed to catch
     * obvious configuration mistakes and provide helpful error messages.
     * Specifying these flags is not a substitute for actually validating
     * setting values that are parsed and making sure they are legitimate.
     *
     * Summary of recommended flags:
     *
     * - For most settings, just use standalone ALLOW_BOOL, ALLOW_INT, or
     *   ALLOW_STRING flags.
     *
     * - If your setting accepts multiple values and you want to read all the
     *   values, not just the last value, add | ALLOW_LIST to the flags.
     *
     * - Only use the DISALLOW_NEGATION flag if your setting really cannot
     *   function without a value, so the command line interface will generally
     *   support negation and be more consistent.
     *
     * Detailed description of flags:
     *
     * The ALLOW_STRING, ALLOW_INT, and ALLOW_BOOL flags control what syntaxes are
     * accepted, according to the following chart:
     *
     * | Syntax   | STRING | INT | BOOL |
     * | -------- | :----: | :-: | :--: |
     * | -foo=abc |   X    |     |      |
     * | -foo=123 |   X    |  X  |      |
     * | -foo=0   |   X    |  X  |  X   |
     * | -foo=1   |   X    |  X  |  X   |
     * | -foo     |        |     |  X   |
     * | -foo=    |   X    |  X  |  X   |
     * | -nofoo   |   X    |  X  |  X   |
     * | -nofoo=1 |   X    |  X  |  X   |
     *
     * Once validated, settings can be retrieved by called GetSetting(),
     * GetArg(), GetIntArg(), and GetBoolArg(). GetSetting() is the most general
     * way to access settings, returning them as JSON values. The other
     * functions just wrap GetSetting() for convenience.
     *
     * As can be seen in the chart, the default behavior of the flags is not
     * very restrictive, although it can be restricted further. It tries to
     * accommodate parsing command lines and configuration files written by
     * human beings, not just machines, understanding that users may have
     * different configuration styles and debugging needs. So the flags do not
     * mandate one way to configure things or try to prevent every possible
     * error, but instead catch the most common and blatant errors, and allow
     * application code to impose additional restrictions, since application
     * code needs to parse settings and reject invalid values anyway.
     *
     * Specifically, by default:
     *
     * - All settings can be specified multiple times, not just ALLOW_LIST
     *   settings. This allows users to override the config file from the
     *   command line, and override earlier command line settings with later
     *   ones. Application code can disable this behavior by calling the
     *   GetArgs() function and raising an error if more than one value is
     *   returned.
     *
     * - All settings can be negated. This provides a consistent command line
     *   interface where settings support -nofoo syntax when meaningful.
     *   GetSetting() returns a false JSON value for negated settings, and
     *   GetArg(), GetIntArg(), and GetBoolArg() return "", 0, and false
     *   respectively. Application code can disallow negation by specifying the
     *   DISALLOW_NEGATION flag, or just handling "", 0, and false values and
     *   rejecting them if they do not make sense.
     *
     * - All settings can be empty. Since all settings are optional, it is
     *   useful to have a way to set them, and a way to unset them. It is also
     *   unambiguous in most cases to treat empty -foo= syntax as not setting a
     *   value, so by default this syntax is allowed and causes GetSetting() to
     *   return JSON "", and GetArg(), GetIntArg() and GetBoolArg() to return
     *   std::nullopt. Application code can override this behavior by rejecting
     *   these values.
     */
    enum Flags : uint32_t {
        ALLOW_ANY = 0x01,         //!< allow any argument value (no type checking)
        ALLOW_BOOL = 0x02,        //!< allow -foo=1, -foo=0, -foo, -nofoo, -nofoo=1, and -foo=
        ALLOW_INT = 0x04,         //!< allow -foo=123, -nofoo, -nofoo=1, and -foo=
        ALLOW_STRING = 0x08,      //!< allow -foo=abc, -nofoo, -nofoo=1, and -foo=
        ALLOW_LIST = 0x10,        //!< allow multiple -foo=bar -foo=baz values
        DISALLOW_NEGATION = 0x20, //!< disallow -nofoo syntax
        DISALLOW_ELISION = 0x40,  //!< disallow -foo syntax that doesn't assign any value

        DEBUG_ONLY = 0x100,
        /* Some options would cause cross-contamination if values for
         * mainnet were used while running on regtest/testnet (or vice-versa).
         * Setting them as NETWORK_ONLY ensures that sharing a config file
         * between mainnet and regtest/testnet won't cause problems due to these
         * parameters by accident. */
        NETWORK_ONLY = 0x200,
        // This argument's value is sensitive (such as a password).
        SENSITIVE = 0x400,
        COMMAND = 0x800,
    };

protected:
    struct Arg
    {
        std::string m_help_param;
        std::string m_help_text;
        unsigned int m_flags;
    };

    mutable RecursiveMutex cs_args;
    common::Settings m_settings GUARDED_BY(cs_args);
    std::vector<std::string> m_command GUARDED_BY(cs_args);
    std::string m_network GUARDED_BY(cs_args);
    std::set<std::string> m_network_only_args GUARDED_BY(cs_args);
    std::map<OptionsCategory, std::map<std::string, Arg>> m_available_args GUARDED_BY(cs_args);
    bool m_accept_any_command GUARDED_BY(cs_args){true};
    std::list<SectionInfo> m_config_sections GUARDED_BY(cs_args);
    std::optional<fs::path> m_config_path GUARDED_BY(cs_args);
    mutable fs::path m_cached_blocks_path GUARDED_BY(cs_args);
    mutable fs::path m_cached_datadir_path GUARDED_BY(cs_args);
    mutable fs::path m_cached_network_datadir_path GUARDED_BY(cs_args);

    [[nodiscard]] bool ReadConfigStream(std::istream& stream, const std::string& filepath, std::string& error, bool ignore_invalid_keys = false);

    /**
     * Returns true if settings values from the default section should be used,
     * depending on the current network and whether the setting is
     * network-specific.
     */
    bool UseDefaultSection(const std::string& arg) const EXCLUSIVE_LOCKS_REQUIRED(cs_args);

 public:
    ArgsManager();
    ~ArgsManager();

    /**
     * @name GetArg Functions
     *
     * GetArg functions are an easy way to access most settings. They are
     * wrappers around the lower-level GetSetting() function that provide
     * greater convenience.
     *
     * Examples:
     *
     *     GetArg("-foo")     // returns "abc" if -foo=abc was specified, or nullopt if unset
     *     GetIntArg("-foo")  // returns 123 if -foo=123 was specified, or nullopt if unset
     *     GetBoolArg("-foo") // returns true if -foo was specified, or nullopt if unset
     *     GetBoolArg("-foo") // returns false if -nofoo was specified, or nullopt if unset
     *
     * If no type flags (ALLOW_STRING, ALLOW_INT, or ALLOW_BOOL) are set, GetArg
     * functions do many type coercions and can have surprising behaviors which
     * legacy code relies on, like parsing -nofoo as string "0" or -foo=true as
     * boolean false.
     *
     * If any type flags are set, then:
     *
     * - Only GetArg functions with types matching the flags can be called. For
     *   example, it is an error to call GetIntArg() if ALLOW_INT is not set.
     *
     * - GetArg functions act like std::get_if<T>(), returning null if the
     *   requested type is not available or the setting is unspecified or empty.
     *
     * - "Widening" type conversions from smaller to bigger types are done if
     *   unambiguous (bool -> int -> string). For example, if settings.json
     *   contains {"foo":123}, GetArg("-foo") will return "123". If it contains
     *   {"foo":true}, GetIntArg("-foo") will return 1.
     *
     * - "Narrowing" type conversions in the other direction are not done even
     *   when they would be unambiguous. For example, if settings.json contains
     *   {"foo":"abc"} or {"foo":"123"} GetIntArg("-foo") will return nullopt in
     *   both cases.
     *
     * More examples of GetArg function usage can be found in the
     * @ref example_options::ReadOptions() function in
     * @ref argsman_tests.cpp
     *@{*/
    std::optional<std::string> GetArg(const std::string& strArg) const;
    std::optional<int64_t> GetIntArg(const std::string& strArg) const;
    std::optional<bool> GetBoolArg(const std::string& strArg) const;
    /**@}*/

    /**
     * Get setting value.
     *
     * Result will be null if setting was unspecified, true if `-setting`
     * argument was passed, false if `-nosetting` argument was passed, and will
     * be a string, integer, or boolean if a `-setting=value` argument was
     * passed (which of the three depends on ALLOW_STRING, ALLOW_INT, and
     * ALLOW_BOOL flags). See \ref InterpretValue for a full description of how
     * command line and configuration strings map to JSON values.
     */
    common::SettingsValue GetSetting(const std::string& arg) const;

    /**
     * Get list of setting values.
     */
    std::vector<common::SettingsValue> GetSettingsList(const std::string& arg) const;

    /**
     * Select the network in use
     */
    void SelectConfigNetwork(const std::string& network);

    [[nodiscard]] bool ParseParameters(int argc, const char* const argv[], std::string& error);

    /**
     * Return config file path (read-only)
     */
    fs::path GetConfigFilePath() const;
    void SetConfigFilePath(fs::path);
    [[nodiscard]] bool ReadConfigFiles(std::string& error, bool ignore_invalid_keys = false);

    /**
     * Log warnings for options in m_section_only_args when
     * they are specified in the default section but not overridden
     * on the command line or in a network-specific section in the
     * config file.
     */
    std::set<std::string> GetUnsuitableSectionOnlyArgs() const;

    /**
     * Log warnings for unrecognized section names in the config file.
     */
    std::list<SectionInfo> GetUnrecognizedSections() const;

    struct Command {
        /** The command (if one has been registered with AddCommand), or empty */
        std::string command;
        /**
         * If command is non-empty: Any args that followed it
         * If command is empty: The unregistered command and any args that followed it
         */
        std::vector<std::string> args;
    };
    /**
     * Get the command and command args (returns std::nullopt if no command provided)
     */
    std::optional<const Command> GetCommand() const;

    /**
     * Get blocks directory path
     *
     * @return Blocks path which is network specific
     */
    fs::path GetBlocksDirPath() const;

    /**
     * Get data directory path
     *
     * @return Absolute path on success, otherwise an empty path when a non-directory path would be returned
     */
    fs::path GetDataDirBase() const { return GetDataDir(false); }

    /**
     * Get data directory path with appended network identifier
     *
     * @return Absolute path on success, otherwise an empty path when a non-directory path would be returned
     */
    fs::path GetDataDirNet() const { return GetDataDir(true); }

    /**
     * Clear cached directory paths
     */
    void ClearPathCache();

    /**
     * Return a vector of strings of the given argument
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return command-line arguments
     */
    std::vector<std::string> GetArgs(const std::string& strArg) const;

    /**
     * Return true if the given argument has been manually set
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return true if the argument has been set
     */
    bool IsArgSet(const std::string& strArg) const;

    /**
     * Return true if the argument was originally passed as a negated option,
     * i.e. -nofoo.
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return true if the argument was passed negated
     */
    bool IsArgNegated(const std::string& strArg) const;

    /**
     * Return string argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param strDefault (e.g. "1")
     * @return command-line argument or default value
     */
    std::string GetArg(const std::string& strArg, const std::string& strDefault) const;

    /**
     * Return path argument or default value
     *
     * @param arg Argument to get a path from (e.g., "-datadir", "-blocksdir" or "-walletdir")
     * @param default_value Optional default value to return instead of the empty path.
     * @return normalized path if argument is set, with redundant "." and ".."
     * path components and trailing separators removed (see patharg unit test
     * for examples or implementation for details). If argument is empty or not
     * set, default_value is returned unchanged.
     */
    fs::path GetPathArg(std::string arg, const fs::path& default_value = {}) const;

    /**
     * Return integer argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param nDefault (e.g. 1)
     * @return command-line argument (0 if invalid number) or default value
     */
    int64_t GetIntArg(const std::string& strArg, int64_t nDefault) const;

    /**
     * Return boolean argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param fDefault (true or false)
     * @return command-line argument or default value
     */
    bool GetBoolArg(const std::string& strArg, bool fDefault) const;

    /**
     * Set an argument if it doesn't already have a value
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param strValue Value (e.g. "1")
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetArg(const std::string& strArg, const std::string& strValue);

    /**
     * Set a boolean argument if it doesn't already have a value
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param fValue Value (e.g. false)
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetBoolArg(const std::string& strArg, bool fValue);

    // Forces an arg setting. Called by SoftSetArg() if the arg hasn't already
    // been set. Also called directly in testing.
    void ForceSetArg(const std::string& strArg, const std::string& strValue);

    /**
     * Returns the appropriate chain type from the program arguments.
     * @return ChainType::MAIN by default; raises runtime error if an invalid
     * combination, or unknown chain is given.
     */
    ChainType GetChainType() const;

    /**
     * Returns the appropriate chain type string from the program arguments.
     * @return ChainType::MAIN string by default; raises runtime error if an
     * invalid combination is given.
     */
    std::string GetChainTypeString() const;

    /**
     * Add argument
     */
    void AddArg(const std::string& name, const std::string& help, unsigned int flags, const OptionsCategory& cat);

    /**
     * Add subcommand
     */
    void AddCommand(const std::string& cmd, const std::string& help);

    /**
     * Add many hidden arguments
     */
    void AddHiddenArgs(const std::vector<std::string>& args);

    /**
     * Clear available arguments
     */
    void ClearArgs() {
        LOCK(cs_args);
        m_available_args.clear();
        m_network_only_args.clear();
    }

    /**
     * Check CLI command args
     *
     * @throws std::runtime_error when multiple CLI_COMMAND arguments are specified
     */
    void CheckMultipleCLIArgs() const;

    /**
     * Get the help string
     */
    std::string GetHelpMessage() const;

    /**
     * Return Flags for known arg.
     * Return nullopt for unknown arg.
     */
    std::optional<unsigned int> GetArgFlags(const std::string& name) const;

    /**
     * Get settings file path, or return false if read-write settings were
     * disabled with -nosettings.
     */
    bool GetSettingsPath(fs::path* filepath = nullptr, bool temp = false, bool backup = false) const;

    /**
     * Read settings file. Push errors to vector, or log them if null.
     */
    bool ReadSettingsFile(std::vector<std::string>* errors = nullptr);

    /**
     * Write settings file or backup settings file. Push errors to vector, or
     * log them if null.
     */
    bool WriteSettingsFile(std::vector<std::string>* errors = nullptr, bool backup = false) const;

    /**
     * Get current setting from config file or read/write settings file,
     * ignoring nonpersistent command line or forced settings values.
     */
    common::SettingsValue GetPersistentSetting(const std::string& name) const;

    /**
     * Access settings with lock held.
     */
    template <typename Fn>
    void LockSettings(Fn&& fn)
    {
        LOCK(cs_args);
        fn(m_settings);
    }

    /**
     * Log the config file options and the command line arguments,
     * useful for troubleshooting.
     */
    void LogArgs() const;

private:
    bool CheckArgFlags(const std::string& name, uint32_t require, uint32_t forbid, const char* context) const;

    /**
     * Get data directory path
     *
     * @param net_specific Append network identifier to the returned path
     * @return Absolute path on success, otherwise an empty path when a non-directory path would be returned
     */
    fs::path GetDataDir(bool net_specific) const;

    /**
     * Return -regtest/-signet/-testnet/-testnet4/-chain= setting as a ChainType enum if a
     * recognized chain type was set, or as a string if an unrecognized chain
     * name was set. Raise an exception if an invalid combination of flags was
     * provided.
     */
    std::variant<ChainType, std::string> GetChainArg() const;

    // Helper function for LogArgs().
    void logArgsPrefix(
        const std::string& prefix,
        const std::string& section,
        const std::map<std::string, std::vector<common::SettingsValue>>& args) const;
};

//! Whether the type of the argument has been specified and extra validation
//! rules should apply.
inline bool IsTypedArg(uint32_t flags)
{
    return flags & (ArgsManager::ALLOW_BOOL | ArgsManager::ALLOW_INT |  ArgsManager::ALLOW_STRING);
}

extern ArgsManager gArgs;

/**
 * @return true if help has been requested via a command-line arg
 */
bool HelpRequested(const ArgsManager& args);

/** Add help options to the args manager */
void SetupHelpOptions(ArgsManager& args);

extern const std::vector<std::string> TEST_OPTIONS_DOC;

/** Checks if a particular test option is present in -test command-line arg options */
bool HasTestOption(const ArgsManager& args, const std::string& test_option);

/**
 * Format a string to be used as group of options in help messages
 *
 * @param message Group name (e.g. "RPC server options:")
 * @return the formatted string
 */
std::string HelpMessageGroup(const std::string& message);

/**
 * Format a string to be used as option description in help messages
 *
 * @param option Option message (e.g. "-rpcuser=<user>")
 * @param message Option description (e.g. "Username for JSON-RPC connections")
 * @return the formatted string
 */
std::string HelpMessageOpt(const std::string& option, const std::string& message);

namespace common {
#ifdef WIN32
class WinCmdLineArgs
{
public:
    WinCmdLineArgs();
    ~WinCmdLineArgs();
    std::pair<int, char**> get();

private:
    int argc;
    char** argv;
    std::vector<std::string> args;
};
#endif
} // namespace common

#endif // BITCOIN_COMMON_ARGS_H
