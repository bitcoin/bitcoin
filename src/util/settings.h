// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SETTINGS_H
#define BITCOIN_UTIL_SETTINGS_H

#include <map>
#include <string>
#include <vector>

class UniValue;

namespace util {

//! Settings value type (string/integer/boolean/null variant).
//!
//! @note UniValue is used here for convenience and because it can be easily
//!       serialized in a readable format. But any other variant type that can
//!       be assigned strings, int64_t, and bool values and has get_str(),
//!       get_int64(), get_bool(), isNum(), isBool(), isFalse(), isTrue() and
//!       isNull() methods can be substituted if there's a need to move away
//!       from UniValue. (An implementation with boost::variant was posted at
//!       https://github.com/bitcoin/bitcoin/pull/15934/files#r337691812)
using SettingsValue = UniValue;

//! Stored bitcoin settings. This struct combines settings from the command line
//! and a read-only configuration file.
struct Settings {
    //! Map of setting name to forced setting value.
    std::map<std::string, SettingsValue> forced_settings;
    //! Map of setting name to list of command line values.
    std::map<std::string, std::vector<SettingsValue>> command_line_options;
    //! Map of config section name and setting name to list of config file values.
    std::map<std::string, std::map<std::string, std::vector<SettingsValue>>> ro_config;
};

//! Get settings value from combined sources: forced settings, command line
//! arguments and the read-only config file.
//!
//! @param ignore_default_section_config - ignore values in the default section
//!                                        of the config file (part before any
//!                                        [section] keywords)
//! @param get_chain_name - enable special backwards compatible behavior
//!                         for GetChainName
SettingsValue GetSetting(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config,
    bool get_chain_name);

//! Get combined setting value similar to GetSetting(), except if setting was
//! specified multiple times, return a list of all the values specified.
std::vector<SettingsValue> GetSettingsList(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config);

//! Return true if a setting is set in the default config file section, and not
//! overridden by a higher priority command-line or network section value.
//!
//! This is used to provide user warnings about values that might be getting
//! ignored unintentionally.
bool OnlyHasDefaultSectionSetting(const Settings& settings, const std::string& section, const std::string& name);

//! Accessor for list of settings that skips negated values when iterated over.
//! The last boolean `false` value in the list and all earlier values are
//! considered negated.
struct SettingsSpan {
    explicit SettingsSpan() = default;
    explicit SettingsSpan(const SettingsValue& value) noexcept : SettingsSpan(&value, 1) {}
    explicit SettingsSpan(const SettingsValue* data, size_t size) noexcept : data(data), size(size) {}
    explicit SettingsSpan(const std::vector<SettingsValue>& vec) noexcept;
    const SettingsValue* begin() const; //!< Pointer to first non-negated value.
    const SettingsValue* end() const;   //!< Pointer to end of values.
    bool empty() const;                 //!< True if there are any non-negated values.
    bool last_negated() const;          //!< True if the last value is negated.
    size_t negated() const;             //!< Number of negated values.

    const SettingsValue* data = nullptr;
    size_t size = 0;
};

//! Map lookup helper.
template <typename Map, typename Key>
auto FindKey(Map&& map, Key&& key) -> decltype(&map.at(key))
{
    auto it = map.find(key);
    return it == map.end() ? nullptr : &it->second;
}

} // namespace util

#endif // BITCOIN_UTIL_SETTINGS_H
