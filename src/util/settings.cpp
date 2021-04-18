// Copyright (c) 2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/settings.h>

#include <tinyformat.h>
#include <univalue.h>

namespace util {
namespace {

enum class Source {
   FORCED,
   COMMAND_LINE,
   RW_SETTINGS,
   CONFIG_FILE_NETWORK_SECTION,
   CONFIG_FILE_DEFAULT_SECTION
};

//! Merge settings from multiple sources in precedence order:
//! Forced config > command line > read-write settings file > config file network-specific section > config file default section
//!
//! This function is provided with a callback function fn that contains
//! specific logic for how to merge the sources.
template <typename Fn>
static void MergeSettings(const Settings& settings, const std::string& section, const std::string& name, Fn&& fn)
{
    // Merge in the forced settings
    if (auto* value = FindKey(settings.forced_settings, name)) {
        fn(SettingsSpan(*value), Source::FORCED);
    }
    // Merge in the command-line options
    if (auto* values = FindKey(settings.command_line_options, name)) {
        fn(SettingsSpan(*values), Source::COMMAND_LINE);
    }
    // Merge in the read-write settings
    if (const SettingsValue* value = FindKey(settings.rw_settings, name)) {
        fn(SettingsSpan(*value), Source::RW_SETTINGS);
    }
    // Merge in the network-specific section of the config file
    if (!section.empty()) {
        if (auto* map = FindKey(settings.ro_config, section)) {
            if (auto* values = FindKey(*map, name)) {
                fn(SettingsSpan(*values), Source::CONFIG_FILE_NETWORK_SECTION);
            }
        }
    }
    // Merge in the default section of the config file
    if (auto* map = FindKey(settings.ro_config, "")) {
        if (auto* values = FindKey(*map, name)) {
            fn(SettingsSpan(*values), Source::CONFIG_FILE_DEFAULT_SECTION);
        }
    }
}
} // namespace

bool ReadSettings(const fs::path& path, std::map<std::string, SettingsValue>& values, std::vector<std::string>& errors)
{
    values.clear();
    errors.clear();

    fsbridge::ifstream file;
    file.open(path);
    if (!file.is_open()) return true; // Ok for file not to exist.

    SettingsValue in;
    if (!in.read(std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()})) {
        errors.emplace_back(strprintf("Unable to parse settings file %s", path.string()));
        return false;
    }

    if (file.fail()) {
        errors.emplace_back(strprintf("Failed reading settings file %s", path.string()));
        return false;
    }
    file.close(); // Done with file descriptor. Release while copying data.

    if (!in.isObject()) {
        errors.emplace_back(strprintf("Found non-object value %s in settings file %s", in.write(), path.string()));
        return false;
    }

    const std::vector<std::string>& in_keys = in.getKeys();
    const std::vector<SettingsValue>& in_values = in.getValues();
    for (size_t i = 0; i < in_keys.size(); ++i) {
        auto inserted = values.emplace(in_keys[i], in_values[i]);
        if (!inserted.second) {
            errors.emplace_back(strprintf("Found duplicate key %s in settings file %s", in_keys[i], path.string()));
        }
    }
    return errors.empty();
}

bool WriteSettings(const fs::path& path,
    const std::map<std::string, SettingsValue>& values,
    std::vector<std::string>& errors)
{
    SettingsValue out(SettingsValue::VOBJ);
    for (const auto& value : values) {
        out.__pushKV(value.first, value.second);
    }
    fsbridge::ofstream file;
    file.open(path);
    if (file.fail()) {
        errors.emplace_back(strprintf("Error: Unable to open settings file %s for writing", path.string()));
        return false;
    }
    file << out.write(/* prettyIndent= */ 1, /* indentLevel= */ 4) << std::endl;
    file.close();
    return true;
}

