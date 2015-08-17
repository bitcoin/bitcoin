/**
 * @file mbstring.cpp
 *
 * This file provides locale-independent functions to validate and sanitize
 * UTF-8 encoded strings, including support for multibyte characters.
 */

#include "omnicore/mbstring.h"

#include <stddef.h>
#include <string>

namespace mastercore
{
namespace mbstring
{
/**
 * Converts char to unsigned char.
 */
inline unsigned char unsign(char c)
{
    return ((c >= 0) ? c : 256 + c);
}

/**
 * Returns the expected number of the next UTF-8 encoded bytes.
 *
 * Note: more than 4 bytes are not considered as valid UTF-8.
 */
int get_mblen(unsigned char c)
{
    if ((c & 0xFF) == 0x00) return 0;
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    if ((c & 0xFC) == 0xF8) return 5;
    if ((c & 0xFE) == 0xFC) return 6;
    if ((c & 0xFF) == 0xFE) return 7;

    return -1;
}

/**
 * Returns the expected number of the next UTF-8 encoded bytes.
 */
int get_mblen(const char* s, size_t n)
{
    if (n == 0) return 0;

    int len = get_mblen(unsign(s[0]));

    return (((int) n >= len) ? len : -1);
}

/**
 * Checks if the given characters qualify as valid UTF-8 encoded.
 */
bool check_mb(const char* s, size_t n)
{
    switch (n) {
        case 0:
            return true;
        case 1:
            return (unsign(s[0]) <= 0x7F);
        case 2:
            // C0-C1 are invalid
            return ((0xC2 <= unsign(s[0]) && unsign(s[0]) <= 0xDF) &&
                    (0x80 <= unsign(s[1]) && unsign(s[1]) <= 0xBF));
        case 3:
            // Invalid subrange:
            if ((unsign(s[0]) == 0xE0) && !(0xA0 <= unsign(s[1]) && unsign(s[1]) <= 0xBF)) {
                return false;
            }
            // Invalid subrange:
            if ((unsign(s[0]) == 0xED) && !(0x80 <= unsign(s[1]) && unsign(s[1]) <= 0x9F)) {
                return false;
            }
            return ((0xE0 <= unsign(s[0]) && unsign(s[0]) <= 0xEF) &&
                    (0x80 <= unsign(s[1]) && unsign(s[1]) <= 0xBF) &&
                    (0x80 <= unsign(s[2]) && unsign(s[2]) <= 0xBF));
        case 4:
            // Invalid subrange:
            if ((unsign(s[0]) == 0xF0) && !(0x90 <= unsign(s[1]) && unsign(s[1]) <= 0xBF)) {
                return false;
            }
            // Invalid subrange:
            if ((unsign(s[0]) == 0xF4) && !(0x80 <= unsign(s[1]) && unsign(s[1]) <= 0x8F)) {
                return false;
            }
            return ((0xF0 <= unsign(s[0]) && unsign(s[0]) <= 0xF4) && // not F7!
                    (0x80 <= unsign(s[1]) && unsign(s[1]) <= 0xBF) &&
                    (0x80 <= unsign(s[2]) && unsign(s[2]) <= 0xBF) &&
                    (0x80 <= unsign(s[3]) && unsign(s[3]) <= 0xBF));
    }

    return false;
}

} // namespace mbstring


/**
 * Replaces invalid UTF-8 characters or character sequences with question marks.
 *
 * The size of the result is always equal to the size of the input. Multibyte
 * sequences are supported. Reserved unicode characters are not considered as
 * invalid.
 */
std::string SanitizeInvalidUTF8(const std::string& s)
{
    std::string result(s.begin(), s.end());

    size_t pos = 0;
    while (pos < s.size()) {
        int next = mbstring::get_mblen(&s[pos], s.size()-pos);
        if (!mbstring::check_mb(&s[pos], next)) {
            result[pos] = '?';
            next = 1;
        }
        pos += next;
    }

    return result;
}

} // namespace mastercore
