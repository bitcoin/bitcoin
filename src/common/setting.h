// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SETTING_H
#define BITCOIN_COMMON_SETTING_H

#include <common/args.h>

#include <univalue.h>

namespace common {
//! State representing a setting that is unset
struct Unset {};
//! State representing a setting that is enabled without a value ("-setting")
struct Enabled {};
//! State representing a setting that is disabled ("-nosetting")
struct Disabled {};

struct SettingOptions {
    bool legacy{false};
    bool debug_only{false};
    bool network_only{false};
    bool sensitive{false};
    bool disallow_negation{false};
    bool disallow_elision{false};
};

template<size_t N, typename T=char[N]>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    constexpr StringLiteral(std::nullptr_t) {
    }
    T value{};
};

StringLiteral(std::nullptr_t) -> StringLiteral<1, std::nullptr_t>;
} // namespace common

#include <common/setting_internal.h>

namespace common {
//! Setting template class used to declare compile-time Setting types which are
//! used to register and retrieve settings.
template<StringLiteral summary, typename T, SettingOptions options = SettingOptions{}, StringLiteral help = nullptr, auto help_fn = nullptr, auto default_fn = nullptr, auto get_fn = nullptr, OptionsCategory category = OptionsCategory::OPTIONS>
struct Setting {
    using value_t = internal::RemoveOptional<T>::type;

    static void Register(ArgsManager& manager, auto&&... register_options)
    {
        internal::SettingRegister<T, options, help>(manager, summary, category, help_fn, default_fn, get_fn, register_options...);
    }

    static T Get(const ArgsManager& manager)
    {
        return internal::SettingGet<T, options>(manager, summary.value, default_fn, get_fn);
    }

    static value_t Get(const ArgsManager& manager, const value_t& default_value)
    {
        return internal::SettingGet<value_t, options>(manager, summary.value, [&] { return default_value; }, get_fn);
    }

    static SettingsValue Value(const ArgsManager& manager)
    {
        return internal::SettingGet<SettingsValue, options>(manager, summary.value, nullptr, nullptr);
    }

    // Convenient accessors to set template parameters by name
    template<auto _help_fn>
    using HelpFn = Setting<summary, T, options, help, _help_fn, default_fn, get_fn, category>;
    template<auto... args>
    using HelpArgs = HelpFn<internal::HelpFormat<args...>{}>;
    template<auto _default>
    using Default = Setting<summary, T, options, help, help_fn, internal::Constant<_default>{}, get_fn, category>;
    template<auto _default_fn>
    using DefaultFn = Setting<summary, T, options, help, help_fn, _default_fn, get_fn, category>;
    template<auto _get_fn>
    using GetFn = Setting<summary, T, options, help, help_fn, default_fn, _get_fn, category>;
    template<OptionsCategory _category>
    using Category = Setting<summary, T, options, help, help_fn, default_fn, get_fn, _category>;
    using Hidden = Category<OptionsCategory::HIDDEN>;
};
} // namespace common

#endif // BITCOIN_COMMON_SETTING_H
