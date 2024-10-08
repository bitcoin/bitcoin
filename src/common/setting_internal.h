// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SETTING_INTERNAL_H
#define BITCOIN_COMMON_SETTING_INTERNAL_H

#include <common/args.h>
#include <common/settings.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/strencodings.h>

namespace common {
namespace internal {
template <typename T>
struct RemoveOptional {
    using type = T;
};
template <typename U>
struct RemoveOptional<std::optional<U>> {
    using type = U;
};

template <typename T, auto get_fn = nullptr>
struct SettingTraitsBase {
    using setting_t = T;
    static constexpr bool is_list{false};
    static bool Get(const SettingsValue& setting, setting_t& out)
    {
        if constexpr (!std::is_same_v<decltype(get_fn), std::nullptr_t>) {
            if (auto value{get_fn(setting)}) out = *value;
            // For legacy settings, always return true if any value is present.
            // Legacy settings are unchecked and untyped and any setting can be
            // retrieved as any type through unsafe coercions.
            return !setting.isNull();
        }
        return false;
    }
};

template <typename T, SettingOptions options>
struct SettingTraits {
};

template <SettingOptions options>
struct SettingTraits<Unset, options> : SettingTraitsBase<Unset> {
    static bool Get(const SettingsValue& setting, setting_t&) { return setting.isNull(); }
};

template <SettingOptions options>
struct SettingTraits<Enabled, options> : SettingTraitsBase<Enabled> {
    static bool Get(const SettingsValue& setting, setting_t&) { return setting.isTrue(); }
};

template <SettingOptions options>
struct SettingTraits<Disabled, options> : SettingTraitsBase<Disabled> {
    static bool Get(const SettingsValue& setting, setting_t&) { return setting.isFalse(); }
};

//! Helper to avoid needing to write long static_cast expressions to
//! disambiguate SettingTo{Bool,Int,String} function pointers.
template <typename T>
constexpr auto GetPtr(std::optional<T> (*ptr)(const SettingsValue&))
{
    return ptr;
}

template <SettingOptions options>
struct SettingTraits<bool, options> : SettingTraitsBase<bool, GetPtr(SettingToBool)> {
};

template <typename T, SettingOptions options>
    requires std::is_integral_v<T>
struct SettingTraits<T, options> : SettingTraitsBase<T, GetPtr(SettingToInt)> {
};

template <SettingOptions options>
struct SettingTraits<std::string, options> : SettingTraitsBase<std::string, GetPtr(SettingToString)> {
};

template <SettingOptions options>
struct SettingTraits<fs::path, options> : SettingTraitsBase<fs::path, &SettingToPath> {
};

template <typename T, SettingOptions options>
struct SettingTraits<std::optional<T>, options> : SettingTraits<T, options> {
    using setting_t = std::optional<T>;
    using wrapped_t = SettingTraits<T, options>;
    static constexpr bool is_list{wrapped_t::is_list};

    static bool Get(const SettingsValue& setting, setting_t& out)
    {
        bool unset{!out};
        if (unset) out.emplace();
        bool got{wrapped_t::Get(setting, *out)};
        if (unset && !got) out.reset();
        return got;
    }
};

template <typename T, SettingOptions options>
struct SettingTraits<std::vector<T>, options> : SettingTraits<T, options> {
    using setting_t = std::vector<T>;
    using wrapped_t = SettingTraits<T, options>;
    static constexpr bool is_list{true};

    static bool Get(const SettingsValue& setting, setting_t& out)
    {
        T elem;
        if (wrapped_t::Get(setting, elem)) {
            out.emplace_back(std::move(elem));
            return true;
        }
        return false;
    }
};

template <SettingOptions options>
struct SettingTraits<SettingsValue, options> : SettingTraitsBase<SettingsValue> {
    static bool Get(const SettingsValue& setting, setting_t& out)
    {
        out = setting;
        return true;
    }
};

inline std::string_view SettingName(std::string_view summary)
{
    return summary.substr(0, summary.find_first_of('='));
}

consteval int SettingFlags(SettingOptions options)
{
    int flags = 0;
    if (options.debug_only) flags |= ArgsManager::DEBUG_ONLY;
    if (options.network_only) flags |= ArgsManager::NETWORK_ONLY;
    if (options.sensitive) flags |= ArgsManager::SENSITIVE;
    if (options.disallow_negation) flags |= ArgsManager::DISALLOW_NEGATION;
    if (options.disallow_elision) flags |= ArgsManager::DISALLOW_ELISION;
    return flags;
}

template <typename T, typename DefaultFn>
T ConstructDefault(DefaultFn default_fn)
{
    if constexpr (std::is_same_v<DefaultFn, std::nullptr_t>) {
        return T{};
    } else {
        return default_fn();
    }
}

template <auto _value>
struct Constant {
    static constexpr auto value = _value;
    using type = decltype(value);
    constexpr auto operator()() const { return value; }
};

template <typename T, auto help_str, typename DefaultFn>
struct HelpArg {
    DefaultFn default_fn;

    HelpArg(DefaultFn default_fn) : default_fn{std::move(default_fn)} {}
    T DefaultValue() const { return ConstructDefault<T>(default_fn); }
    std::string Format(const auto&... args) const
    {
        constexpr util::ConstevalFormatString<sizeof...(args)> fmt{help_str.value};
        return strprintf(fmt, args...);
    }
};

template <auto... args>
struct HelpFormat {
    auto operator()(const auto& help) const { return help.Format(args...); };
};

template <typename T, auto help_str, typename HelpFn, typename DefaultFn>
std::string SettingHelp(HelpFn help_fn, DefaultFn default_fn, const auto&... args)
{
    std::string ret;
    if constexpr (!std::is_same_v<HelpFn, std::nullptr_t>) {
        ret = help_fn(HelpArg<T, help_str, DefaultFn>{default_fn}, args...);
    } else if constexpr (!std::is_same_v<DefaultFn, std::nullptr_t>) {
        ret = strprintf(util::ConstevalFormatString<1>{help_str.value}, default_fn());
    } else if constexpr (!std::is_same_v<decltype(help_str.value), std::nullptr_t>) {
        ret = help_str.value;
    }
    return ret;
}

template <SettingOptions options>
void SettingRegister(ArgsManager& manager, const std::string& summary, const std::string& help, OptionsCategory category)
{
    manager.AddArg(summary, help, SettingFlags(options), category);
}

template <typename T, SettingOptions options, typename DefaultFn>
T SettingGet(const ArgsManager& manager, std::string_view summary, DefaultFn default_fn)
{
    using Traits = SettingTraits<T, options>;
    const std::string name{SettingName(summary)};
    T ret{ConstructDefault<T>(default_fn)};
    if constexpr (Traits::is_list) {
        for (const SettingsValue& setting : manager.GetSettingsList(name)) {
            Traits::Get(setting, ret);
        }
    } else {
        SettingsValue setting{manager.GetSetting(name)};
        Traits::Get(setting, ret);
    }
    return ret;
}
} // namespace internal
} // namespace common

#endif // BITCOIN_COMMON_SETTING_INTERNAL_H
