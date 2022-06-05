// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_PRETTY_DATA_H
#define BITCOIN_TEST_UTIL_PRETTY_DATA_H

#include <script/script_error.h>
#include <span.h>
#include <test/util/traits.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <array>
#include <charconv>
#include <iomanip>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <string_view>

/**
 * Integer type to string (without locale issues)
 *
 * (This seems rather elaborate to avoid locale issues with `std::to_string`.
 * One can't help but think the C++ committee could have provided a nicer
 * wrapper for it.)
 *
 */
template <typename I>
std::string as_string(I i, int base = 10)
{
    std::array<char, 24> a{0};
    if (const auto [ptr, ec] = std::to_chars(a.data(), a.data() + a.size(), i);
        ec == std::errc()) {
        return {a.data(), static_cast<std::string::size_type>(ptr - a.data())};
    }
    return {}; // EOVERFLOW - should never happen as buffer `a` is large enough
}

/**
 * Outputs to a stream as hex - this unspecialized primary template doesn't
 * implement `operator<<` - it must be specialized for suitable types.
 *
 * Convert something to a hex string in a context suitable for output to a stream
 */
template <typename US, typename Enable = void>
struct Hex {
    explicit Hex(US v) {}
}; // primary template - but invalid

/**
 * Outputs to stream as hex - suitable for any integral type
 */
template <typename US>
struct Hex<US, typename std::enable_if_t<std::is_integral_v<US>>> {
    explicit Hex(US v) : m_value(v) {}

    // Unfortunately, `<<` operators for streams have overloads for `char`,
    // `signed char`, and `unsigned char` that output as _characters_ not integers.
    // Must force a widening for those types.
    using USBase = std::conditional_t<std::is_same_v<char, US>, int,
                                      std::conditional_t<std::is_same_v<signed char, US>, int,
                                                         std::conditional_t<std::is_same_v<unsigned char, US>, unsigned int, US>>>;

    const USBase m_value;

    friend std::ostream& operator<<(std::ostream& os, Hex hex)
    {
        auto flags = os.flags();
        os << std::setw(2 + 2 * sizeof(US)) << std::setfill('0')
           << std::hex << std::showbase << std::nouppercase << std::internal
           << hex.m_value;
        os.flags(flags);
        return os;
    }
};

/**
 * Outputs to stream as hex - suitable for any `base_blob<>` type, e.g., `uint256`
 */
template <typename BLOB>
struct Hex<BLOB, typename std::enable_if_t<traits::is_base_of_template_of_uint_v<base_blob, BLOB>>> {
    explicit Hex(const BLOB v) : m_value(v) {}
    const BLOB m_value;

    friend std::ostream& operator<<(std::ostream& os, Hex hex)
    {
        os << "0x" << hex.m_value.GetHex();
        return os;
    }
};

/**
 * Outputs to stream as hex - suitable for any `Span` with an overload of `HexStr`
 */
template <>
struct Hex<Span<unsigned char>, void> {
    explicit Hex(const Span<unsigned char> v) : m_value(v) {}
    const Span<const unsigned char> m_value;

    friend std::ostream& operator<<(std::ostream& os, Hex hex)
    {
        os << "0x" << HexStr(hex.m_value);
        return os;
    }
};

/**
 * Conversion to hex string for any "container" supporting `begin()` and `end()`
 *
 * (Another overload for functions defined in `src/util/strencodings.h`)
 */
template <typename C, typename = traits::ok_for_range_based_for<C>>
std::string HexStr(C container)
{
    constexpr auto to_hexit = [](unsigned char c) -> char {
        return "0123456789abcdef"[c];
    };

    std::string r;
    r.reserve(container.size() * 2);
    for (unsigned char c : container) {
        r.push_back(to_hexit(c >> 4 & 0x0F));
        r.push_back(to_hexit(c & 0x0F));
    }
    return r;
}

/**
 * Return a map from script verification flag name to its binary value (word with
 * single bit set).
 *
 * The names are the enum name with the prefix `SCRIPT_VERIFY_` removed.
 */
const std::map<std::string_view, unsigned int>& MapFlagNames();

/**
 * Given a comma-separated string of script verification flags (from
 * `script/interpreter.h`) return the flag word (word with the flag bits set)
 */
unsigned int ParseScriptFlags(std::string_view strFlags, bool issue_boost_error = true);

/**
 * Given a flag word (word with script verification flag bits set) return
 * a comma-separated string of the flags by name.
 */
std::string FormatScriptFlags(unsigned int flags);

/**
 * Given the name of script err return the error value.
 *
 * The name is the enum name with the prefix `SCRIPT_ERR_` removed.
 */
ScriptError_t ParseScriptError(std::string_view err, bool issue_boost_error = true);

/**
 * Given a script error value return its string name.
 *
 * The name is the enum name with the prefix `SCRIPT_ERR_` removed.  Note
 * that this is different from `ScriptErrorString` (`script_error.h`) which
 * returns an English phrase describing the error.
 */
std::string FormatScriptError(ScriptError_t err, bool issue_boost_error = true);

namespace test::util::literals {

/**
 * User-defined literal for converting script verify flag name to flag word.
 */
inline unsigned int operator"" _svf(const char* s, size_t len)
{
    std::string_view sv{s, len};
    if (auto r = MapFlagNames().find(sv); r != MapFlagNames().end()) return r->second;
    throw std::invalid_argument("invalid script verify flag name");
}

/**
 * User-defined literal for converting hex strings to byte vectors.
 */
inline std::vector<unsigned char> operator"" _hex(const char* s, size_t len)
{
    std::string_view sv{s, len};
    if (IsHex(sv)) return ParseHex(sv);
    throw std::invalid_argument("invalid hex literal");
}

/**
 * Literal format for a `vector<unsigned char>` initialized from a string literal.
 *
 * Characters in the string are used _directly_, not interpreted as hex.
 */
inline std::vector<unsigned char> operator"" _bv(const char* s, size_t len)
{
    std::string_view sv{s, len};
    std::vector<unsigned char> r(sv.begin(), sv.end());
    return r;
}

} // namespace test::util::literals

#endif // BITCOIN_TEST_UTIL_PRETTY_DATA_H
