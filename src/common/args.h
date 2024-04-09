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
std::optional<std::string> SettingToString(const common::SettingsValue&);

int64_t SettingToInt(const common::SettingsValue&, int64_t);
std::optional<int64_t> SettingToInt(const common::SettingsValue&);

bool SettingToBool(const common::SettingsValue&, bool);
std::optional<bool> SettingToBool(const common::SettingsValue&);

class ArgsManager
{
public:
    /**
     * Flags controlling how config and command line arguments are validated and
     * interpreted.
     */
    enum Flags : uint32_t {
        ALLOW_ANY = 0x01,         //!< disable validation
        // ALLOW_BOOL = 0x02,     //!< unimplemented, draft implementation in #16545
        // ALLOW_INT = 0x04,      //!< unimplemented, draft implementation in #16545
        // ALLOW_STRING = 0x08,   //!< unimplemented, draft implementation in #16545
        // ALLOW_LIST = 0x10,     //!< unimplemented, draft implementation in #16545
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
    /**
     * Get setting value.
     *
     * Result will be null if setting was unset, true if "-setting" argument was passed
     * false if "-nosetting" argument was passed, and a string if a "-setting=value"
     * argument was passed.
     */
    common::SettingsValue GetSetting(const std::string& arg) const;

    /**
     * Get list of setting values.
     */
    std::vector<common::SettingsValue> GetSettingsList(const std::string& arg) const;

    ArgsManager();
    ~ArgsManager();

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
     * Get signet data directory path.
     * If a signet-challange argument is provided, it is used in constructing the directory path.
     *
     * @return The path to the signet data directory.
    */
    fs::path GetSignetDataDir() const;

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
    std::optional<std::string> GetArg(const std::string& strArg) const;

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
    std::optional<int64_t> GetIntArg(const std::string& strArg) const;

    /**
     * Return boolean argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param fDefault (true or false)
     * @return command-line argument or default value
     */
    bool GetBoolArg(const std::string& strArg, bool fDefault) const;
    std::optional<bool> GetBoolArg(const std::string& strArg) const;

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
    /**
     * Get data directory path
     *
     * @param net_specific Append network identifier to the returned path
     * @return Absolute path on success, otherwise an empty path when a non-directory path would be returned
     */
    fs::path GetDataDir(bool net_specific) const;

    /**
     * Return -regtest/-signet/-testnet/-chain= setting as a ChainType enum if a
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