SettingsValue GetSetting(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config,
    bool get_chain_name)
{
    SettingsValue result;
    bool done = false; // Done merging any more settings sources.
    MergeSettings(settings, section, name, [&](SettingsSpan span, Source source) {
        // Weird behavior preserved for backwards compatibility: Apply negated
        // setting even if non-negated setting would be ignored. A negated
        // value in the default section is applied to network specific options,
        // even though normal non-negated values there would be ignored.
        const bool never_ignore_negated_setting = span.last_negated();

        // Weird behavior preserved for backwards compatibility: Take first
        // assigned value instead of last. In general, later settings take
        // precedence over early settings, but for backwards compatibility in
        // the config file the precedence is reversed for all settings except
        // chain name settings.
        const bool reverse_precedence =
            (source == Source::CONFIG_FILE_NETWORK_SECTION || source == Source::CONFIG_FILE_DEFAULT_SECTION) &&
            !get_chain_name;

        // Weird behavior preserved for backwards compatibility: Negated
        // -regtest and -testnet arguments which you would expect to override
        // values set in the configuration file are currently accepted but
        // silently ignored. It would be better to apply these just like other
        // negated values, or at least warn they are ignored.
        const bool skip_negated_command_line = get_chain_name;

        if (done) return;

        // Ignore settings in default config section if requested.
        if (ignore_default_section_config && source == Source::CONFIG_FILE_DEFAULT_SECTION &&
            !never_ignore_negated_setting) {
            return;
        }

        // Skip negated command line settings.
        if (skip_negated_command_line && span.last_negated()) return;

        if (!span.empty()) {
            result = reverse_precedence ? span.begin()[0] : span.end()[-1];
            done = true;
        } else if (span.last_negated()) {
            result = false;
            done = true;
        }
    });
    return result;
}

std::vector<SettingsValue> GetSettingsList(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config)
{
    std::vector<SettingsValue> result;
    bool done = false; // Done merging any more settings sources.
    bool prev_negated_empty = false;
    MergeSettings(settings, section, name, [&](SettingsSpan span, Source source) {
        // Weird behavior preserved for backwards compatibility: Apply config
        // file settings even if negated on command line. Negating a setting on
        // command line will ignore earlier settings on the command line and
        // ignore settings in the config file, unless the negated command line
        // value is followed by non-negated value, in which case config file
        // settings will be brought back from the dead (but earlier command
        // line settings will still be ignored).
        const bool add_zombie_config_values =
            (source == Source::CONFIG_FILE_NETWORK_SECTION || source == Source::CONFIG_FILE_DEFAULT_SECTION) &&
            !prev_negated_empty;

        // Ignore settings in default config section if requested.
        if (ignore_default_section_config && source == Source::CONFIG_FILE_DEFAULT_SECTION) return;

        // Add new settings to the result if isn't already complete, or if the
        // values are zombies.
        if (!done || add_zombie_config_values) {
            for (const auto& value : span) {
                if (value.isArray()) {
                    result.insert(result.end(), value.getValues().begin(), value.getValues().end());
                } else {
                    result.push_back(value);
                }
            }
        }

        // If a setting was negated, or if a setting was forced, set
        // done to true to ignore any later lower priority settings.
        done |= span.negated() > 0 || source == Source::FORCED;

        // Update the negated and empty state used for the zombie values check.
        prev_negated_empty |= span.last_negated() && result.empty();
    });
    return result;
}

bool OnlyHasDefaultSectionSetting(const Settings& settings, const std::string& section, const std::string& name)
{
    bool has_default_section_setting = false;
    bool has_other_setting = false;
    MergeSettings(settings, section, name, [&](SettingsSpan span, Source source) {
        if (span.empty()) return;
        else if (source == Source::CONFIG_FILE_DEFAULT_SECTION) has_default_section_setting = true;
        else has_other_setting = true;
    });
    // If a value is set in the default section and not explicitly overwritten by the
    // user on the command line or in a different section, then we want to enable
    // warnings about the value being ignored.
    return has_default_section_setting && !has_other_setting;
}

SettingsSpan::SettingsSpan(const std::vector<SettingsValue>& vec) noexcept : SettingsSpan(vec.data(), vec.size()) {}
const SettingsValue* SettingsSpan::begin() const { return data + negated(); }
const SettingsValue* SettingsSpan::end() const { return data + size; }
bool SettingsSpan::empty() const { return size == 0 || last_negated(); }
bool SettingsSpan::last_negated() const { return size > 0 && data[size - 1].isFalse(); }
size_t SettingsSpan::negated() const
{
    for (size_t i = size; i > 0; --i) {
        if (data[i - 1].isFalse()) return i; // Return number of negated values (position of last false value)
    }
    return 0;
}

} // namespace util
