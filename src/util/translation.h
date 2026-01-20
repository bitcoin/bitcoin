// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRANSLATION_H
#define BITCOIN_UTIL_TRANSLATION_H

#include <tinyformat.h>
#include <util/string.h>

#include <cassert>
#include <functional>
#include <string>

/** Translate a message to the native language of the user. */
using TranslateFn = std::function<std::string(const char*)>;
const extern TranslateFn G_TRANSLATION_FUN;

/**
 * Bilingual messages:
 *   - in GUI: user's native language + untranslated (i.e. English)
 *   - in log and stderr: untranslated only
 */
struct bilingual_str {
    std::string original;
    std::string translated;

    bilingual_str& operator+=(const bilingual_str& rhs)
    {
        original += rhs.original;
        translated += rhs.translated;
        return *this;
    }

    bool empty() const
    {
        return original.empty();
    }

    void clear()
    {
        original.clear();
        translated.clear();
    }
};

inline bilingual_str operator+(bilingual_str lhs, const bilingual_str& rhs)
{
    lhs += rhs;
    return lhs;
}

namespace util {
//! Compile-time literal string that can be translated with an optional translation function.
struct TranslatedLiteral {
    const char* const original;
    const TranslateFn* translate_fn;

    consteval TranslatedLiteral(const char* str, const TranslateFn* fn = &G_TRANSLATION_FUN) : original{str}, translate_fn{fn} { assert(original); }
    operator std::string() const { return translate_fn && *translate_fn ? (*translate_fn)(original) : original; }
    operator bilingual_str() const { return {original, std::string{*this}}; }
};

// TranslatedLiteral operators for formatting and adding to strings.
inline std::ostream& operator<<(std::ostream& os, const TranslatedLiteral& lit) { return os << std::string{lit}; }
template<typename T>
T operator+(const T& lhs, const TranslatedLiteral& rhs) { return lhs + static_cast<T>(rhs); }
template<typename T>
T operator+(const TranslatedLiteral& lhs, const T& rhs) { return static_cast<T>(lhs) + rhs; }

template <unsigned num_params>
struct BilingualFmt {
    const ConstevalFormatString<num_params> original;
    TranslatedLiteral lit;
    consteval BilingualFmt(TranslatedLiteral l) : original{l.original}, lit{l} {}
};
} // namespace util

consteval auto _(util::TranslatedLiteral str) { return str; }

/** Mark a bilingual_str as untranslated */
inline bilingual_str Untranslated(std::string original) { return {original, original}; }

// Provide an overload of tinyformat::format for BilingualFmt format strings and bilingual_str or TranslatedLiteral args.
namespace tinyformat {
template <typename... Args>
bilingual_str format(util::BilingualFmt<sizeof...(Args)> fmt, const Args&... args)
{
    const auto original_arg{[](const auto& arg) -> const auto& {
        if constexpr (std::is_same_v<decltype(arg), const bilingual_str&>) {
            return arg.original;
        } else if constexpr (std::is_same_v<decltype(arg), const util::TranslatedLiteral&>) {
            return arg.original;
        } else {
            return arg;
        }
    }};
    const auto translated_arg{[](const auto& arg) -> const auto& {
        if constexpr (std::is_same_v<decltype(arg), const bilingual_str&>) {
            return arg.translated;
        } else {
            return arg;
        }
    }};
    return bilingual_str{tfm::format(fmt.original, original_arg(args)...),
                         tfm::format(RuntimeFormat{std::string{fmt.lit}}, translated_arg(args)...)};
}
} // namespace tinyformat

#endif // BITCOIN_UTIL_TRANSLATION_H
