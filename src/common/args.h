// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_COMMON_ARGS_H
#define BITCOIN_COMMON_ARGS_H
#include <common/settings.h>
#include <compat/compat.h>
#include <sync.h>
#include <util/chaintype.h>
#include <util/fs.h>
#include <cstdint>
#include <iosfwd>
#include <list>
#include <map>
#include <optional>
#include <set>
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
     * - If your setting causes a new action to be performed, and does not
     *   require a value, add | ALLOW_BOOL to the flags. Adding it just allows
     *   the setting to be specified alone on the command line without a value,
     *   as "-foo" instead of "-foo=value".
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
     * | Syntax   | STRING | INT | BOOL | STRING\|BOOL | INT\|BOOL |
     * | -------- | :----: | :-: | :--: | :----------: | :-------: |
     * | -foo=abc |   X    |     |      |      X       |           |
     * | -foo=123 |   X    |  X  |      |      X       |     X     |
     * | -foo=0   |   X    |  X  |  X   |      X       |     X     |
     * | -foo=1   |   X    |  X  |  X   |      X       |     X     |
     * | -foo     |        |     |  X   |      X       |     X     |
     * | -foo=    |   X    |  X  |  X   |      X       |     X     |
     * | -nofoo   |   X    |  X  |  X   |      X       |     X     |
     * | -nofoo=1 |   X    |  X  |  X   |      X       |     X     |
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
        ALLOW_ANY = 0x01,         //!< disable validation
        // ALLOW_BOOL = 0x02,     //!< unimplemented, draft implementation in #16545
        // ALLOW_INT = 0x04,      //!< unimplemented, draft implementation in #16545
        // ALLOW_STRING = 0x08,   //!< unimplemented, draft implementation in #16545
        // ALLOW_LIST = 0x10,     //!< unimplemented, draft implementation in #16545
        ALLOW_ANY = 0x01,         //!< allow any argument value (no type checking)
        ALLOW_BOOL = 0x02,        //!< allow -foo=1, -foo=0, -foo, -nofoo, -nofoo=1, and -foo=
        ALLOW_INT = 0x04,         //!< allow -foo=123, -nofoo, -nofoo=1, and -foo=
        ALLOW_STRING = 0x08,      //!< allow -foo=abc, -nofoo, -nofoo=1, and -foo=
        ALLOW_LIST = 0x10,        //!< allow multiple -foo=bar -foo=baz values
        DISALLOW_NEGATION = 0x20, //!< disallow -nofoo syntax
        DISALLOW_ELISION = 0x40,  //!< disallow -foo syntax that doesn't assign any value

@@ -154,12 +226,73 @@ class ArgsManager
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
     *   when they would be unambiguous. This makes it possible to distinguish
     *   values by type by checking for narrow types first. For example, to
     *   handle boolean settings.json or command line values (-foo -nofoo)
     *   differently than string values (-foo=abc), you can write:
     *
     *       if (auto foo{args.GetBoolArg("-foo")}) {
     *           // handle -foo or -nofoo bool in foo.value()
     *       } else if (auto foo{args.GetArg("-foo")}) {
     *           // handle -foo=abc string in foo.value()
     *       } else {
     *           // handle unset setting
     *       }
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
     * Result will be null if setting was unset, true if "-setting" argument was passed
     * false if "-nosetting" argument was passed, and a string if a "-setting=value"
     * argument was passed.
     * Result will be null if setting was unspecified, true if `-setting`
     * argument was passed, false if `-nosetting` argument was passed, and will
     * be a string, integer, or boolean if a `-setting=value` argument was
     * passed (which of the three depends on ALLOW_STRING, ALLOW_INT, and
     * ALLOW_BOOL flags). See \ref InterpretValue for a full description of how
     * command line and configuration strings map to JSON values.
     */
    common::SettingsValue GetSetting(const std::string& arg) const;

@@ -168,9 +301,6 @@ class ArgsManager
     */
    std::vector<common::SettingsValue> GetSettingsList(const std::string& arg) const;

    ArgsManager();
    ~ArgsManager();

    /**
     * Select the network in use
     */
@@ -271,7 +401,6 @@ class ArgsManager
     * @return command-line argument or default value
     */
    std::string GetArg(const std::string& strArg, const std::string& strDefault) const;
    std::optional<std::string> GetArg(const std::string& strArg) const;

    /**
     * Return path argument or default value
@@ -293,7 +422,6 @@ class ArgsManager
     * @return command-line argument (0 if invalid number) or default value
     */
    int64_t GetIntArg(const std::string& strArg, int64_t nDefault) const;
    std::optional<int64_t> GetIntArg(const std::string& strArg) const;

    /**
     * Return boolean argument or default value
@@ -303,7 +431,6 @@ class ArgsManager
     * @return command-line argument or default value
     */
    bool GetBoolArg(const std::string& strArg, bool fDefault) const;
    std::optional<bool> GetBoolArg(const std::string& strArg) const;

    /**
     * Set an argument if it doesn't already have a value
@@ -423,6 +550,8 @@ class ArgsManager
    void LogArgs() const;

private:
    bool CheckArgFlags(const std::string& name, uint32_t require, uint32_t forbid, const char* context) const;

    /**
     * Get data directory path
     *
@@ -446,6 +575,13 @@ class ArgsManager
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
