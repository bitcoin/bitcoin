// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Utilities for converting data from/to strings.
 */
#ifndef BITCOIN_UTIL_STRENCODINGS_H
#define BITCOIN_UTIL_STRENCODINGS_H

#include <crypto/hex_base.h> // IWYU pragma: export
#include <span.h>
#include <util/string.h>

#include <array>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>      // IWYU pragma: export
#include <string_view> // IWYU pragma: export
#include <system_error>
#include <type_traits>
#include <vector>

/** Used by SanitizeString() */
enum SafeChars
{
    SAFE_CHARS_DEFAULT, //!< The full set of allowed chars
    SAFE_CHARS_UA_COMMENT, //!< BIP-0014 subset
    SAFE_CHARS_FILENAME, //!< Chars allowed in filenames
    SAFE_CHARS_URI, //!< Chars allowed in URIs (RFC 3986)
};

/**
 * Used by ParseByteUnits()
 * Lowercase base 1000
 * Uppercase base 1024
*/
enum class ByteUnit : uint64_t {
    NOOP = 1ULL,
    k = 1000ULL,
    K = 1024ULL,
    m = 1'000'000ULL,
    M = 1ULL << 20,
    g = 1'000'000'000ULL,
    G = 1ULL << 30,
    t = 1'000'000'000'000ULL,
    T = 1ULL << 40,
};

/**
* Remove unsafe chars. Safe chars chosen to allow simple messages/URLs/email
* addresses, but avoid anything even possibly remotely dangerous like & or >
* @param[in] str    The string to sanitize
* @param[in] rule   The set of safe chars to choose (default: least restrictive)
* @return           A new string without unsafe chars
*/
std::string SanitizeString(std::string_view str, int rule = SAFE_CHARS_DEFAULT);
/** Parse the hex string into bytes (uint8_t or std::byte). Ignores whitespace. Returns nullopt on invalid input. */
template <typename Byte = std::byte>
std::optional<std::vector<Byte>> TryParseHex(std::string_view str);
/** Like TryParseHex, but returns an empty vector on invalid input. */
template <typename Byte = uint8_t>
std::vector<Byte> ParseHex(std::string_view hex_str)
{
    return TryParseHex<Byte>(hex_str).value_or(std::vector<Byte>{});
}
/* Returns true if each character in str is a hex character, and has an even
 * number of hex digits.*/
bool IsHex(std::string_view str);
std::optional<std::vector<unsigned char>> DecodeBase64(std::string_view str);
std::string EncodeBase64(std::span<const unsigned char> input);
inline std::string EncodeBase64(std::span<const std::byte> input) { return EncodeBase64(MakeUCharSpan(input)); }
inline std::string EncodeBase64(std::string_view str) { return EncodeBase64(MakeUCharSpan(str)); }
std::optional<std::vector<unsigned char>> DecodeBase32(std::string_view str);

/**
 * Base32 encode.
 * If `pad` is true, then the output will be padded with '=' so that its length
 * is a multiple of 8.
 */
std::string EncodeBase32(std::span<const unsigned char> input, bool pad = true);

/**
 * Base32 encode.
 * If `pad` is true, then the output will be padded with '=' so that its length
 * is a multiple of 8.
 */
std::string EncodeBase32(std::string_view str, bool pad = true);

/**
 * Splits socket address string into host string and port value.
 * Validates port value.
 *
 * @param[in] in        The socket address string to split.
 * @param[out] portOut  Port-portion of the input, if found and parsable.
 * @param[out] hostOut  Host-portion of the input, if found.
 * @return              true if port-portion is absent or within its allowed range, otherwise false
 */
bool SplitHostPort(std::string_view in, uint16_t& portOut, std::string& hostOut);

