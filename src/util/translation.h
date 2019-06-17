// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TRANSLATION_H
#define BITCOIN_UTIL_TRANSLATION_H

/** Translate a message to the native language of the user. */
const extern std::function<std::string(const char*)> G_TRANSLATION_FUN;

/**
 * Translation function.
 * If no translation function is set, simply return the input.
 */
inline std::string _(const char* psz)
{
    return G_TRANSLATION_FUN ? (G_TRANSLATION_FUN)(psz) : psz;
}

#endif // BITCOIN_UTIL_TRANSLATION_H
