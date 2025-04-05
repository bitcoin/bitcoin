// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>

#include <crypto/hex_base.h>
#include <span.h>

#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
    CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
    CHARS_ALPHA_NUM + "!*'();:@&=+$,/?#[]-_.~%", // SAFE_CHARS_URI
};

std::string SanitizeString(std::string_view str, int rule)
{
    std::string result;
    for (char c : str) {
        if (SAFE_CHARS[rule].find(c) != std::string::npos) {
            result.push_back(c);
        }
    }
    return result;
}

bool IsHex(std::string_view str)
{
    for (char c : str) {
        if (HexDigit(c) < 0) return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}

template <typename Byte>
std::optional<std::vector<Byte>> TryParseHex(std::string_view str)
{
    std::vector<Byte> vch;
    vch.reserve(str.size() / 2); // two hex characters form a single byte

    auto it = str.begin();
    while (it != str.end()) {
        if (IsSpace(*it)) {
            ++it;
            continue;
        }
        auto c1 = HexDigit(*(it++));
        if (it == str.end()) return std::nullopt;
        auto c2 = HexDigit(*(it++));
        if (c1 < 0 || c2 < 0) return std::nullopt;
        vch.push_back(Byte(c1 << 4) | Byte(c2));
    }
    return vch;
}
template std::optional<std::vector<std::byte>> TryParseHex(std::string_view);
template std::optional<std::vector<uint8_t>> TryParseHex(std::string_view);

bool SplitHostPort(std::string_view in, uint16_t& portOut, std::string& hostOut)
{
    bool valid = false;
    size_t colon = in.find_last_of(':');
    // if a : is found, and it either follows a [...], or no other : is in the string, treat it as port separator
    bool fHaveColon = colon != in.npos;
    bool fBracketed = fHaveColon && (in[0] == '[' && in[colon - 1] == ']'); // if there is a colon, and in[0]=='[', colon is not 0, so in[colon-1] is safe
    bool fMultiColon{fHaveColon && colon != 0 && (in.find_last_of(':', colon - 1) != in.npos)};
    if (fHaveColon && (colon == 0 || fBracketed || !fMultiColon)) {
        uint16_t n;
        if (ParseUInt16(in.substr(colon + 1), &n)) {
            in = in.substr(0, colon);
            portOut = n;
            valid = (portOut != 0);
        }
    } else {
        valid = true;
    }
    if (in.size() > 0 && in[0] == '[' && in[in.size() - 1] == ']') {
        hostOut = in.substr(1, in.size() - 2);
    } else {
        hostOut = in;
    }

    return valid;
}

std::string EncodeBase64(std::span<const unsigned char> input)
{
    static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string str;
    str.reserve(((input.size() + 2) / 3) * 4);
    ConvertBits<8, 6, true>([&](int v) { str += pbase64[v]; }, input.begin(), input.end());
    while (str.size() % 4) str += '=';
    return str;
}

std::optional<std::vector<unsigned char>> DecodeBase64(std::string_view str)
{
    static const int8_t decode64_table[256]{
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    if (str.size() % 4 != 0) return {};
    /* One or two = characters at the end are permitted. */
    if (str.size() >= 1 && str.back() == '=') str.remove_suffix(1);
    if (str.size() >= 1 && str.back() == '=') str.remove_suffix(1);

    std::vector<unsigned char> ret;
    ret.reserve((str.size() * 3) / 4);
    bool valid = ConvertBits<6, 8, false>(
        [&](unsigned char c) { ret.push_back(c); },
        str.begin(), str.end(),
        [](char c) { return decode64_table[uint8_t(c)]; }
    );
    if (!valid) return {};

    return ret;
}

std::string EncodeBase32(std::span<const unsigned char> input, bool pad)
{
    static const char *pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

    std::string str;
    str.reserve(((input.size() + 4) / 5) * 8);
    ConvertBits<8, 5, true>([&](int v) { str += pbase32[v]; }, input.begin(), input.end());
    if (pad) {
        while (str.size() % 8) {
            str += '=';
        }
    }
    return str;
}

std::string EncodeBase32(std::string_view str, bool pad)
{
    return EncodeBase32(MakeUCharSpan(str), pad);
}

std::optional<std::vector<unsigned char>> DecodeBase32(std::string_view str)
{
    static const int8_t decode32_table[256]{
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1,  0,  1,  2,
         3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    if (str.size() % 8 != 0) return {};
    /* 1, 3, 4, or 6 padding '=' suffix characters are permitted. */
    if (str.size() >= 1 && str.back() == '=') str.remove_suffix(1);
    if (str.size() >= 2 && str.substr(str.size() - 2) == "==") str.remove_suffix(2);
    if (str.size() >= 1 && str.back() == '=') str.remove_suffix(1);
    if (str.size() >= 2 && str.substr(str.size() - 2) == "==") str.remove_suffix(2);

    std::vector<unsigned char> ret;
    ret.reserve((str.size() * 5) / 8);
    bool valid = ConvertBits<5, 8, false>(
        [&](unsigned char c) { ret.push_back(c); },
        str.begin(), str.end(),
        [](char c) { return decode32_table[uint8_t(c)]; }
    );

    if (!valid) return {};

    return ret;
}

namespace {
template <typename T>
bool ParseIntegral(std::string_view str, T* out)
{
    static_assert(std::is_integral_v<T>);
    // Replicate the exact behavior of strtol/strtoll/strtoul/strtoull when
    // handling leading +/- for backwards compatibility.
    if (str.length() >= 2 && str[0] == '+' && str[1] == '-') {
        return false;
    }
    const std::optional<T> opt_int = ToIntegral<T>((!str.empty() && str[0] == '+') ? str.substr(1) : str);
    if (!opt_int) {
        return false;
    }
    if (out != nullptr) {
        *out = *opt_int;
    }
    return true;
}
}; // namespace

bool ParseInt32(std::string_view str, int32_t* out)
{
    return ParseIntegral<int32_t>(str, out);
}

bool ParseInt64(std::string_view str, int64_t* out)
{
    return ParseIntegral<int64_t>(str, out);
}

bool ParseUInt8(std::string_view str, uint8_t* out)
{
    return ParseIntegral<uint8_t>(str, out);
}

bool ParseUInt16(std::string_view str, uint16_t* out)
{
    return ParseIntegral<uint16_t>(str, out);
}

bool ParseUInt32(std::string_view str, uint32_t* out)
{
    return ParseIntegral<uint32_t>(str, out);
}

bool ParseUInt64(std::string_view str, uint64_t* out)
{
    return ParseIntegral<uint64_t>(str, out);
}

std::string FormatParagraph(std::string_view in, size_t width, size_t indent)
{
    assert(width >= indent);
    std::stringstream out;
    size_t ptr = 0;
    size_t indented = 0;
    while (ptr < in.size())
    {
        size_t lineend = in.find_first_of('\n', ptr);
        if (lineend == std::string::npos) {
            lineend = in.size();
        }
        const size_t linelen = lineend - ptr;
        const size_t rem_width = width - indented;
        if (linelen <= rem_width) {
            out << in.substr(ptr, linelen + 1);
            ptr = lineend + 1;
            indented = 0;
        } else {
            size_t finalspace = in.find_last_of(" \n", ptr + rem_width);
            if (finalspace == std::string::npos || finalspace < ptr) {
                // No place to break; just include the entire word and move on
                finalspace = in.find_first_of("\n ", ptr);
                if (finalspace == std::string::npos) {
                    // End of the string, just add it and break
                    out << in.substr(ptr);
                    break;
                }
            }
            out << in.substr(ptr, finalspace - ptr) << "\n";
            if (in[finalspace] == '\n') {
                indented = 0;
            } else if (indent) {
                out << std::string(indent, ' ');
                indented = indent;
            }
            ptr = finalspace + 1;
        }
    }
    return out.str();
}

/** Upper bound for mantissa.
 * 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
 * Larger integers cannot consist of arbitrary combinations of 0-9:
 *
 *   999999999999999999  1^18-1
 *  9223372036854775807  (1<<63)-1  (max int64_t)
 *  9999999999999999999  1^19-1     (would overflow)
 */
static const int64_t UPPER_BOUND = 1000000000000000000LL - 1LL;

/** Helper function for ParseFixedPoint */
static inline bool ProcessMantissaDigit(char ch, int64_t &mantissa, int &mantissa_tzeros)
{
    if(ch == '0')
        ++mantissa_tzeros;
    else {
        for (int i=0; i<=mantissa_tzeros; ++i) {
            if (mantissa > (UPPER_BOUND / 10LL))
                return false; /* overflow */
            mantissa *= 10;
        }
        mantissa += ch - '0';
        mantissa_tzeros = 0;
    }
    return true;
}

bool ParseFixedPoint(std::string_view val, int decimals, int64_t *amount_out)
{
    int64_t mantissa = 0;
    int64_t exponent = 0;
    int mantissa_tzeros = 0;
    bool mantissa_sign = false;
    bool exponent_sign = false;
    int ptr = 0;
    int end = val.size();
    int point_ofs = 0;

    if (ptr < end && val[ptr] == '-') {
        mantissa_sign = true;
        ++ptr;
    }
    if (ptr < end)
    {
        if (val[ptr] == '0') {
            /* pass single 0 */
            ++ptr;
        } else if (val[ptr] >= '1' && val[ptr] <= '9') {
            while (ptr < end && IsDigit(val[ptr])) {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
            }
        } else return false; /* missing expected digit */
    } else return false; /* empty string or loose '-' */
    if (ptr < end && val[ptr] == '.')
    {
        ++ptr;
        if (ptr < end && IsDigit(val[ptr]))
        {
            while (ptr < end && IsDigit(val[ptr])) {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
                ++point_ofs;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr < end && (val[ptr] == 'e' || val[ptr] == 'E'))
    {
        ++ptr;
        if (ptr < end && val[ptr] == '+')
            ++ptr;
        else if (ptr < end && val[ptr] == '-') {
            exponent_sign = true;
            ++ptr;
        }
        if (ptr < end && IsDigit(val[ptr])) {
            while (ptr < end && IsDigit(val[ptr])) {
                if (exponent > (UPPER_BOUND / 10LL))
                    return false; /* overflow */
                exponent = exponent * 10 + val[ptr] - '0';
                ++ptr;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr != end)
        return false; /* trailing garbage */

    /* finalize exponent */
    if (exponent_sign)
        exponent = -exponent;
    exponent = exponent - point_ofs + mantissa_tzeros;

    /* finalize mantissa */
    if (mantissa_sign)
        mantissa = -mantissa;

    /* convert to one 64-bit fixed-point value */
    exponent += decimals;
    if (exponent < 0)
        return false; /* cannot represent values smaller than 10^-decimals */
    if (exponent >= 18)
        return false; /* cannot represent values larger than or equal to 10^(18-decimals) */

    for (int i=0; i < exponent; ++i) {
        if (mantissa > (UPPER_BOUND / 10LL) || mantissa < -(UPPER_BOUND / 10LL))
            return false; /* overflow */
        mantissa *= 10;
    }
    if (mantissa > UPPER_BOUND || mantissa < -UPPER_BOUND)
        return false; /* overflow */

    if (amount_out)
        *amount_out = mantissa;

    return true;
}

std::string ToLower(std::string_view str)
{
    std::string r;
    r.reserve(str.size());
    for (auto ch : str) r += ToLower(ch);
    return r;
}

std::string ToUpper(std::string_view str)
{
    std::string r;
    r.reserve(str.size());
    for (auto ch : str) r += ToUpper(ch);
    return r;
}

std::string Capitalize(std::string str)
{
    if (str.empty()) return str;
    str[0] = ToUpper(str.front());
    return str;
}

std::optional<uint64_t> ParseByteUnits(std::string_view str, ByteUnit default_multiplier)
{
    if (str.empty()) {
        return std::nullopt;
    }
    auto multiplier = default_multiplier;
    char unit = str.back();
    switch (unit) {
    case 'k':
        multiplier = ByteUnit::k;
        break;
    case 'K':
        multiplier = ByteUnit::K;
        break;
    case 'm':
        multiplier = ByteUnit::m;
        break;
    case 'M':
        multiplier = ByteUnit::M;
        break;
    case 'g':
        multiplier = ByteUnit::g;
        break;
    case 'G':
        multiplier = ByteUnit::G;
        break;
    case 't':
        multiplier = ByteUnit::t;
        break;
    case 'T':
        multiplier = ByteUnit::T;
        break;
    default:
        unit = 0;
        break;
    }

    uint64_t unit_amount = static_cast<uint64_t>(multiplier);
    auto parsed_num = ToIntegral<uint64_t>(unit ? str.substr(0, str.size() - 1) : str);
    if (!parsed_num || parsed_num > std::numeric_limits<uint64_t>::max() / unit_amount) { // check overflow
        return std::nullopt;
    }
    return *parsed_num * unit_amount;
}
