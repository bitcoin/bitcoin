// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRANSLATION_H
#define BITCOIN_UTIL_TRANSLATION_H

#include <tinyformat.h>
#include <functional>

/**
 * Bilingual messages:
 *   - in GUI: user's native language + untranslated (i.e. English)
 *   - in log and stderr: untranslated only
 */
struct bilingual_str {
    std::string original;
    std::string translated;
};

namespace tinyformat {
template <typename... Args>
bilingual_str format(const bilingual_str& fmt, const Args&... args)
{
    return bilingual_str{format(fmt.original, args...), format(fmt.translated, args...)};
}
} // namespace tinyformat

/** Translate a message to the native language of the user. */
const extern std::function<std::string(const char*)> G_TRANSLATION_FUN;

/**
 * Translation function.
 * If no translation function is set, simply return the input.
 */
inline bilingual_str _(const char* psz)
{
    return bilingual_str{psz, G_TRANSLATION_FUN ? (G_TRANSLATION_FUN)(psz) : psz};
}

#endif // BITCOIN_UTIL_TRANSLATION_H
