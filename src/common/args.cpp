// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <common/args.h>
#include <chainparamsbase.h>
#include <common/settings.h>
#include <logging.h>
#include <sync.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/strencodings.h>
#include <util/string.h>
#ifdef WIN32
#include <codecvt>
#include <shellapi.h>
#include <shlobj.h>
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
const char * const BITCOIN_CONF_FILENAME = "bitcoin.conf";
const char * const BITCOIN_SETTINGS_FILENAME = "settings.json";
ArgsManager gArgs;
/**
 * Interpret a string argument as a boolean.
 *
 * The definition of LocaleIndependentAtoi<int>() requires that non-numeric string values
 * like "foo", return 0. This means that if a user unintentionally supplies a
 * non-integer argument here, the return value is always false. This means that
 * -foo=false does what the user probably expects, but -foo=true is well defined
 * but does not do what they probably expected.
 *
 * The return value of LocaleIndependentAtoi<int>(...) is zero when given input not
 * representable as an int.
 *
 * For a more extensive discussion of this topic (and a wide range of opinions
 * on the Right Way to change this code), see PR12713.
 */
static bool InterpretBool(const std::string& strValue)
{
    if (strValue.empty())
        return true;
    return (LocaleIndependentAtoi<int>(strValue) != 0);
}
static std::string SettingName(const std::string& arg)
{
    return arg.size() > 0 && arg[0] == '-' ? arg.substr(1) : arg;
}
/**
 * Parse "name", "section.name", "noname", "section.noname" settings keys.
 *
 * @note Where an option was negated can be later checked using the
 * IsArgNegated() method. One use case for this is to have a way to disable
 * options that are not normally boolean (e.g. using -nodebuglogfile to request
 * that debug log output is not sent to any file at all).
 */
KeyInfo InterpretKey(std::string key)
{
    KeyInfo result;
    // Split section name from key name for keys like "testnet.foo" or "regtest.bar"
    size_t option_index = key.find('.');
    if (option_index != std::string::npos) {
        result.section = key.substr(0, option_index);
        key.erase(0, option_index + 1);
    }
    if (key.starts_with("no")) {
        key.erase(0, 2);
        result.negated = true;
    }
    result.name = key;
    return result;
}
/**
 * Interpret settings value based on registered flags.
 *
 * @param[in]   key      key information to know if key was negated
 * @param[in]   value    string value of setting to be parsed
 * @param[in]   flags    ArgsManager registered argument flags
 * @param[out]  error    Error description if settings value is not valid
 *
 * @return parsed settings value if it is valid, otherwise nullopt accompanied
 * @return parsed settings value if it is valid, otherwise `nullopt` accompanied
 * by a descriptive error string
 *
 * @note By design, the \ref InterpretValue function does mostly lossless
 * conversions of command line arguments and configuration file values to JSON
 * `common::SettingsValue` values, so higher level application code and GetArg
 * helper methods can unambiguously determine original configuration strings
 * from the JSON values, and flexibly interpret settings and provide good error
 * feedback. Specifically:
 * \n
 * - JSON `null` value is never returned and is reserved for settings that were
 *   not configured at all.
 *
 * - JSON `false` value is returned for negated settings like `-nosetting` or
 *   `-nosetting=1`. `false` is also returned for boolean-only settings that
 *   have the ALLOW_BOOL flag and false values like `setting=0`.
 *
 * - JSON `true` value is returned for settings that have the ALLOW_BOOL flag
 *   and are specified on the command line without a value like `-setting`.
 *   `true` is also returned for boolean-only settings that have the ALLOW_BOOL
 *   flag and true values like `setting=1`. `true` is also returned for untyped
 *   legacy settings (see \ref IsTypedArg) that use double negation like
 *   `-nosetting=0`.
 *
 * - JSON `""` empty string value is returned for settings like `-setting=`
 *   that specify empty values. `""` is also returned for untyped legacy
 *   settings (see \ref IsTypedArg) that are specified on the command line
 *   without a value like `-setting`.
 *
 * - JSON strings like `"abc"` are returned for settings like `-setting=abc` if
 *   the setting has the ALLOW_STRING flag or is an untyped legacy setting.
 *
 * - JSON numbers like `123` are returned for settings like `-setting=123` if
 *   the setting enables integer parsing with the ALLOW_INT flag.
 */