// LocaleIndependentAtoi is provided for backwards compatibility reasons.
//
// New code should use ToIntegral.
//
// The goal of LocaleIndependentAtoi is to replicate the defined behaviour of
// std::atoi as it behaves under the "C" locale, and remove some undefined
// behavior. If the parsed value is bigger than the integer type's maximum
// value, or smaller than the integer type's minimum value, std::atoi has
// undefined behavior, while this function returns the maximum or minimum
// values, respectively.
template <typename T>
T LocaleIndependentAtoi(std::string_view str)
{
    static_assert(std::is_integral_v<T>);
    T result;
    // Emulate atoi(...) handling of white space and leading +/-.
    std::string_view s = util::TrimStringView(str);
    if (!s.empty() && s[0] == '+') {
        if (s.length() >= 2 && s[1] == '-') {
            return 0;
        }
        s = s.substr(1);
    }
    auto [_, error_condition] = std::from_chars(s.data(), s.data() + s.size(), result);
    if (error_condition == std::errc::result_out_of_range) {
        if (s.length() >= 1 && s[0] == '-') {
            // Saturate underflow, per strtoll's behavior.
            return std::numeric_limits<T>::min();
        } else {
            // Saturate overflow, per strtoll's behavior.
            return std::numeric_limits<T>::max();
        }
    } else if (error_condition != std::errc{}) {
        return 0;
    }
    return result;
}

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
 * Tests if the given character is a whitespace character. The whitespace characters
 * are: space, form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal
 * tab ('\t'), and vertical tab ('\v').
 *
 * This function is locale independent. Under the C locale this function gives the
 * same result as std::isspace.
 *
 * @param[in] c     character to test
 * @return          true if the argument is a whitespace character; otherwise false
 */
constexpr inline bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

/**
 * Convert string to integral type T. Leading whitespace, a leading +, or any
 * trailing character fail the parsing. The required format expressed as regex
 * is `-?[0-9]+`. The minus sign is only permitted for signed integer types.
 *
 * @returns std::nullopt if the entire string could not be parsed, or if the
 *   parsed value is not in the range representable by the type T.
 */
template <typename T>
std::optional<T> ToIntegral(std::string_view str)
{
    static_assert(std::is_integral_v<T>);
    T result;
    const auto [first_nonmatching, error_condition] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (first_nonmatching != str.data() + str.size() || error_condition != std::errc{}) {
        return std::nullopt;
    }
    return result;
}

/**
 * Format a paragraph of text to a fixed width, adding spaces for
 * indentation to any added line.
 */
std::string FormatParagraph(std::string_view in, size_t width = 79, size_t indent = 0);

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
        accumulator |= size_t(a[i] ^ b[i%b.size()]);
    return accumulator == 0;
}

/** Parse number as fixed point according to JSON number syntax.
 * @returns true on success, false on error.
 * @note The result must be in the range (-10^18,10^18), otherwise an overflow error will trigger.
 */
[[nodiscard]] bool ParseFixedPoint(std::string_view, int decimals, int64_t *amount_out);

namespace {
/** Helper class for the default infn argument to ConvertBits (just returns the input). */
struct IntIdentity
{
    [[maybe_unused]] int operator()(int x) const { return x; }
};

} // namespace

