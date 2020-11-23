// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_STR_H
#define BITCOIN_TEST_UTIL_STR_H

#include <string>

bool CaseInsensitiveEqual(const std::string& s1, const std::string& s2);

/**
 * Increment a string. Useful to enumerate all fixed length strings with
 * characters in [min_char, max_char].
 */
template <typename CharType, size_t StringLength>
bool NextString(CharType (&string)[StringLength], CharType min_char, CharType max_char)
{
    for (CharType& elem : string) {
        bool has_next = elem != max_char;
        elem = elem < min_char || elem >= max_char ? min_char : CharType(elem + 1);
        if (has_next) return true;
    }
    return false;
}

/**
 * Iterate over string values and call function for each string without
 * successive duplicate characters.
 */
template <typename CharType, size_t StringLength, typename Fn>
void ForEachNoDup(CharType (&string)[StringLength], CharType min_char, CharType max_char, Fn&& fn) {
    for (bool has_next = true; has_next; has_next = NextString(string, min_char, max_char)) {
        int prev = -1;
        bool skip_string = false;
        for (CharType c : string) {
            if (c == prev) skip_string = true;
            if (skip_string || c < min_char || c > max_char) break;
            prev = c;
        }
        if (!skip_string) fn();
    }
}

#endif // BITCOIN_TEST_UTIL_STR_H