std::optional<common::SettingsValue> InterpretValue(const KeyInfo& key, const std::string* value,
                                                  unsigned int flags, std::string& error)
                                                    unsigned int flags, std::string& error)
{
    // Return negated settings as false values.
    if (key.negated) {
        if (flags & ArgsManager::DISALLOW_NEGATION) {
            error = strprintf("Negating of -%s is meaningless and therefore forbidden", key.name);
            return std::nullopt;
        }
        if (IsTypedArg(flags)) {
            // If argument is typed, only allow negation with no value or with
            // literal "1" value. Avoid calling InterpretBool and accepting
            // other values which could be ambiguous.
            if (value && *value != "1") {
                error = strprintf("Cannot negate -%s at the same time as setting a value ('%s').", key.name, *value);
                return std::nullopt;
            }
            return false;
        }
        // Double negatives like -nofoo=0 are supported (but discouraged)
        if (value && !InterpretBool(*value)) {
            LogPrintf("Warning: parsed potentially confusing double-negative -%s=%s\n", key.name, *value);
            return true;
        }
        return false;
    }
    if (!value && (flags & ArgsManager::DISALLOW_ELISION)) {
        error = strprintf("Can not set -%s with no value. Please specify value with -%s=value.", key.name, key.name);
        return std::nullopt;
    if (value) {
        if ((flags & ArgsManager::ALLOW_STRING) || !IsTypedArg(flags) || value->empty()) return *value;
        if (flags & ArgsManager::ALLOW_INT) {
            if (auto parsed_int = ToIntegral<int64_t>(*value)) return *parsed_int;
        }
        if (flags & ArgsManager::ALLOW_BOOL) {
            if (*value == "0") return false;
            if (*value == "1") return true;
        }
        error = strprintf("Cannot set -%s value to '%s'.", key.name, *value);
    } else {
        if (flags & ArgsManager::ALLOW_BOOL) return true;
        if (!(flags & ArgsManager::DISALLOW_ELISION) && !IsTypedArg(flags)) return "";
        error = strprintf("Cannot set -%s with no value. Please specify value with -%s=value.", key.name, key.name);
    }
    if (flags & ArgsManager::ALLOW_STRING) {
        error = strprintf("%s %s", error, "It must be set to a string.");
    } else if (flags & ArgsManager::ALLOW_INT) {
        error = strprintf("%s %s", error, "It must be set to an integer.");
    } else if (flags & ArgsManager::ALLOW_BOOL) {
        error = strprintf("%s %s", error, "It must be set to 0 or 1.");
    }
    return value ? *value : "";
    return std::nullopt;
}

//! Return string if setting is a nonempty string or number (-setting=abc,
//! -setting=123), "" if setting is false (-nosetting), otherwise return
//! nullopt. For legacy untyped args, coerce bool settings to strings as well.
static inline std::optional<std::string> ConvertToString(const common::SettingsValue& value, bool typed_arg)
{
    if (value.isStr() && !value.get_str().empty()) return value.get_str();
    if (value.isNum()) return value.getValStr();
    if (typed_arg && value.isFalse()) return "";
    if (!typed_arg && !value.isNull()) {
        if (value.isBool()) return value.get_bool() ? "1" : "0";
        return value.get_str();
    }
    return {};
}

//! Return int64 if setting is a number or bool, otherwise return nullopt. For
//! legacy untyped args, coerce string settings as well.
static inline std::optional<int64_t> ConvertToInt(const common::SettingsValue& value, bool typed_arg)
{
    if (value.isNum()) return value.getInt<int64_t>();
    if (value.isBool()) return value.get_bool();
    if (!typed_arg && !value.isNull()) return LocaleIndependentAtoi<int64_t>(value.get_str());
    return {};
}

//! Return bool if setting is a bool, otherwise return nullopt. For legacy
//! untyped args, coerce string settings as well.
static inline std::optional<bool> ConvertToBool(const common::SettingsValue& value, bool typed_arg)
{
    if (value.isBool()) return value.get_bool();
    if (!typed_arg && !value.isNull()) return InterpretBool(value.get_str());
    return {};
}

// Define default constructor and destructor that are not inline, so code instantiating this class doesn't need to
@@ -269,6 +363,30 @@ std::optional<unsigned int> ArgsManager::GetArgFlags(const std::string& name) co
    return std::nullopt;
}

