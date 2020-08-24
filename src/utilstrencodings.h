// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Utilities for converting data from/to strings.
 */
#ifndef BITCOIN_UTILSTRENCODINGS_H
#define BITCOIN_UTILSTRENCODINGS_H

#include <attributes.h>
#include <span.h>

#include <cstdint>
#include <string>
#include <vector>

#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))
#define UBEGIN(a)           ((unsigned char*)&(a))
#define UEND(a)             ((unsigned char*)&((&(a))[1]))
#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

/** Used by SanitizeString() */
enum SafeChars
{
    SAFE_CHARS_DEFAULT, //!< The full set of allowed chars
    SAFE_CHARS_UA_COMMENT, //!< BIP-0014 subset
    SAFE_CHARS_FILENAME, //!< Chars allowed in filenames
};

/**
* Remove unsafe chars. Safe chars chosen to allow simple messages/URLs/email
* addresses, but avoid anything even possibly remotely dangerous like & or >
* @param[in] str    The string to sanitize
* @param[in] rule   The set of safe chars to choose (default: least restrictive)
* @return           A new string without unsafe chars
*/
std::string SanitizeString(const std::string& str, int rule = SAFE_CHARS_DEFAULT);
std::vector<unsigned char> ParseHex(const char* psz);
std::vector<unsigned char> ParseHex(const std::string& str);
signed char HexDigit(char c);
/* Returns true if each character in str is a hex character, and has an even
 * number of hex digits.*/
bool IsHex(const std::string& str);
/**
* Return true if the string is a hex number, optionally prefixed with "0x"
*/
bool IsHexNumber(const std::string& str);
std::vector<unsigned char> DecodeBase64(const char* p, bool* pf_invalid = nullptr);
std::string DecodeBase64(const std::string& str, bool* pf_invalid = nullptr);
std::string EncodeBase64(const unsigned char* pch, size_t len);
std::string EncodeBase64(const std::string& str);
std::vector<unsigned char> DecodeBase32(const char* p, bool* pf_invalid = nullptr);
std::string DecodeBase32(const std::string& str, bool* pf_invalid = nullptr);
std::string EncodeBase32(const unsigned char* pch, size_t len);
std::string EncodeBase32(const std::string& str);

void SplitHostPort(std::string in, int &portOut, std::string &hostOut);
std::string i64tostr(int64_t n);
std::string itostr(int n);
int64_t atoi64(const char* psz);
int64_t atoi64(const std::string& str);
int atoi(const std::string& str);

/**
 * Tests if the given character is a decimal digit.
 * @param[in] c     character to test
 * @return          true if the argument is a decimal digit; otherwise false.
 */
constexpr bool IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

/**
 * Convert string to signed 32-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
NODISCARD bool ParseInt32(const std::string& str, int32_t *out);

/**
 * Convert string to signed 64-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
NODISCARD bool ParseInt64(const std::string& str, int64_t *out);

/**
 * Convert decimal string to unsigned 8-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
NODISCARD bool ParseUInt8(const std::string& str, uint8_t *out);

/**
 * Convert decimal string to unsigned 32-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
NODISCARD bool ParseUInt32(const std::string& str, uint32_t *out);

/**
 * Convert decimal string to unsigned 64-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
NODISCARD bool ParseUInt64(const std::string& str, uint64_t *out);

/**
 * Convert string to double with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid double,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
NODISCARD bool ParseDouble(const std::string& str, double *out);

/**
 * Convert a span of bytes to a lower-case hexadecimal string.
 */
std::string HexStr(const Span<const uint8_t> s);
inline std::string HexStr(const Span<const char> s) { return HexStr(MakeUCharSpan(s)); }

/**
 * Format a paragraph of text to a fixed width, adding spaces for
 * indentation to any added line.
 */
std::string FormatParagraph(const std::string& in, size_t width = 79, size_t indent = 0);

/**
 * Timing-attack-resistant comparison.
 * Takes time proportional to length
 * of first argument.
 */
template <typename T>
bool TimingResistantEqual(const T& a, const T& b)
{
    if (b.size() == 0) return a.size() == 0;
    size_t accumulator = a.size() ^ b.size();
    for (size_t i = 0; i < a.size(); i++)
        accumulator |= a[i] ^ b[i%b.size()];
    return accumulator == 0;
}

/** Parse number as fixed point according to JSON number syntax.
 * See http://json.org/number.gif
 * @returns true on success, false on error.
 * @note The result must be in the range (-10^18,10^18), otherwise an overflow error will trigger.
 */
NODISCARD bool ParseFixedPoint(const std::string &val, int decimals, int64_t *amount_out);

/** Convert from one power-of-2 number base to another. */
template<int frombits, int tobits, bool pad, typename O, typename I>
bool ConvertBits(const O& outfn, I it, I end) {
    size_t acc = 0;
    size_t bits = 0;
    constexpr size_t maxv = (1 << tobits) - 1;
    constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
    while (it != end) {
        acc = ((acc << frombits) | *it) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            outfn((acc >> bits) & maxv);
        }
        ++it;
    }
    if (pad) {
        if (bits) outfn((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}

#endif // BITCOIN_UTILSTRENCODINGS_H
