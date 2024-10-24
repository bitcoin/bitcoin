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
const extern std::function<std::string(const char*)> G_TRANSLATION_FUN;

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
/**
 * Translation function.
 * If no translation function is set, simply return the input.
 */
inline std::string translate(const char* lit)
{
    return G_TRANSLATION_FUN ? G_TRANSLATION_FUN(lit) : lit;
}
/** Type to denote whether an original string literal is translatable */
template <bool translatable = true>
struct Translatable {
    const char* const lit;
    consteval Translatable(const char* str) : lit{str} { assert(lit); }
    std::string translate() const { return translatable ? util::translate(lit) : lit; }
    operator bilingual_str() const { return {lit, translate()}; }
};
template <unsigned num_params>
struct bilingual_fmt {
    const ConstevalFormatString<num_params> original;
    const bool translatable;
    template <bool translatable>
    consteval bilingual_fmt(Translatable<translatable> o) : original{o.lit}, translatable{translatable}
    {
    }
    std::string translate() { return translatable ? util::translate(original.fmt) : original.fmt; }
};
} // namespace util

consteval auto _(util::Translatable<true> str) { return str; }

/** Mark a bilingual_str as untranslated */
inline bilingual_str Untranslated(std::string original) { return {original, original}; }
consteval util::Translatable<false> Untranslated(const char* original) { return original; }

// Provide an overload of tinyformat::format which can take bilingual_str arguments.
namespace tinyformat {
template <typename... Args>
bilingual_str format(util::bilingual_fmt<sizeof...(Args)> fmt, const Args&... args)
{
    const auto translate_arg{[](const auto& arg, bool translated) -> const auto& {
        if constexpr (std::is_same_v<decltype(arg), const bilingual_str&>) {
            return translated ? arg.translated : arg.original;
        } else {
            return arg;
        }
    }};
    return bilingual_str{tfm::format(fmt.original, translate_arg(args, false)...),
                         tfm::format(fmt.translate(), translate_arg(args, true)...)};
}
} // namespace tinyformat

#endif // BITCOIN_UTIL_TRANSLATION_H
