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

template<typename T, auto get_fn = nullptr>
struct SettingTraitsBase
{
    using setting_t = T;
    static constexpr bool is_list{false};
    static bool Get(const common::SettingsValue& setting, setting_t& out)
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

template<typename T, SettingOptions options>
struct SettingTraits {};

template<SettingOptions options>
struct SettingTraits<Unset, options> : SettingTraitsBase<Unset>
{
    static bool Get(const common::SettingsValue& setting, setting_t&) { return setting.isNull(); }
};

template<SettingOptions options>
struct SettingTraits<Enabled, options> : SettingTraitsBase<Enabled>
{
    static bool Get(const common::SettingsValue& setting, setting_t&) { return setting.isTrue(); }
};

template<SettingOptions options>
struct SettingTraits<Disabled, options> : SettingTraitsBase<Disabled>
{
    static bool Get(const common::SettingsValue& setting, setting_t&) { return setting.isFalse(); }
};

template<typename T>
constexpr auto GetPtr(std::optional<T>(*ptr)(const common::SettingsValue&)) { return ptr; }

template<SettingOptions options>
struct SettingTraits<bool, options> : SettingTraitsBase<bool, GetPtr(SettingToBool)> {};

template<typename T, SettingOptions options>
requires std::is_integral_v<T>
struct SettingTraits<T, options> : SettingTraitsBase<T, GetPtr(SettingToInt)> {};

template<SettingOptions options>
struct SettingTraits<std::string, options> : SettingTraitsBase<std::string, GetPtr(SettingToString)> {};

template<SettingOptions options>
struct SettingTraits<fs::path, options> : SettingTraitsBase<fs::path, &SettingToPath> {};

template<typename T, SettingOptions options>
struct SettingTraits<std::optional<T>, options> : SettingTraits<T, options>
{
    using setting_t = std::optional<T>;
    using wrapped_t = SettingTraits<T, options>;
    static constexpr bool is_list{wrapped_t::is_list};

    static bool Get(const common::SettingsValue& setting, setting_t& out)
    {
        bool unset{!out};
        if (unset) out.emplace();
        bool got{wrapped_t::Get(setting, *out)};
        if (unset && !got) out.reset();
        return got;
    }
};

template<typename T, SettingOptions options>
struct SettingTraits<std::vector<T>, options> : SettingTraits<T, options>
{
    using setting_t = std::vector<T>;
    using wrapped_t = SettingTraits<T, options>;
    static constexpr bool is_list{true};

    static bool Get(const common::SettingsValue& setting, setting_t& out)
    {
        T elem;
        if (wrapped_t::Get(setting, elem)) {
            out.emplace_back(std::move(elem));
            return true;
        }
        return false;
    }
};

template<SettingOptions options>
struct SettingTraits<SettingsValue, options> : SettingTraitsBase<SettingsValue>
{
    static bool Get(const common::SettingsValue& setting, setting_t& out) { out = setting; return true; }
};

inline std::string_view SettingName(std::string_view summary)
{
    return summary.substr(0, summary.find_first_of('='));
}

template<SettingOptions options>
constexpr int SettingFlags()
{
    int flags = 0;
    if (options.debug_only)        flags |= ArgsManager::DEBUG_ONLY;
    if (options.network_only)      flags |= ArgsManager::NETWORK_ONLY;
    if (options.sensitive)         flags |= ArgsManager::SENSITIVE;
    if (options.disallow_negation) flags |= ArgsManager::DISALLOW_NEGATION;
    if (options.disallow_elision)  flags |= ArgsManager::DISALLOW_ELISION;
    return flags;
}

template<auto _value>
struct Constant {
    static constexpr auto value = _value;
    using type = decltype(value);
    constexpr auto operator()() { return value; }
};

template<typename T, SettingOptions options, auto help>
void SettingRegister(ArgsManager& manager, auto summary, OptionsCategory category, auto help_fn, auto default_fn, auto get_fn, auto&&... register_options)
{
    std::string help_str;
    if constexpr (!std::is_same_v<decltype(help_fn), std::nullptr_t>) {
        help_str = help_fn(Constant<help>{}, register_options...);
    } else if constexpr (!std::is_same_v<decltype(default_fn), std::nullptr_t>) {
        help_str = strprintf(util::ConstevalFormatString<1>{help.value}, default_fn());
    } else if constexpr (!std::is_same_v<decltype(help.value), std::nullptr_t>) {
        help_str = help.value;
    }
    manager.AddArg(summary.value, help_str, SettingFlags<options>(), category);
}

template<typename T, SettingOptions options>
T SettingConstruct(auto default_fn)
{
    if constexpr (std::is_same_v<decltype(default_fn), std::nullptr_t>) {
        return {};
    } else {
        return default_fn();
    }
}

template<typename T, SettingOptions options>
T SettingGet(const ArgsManager& manager, std::string_view summary, auto default_fn, auto get_fn)
{
    using Traits = SettingTraits<T, options>;
    const std::string name{SettingName(summary)};
    T out{SettingConstruct<T, options>(default_fn)};

    auto get{[&](const SettingsValue& setting) {
        if constexpr (std::is_same_v<decltype(get_fn), std::nullptr_t>) {
            return Traits::Get(setting, out);
        } else {
            return get_fn(setting, out);
        }
    }};

    if constexpr (Traits::is_list) {
        for (const SettingsValue& setting : manager.GetSettingsList(name)) {
            get(setting);
        }
    } else {
        SettingsValue setting{manager.GetSetting(name)};
        get(setting);
    }
    return out;
}
} // namespace internal
} // namespace common

namespace tinyformat {
template<auto F>
auto format(common::internal::Constant<F>, const auto&... args)
{
    constexpr util::ConstevalFormatString<sizeof...(args)> fmt{F.value};
    return strprintf(fmt, args...);
}
} // namespace tinyformat

namespace common {
namespace internal {
template<auto... args>
struct HelpFormat {
    auto operator()(const auto& fmt) const { return strprintf(fmt, args...); };
};
} // namespace internal
} // namespace common

#endif // BITCOIN_COMMON_SETTING_INTERNAL_H
