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

template <size_t N, typename T = char[N]>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }
    constexpr StringLiteral(std::nullptr_t)
    {
    }
    T value{};
};

StringLiteral(std::nullptr_t) -> StringLiteral<1, std::nullptr_t>;
} // namespace common

#include <common/setting_internal.h>

namespace common {
//! Setting template class used to declare name, type, and behavior of command
//! line and configuration settings.
//!
//! The template takes 4 parameters meant to be specified by users, followed by
//! additional parameters that are used internally.
//!
//! @tparam summary string like "-fastprune" or "-blocksdir=<dir>" with the
//!     name of the setting and optional argument information
//!
//! @tparam T type of the setting. Accepts simple types like bool, int, and
//!     std::string, or and composite types like std::optional<std::string> or
//!     std::vector<int>.
//!
//! @tparam options SettingOptions instance specifying additional options to
//!        apply to this setting.
//!
//! @tparam help_str optional help string describing purpose and behavior of the
//!     setting. It can be a formatted string accepting printf-style % arguments,
//!     which can be formatted at runtime using HelpFn/HelpArgs or
//!     DefaultFn/DefaultArgs features (described below).
//!
//! Setting template class defining Setting types which are used to register and
//! retrieve settings.
template <StringLiteral summary, typename T, SettingOptions options = SettingOptions{}, StringLiteral help_str = nullptr, auto help_fn = nullptr, auto default_fn = nullptr, OptionsCategory category = OptionsCategory::OPTIONS>
struct Setting {
    using value_t = internal::RemoveOptional<T>::type;

    static std::string Help(const auto&... args)
    {
        return internal::SettingHelp<T, help_str>(help_fn, default_fn, args...);
    }

    static void Register(ArgsManager& manager, const auto&... args)
    {
        internal::SettingRegister<options>(manager, summary.value, Help(args...), category);
    }

    static T Get(const ArgsManager& manager)
    {
        return internal::SettingGet<T, options>(manager, summary.value, default_fn);
    }

    static value_t Get(const ArgsManager& manager, const value_t& default_value)
    {
        return internal::SettingGet<value_t, options>(manager, summary.value, [&] { return default_value; });
    }

    static SettingsValue Value(const ArgsManager& manager)
    {
        return internal::SettingGet<SettingsValue, options>(manager, summary.value, nullptr);
    }

    //! HelpFn accessor specifying a lambda or callback function that can format the
    //! help string. The function should take the format string as an argument,
    //! plus any optional register_options passed to the Register method, and
    //! return a formatted std::string.
    template <auto _help_fn>
    using HelpFn = Setting<summary, T, options, help_str, _help_fn, default_fn, category>;
    //! HelpArgs accessor which is a simpler alternative to HelpFn and can be
    //! used with the format string does not require any runtime values. It
    //! accept a list of constexpr values, and calls tinyformat to format the
    //! help string with those values.
    template <auto... args>
    using HelpArgs = HelpFn<internal::HelpFormat<args...>{}>;
    //! Default accessor which sets a default value for the Setting. The
    //! specified value will be returned by Setting::Get calls if the setting
    //! was not specified in the command line, config file, or settings.json
    //! file. The specified value will also be substituted in the help string if
    //! HelpArgs or HelpFn are not specified.
    template <auto _default>
    using Default = Setting<summary, T, options, help_str, help_fn, internal::Constant<_default>{}, category>;
    //! DefaultFn accessor which sets a default value for the Setting. This
    //! provides the same functionality as the Default<> accessor described
    //! above, accept it accepts a lambda expression instead of a constexpr
    //! value so the value does not have to be known at compile time.
    template <auto _default_fn>
    using DefaultFn = Setting<summary, T, options, help_str, help_fn, _default_fn, category>;
    //! Category accessor overriding default setting OptionsCategory.
    template <OptionsCategory _category>
    using Category = Setting<summary, T, options, help_str, help_fn, default_fn, _category>;
    //! Hidden accessor for conveniently specifying OptionsCategory::HIDDEN.
    using Hidden = Category<OptionsCategory::HIDDEN>;
};
} // namespace common

#endif // BITCOIN_COMMON_SETTING_H
