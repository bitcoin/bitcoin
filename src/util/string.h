// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_STRING_H
#define BITCOIN_UTIL_STRING_H

#include <span.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <locale>
#include <sstream>
#include <string>      // IWYU pragma: export
#include <string_view> // IWYU pragma: export
#include <vector>

namespace util {
namespace detail {
template <unsigned num_params>
constexpr static void CheckNumFormatSpecifiers(const char* str)
{
    unsigned count_normal{0}; // Number of "normal" specifiers, like %s
    unsigned count_pos{0};    // Max number in positional specifier, like %8$s
    for (auto it{str}; *it != '\0'; ++it) {
        if (*it != '%' || *++it == '%') continue; // Skip escaped %%

        auto add_arg = [&] {
            unsigned maybe_num{0};
            while ('0' <= *it && *it <= '9') {
                maybe_num *= 10;
                maybe_num += *it - '0';
                ++it;
            }

            if (*it == '$') {
                ++it;
                // Positional specifier, like %8$s
                if (maybe_num == 0) throw "Positional format specifier must have position of at least 1";
                count_pos = std::max(count_pos, maybe_num);
            } else {
                // Non-positional specifier, like %s
                ++count_normal;
            }
        };

        // Increase argument count and consume positional specifier, if present.
        add_arg();

        // Consume flags.
        while (*it == '#' || *it == '0' || *it == '-' || *it == ' ' || *it == '+') ++it;

        auto parse_size = [&] {
            if (*it == '*') {
                ++it;
                add_arg();
            } else {
                while ('0' <= *it && *it <= '9') ++it;
            }
        };

        // Consume dynamic or static width value.
        parse_size();

        // Consume dynamic or static precision value.
        if (*it == '.') {
            ++it;
            parse_size();
        }

        if (*it == '\0') throw "Format specifier incorrectly terminated by end of string";

        // Length and type in "[flags][width][.precision][length]type"
        // is not checked. Parsing continues with the next '%'.
    }
    if (count_normal && count_pos) throw "Format specifiers must be all positional or all non-positional!";
    unsigned count{count_normal | count_pos};
    if (num_params != count) throw "Format specifier count must match the argument count!";
}
} // namespace detail

/**
 * @brief A wrapper for a compile-time partially validated format string
 *
 * This struct can be used to enforce partial compile-time validation of format
 * strings, to reduce the likelihood of tinyformat throwing exceptions at
 * run-time. Validation is partial to try and prevent the most common errors
 * while avoiding re-implementing the entire parsing logic.
 */
template <unsigned num_params>
struct ConstevalFormatString {
    const char* const fmt;
    consteval ConstevalFormatString(const char* str) : fmt{str} { detail::CheckNumFormatSpecifiers<num_params>(fmt); }
};

void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute);

/** Split a string on any char found in separators, returning a vector.
 *
 * If sep does not occur in sp, a singleton with the entirety of sp is returned.
 *
 * Note that this function does not care about braces, so splitting
 * "foo(bar(1),2),3) on ',' will return {"foo(bar(1)", "2)", "3)"}.
 */
template <typename T = std::span<const char>>
std::vector<T> Split(const std::span<const char>& sp, std::string_view separators)
{
    std::vector<T> ret;
    auto it = sp.begin();
    auto start = it;
    while (it != sp.end()) {
        if (separators.find(*it) != std::string::npos) {
            ret.emplace_back(start, it);
            start = it + 1;
        }
        ++it;
    }
    ret.emplace_back(start, it);
    return ret;
}

/** Split a string on every instance of sep, returning a vector.
 *
 * If sep does not occur in sp, a singleton with the entirety of sp is returned.
 *
 * Note that this function does not care about braces, so splitting
 * "foo(bar(1),2),3) on ',' will return {"foo(bar(1)", "2)", "3)"}.
 */
template <typename T = std::span<const char>>
std::vector<T> Split(const std::span<const char>& sp, char sep)
{
    return Split<T>(sp, std::string_view{&sep, 1});
}

[[nodiscard]] inline std::vector<std::string> SplitString(std::string_view str, char sep)
{
    return Split<std::string>(str, sep);
}

[[nodiscard]] inline std::vector<std::string> SplitString(std::string_view str, std::string_view separators)
{
    return Split<std::string>(str, separators);
}

[[nodiscard]] inline std::string_view TrimStringView(std::string_view str, std::string_view pattern = " \f\n\r\t\v")
{
    std::string::size_type front = str.find_first_not_of(pattern);
    if (front == std::string::npos) {
        return {};
    }
    std::string::size_type end = str.find_last_not_of(pattern);
    return str.substr(front, end - front + 1);
}

[[nodiscard]] inline std::string TrimString(std::string_view str, std::string_view pattern = " \f\n\r\t\v")
{
    return std::string(TrimStringView(str, pattern));
}

[[nodiscard]] inline std::string_view RemoveSuffixView(std::string_view str, std::string_view suffix)
{
    if (str.ends_with(suffix)) {
        return str.substr(0, str.size() - suffix.size());
    }
    return str;
}

[[nodiscard]] inline std::string_view RemovePrefixView(std::string_view str, std::string_view prefix)
{
    if (str.starts_with(prefix)) {
        return str.substr(prefix.size());
    }
    return str;
}

[[nodiscard]] inline std::string RemovePrefix(std::string_view str, std::string_view prefix)
{
    return std::string(RemovePrefixView(str, prefix));
}

/**
 * Join all container items. Typically used to concatenate strings but accepts
 * containers with elements of any type.
 *
 * @param container The items to join
 * @param separator The separator
 * @param unary_op  Apply this operator to each item
 */
template <typename C, typename S, typename UnaryOp>
// NOLINTNEXTLINE(misc-no-recursion)
auto Join(const C& container, const S& separator, UnaryOp unary_op)
{
    decltype(unary_op(*container.begin())) ret;
    bool first{true};
    for (const auto& item : container) {
        if (!first) ret += separator;
        ret += unary_op(item);
        first = false;
    }
    return ret;
}

template <typename C, typename S>
auto Join(const C& container, const S& separator)
{
    return Join(container, separator, [](const auto& i) { return i; });
}

/**
 * Create an unordered multi-line list of items.
 */
inline std::string MakeUnorderedList(const std::vector<std::string>& items)
{
    return Join(items, "\n", [](const std::string& item) { return "- " + item; });
}

/**
 * Check if a string does not contain any embedded NUL (\0) characters
 */
[[nodiscard]] inline bool ContainsNoNUL(std::string_view str) noexcept
{
    for (auto c : str) {
        if (c == 0) return false;
    }
    return true;
}

/**
 * Locale-independent version of std::to_string
 */
template <typename T>
std::string ToString(const T& t)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << t;
    return oss.str();
}

/**
 * Check whether a container begins with the given prefix.
 */
template <typename T1, size_t PREFIX_LEN>
[[nodiscard]] inline bool HasPrefix(const T1& obj,
                                const std::array<uint8_t, PREFIX_LEN>& prefix)
{
    return obj.size() >= PREFIX_LEN &&
           std::equal(std::begin(prefix), std::end(prefix), std::begin(obj));
}
} // namespace util

#endif // BITCOIN_UTIL_STRING_H