/** Convert from one power-of-2 number base to another. */
template<int frombits, int tobits, bool pad, typename O, typename It, typename I = IntIdentity>
bool ConvertBits(O outfn, It it, It end, I infn = {}) {
    size_t acc = 0;
    size_t bits = 0;
    constexpr size_t maxv = (1 << tobits) - 1;
    constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
    while (it != end) {
        int v = infn(*it);
        if (v < 0) return false;
        acc = ((acc << frombits) | v) & max_acc;
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

/**
 * Converts the given character to its lowercase equivalent.
 * This function is locale independent. It only converts uppercase
 * characters in the standard 7-bit ASCII range.
 * This is a feature, not a limitation.
 *
 * @param[in] c     the character to convert to lowercase.
 * @return          the lowercase equivalent of c; or the argument
 *                  if no conversion is possible.
 */
constexpr char ToLower(char c)
{
    return (c >= 'A' && c <= 'Z' ? (c - 'A') + 'a' : c);
}

/**
 * Returns the lowercase equivalent of the given string.
 * This function is locale independent. It only converts uppercase
 * characters in the standard 7-bit ASCII range.
 * This is a feature, not a limitation.
 *
 * @param[in] str   the string to convert to lowercase.
 * @returns         lowercased equivalent of str
 */
std::string ToLower(std::string_view str);

/**
 * Converts the given character to its uppercase equivalent.
 * This function is locale independent. It only converts lowercase
 * characters in the standard 7-bit ASCII range.
 * This is a feature, not a limitation.
 *
 * @param[in] c     the character to convert to uppercase.
 * @return          the uppercase equivalent of c; or the argument
 *                  if no conversion is possible.
 */
constexpr char ToUpper(char c)
{
    return (c >= 'a' && c <= 'z' ? (c - 'a') + 'A' : c);
}

/**
 * Returns the uppercase equivalent of the given string.
 * This function is locale independent. It only converts lowercase
 * characters in the standard 7-bit ASCII range.
 * This is a feature, not a limitation.
 *
 * @param[in] str   the string to convert to uppercase.
 * @returns         UPPERCASED EQUIVALENT OF str
 */
std::string ToUpper(std::string_view str);

/**
 * Capitalizes the first character of the given string.
 * This function is locale independent. It only converts lowercase
 * characters in the standard 7-bit ASCII range.
 * This is a feature, not a limitation.
 *
 * @param[in] str   the string to capitalize.
 * @returns         string with the first letter capitalized.
 */
std::string Capitalize(std::string str);

/**
 * Parse a string with suffix unit [k|K|m|M|g|G|t|T].
 * Must be a whole integer, fractions not allowed (0.5t), no whitespace or +-
 * Lowercase units are 1000 base. Uppercase units are 1024 base.
 * Examples: 2m,27M,19g,41T
 *
 * @param[in] str                  the string to convert into bytes
 * @param[in] default_multiplier   if no unit is found in str use this unit
 * @returns                        optional uint64_t bytes from str or nullopt
 *                                 if ToIntegral is false, str is empty, trailing whitespace or overflow
 */
std::optional<uint64_t> ParseByteUnits(std::string_view str, ByteUnit default_multiplier);

namespace util {
/** consteval version of HexDigit() without the lookup table. */
consteval uint8_t ConstevalHexDigit(const char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 0xa;

    throw "Only lowercase hex digits are allowed, for consistency";
}

namespace detail {
template <size_t N>
struct Hex {
    std::array<std::byte, N / 2> bytes{};
    consteval Hex(const char (&hex_str)[N])
        // 2 hex digits required per byte + implicit null terminator
        requires(N % 2 == 1)
    {
        if (hex_str[N - 1]) throw "null terminator required";
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            bytes[i] = static_cast<std::byte>(
                (ConstevalHexDigit(hex_str[2 * i]) << 4) |
                 ConstevalHexDigit(hex_str[2 * i + 1]));
        }
    }
};
} // namespace detail

/**
 * ""_hex is a compile-time user-defined literal returning a
 * `std::array<std::byte>`, equivalent to ParseHex(). Variants provided:
 *
 * - ""_hex_v: Returns `std::vector<std::byte>`, useful for heap allocation or
 *   variable-length serialization.
 *
 * - ""_hex_u8: Returns `std::array<uint8_t>`, for cases where `std::byte` is
 *   incompatible.
 *
 * - ""_hex_v_u8: Returns `std::vector<uint8_t>`, combining heap allocation with
 *   `uint8_t`.
 *
 * @warning It could be necessary to use vector instead of array variants when
 *   serializing, or vice versa, because vectors are assumed to be variable-
 *   length and serialized with a size prefix, while arrays are considered fixed
 *   length and serialized with no prefix.
 *
 * @warning It may be preferable to use vector variants to save stack space when
 *   declaring local variables if hex strings are large. Alternatively variables
 *   could be declared constexpr to avoid using stack space.
 *
 * @warning Avoid `uint8_t` variants when not necessary, as the codebase
 *   migrates to use `std::byte` instead of `unsigned char` and `uint8_t`.
 *
 * @note One reason ""_hex uses `std::array` instead of `std::vector` like
 *   ParseHex() does is because heap-based containers cannot cross the compile-
 *   time/runtime barrier.
 */
inline namespace hex_literals {

template <util::detail::Hex str>
constexpr auto operator""_hex() { return str.bytes; }

template <util::detail::Hex str>
constexpr auto operator""_hex_u8() { return std::bit_cast<std::array<uint8_t, str.bytes.size()>>(str.bytes); }

template <util::detail::Hex str>
constexpr auto operator""_hex_v() { return std::vector<std::byte>{str.bytes.begin(), str.bytes.end()}; }

template <util::detail::Hex str>
inline auto operator""_hex_v_u8() { return std::vector<uint8_t>{UCharCast(str.bytes.data()), UCharCast(str.bytes.data() + str.bytes.size())}; }

} // inline namespace hex_literals
} // namespace util

#endif // BITCOIN_UTIL_STRENCODINGS_H