/**
 * Check that arg has the right flags for use in a given context. Raises
 * logic_error if this isn't the case, indicating the argument was registered
 * with bad AddArg flags.
 *
 * Returns true if the arg is registered and has type checking enabled. Returns
 * false if the arg was never registered or is untyped.
 */
bool ArgsManager::CheckArgFlags(const std::string& name,
    uint32_t require,
    uint32_t forbid,
    const char* context) const
{
    std::optional<unsigned int> flags = GetArgFlags(name);
    if (!flags) return false;
    if (!IsTypedArg(*flags)) require &= ~(ALLOW_BOOL | ALLOW_INT | ALLOW_STRING);
    if ((*flags & require) != require || (*flags & forbid) != 0) {
        throw std::logic_error(
            strprintf("Bug: Can't call %s on arg %s registered with flags 0x%08x (requires 0x%x, disallows 0x%x)",
                context, name, *flags, require, forbid));
    }
    return IsTypedArg(*flags);
}

fs::path ArgsManager::GetPathArg(std::string arg, const fs::path& default_value) const
{
    if (IsArgNegated(arg)) return fs::path{};
@@ -361,9 +479,10 @@ std::optional<const ArgsManager::Command> ArgsManager::GetCommand() const

std::vector<std::string> ArgsManager::GetArgs(const std::string& strArg) const
{
    bool typed_arg = CheckArgFlags(strArg, /*require=*/ ALLOW_STRING | ALLOW_LIST, /*forbid=*/ 0, __func__);
    std::vector<std::string> result;
    for (const common::SettingsValue& value : GetSettingsList(strArg)) {
        result.push_back(value.isFalse() ? "0" : value.isTrue() ? "1" : value.get_str());
        result.push_back(ConvertToString(value, typed_arg).value_or(""));
    }
    return result;
}
@@ -461,22 +580,13 @@ std::string ArgsManager::GetArg(const std::string& strArg, const std::string& st

std::optional<std::string> ArgsManager::GetArg(const std::string& strArg) const
{
    const common::SettingsValue value = GetSetting(strArg);
    return SettingToString(value);
}

std::optional<std::string> SettingToString(const common::SettingsValue& value)
{
    if (value.isNull()) return std::nullopt;
    if (value.isFalse()) return "0";
    if (value.isTrue()) return "1";
    if (value.isNum()) return value.getValStr();
    return value.get_str();
    bool typed_arg = CheckArgFlags(strArg, /*require=*/ ALLOW_STRING, /*forbid=*/ ALLOW_LIST, __func__);
    return ConvertToString(GetSetting(strArg), typed_arg);
}

std::string SettingToString(const common::SettingsValue& value, const std::string& strDefault)
{
    return SettingToString(value).value_or(strDefault);
    return ConvertToString(value, /*typed_arg=*/false).value_or(strDefault);
}

int64_t ArgsManager::GetIntArg(const std::string& strArg, int64_t nDefault) const
@@ -486,22 +596,13 @@ int64_t ArgsManager::GetIntArg(const std::string& strArg, int64_t nDefault) cons

std::optional<int64_t> ArgsManager::GetIntArg(const std::string& strArg) const
{
    const common::SettingsValue value = GetSetting(strArg);
    return SettingToInt(value);
}

std::optional<int64_t> SettingToInt(const common::SettingsValue& value)
{
    if (value.isNull()) return std::nullopt;
    if (value.isFalse()) return 0;
    if (value.isTrue()) return 1;
    if (value.isNum()) return value.getInt<int64_t>();
    return LocaleIndependentAtoi<int64_t>(value.get_str());
    bool typed_arg = CheckArgFlags(strArg, /*require=*/ ALLOW_INT, /*forbid=*/ ALLOW_LIST, __func__);
    return ConvertToInt(GetSetting(strArg), typed_arg);
}

int64_t SettingToInt(const common::SettingsValue& value, int64_t nDefault)
{
    return SettingToInt(value).value_or(nDefault);
    return ConvertToInt(value, /*typed_arg=*/false).value_or(nDefault);
}

bool ArgsManager::GetBoolArg(const std::string& strArg, bool fDefault) const
@@ -511,20 +612,13 @@ bool ArgsManager::GetBoolArg(const std::string& strArg, bool fDefault) const

std::optional<bool> ArgsManager::GetBoolArg(const std::string& strArg) const
{
    const common::SettingsValue value = GetSetting(strArg);
    return SettingToBool(value);
}

std::optional<bool> SettingToBool(const common::SettingsValue& value)
{
    if (value.isNull()) return std::nullopt;
    if (value.isBool()) return value.get_bool();
    return InterpretBool(value.get_str());
    bool typed_arg = CheckArgFlags(strArg, /*require=*/ ALLOW_BOOL, /*forbid=*/ ALLOW_LIST, __func__);
    return ConvertToBool(GetSetting(strArg), typed_arg);
}

bool SettingToBool(const common::SettingsValue& value, bool fDefault)
{
    return SettingToBool(value).value_or(fDefault);
    return ConvertToBool(value, /*typed_arg=*/false).value_or(fDefault);
}

bool ArgsManager::SoftSetArg(const std::string& strArg, const std::string& strValue)
@@ -537,15 +631,17 @@ bool ArgsManager::SoftSetArg(const std::string& strArg, const std::string& strVa

bool ArgsManager::SoftSetBoolArg(const std::string& strArg, bool fValue)
{
    if (fValue)
        return SoftSetArg(strArg, std::string("1"));
    else
        return SoftSetArg(strArg, std::string("0"));
    LOCK(cs_args);
    CheckArgFlags(strArg, /*require=*/ ALLOW_BOOL, /*forbid=*/ ALLOW_LIST, __func__);
    if (IsArgSet(strArg)) return false;
    m_settings.forced_settings[SettingName(strArg)] = fValue;
    return true;
}

void ArgsManager::ForceSetArg(const std::string& strArg, const std::string& strValue)
{
    LOCK(cs_args);
    CheckArgFlags(strArg, /*require=*/ ALLOW_STRING, /*forbid=*/ 0, __func__);
    m_settings.forced_settings[SettingName(strArg)] = strValue;
}

@@ -580,6 +676,20 @@ void ArgsManager::AddArg(const std::string& name, const std::string& help, unsig
    if (flags & ArgsManager::NETWORK_ONLY) {
        m_network_only_args.emplace(arg_name);
    }

    // Disallow flag combinations that would result in nonsensical behavior or a bad UX.
    if ((flags & ALLOW_ANY) && (flags & (ALLOW_BOOL | ALLOW_INT | ALLOW_STRING))) {
        throw std::logic_error(strprintf("Bug: bad %s flags. ALLOW_{BOOL|INT|STRING} flags are incompatible with "
                                         "ALLOW_ANY (typed arguments need to be type checked)", arg_name));
    }
    if ((flags & ALLOW_BOOL) && (flags & DISALLOW_ELISION)) {
        throw std::logic_error(strprintf("Bug: bad %s flags. ALLOW_BOOL flag is incompatible with DISALLOW_ELISION "
                                         "(boolean arguments should not require argument values)", arg_name));
    }
    if ((flags & ALLOW_INT) && (flags & ALLOW_STRING)) {
        throw std::logic_error(strprintf("Bug: bad %s flags. ALLOW_INT flag is incompatible with ALLOW_STRING "
                                         "(any valid integer is also a valid string)", arg_name));
    }
}

void ArgsManager::AddHiddenArgs(const std::vector<std::string>& names)
@@ -794,7 +904,7 @@ std::variant<ChainType, std::string> ArgsManager::GetChainArg() const
            /* ignore_default_section_config= */ false,
            /*ignore_nonpersistent=*/false,
            /* get_chain_type= */ true);
        return value.isNull() ? false : value.isBool() ? value.get_bool() : InterpretBool(value.get_str());
        return ConvertToBool(value, /*typed_arg=*/false).value_or(false);
    };

    const bool fRegTest = get_net("-regtest");